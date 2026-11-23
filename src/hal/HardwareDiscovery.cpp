// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/HardwareDiscovery.h"
#include "hal/Clv0001KeyboardBackend.h"
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

bool HardwareDiscovery::isVerifiedFanDriverName(const QString& driverName) {
    return driverName.trimmed() == QLatin1String("acer_ec_fans");
}

bool HardwareDiscovery::hasValidFanPwmAttributes(const QString& hwmonDir) {
    const QString pwmPath = hwmonDir + QStringLiteral("/pwm1");
    const QString enablePath = hwmonDir + QStringLiteral("/pwm1_enable");
    if (!QFileInfo::exists(pwmPath) || !QFileInfo::exists(enablePath)) {
        return false;
    }
    bool okPwm = false;
    const int pwmVal = SysfsRoot::readFileTrimmed(pwmPath, &okPwm).toInt(&okPwm);
    bool okEn = false;
    const int enVal = SysfsRoot::readFileTrimmed(enablePath, &okEn).toInt(&okEn);
    return okPwm && pwmVal >= 0 && pwmVal <= 255 && okEn && enVal >= 0 && enVal <= 2;
}

bool HardwareDiscovery::isVerifiedWritableFanPwmHwmon(const QString& hwmonDir) {
    if (!hasValidFanPwmAttributes(hwmonDir)) {
        return false;
    }
    const QString drvName = SysfsRoot::readFileTrimmed(hwmonDir + QStringLiteral("/name"));
    return isVerifiedFanDriverName(drvName);
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
        if (isVerifiedWritableFanPwmHwmon(candidate)) {
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
        dmi.odmSubsystemNote = QStringLiteral("Clevo/Kapok ODM Platform (Kelilu_RTH; WMI ABBC0F6B/C/D; ACPI CLV0001 \\_SB_.DCHU)");
    } else {
        dmi.odmSubsystemNote = QStringLiteral("Standard Platform");
    }

    const QString procVersion = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/version")));
    if (!procVersion.isEmpty()) {
        const QStringList vTokens = procVersion.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        dmi.kernelRelease = (vTokens.size() >= 3 && vTokens.at(0) == QLatin1String("Linux"))
                                ? vTokens.at(2)
                                : procVersion;
    }

    const QString acpiBase = root.resolvePath(QStringLiteral("/sys/bus/acpi/devices"));
    QDir acpiDir(acpiBase);
    if (acpiDir.exists()) {
        const QStringList entries = acpiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& entry : entries) {
            if (entry.startsWith(QLatin1String("CLV0001")) ||
                entry.startsWith(QLatin1String("CLV0002")) ||
                entry.startsWith(QLatin1String("PNP0C09")) ||
                entry.startsWith(QLatin1String("ACPI0003")) ||
                entry.startsWith(QLatin1String("PNP0C0A"))) {
                const QString devDir = acpiDir.filePath(entry);
                const QString hid = SysfsRoot::readFileTrimmed(devDir + QStringLiteral("/hid"));
                const QString path = SysfsRoot::readFileTrimmed(devDir + QStringLiteral("/path"));
                const QString st = SysfsRoot::readFileTrimmed(devDir + QStringLiteral("/status"));
                dmi.acpiDevices.append(
                    QStringLiteral("%1 (hid=%2, path=%3, status=%4)")
                        .arg(entry,
                             hid.isEmpty() ? QStringLiteral("?") : hid,
                             path.isEmpty() ? QStringLiteral("?") : path,
                             st.isEmpty() ? QStringLiteral("?") : st)
                );
            }
        }
    }

    const QString modulesText = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/modules")));
    if (!modulesText.isEmpty()) {
        const QStringList interesting = {
            QStringLiteral("acer_clv0001_kbd"),
            QStringLiteral("acer_wmi"),
            QStringLiteral("nvidia"),
            QStringLiteral("nvidia_modeset"),
            QStringLiteral("i915"),
            QStringLiteral("xe"),
            QStringLiteral("coretemp"),
            QStringLiteral("intel_rapl_msr"),
            QStringLiteral("clevo_acpi"),
            QStringLiteral("alg_rgb")
        };
        for (const QString& line : modulesText.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            const QString modName = line.section(QLatin1Char(' '), 0, 0).trimmed();
            if (interesting.contains(modName) && !dmi.loadedDrivers.contains(modName)) {
                dmi.loadedDrivers.append(modName);
            }
        }
        if (dmi.loadedDrivers.contains(QStringLiteral("acer_wmi"))) {
            dmi.acerWmiLoaded = true;
        }
    }

    return dmi;
}

core::KeyboardHardwareState HardwareDiscovery::probeKeyboardHardware(const core::DmiIdentity& dmi) {
    core::KeyboardHardwareState state;
    const Clv0001Topology clvTopo = Clv0001KeyboardBackend::probeTopology(dmi);
    state.clv0001DevicePresent = clvTopo.devicePresent;
    state.clv0001AcpiPath = clvTopo.acpiPath;
    state.clv0001Status = clvTopo.status;
    state.clv0001DriverBound = clvTopo.driverBound;
    state.lastPayloadHex = clvTopo.lastPayloadHex;

    const QString ledPath = findFirstKeyboardLedPath();

    if (!ledPath.isEmpty()) {
        state.ledSysfsPath = ledPath;
        state.supportsBacklightBrightness = true;
        state.independentHwReadback = true;
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
        state.supportsPresetColors = hasMultiIntensity;
        state.supportsArbitraryRgb = hasMultiIntensity;
        state.backendKind = hasMultiIntensity
                                ? core::KeyboardBackendKind::LinuxLedsMulticolorRgb
                                : core::KeyboardBackendKind::LinuxLedsMonochrome;
        state.backendKindId = hasMultiIntensity
                                  ? QStringLiteral("linux_leds_rgb")
                                  : QStringLiteral("linux_leds_mono");
        state.controlModeBadge = hasMultiIntensity
                                     ? QStringLiteral("RGB LED SYSFS (VERIFIED READBACK)")
                                     : QStringLiteral("MONOCHROME LED SYSFS");
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

    // No /sys/class/leds/*kbd_backlight* device present — check clean-room CLV0001 ACPI _DSM backend
    if (clvTopo.devicePresent && clvTopo.driverBound && clvTopo.dmiEligible) {
        state.backendKind = core::KeyboardBackendKind::Clv0001AcpiDsmPreset;
        state.backendKindId = QStringLiteral("clv0001_acpi_dsm");
        state.controlModeBadge = clvTopo.supportsCustomRgb
                                     ? QStringLiteral("CLV0001 ACPI _DSM 0x67 (17 PRESETS + STATIC 24-BIT RGB · 5 LEVELS · NO HW READBACK)")
                                     : QStringLiteral("CLV0001 ACPI _DSM 0x67 (17 PRESETS ONLY · 5 LEVELS · NO HW READBACK)");
        state.ledSysfsPath = clvTopo.commandPath;
        state.supportsBacklightBrightness = true;
        state.supportsRgbZones = false;
        state.supportsPresetColors = clvTopo.supportsPresets;
        state.supportsArbitraryRgb = clvTopo.supportsCustomRgb;
        state.supportsTimeout = false;
        state.independentHwReadback = false;
        state.zoneCount = 1;
        state.maxBrightness = 4;
        state.discreteBrightnessLevels = 5;
        state.currentPresetName = (clvTopo.lastAppliedPreset == QLatin1String("none") ||
                                   clvTopo.lastAppliedPreset == QLatin1String("custom_rgb") ||
                                   clvTopo.lastAppliedPreset.isEmpty())
                                      ? QStringLiteral("cyan")
                                      : clvTopo.lastAppliedPreset;
        state.currentBrightnessLevel = (clvTopo.lastAppliedStatus == QLatin1String("uninitialized"))
                                           ? 4
                                           : clvTopo.lastAppliedBrightnessLevel;
        state.currentBrightnessPct = Clv0001KeyboardBackend::brightnessLevelToPct(state.currentBrightnessLevel);
        state.capabilityState = core::CapabilityState::PartiallySupported;
        state.hardwareRoutingBadge = clvTopo.supportsCustomRgb
                                         ? QStringLiteral(
                                               "CLV0001 ACPI _DSM 0x67 BOUND (%1 — 17 Presets + Static 24-Bit RGB, Levels 0..4, Accepted Without Physical Readback)"
                                           ).arg(clvTopo.acpiPath)
                                         : QStringLiteral(
                                               "CLV0001 ACPI _DSM 0x67 BOUND (%1 — Preset-Only Driver Module, 17 Presets, Levels 0..4, Accepted Without Physical Readback)"
                                           ).arg(clvTopo.acpiPath);
        state.hardwareExplanation = clvTopo.supportsCustomRgb
                                        ? QStringLiteral(
                                              "Bound to ACPI device CLV0001:00 (%1) via Stage 17+ kernel module `acer_clv0001_kbd` (`supports_custom_rgb=1`). "
                                              "Supports 17 verified named presets and static 24-bit `rgb <R> <G> <B> <level>` colors (levels 0..4 -> 0x00, 0x2F, 0x5F, 0x8F, 0xBF) "
                                              "over `_DSM` UUID `93f224e4-fbdc-4bbf-add6-db71bdc0afad` rev `0` fn `0x67`. "
                                              "Per-zone split, temporal animations (breathing/wave), and physical hardware readback remain unsupported."
                                          ).arg(clvTopo.acpiPath)
                                        : QStringLiteral(
                                              "Bound to ACPI device CLV0001:00 (%1) via legacy preset-only `acer_clv0001_kbd` (`supports_custom_rgb=0`). "
                                              "Supports the 17 verified named presets and 5 discrete brightness levels (0..4) over `_DSM 0x67`. "
                                              "Upgrade kernel module with Stage 17+ `acer_clv0001_kbd` to unlock static 24-bit custom RGB; physical readback remains unsupported."
                                          ).arg(clvTopo.acpiPath);
        return state;
    }

    // No LED class device and CLV0001 either unbound or not detected
    state.ledSysfsPath.clear();
    state.supportsBacklightBrightness = false;
    state.supportsRgbZones = false;
    state.supportsPresetColors = false;
    state.supportsArbitraryRgb = false;
    state.supportsTimeout = false;
    state.independentHwReadback = false;
    state.zoneCount = clvTopo.devicePresent ? 1 : 4;
    state.maxBrightness = clvTopo.devicePresent ? 4 : 100;
    state.currentBrightnessPct = 0;

    if (clvTopo.devicePresent) {
        state.backendKind = core::KeyboardBackendKind::Clv0001AcpiUnbound;
        state.backendKindId = QStringLiteral("clv0001_unbound");
        state.controlModeBadge = QStringLiteral("CLV0001 DETECTED (KERNEL DRIVER UNBOUND — PREVIEW MODE)");
        state.capabilityState = core::CapabilityState::NotDetected;
        state.hardwareRoutingBadge = QStringLiteral(
            "ACPI CLV0001:00 (%1) DETECTED — KERNEL DRIVER `acer_clv0001_kbd` NOT LOADED (STUDIO IN PREVIEW MODE)"
        ).arg(clvTopo.acpiPath);
        state.hardwareExplanation = QStringLiteral(
            "ACPI device `CLV0001:00` (`%1`, status `%2`) is present on `%3`, but no `/sys/class/leds/*kbd_backlight*` class device "
            "exists and clean-room kernel driver `acer_clv0001_kbd` is not currently bound (`clv0001_kbd_command` absent). "
            "You can design presets in Preview Mode, inspect the `[G, R, B, 0xF0]` `_DSM 0x67` packet preview, "
            "or switch to Mock Hardware Mode (`Mock: CLV0001 Bound`) in Diagnostics to test the full write pipeline."
        ).arg(clvTopo.acpiPath,
              clvTopo.status.isEmpty() ? QStringLiteral("15") : clvTopo.status,
              dmi.productName.isEmpty() ? QStringLiteral("A715-59G") : dmi.productName);
        return state;
    }

    const bool hasClevoWmi = dmi.wmiGuids.join(QLatin1Char(',')).contains(QLatin1String("ABBC0F6D"), Qt::CaseInsensitive);
    state.backendKind = core::KeyboardBackendKind::None;
    state.backendKindId = QStringLiteral("none");
    state.controlModeBadge = QStringLiteral("UNSUPPORTED LIGHTING");
    state.capabilityState = hasClevoWmi
                                ? core::CapabilityState::NotDetected
                                : core::CapabilityState::Unsupported;
    state.hardwareRoutingBadge = QStringLiteral("PHYSICAL LED NOT DETECTED IN /sys/class/leds — STUDIO IN PROFILE DESIGN & PREVIEW MODE");
    state.hardwareExplanation = QStringLiteral(
        "No `*kbd_backlight*` class device is exposed under `/sys/class/leds/` on the running kernel (`%1`). "
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
    bool hasReadOnlyTach = false;
    bool hasAnyPwmNode = false;
    {
        QDir hwDir(root.resolvePath(QStringLiteral("/sys/class/hwmon")));
        if (hwDir.exists()) {
            for (const QString& entry : hwDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
                QDir d(hwDir.filePath(entry));
                if (!d.entryList({QStringLiteral("fan*_input")}, QDir::Files).isEmpty()) {
                    hasReadOnlyTach = true;
                }
                if (!d.entryList({QStringLiteral("pwm*")}, QDir::Files).isEmpty()) {
                    hasAnyPwmNode = true;
                }
            }
        }
    }
    const bool hasAcpiEc =
        dmi.acpiDevices.join(QLatin1Char(',')).contains(QLatin1String("PNP0C09"), Qt::CaseInsensitive) ||
        QFileInfo::exists(root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/PNP0C09:00")));

    core::CapabilityState fanCapState = core::CapabilityState::NotDetected;
    bool fanWritable = false;
    QString fanSummary = QStringLiteral("No fan telemetry or PWM control interface detected");
    if (!fanHwmon.isEmpty()) {
        fanCapState = root.isMockActive() ? core::CapabilityState::Simulated : core::CapabilityState::FullySupported;
        fanWritable = true;
        fanSummary = root.isMockActive()
                         ? QStringLiteral("SIMULATED: Mock hwmon fan RPM and PWM interface active with automatic fallback")
                         : QStringLiteral("Kernel hwmon fan RPM and PWM interface active with automatic fallback");
    } else if (hasAnyPwmNode) {
        fanCapState = core::CapabilityState::RequiresVerification;
        fanSummary = QStringLiteral("Unverified or incomplete hwmon PWM node detected; manual writes blocked for thermal safety");
    } else if (hasReadOnlyTach) {
        fanCapState = core::CapabilityState::ReadOnly;
        fanSummary = QStringLiteral("Read-only fan tachometer detected; no verified writable PWM control node");
    } else if (hasAcpiEc) {
        fanCapState = core::CapabilityState::FirmwareAutomatic;
        fanSummary = QStringLiteral("No kernel hwmon fan RPM or PWM node is bound on Kelilu_RTH; INSYDE EC (PNP0C09:00) manages dual fans in hardware Auto mode");
    }

    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.thermal.fans"),
        QStringLiteral("Cooling Fan RPM & Manual/Curve PWM Control"),
        QStringLiteral("Thermal"),
        fanHwmon.isEmpty() ? QStringLiteral("/sys/class/hwmon/*/fan*_input & pwm* (PNP0C09:00 EC Auto)") : fanHwmon,
        fanCapState,
        fanWritable,
        fanSummary,
        QStringLiteral("Per Hardware Safety Invariant, undocumented EC registers are never written. Curve Studio runs in Preview Only mode unless a verified hwmon PWM node is present.")
    });

    // 9. NVIDIA GPU Telemetry
    const bool hasProcNvidia = QFileInfo::exists(
        root.resolvePath(QStringLiteral("/proc/driver/nvidia/gpus/0000:01:00.0/information"))
    );
    const bool gpuAvail = nvmlLoaded || hasProcNvidia || root.isMockActive();
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.gpu.nvidia"),
        QStringLiteral("NVIDIA GeForce RTX 3050 6GB Telemetry & Throttling"),
        QStringLiteral("GPU"),
        QStringLiteral("libnvidia-ml.so.1 (Dynamic Runtime Loader) / PCI 0000:01:00.0"),
        gpuAvail ? core::CapabilityState::ReadOnly : core::CapabilityState::NotDetected,
        false,
        (nvmlLoaded || root.isMockActive())
            ? QStringLiteral("Clocks, VRAM, utilization, temperature, P-state, TGP limits, and throttle reason decoding")
            : (hasProcNvidia
                   ? QStringLiteral("PCI 0000:01:00.0 & /proc/driver/nvidia active (NVML library version mismatch pending reboot)")
                   : QStringLiteral("libnvidia-ml.so.1 not available; application runs without NVIDIA dependency")),
        QStringLiteral("Includes P8 rail-gating power anomaly filter for GA107BM and /proc/driver/nvidia fallback across driver updates.")
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
    const core::CapabilityState batLimitState =
        hasBatLimit
            ? (root.isMockActive() ? core::CapabilityState::Simulated : core::CapabilityState::FullySupported)
            : core::CapabilityState::NotDetected;
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.battery.limit"),
        QStringLiteral("Battery Charge Limit Threshold (80% Care Mode)"),
        QStringLiteral("Battery & Power"),
        QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold"),
        batLimitState,
        hasBatLimit,
        hasBatLimit
            ? (root.isMockActive()
                   ? QStringLiteral("SIMULATED: Mock charge_control_end_threshold attribute detected and writable")
                   : QStringLiteral("Kernel charge_control_end_threshold attribute detected and writable"))
            : QStringLiteral("BATTERY TELEMETRY ONLY — CHARGE LIMIT NOT DETECTED (charge_control_end_threshold not exposed by acpi-battery on Kelilu_RTH)"),
        QStringLiteral("Control is enabled automatically only when the kernel power_supply driver exposes charge_control_end_threshold.")
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
    QString kbdIfacePath = kbd.ledSysfsPath;
    if (kbdIfacePath.isEmpty()) {
        kbdIfacePath = kbd.clv0001DevicePresent
                           ? QStringLiteral("/sys/bus/acpi/devices/CLV0001:00 (acer_clv0001_kbd unbound)")
                           : QStringLiteral("/sys/class/leds/*kbd_backlight* (Absent)");
    }
    caps.push_back(core::CapabilityRecord{
        QStringLiteral("cap.keyboard.lighting"),
        QStringLiteral("Physical Keyboard Backlight & Preset / Zone Control"),
        QStringLiteral("Keyboard Studio"),
        kbdIfacePath,
        kbd.capabilityState,
        kbd.supportsBacklightBrightness,
        kbd.hardwareRoutingBadge,
        kbd.hardwareExplanation
    });

    return caps;
}

} // namespace acc::hal
