---
name: hardware-recon
description: Performs read-only Linux hardware discovery and compatibility research for Acer laptops, including DMI, acer_wmi, ACPI/WMI, hwmon, LED, power-supply, fan, battery, and NVIDIA interfaces. Use before proposing or implementing any laptop hardware feature.
---

# Hardware Reconnaissance Skill

## Mission
Build an evidence-based capability inventory for the actual machine. Do not change hardware state.

## Procedure
1. Inspect the repository first and identify whether the project already has a hardware abstraction layer.
2. Record OS release, kernel, architecture, desktop/session, and relevant driver versions. Verify Fedora 45 rather than assuming it from the prompt.
3. Read only non-sensitive DMI fields such as `sys_vendor`, `product_name`, `product_version`, `board_name`, `bios_vendor`, `bios_version`, and `bios_date`. Never print serial numbers, UUIDs, asset tags, or service tags.
4. Inspect PCI devices and bound drivers with read-only tools (`lspci -nnk`, `lsmod`, `modinfo`). Do not unload modules or change module parameters.
5. Enumerate read-only sysfs interfaces under `/sys/class/hwmon`, `/sys/class/thermal`, `/sys/class/leds`, and `/sys/class/power_supply`. Read attributes only after checking their names and permissions.
6. Inspect relevant Acer WMI/ACPI evidence, kernel logs, and kernel documentation. Filter logs to relevant lines and redact machine-specific identifiers before storing reports.
7. Inspect NVIDIA capability through documented read-only tools such as `nvidia-smi`; use NVML documentation for programmatic access. Do not change clocks, power limits, persistence mode, or GPU routing during reconnaissance.
8. Verify whether a control is writable from official documentation and the actual sysfs/interface semantics. A writable-looking file is not proof that a specific value is safe or supported.
9. Cross-check exact laptop SKU and keyboard hardware. Never infer RGB support from the A715-59G family name alone.
10. Produce a capability matrix with evidence, interface path/API, read/write status, confidence, test status, and risk.

## Output format
For each capability report:
- Capability and user-facing purpose.
- Evidence source and date.
- Exact interface/API observed.
- Read-only / writable / unknown.
- Supported / partially supported / unsupported / not detected / unverified.
- Risks and unresolved questions.
- Recommended next verification step.

## Safe inspection examples
Use commands only after checking availability. Typical read-only examples include:

```bash
cat /etc/fedora-release
uname -r
cat /sys/class/dmi/id/sys_vendor
cat /sys/class/dmi/id/product_name
cat /sys/class/dmi/id/product_version
cat /sys/class/dmi/id/bios_version
lspci -nnk
lsmod | grep -E 'acer|wmi|intel|nvidia'
modinfo acer_wmi
find /sys/class/hwmon -maxdepth 2 -type f -name 'name' -o -name 'temp*_input' -o -name 'fan*_input'
find /sys/class/leds -maxdepth 2 -type f -name 'brightness' -o -name 'max_brightness'
find /sys/class/power_supply -maxdepth 2 -type f
nvidia-smi
```

Treat examples as optional: a missing tool or path is a finding, not a reason to install random drivers or run writes.

## Prohibited actions
- No writes to `/sys`, `/proc`, EC, ACPI, WMI, LED, hwmon, power-supply, or firmware interfaces.
- No `sudo` writes, module unloads, BIOS changes, fan overrides, charge-limit changes, or keyboard LED changes.
- No serial/UUID collection.
- No unreviewed shell scripts from issue comments or forums.
