// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * acer_clv0001_kbd.c — Clean-room Linux platform/ACPI keyboard backlight driver
 * for Acer laptops exposing CLV0001 (\_SB.DCHU) _DSM function 0x67.
 *
 * Copyright (C) 2026 zethasaintlab
 *
 * Protocol facts (independently established from CLV0001 / AcpiBridge DSM interface):
 *   - ACPI HID:       CLV0001 (\_SB.DCHU)
 *   - _DSM UUID:      93f224e4-fbdc-4bbf-add6-db71bdc0afad
 *   - _DSM Revision:  0
 *   - _DSM Function:  0x67 (decimal 103)
 *   - Argument:       1-element ACPI Package wrapping a 4-byte Buffer:
 *                       [0] Green (0x00..0xFF, scaled by discrete brightness level)
 *                       [1] Red   (0x00..0xFF, scaled by discrete brightness level)
 *                       [2] Blue  (0x00..0xFF, scaled by discrete brightness level)
 *                       [3] 0xF0  (fixed command/zone byte)
 *   - Brightness:     5 discrete levels: 0 (0x00), 1 (0x2F), 2 (0x5F), 3 (0x8F), 4 (0xBF)
 *
 * Safety invariants:
 *   1. ONLY function 0x67 is ever invoked. Firmware functions 0x68, 0x69, 0x6A
 *      remain permanently prohibited (system lockup risk reported upstream).
 *   2. acpi_check_dsm() uses a 64-bit function bitmask (`u64 funcs` in
 *      drivers/acpi/utils.c, max index 63). Function 0x67 (103) cannot be
 *      represented as `1ULL << 0x67` (undefined shift). Instead, we verify the
 *      exact CLV0001 HID, \_SB.DCHU path, DMI vendor, and presence of `_DSM`,
 *      then evaluate function 0x67 directly via acpi_evaluate_dsm().
 *   3. Firmware does not expose hardware readback; sysfs reports truthful
 *      `accepted_no_hw_readback` status rather than claiming verified readback.
 */

#include <linux/acpi.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/uuid.h>

#define CLV0001_DSM_REVISION		0
#define CLV0001_DSM_FUNC_SET_COLOR	0x67
#define CLV0001_MIN_INTERVAL_NS		40000000ULL /* 40 ms (max 25 Hz) */
#define CLV0001_CMD_BYTE		0xF0

static const struct dmi_system_id acer_clv0001_dmi_table[] = {
	{
		.ident = "Acer Aspire A715-59G (Kelilu_RTH)",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "Acer"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "Aspire A715-59G"),
			DMI_EXACT_MATCH(DMI_BOARD_NAME, "RTH"),
			DMI_EXACT_MATCH(DMI_BOARD_VENDOR, "Kelilu_RTH"),
		},
	},
	{ }
};

static const guid_t clv0001_dsm_guid = GUID_INIT(
	0x93f224e4, 0xfbdc, 0x4bbf,
	0xad, 0xd6, 0xdb, 0x71, 0xbd, 0xc0, 0xaf, 0xad);

struct clv0001_preset {
	const char *name;
	u8 g;
	u8 r;
	u8 b;
};

static const struct clv0001_preset clv0001_presets[] = {
	{ "off",          0x00, 0x00, 0x00 },
	{ "red",          0x00, 0xFF, 0x00 },
	{ "orange",       0x7F, 0xFF, 0x00 },
	{ "yellow",       0xFF, 0xFF, 0x00 },
	{ "lime",         0xFF, 0x7F, 0x00 },
	{ "light-green",  0xFF, 0x3F, 0x00 },
	{ "green",        0xFF, 0x00, 0x00 },
	{ "green-cyan",   0xFF, 0x00, 0x46 },
	{ "cyan",         0xFF, 0x00, 0xC8 },
	{ "light-blue",   0x7F, 0x00, 0xC8 },
	{ "blue",         0x00, 0x00, 0xC8 },
	{ "violet",       0x00, 0x7F, 0xC8 },
	{ "magenta",      0x00, 0xFF, 0xC8 },
	{ "pink",         0x00, 0xFF, 0x7F },
	{ "flesh",        0x7F, 0xFF, 0x3F },
	{ "bluish-white", 0x7F, 0x3F, 0x7F },
	{ "white",        0xFF, 0xFF, 0xFF },
};

static const u8 clv0001_brightness_scales[5] = {
	0x00, 0x2F, 0x5F, 0x8F, 0xBF
};

struct acer_clv0001_ctx {
	struct mutex lock;
	struct acpi_device *adev;
	acpi_handle dchu_handle;
	char acpi_path[64];
	u64 last_eval_ns;
	bool has_applied;
	const struct clv0001_preset *current_preset;
	int current_brightness;
	u8 last_payload[4];
	char last_status[48];
	char last_acpi_return_type[24];
};

static const struct clv0001_preset *find_preset_by_name(const char *name)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(clv0001_presets); i++) {
		if (strcmp(name, clv0001_presets[i].name) == 0)
			return &clv0001_presets[i];
	}
	return NULL;
}

static const char *acpi_obj_type_name(const union acpi_object *obj)
{
	if (!obj)
		return "none";

	switch (obj->type) {
	case ACPI_TYPE_INTEGER:
		return "integer";
	case ACPI_TYPE_BUFFER:
		return "buffer";
	case ACPI_TYPE_PACKAGE:
		return "package";
	default:
		return "unexpected";
	}
}

static bool is_allowed_acpi_return(const union acpi_object *obj)
{
	if (!obj)
		return false;

	return obj->type == ACPI_TYPE_INTEGER ||
	       obj->type == ACPI_TYPE_BUFFER ||
	       obj->type == ACPI_TYPE_PACKAGE;
}

static int clv0001_send_preset_locked(struct acer_clv0001_ctx *ctx,
				      const struct clv0001_preset *preset,
				      int brightness)
{
	u8 payload[4];
	union acpi_object buf_obj;
	union acpi_object pkg_elements[1];
	union acpi_object pkg_obj;
	union acpi_object *out;
	u8 scale;
	int ret = 0;

	if (brightness < 0 || brightness > 4 || !preset)
		return -EINVAL;

	scale = clv0001_brightness_scales[brightness];
	payload[0] = (u8)(((u16)preset->g * scale) / 0xBF);
	payload[1] = (u8)(((u16)preset->r * scale) / 0xBF);
	payload[2] = (u8)(((u16)preset->b * scale) / 0xBF);
	payload[3] = CLV0001_CMD_BYTE;

	memset(&buf_obj, 0, sizeof(buf_obj));
	buf_obj.type = ACPI_TYPE_BUFFER;
	buf_obj.buffer.length = sizeof(payload);
	buf_obj.buffer.pointer = payload;

	pkg_elements[0] = buf_obj;
	memset(&pkg_obj, 0, sizeof(pkg_obj));
	pkg_obj.type = ACPI_TYPE_PACKAGE;
	pkg_obj.package.count = 1;
	pkg_obj.package.elements = pkg_elements;

	out = acpi_evaluate_dsm(ctx->dchu_handle,
				&clv0001_dsm_guid,
				CLV0001_DSM_REVISION,
				CLV0001_DSM_FUNC_SET_COLOR,
				&pkg_obj);

	ctx->last_eval_ns = ktime_get_ns();

	if (!out) {
		strscpy(ctx->last_status, "error:acpi_eval_failed",
			sizeof(ctx->last_status));
		strscpy(ctx->last_acpi_return_type, "none",
			sizeof(ctx->last_acpi_return_type));
		return -EIO;
	}

	strscpy(ctx->last_acpi_return_type, acpi_obj_type_name(out),
		sizeof(ctx->last_acpi_return_type));

	if (!is_allowed_acpi_return(out)) {
		strscpy(ctx->last_status, "error:unexpected_return_type",
			sizeof(ctx->last_status));
		ret = -EPROTO;
	} else {
		strscpy(ctx->last_status, "accepted_no_hw_readback",
			sizeof(ctx->last_status));
		ctx->current_preset = preset;
		ctx->current_brightness = brightness;
		memcpy(ctx->last_payload, payload, sizeof(payload));
		ctx->has_applied = true;
	}

	ACPI_FREE(out);
	return ret;
}

static struct acer_clv0001_ctx *ctx_from_kobj(struct kobject *kobj)
{
	struct device *dev = kobj_to_dev(kobj);
	struct acpi_device *adev = to_acpi_device(dev);
	void *priv = NULL;

	if (!adev || !adev->handle)
		return NULL;
	if (acpi_bus_get_private_data(adev->handle, &priv) != 0)
		return NULL;
	return priv;
}

static ssize_t clv0001_kbd_command_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	struct acer_clv0001_ctx *ctx = ctx_from_kobj(kobj);
	ssize_t len;

	if (!ctx)
		return -ENODEV;

	mutex_lock(&ctx->lock);
	if (ctx->has_applied && ctx->current_preset)
		len = sysfs_emit(buf, "%s %d\n",
				 ctx->current_preset->name,
				 ctx->current_brightness);
	else
		len = sysfs_emit(buf, "none\n");
	mutex_unlock(&ctx->lock);
	return len;
}

static ssize_t clv0001_kbd_command_store(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf,
					 size_t count)
{
	struct acer_clv0001_ctx *ctx = ctx_from_kobj(kobj);
	char kbuf[64];
	char *p;
	char *tok1;
	char *tok2;
	char *tok3;
	const struct clv0001_preset *preset;
	int brightness = 4;
	u64 now_ns;
	int ret;

	if (!ctx)
		return -ENODEV;
	if (count == 0 || count >= sizeof(kbuf))
		return -EINVAL;

	memcpy(kbuf, buf, count);
	kbuf[count] = '\0';
	p = strim(kbuf);
	if (*p == '\0')
		return -EINVAL;

	tok1 = strsep(&p, " \t");
	if (!tok1 || *tok1 == '\0')
		return -EINVAL;

	if (p) {
		p = skip_spaces(p);
		if (*p != '\0') {
			tok2 = strsep(&p, " \t");
			if (p) {
				p = skip_spaces(p);
				tok3 = (*p != '\0') ? p : NULL;
				if (tok3)
					return -EINVAL;
			}
			if (!tok2 || kstrtoint(tok2, 10, &brightness) != 0)
				return -EINVAL;
			if (brightness < 0 || brightness > 4)
				return -EINVAL;
		}
	} else if (strcmp(tok1, "off") == 0) {
		brightness = 0;
	}

	if (strcmp(tok1, "off") == 0)
		brightness = 0;

	preset = find_preset_by_name(tok1);
	if (!preset)
		return -EINVAL;

	if (mutex_lock_interruptible(&ctx->lock))
		return -ERESTARTSYS;

	now_ns = ktime_get_ns();
	if (ctx->last_eval_ns != 0 &&
	    now_ns > ctx->last_eval_ns &&
	    (now_ns - ctx->last_eval_ns) < CLV0001_MIN_INTERVAL_NS) {
		strscpy(ctx->last_status, "error:rate_limited",
			sizeof(ctx->last_status));
		mutex_unlock(&ctx->lock);
		return -EBUSY;
	}

	ret = clv0001_send_preset_locked(ctx, preset, brightness);
	mutex_unlock(&ctx->lock);

	return ret ? ret : (ssize_t)count;
}

static ssize_t clv0001_kbd_state_show(struct kobject *kobj,
				      struct kobj_attribute *attr,
				      char *buf)
{
	struct acer_clv0001_ctx *ctx = ctx_from_kobj(kobj);
	ssize_t len;

	if (!ctx)
		return -ENODEV;

	mutex_lock(&ctx->lock);
	len = sysfs_emit(
		buf,
		"status=%s\n"
		"preset=%s\n"
		"brightness_level=%d\n"
		"payload_grb=%02X %02X %02X %02X\n"
		"acpi_return_type=%s\n"
		"physical_readback=unsupported\n",
		ctx->last_status,
		ctx->current_preset ? ctx->current_preset->name : "none",
		ctx->current_brightness,
		ctx->last_payload[0],
		ctx->last_payload[1],
		ctx->last_payload[2],
		ctx->last_payload[3],
		ctx->last_acpi_return_type);
	mutex_unlock(&ctx->lock);
	return len;
}

static ssize_t clv0001_kbd_presets_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	size_t i;
	ssize_t len = 0;

	len += sysfs_emit_at(buf, len, "presets=");
	for (i = 0; i < ARRAY_SIZE(clv0001_presets); i++) {
		len += sysfs_emit_at(buf, len, "%s%s",
				     (i == 0) ? "" : ",",
				     clv0001_presets[i].name);
	}
	len += sysfs_emit_at(buf, len, "\nbrightness_levels=0,1,2,3,4\n");
	return len;
}

static struct kobj_attribute clv0001_kbd_command_attr =
	__ATTR(clv0001_kbd_command, 0600,
	       clv0001_kbd_command_show, clv0001_kbd_command_store);

static struct kobj_attribute clv0001_kbd_state_attr =
	__ATTR_RO(clv0001_kbd_state);

static struct kobj_attribute clv0001_kbd_presets_attr =
	__ATTR_RO(clv0001_kbd_presets);

static struct attribute *acer_clv0001_attrs[] = {
	&clv0001_kbd_command_attr.attr,
	&clv0001_kbd_state_attr.attr,
	&clv0001_kbd_presets_attr.attr,
	NULL,
};

static const struct attribute_group acer_clv0001_attr_group = {
	.attrs = acer_clv0001_attrs,
};

static int acer_clv0001_probe(struct platform_device *pdev)
{
	struct acpi_device *adev = ACPI_COMPANION(&pdev->dev);
	acpi_handle handle = ACPI_HANDLE(&pdev->dev);
	struct acpi_buffer path_buf = { ACPI_ALLOCATE_BUFFER, NULL };
	const char *acpi_hid;
	struct acer_clv0001_ctx *ctx;
	acpi_status status;
	int ret;

	if (!adev || !handle)
		return -ENODEV;

	acpi_hid = acpi_device_hid(adev);
	if (!acpi_hid || strcmp(acpi_hid, "CLV0001") != 0)
		return -ENODEV;

	if (!dmi_check_system(acer_clv0001_dmi_table)) {
		dev_info(&pdev->dev,
			 "skipping CLV0001: DMI identity does not match verified Acer Aspire A715-59G (RTH/Kelilu_RTH)\n");
		return -ENODEV;
	}

	status = acpi_get_name(handle, ACPI_FULL_PATHNAME_NO_TRAILING, &path_buf);
	if (ACPI_FAILURE(status) || !path_buf.pointer) {
		kfree(path_buf.pointer);
		return -ENODEV;
	}

	if (strcmp((const char *)path_buf.pointer, "\\_SB.DCHU") != 0 &&
	    strcmp((const char *)path_buf.pointer, "\\_SB_.DCHU") != 0) {
		dev_warn(&pdev->dev,
			 "unexpected CLV0001 ACPI path '%s'\n",
			 (const char *)path_buf.pointer);
		kfree(path_buf.pointer);
		return -ENODEV;
	}

	if (!acpi_has_method(handle, "_DSM")) {
		dev_err(&pdev->dev, "CLV0001 device lacks _DSM method\n");
		kfree(path_buf.pointer);
		return -ENODEV;
	}

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		kfree(path_buf.pointer);
		return -ENOMEM;
	}

	mutex_init(&ctx->lock);
	ctx->adev = adev;
	ctx->dchu_handle = handle;
	strscpy(ctx->acpi_path, (const char *)path_buf.pointer,
		sizeof(ctx->acpi_path));
	kfree(path_buf.pointer);

	strscpy(ctx->last_status, "uninitialized", sizeof(ctx->last_status));
	strscpy(ctx->last_acpi_return_type, "none",
		sizeof(ctx->last_acpi_return_type));
	ctx->current_brightness = 0;
	ctx->last_payload[3] = CLV0001_CMD_BYTE;

	ret = acpi_bus_attach_private_data(adev->handle, ctx);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, ctx);

	ret = sysfs_create_group(&adev->dev.kobj, &acer_clv0001_attr_group);
	if (ret) {
		platform_set_drvdata(pdev, NULL);
		acpi_bus_detach_private_data(adev->handle);
		return ret;
	}

	dev_info(&pdev->dev,
		 "bound CLV0001 (%s) _DSM 0x67 keyboard backlight sysfs interface\n",
		 ctx->acpi_path);
	return 0;
}

static void acer_clv0001_remove(struct platform_device *pdev)
{
	struct acer_clv0001_ctx *ctx = platform_get_drvdata(pdev);

	if (ctx && ctx->adev) {
		sysfs_remove_group(&ctx->adev->dev.kobj,
				   &acer_clv0001_attr_group);
		acpi_bus_detach_private_data(ctx->adev->handle);
		platform_set_drvdata(pdev, NULL);
	}
}

static const struct acpi_device_id acer_clv0001_acpi_ids[] = {
	{ "CLV0001", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, acer_clv0001_acpi_ids);

static struct platform_driver acer_clv0001_driver = {
	.probe = acer_clv0001_probe,
	.remove = acer_clv0001_remove,
	.driver = {
		.name = "acer_clv0001_kbd",
		.acpi_match_table = acer_clv0001_acpi_ids,
	},
};

module_platform_driver(acer_clv0001_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zethasaintlab");
MODULE_DESCRIPTION("Clean-room CLV0001 (_DSM 0x67) keyboard backlight driver for Acer Aspire A715-59G");
MODULE_VERSION("1.0.0");
