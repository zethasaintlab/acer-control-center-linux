// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/ColorScience.h"
#include "core/DomainModels.h"
#include <QString>
#include <array>
#include <cstdint>
#include <vector>

namespace acc::hal {

struct Clv0001Topology {
    bool devicePresent = false;
    QString hid;
    QString acpiPath;
    QString status;
    bool driverBound = false;
    bool supportsPresets = false;
    bool supportsCustomRgb = false;
    QString sysfsDeviceDir;
    QString commandPath;
    QString statePath;
    QString presetsPath;
    bool dmiEligible = false;
    QString lastAppliedStatus = QStringLiteral("unavailable");
    QString lastAppliedPreset = QStringLiteral("unknown");
    int lastAppliedBrightnessLevel = -1;
    QString lastPayloadHex = QStringLiteral("UNAVAILABLE");
    QString lastAcpiReturnType = QStringLiteral("none");
};

/**
 * Clean-room backend for Acer CLV0001 (\_SB.DCHU) keyboard backlight control
 * using ACPI _DSM UUID 93f224e4-fbdc-4bbf-add6-db71bdc0afad, revision 0, function 0x67.
 *
 * SAFETY & VERIFICATION INVARIANTS:
 * 1. Exposes ONLY function 0x67 via the narrowly scoped `clv0001_kbd_command` sysfs
 *    attribute provided by `acer_clv0001_kbd`. Prohibited functions 0x68, 0x69, 0x6A are
 *    never called or exposed.
 * 2. Enforces exact 4-byte GRB payload order [G, R, B, 0xF0], 5 discrete brightness levels
 *    (0..4 => 0x00, 0x2F, 0x5F, 0x8F, 0xBF), the 17 verified presets, and Stage 17
 *    static custom RGB (`rgb R G B brightness_level`) only when `supports_custom_rgb=1`
 *    is explicitly advertised in `clv0001_kbd_presets`.
 * 3. Never fabricates physical readback: `CLV0001` _DSM 0x67 is write-only, so
 *    successful ACPI evaluation reports `accepted_no_hw_readback` with
 *    `physicalStateVerified = false`.
 */
class Clv0001KeyboardBackend {
public:
    static constexpr const char* kAcpiHid = "CLV0001";
    static constexpr const char* kDsmUuid = "93f224e4-fbdc-4bbf-add6-db71bdc0afad";
    static constexpr uint32_t kDsmRevision = 0U;
    static constexpr uint32_t kDsmFunctionSetColor = 0x67U;
    static constexpr uint8_t kCommandByte = 0xF0U;
    static constexpr int kDefaultMinIntervalMs = 40;

    static const std::vector<core::Clv0001PresetInfo>& verifiedPresets();
    static bool findPreset(const QString& name, core::Clv0001PresetInfo& outPreset);

    static uint8_t brightnessScaleForLevel(int level);
    static int brightnessPctToLevel(int pct);
    static int brightnessLevelToPct(int level);

    static bool buildFirmwarePayload(
        const QString& presetName,
        int brightnessLevel,
        std::array<uint8_t, 4>& outPayload,
        QString* outError = nullptr
    );
    static bool buildCustomRgbFirmwarePayload(
        int r,
        int g,
        int b,
        int brightnessLevel,
        std::array<uint8_t, 4>& outPayload,
        QString* outError = nullptr
    );
    static QString formatPayloadHex(const std::array<uint8_t, 4>& payload);

    // Maps an arbitrary RGB color to the nearest of the 17 verified CLV0001 presets
    static QString findNearestPresetName(const core::RgbColor& color, bool* outExactMatch = nullptr);

    // Reproduces the legacy alg-cli kernel parser (`sscanf(kbuf, "%31s %d", ...)`)
    // to prove ABI mismatch with `rgb R G B brightness` strings in automated tests.
    static bool simulateLegacyAlgWriteParser(
        const QString& rawWriteBuffer,
        QString* outParsedColor = nullptr,
        int* outParsedBrightness = nullptr,
        QString* outFailureReason = nullptr
    );

    static bool isDmiEligible(const core::DmiIdentity& dmi, QString* outReason = nullptr);
    static Clv0001Topology probeTopology(const core::DmiIdentity& dmi);

    static core::ControlResult applyPresetCommand(
        const core::DmiIdentity& dmi,
        const QString& presetName,
        int brightnessLevel
    );

    static core::ControlResult applyCustomRgbCommand(
        const core::DmiIdentity& dmi,
        int red,
        int green,
        int blue,
        int brightnessLevel
    );

    static void setMinCommandIntervalMsForTesting(int intervalMs);
    static void resetRateLimiterForTesting();
};

} // namespace acc::hal
