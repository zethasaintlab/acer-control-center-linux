// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/HardwareDiscovery.h"
#include "hal/SysfsRoot.h"

#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace acc::hal {

QString HardwareDiscovery::findHwmonByName(const QString& driverName) {
    const QString hwmonBase = SysfsRoot::instance().resolvePath(QStringLiteral("/sys/class/hwmon"));
    QDir dir(hwmonBase);
    if (!dir.exists()) {
        return QString();
    }
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& entry : entries) {
        const QString candidate = dir.filePath(entry);
        const QString name = SysfsRoot::readFileTrimmed(candidate + QStringLiteral("/name"));
        if (name == driverName) {
            return candidate;
        }
    }
    return QString();
}

QString HardwareDiscovery::findFirstKeyboardLedPath() {
    const QString ledsBase = SysfsRoot::instance().resolvePath(QStringLiteral("/sys/class/leds"));
    QDir dir(ledsBase);
    if (!dir.exists()) {
        return QString();
    }
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& entry : entries) {
        if (entry.contains(QLatin1String("kbd_backlight"), Qt::CaseInsensitive) ||
            entry.contains(QLatin1String("keyboard"), Qt::CaseInsensitive)) {
            const QString candidate = dir.filePath(entry);
            if (QFileInfo::exists(candidate + QStringLiteral("/brightness"))) {
                return candidate;
            }
        }
    }
    return QString();
}

QString HardwareDiscovery::findFirstFanPwmHwmonPath() {
    const QString hwmonBase = SysfsRoot::instance().resolvePath(QStringLiteral("/sys/class/hwmon"));
    QDir dir(hwmonBase);
    if (!dir.exists()) {
        return QString();
    }
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& entry : entries) {
        const QString candidate = dir.filePath(entry);
        if (QFileInfo::exists(candidate + QStringLiteral("/fan1_input")) ||
            QFileInfo::exists(candidate + QStringLiteral("/pwm1"))) {
            return candidate;
        }
    }
    return QString();
}

core::DmiIdentity HardwareDiscovery::probeDmiAndWmi() {
    core::DmiIdentity dmi;
    const auto& root = SysfsRoot::instance();

    dmi.sysVendor = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/sys_vendor")));
    dmi.productName = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/product_name")));
    dmi.productVersion = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/product_version")));
    dmi.boardName = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/board_name")));
    dmi.boardVendor = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/board_vendor")));
    dmi.biosVendor = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/bios_vendor")));
    dmi.biosVersion = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/bios_version")));
    dmi.biosDate = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/bios_date")));
    dmi.chassisType = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/dmi/id/chassis_type")));

    const QString wmiBase = root.resolvePath(QStringLiteral("/sys/bus/wmi/devices"));
    QDir wmiDir(wmiBase);
    if (wmiDir.exists()) {
        const QStringList entries = wmiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& entry : entries) {
            dmi.wmiGuids.append(entry);
        }
    }

    dmi.acerWmiLoaded = QFileInfo::exists(root.resolvePath(QStringLiteral("/sys/module/acer_wmi")));
    if (dmi.boardVendor.contains(QLatin1String("Kelilu_RTH"), Qt::CaseInsensitive) ||
        dmi.wmiGuids.join(QLatin1Char(',')).contains(QLatin1String("ABBC0F6"), Qt::CaseInsensitive)) {
        dmi.odmSubsystemNote = QStringLiteral("Clevo/Kapok ODM Platform (Kelilu_RTH; WMI ABBC0F6B/C/D)");
    } else {
        dmi.odmSubsystemNote = QStringLiteral("Standard Platform");
    }

    return dmi;
}

core::KeyboardHardwareState HardwareDiscovery::probeKeyboardHardware(const core::DmiIdentity& dmi) {
    core::KeyboardHardwareState state;
    const QString ledPath = findFirstKeyboardLedPath();

    if (!ledPath.isEmpty()) {
        state.ledSysfsPath = ledPath;
        state.supportsBacklightBrightness = true;
        bool okMax = false;
        const int maxB = SysfsRoot::readFileTrimmed(ledPath + QStringLiteral("/max_brightness"), &okMax).toInt();
        state.maxBrightness = (okMax && maxB > 0) ? maxB : 100;

        bool okCur = false;
        const int curB = SysfsRoot::readFileTrimmed(ledPath + QStringLiteral("/brightness"), &okCur).toInt();
        state.currentBrightnessPct = (okCur && state.maxBrightness > 0)
                                         ? std::clamp((curB * 100) / state.maxBrightness, 0, 100)
                                         : 80;

        const bool hasMultiIntensity = QFileInfo::exists(ledPath + QStringLiteral("/multi_intensity"));
        state.supportsRgbZones = hasMultiIntensity;
        state.zoneCount = hasMultiIntensity ? 4 : 1;
        state.capabilityState = hasMultiIntensity
                                    ? core::CapabilityState::FullySupported
                                    : core::CapabilityState::PartiallySupported;
        state.hardwareRoutingBadge = SysfsRoot::instance().isMockActive()
                                         ? QStringLiteral("MOCK LED SYSFS ACTIVE (%1)").arg(QFileInfo(ledPath).fileName())
                                         : QStringLiteral("KERNEL LED DEVICE ACTIVE (%1)").arg(QFileInfo(ledPath).fileName());
        state.hardwareExplanation = hasMultiIntensity
                                        ? QStringLiteral("Multicolor LED sysfs interface detected (`multi_intensity`). Unrestricted sRGB color & brightness writes are verified via readback.")
                                        : QStringLiteral("Single-color keyboard backlight detected (`brightness`). Brightness is hardware-controllable; physical RGB color changes are unsupported by this LED node.");
        return state;
    }

    // No /sys/class/leds/*kbd_backlight* device present
    state.ledSysfsPath.clear();
    state.supportsBacklightBrightness = false;
    state.supportsRgbZones = false;
    state.supportsTimeout = false;
    state.zoneCount = 4;
    state.maxBrightness = 100;
    state.currentBrightnessPct = 0;

    const bool hasClevoWmi = dmi.wmiGuids.join(QLatin1Char(',')).contains(QLatin1String("ABBC0F6D"), Qt::CaseInsensitive);
    state.capabilityState = hasClevoWmi
                                ? core::CapabilityState::NotDetected
                                : core::CapabilityState::Unsupported;
    state.hardwareRoutingBadge = QStringLiteral("PHYSICAL LED NOT DETECTED IN /sys/class/leds — STUDIO IN PROFILE DESIGN & PREVIEW MODE");
    state.hardwareExplanation = QStringLiteral(
        "No `*kbd_backlight*` class device is exposed under `/sys/class/leds/` on the running kernel (`%1`). "
        "On the Aspire A715-59G (`Kelilu_RTH`), physical keyboard illumination is controlled by EC firmware shortcuts (`Fn+F9`/`F10`) or unbound ODM WMI (`ABBC0F6D`). "
        "You can design, preview, and save unrestricted HSV/HSL/RGB/Hex lighting profiles here, or enable Mock Hardware Mode in Diagnostics to test the full D-Bus/sysfs write pipeline."
    ).arg(dmi.productName.isEmpty() ? QStringLiteral("A715-59G") : dmi.productName);

    return state;
}

std::vector<core::CapabilityRecord> HardwareDiscovery::probeAllCapabilities(bool nvmlLoaded) {
    std::vector<core::CapabilityRecord> caps;
    const auto& root = SysfsRoot::instance();
    const core::DmiIdentity dmi = probeDmiAndWmi();

    // 1. CPU Temperature
    const QString coretempPath = findHwmonByName(QStringLiteral("coretemp"));
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.cpu.temp"),
        QStringLiteral("CPU Package & Per-Core Temperatures"),
        QStringLiteral("CPU & Platform"),
        coretempPath.isEmpty() ? QStringLiteral("/sys/class/hwmon (coretemp)") : coretempPath,
        coretempPath.isEmpty() ? core::CapabilityState::NotDetected : core::CapabilityState::ReadOnly,
        false,
        coretempPath.isEmpty() ? QStringLiteral("coretemp hwmon driver not found")
                               : QStringLiteral("Package + 10 physical core thermal diodes exposed via coretemp"),
        QStringLiteral("Read-only millidegree Celsius telemetry from Intel digital thermal sensors.")
    });

    // 2. CPU Utilization & Frequency
    const bool hasStat = QFileInfo::exists(root.resolvePath(QStringLiteral("/proc/stat")));
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.cpu.load_freq"),
        QStringLiteral("CPU Utilization & Per-Core Frequency"),
        QStringLiteral("CPU & Platform"),
        QStringLiteral("/proc/stat & /sys/devices/system/cpu/cpu*/cpufreq"),
        hasStat ? core::CapabilityState::ReadOnly : core::CapabilityState::NotDetected,
        false,
        QStringLiteral("16-thread load delta & Raptor Lake HWP per-core frequency telemetry"),
        QStringLiteral("Sampled asynchronously on worker thread without blocking UI.")
    });

    // 3. CPU RAPL Package Power
    const QString raplPath = root.resolvePath(QStringLiteral("/sys/class/powercap/intel-rapl:0/energy_uj"));
    const bool hasRapl = QFileInfo::exists(raplPath);
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.cpu.power"),
        QStringLiteral("CPU Package & Platform Power (Intel RAPL)"),
        QStringLiteral("CPU & Platform"),
        QStringLiteral("/sys/class/powercap/intel-rapl:0/energy_uj"),
        hasRapl ? core::CapabilityState::ReadOnly : core::CapabilityState::NotDetected,
        false,
        QStringLiteral("Package, Core, and Platform energy counters (0400 root-only)"),
        QStringLiteral("Read safely via privileged D-Bus service (GetRaplPowerSnapshot) or mock fixture.")
    });

    // 4. CPU P-State & EPP Tuning
    const QString pstatePath = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"));
    const bool hasPstate = QFileInfo::exists(pstatePath);
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.cpu.pstate"),
        QStringLiteral("Intel P-State Governor, EPP & Turbo Control"),
        QStringLiteral("Performance"),
        QStringLiteral("/sys/devices/system/cpu/intel_pstate & cpufreq"),
        hasPstate ? core::CapabilityState::FullySupported : core::CapabilityState::NotDetected,
        hasPstate,
        QStringLiteral("Supports powersave/performance governors, HWP EPP hints, Turbo toggle, and min/max perf %"),
        QStringLiteral("Privileged writes are validated against scaling_available_governors and verified via immediate readback.")
    });

    // 5. System Power Profiles (tuned-ppd)
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.power.profile"),
        QStringLiteral("Fedora System Power Profiles (TuneD / PPD)"),
        QStringLiteral("Performance"),
        QStringLiteral("D-Bus net.hadess.PowerProfiles & com.redhat.tuned"),
        core::CapabilityState::FullySupported,
        true,
        QStringLiteral("Coordinates Quiet, Balanced, and Performance operating modes with Fedora 45 TuneD"),
        QStringLiteral("Avoids daemon conflicts by delegating system-wide power policy to tuned-ppd.")
    });

    // 6. Memory & PSI Pressure
    const bool hasPsi = QFileInfo::exists(root.resolvePath(QStringLiteral("/proc/pressure/memory")));
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.mem.psi"),
        QStringLiteral("RAM Allocation & Linux PSI Pressure Stall Information"),
        QStringLiteral("Memory"),
        QStringLiteral("/proc/meminfo & /proc/pressure/{cpu,memory,io}"),
        hasPsi ? core::CapabilityState::ReadOnly : core::CapabilityState::PartiallySupported,
        false,
        QStringLiteral("Tracks physical memory, cache, buffers, and 10s/60s PSI stall percentages"),
        QStringLiteral("Zero-privilege kernel telemetry.")
    });

    // 7. Board & NVMe Thermal Sensors
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.thermal.sensors"),
        QStringLiteral("NVMe Gen4 SSD, Wi-Fi & ACPI Thermal Zones"),
        QStringLiteral("Thermal"),
        QStringLiteral("/sys/class/hwmon/hwmon{1,3,4,6} & /sys/class/thermal/thermal_zone0..7"),
        core::CapabilityState::ReadOnly,
        false,
        QStringLiteral("Monitors dual Phison NVMe SSDs, iwlwifi_1, acpitz, TCPU, SEN2, and SEN3"),
        QStringLiteral("All exposed board and storage thermal sensors are polled in real time.")
    });

    // 8. Fan RPM & PWM Control
    const QString fanHwmon = findFirstFanPwmHwmonPath();
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.thermal.fans"),
        QStringLiteral("Cooling Fan RPM & Manual/Curve PWM Control"),
        QStringLiteral("Thermal"),
        fanHwmon.isEmpty() ? QStringLiteral("/sys/class/hwmon/*/fan*_input & pwm* (Absent)") : fanHwmon,
        fanHwmon.isEmpty() ? core::CapabilityState::NotDetected : core::CapabilityState::FullySupported,
        !fanHwmon.isEmpty(),
        fanHwmon.isEmpty()
            ? QStringLiteral("No kernel hwmon fan RPM or PWM node is bound on Kelilu_RTH; EC manages fans in Auto mode")
            : QStringLiteral("Kernel hwmon fan RPM and PWM interface active with automatic fallback"),
        QStringLiteral("Per Hardware Safety Invariant, undocumented EC registers are never written. Curve Studio runs in Preview Mode unless a verified hwmon PWM node is present.")
    });

    // 9. NVIDIA GPU Telemetry
    const bool gpuAvail = nvmlLoaded || root.isMockActive();
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.gpu.nvidia"),
        QStringLiteral("NVIDIA GeForce RTX 3050 6GB Telemetry & Throttling"),
        QStringLiteral("GPU"),
        QStringLiteral("libnvidia-ml.so.1 (Dynamic Runtime Loader) / PCI 0000:01:00.0"),
        gpuAvail ? core::CapabilityState::ReadOnly : core::CapabilityState::NotDetected,
        false,
        gpuAvail
            ? QStringLiteral("Clocks, VRAM, utilization, temperature, P-state, TGP limits, and throttle reason decoding")
            : QStringLiteral("libnvidia-ml.so.1 not available; application runs without NVIDIA dependency"),
        QStringLiteral("Includes P8 rail-gating power anomaly filter for GA107BM on driver 615.78.08.")
    });

    // 10. Battery Telemetry & Capacity Ratio
    const QString batPath = root.resolvePath(QStringLiteral("/sys/class/power_supply/BAT0"));
    const bool hasBat = QFileInfo::exists(batPath);
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.battery.telemetry"),
        QStringLiteral("Battery Telemetry, Discharge Wattage & Estimated Capacity Ratio"),
        QStringLiteral("Battery & Power"),
        QStringLiteral("/sys/class/power_supply/BAT0 & AC"),
        hasBat ? core::CapabilityState::ReadOnly : core::CapabilityState::NotDetected,
        false,
        QStringLiteral("Current charge %, voltage, current, computed power (W), full-charge vs design capacity, and fuel-gauge capacity ratio"),
        QStringLiteral("charge_full / charge_full_design is a smart-battery fuel-gauge estimate subject to termination and temperature variance, not a definitive cell health test. Serial numbers in BAT0/serial_number and uevent are never displayed or exported.")
    });

    // 11. Battery Charge Threshold
    const QString batLimitPath = root.resolvePath(QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold"));
    const bool hasBatLimit = QFileInfo::exists(batLimitPath);
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.battery.limit"),
        QStringLiteral("Battery Charge Limit Threshold (80% Care Mode)"),
        QStringLiteral("Battery & Power"),
        QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold"),
        hasBatLimit ? core::CapabilityState::FullySupported : core::CapabilityState::NotDetected,
        hasBatLimit,
        hasBatLimit
            ? QStringLiteral("Kernel charge_control_end_threshold attribute detected and writable")
            : QStringLiteral("charge_control_end_threshold not exposed by acpi-battery on Kelilu_RTH"),
        QStringLiteral("Control is enabled automatically when the kernel power_supply driver exposes charge_control_end_threshold.")
    });

    // 12. Display Backlight
    const QString blPath = root.resolvePath(QStringLiteral("/sys/class/backlight/intel_backlight/brightness"));
    const bool hasBl = QFileInfo::exists(blPath);
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.backlight.display"),
        QStringLiteral("Internal eDP Display Backlight (intel_backlight)"),
        QStringLiteral("Display & Input"),
        QStringLiteral("/sys/class/backlight/intel_backlight"),
        hasBl ? core::CapabilityState::FullySupported : core::CapabilityState::NotDetected,
        hasBl,
        QStringLiteral("48,000-step eDP backlight brightness via systemd-logind / sysfs"),
        QStringLiteral("Verified on Intel Raptor Lake-P UHD Graphics card1-eDP-1.")
    });

    // 13. Keyboard Lighting Studio
    const core::KeyboardHardwareState kbd = probeKeyboardHardware(dmi);
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.keyboard.lighting"),
        QStringLiteral("Physical Keyboard Backlight & Zone RGB Control"),
        QStringLiteral("Keyboard Studio"),
        kbd.ledSysfsPath.isEmpty() ? QStringLiteral("/sys/class/leds/*kbd_backlight* (Absent)") : kbd.ledSysfsPath,
        kbd.capabilityState,
        kbd.supportsBacklightBrightness,
        kbd.hardwareRoutingBadge,
        kbd.hardwareExplanation
    });

    return caps;
}

} // namespace acc::hal
