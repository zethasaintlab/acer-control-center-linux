// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/SysfsRoot.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

namespace acc::hal {

namespace {

bool writeMockFile(const QString& root, const QString& relPath, const QString& content) {
    const QString fullPath = QDir(root).filePath(relPath.startsWith(QLatin1Char('/')) ? relPath.mid(1) : relPath);
    QDir parentDir = QFileInfo(fullPath).dir();
    if (!parentDir.exists() && !parentDir.mkpath(QStringLiteral("."))) {
        return false;
    }
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(content.toUtf8());
    file.close();
    return true;
}

} // namespace

SysfsRoot& SysfsRoot::instance() {
    static SysfsRoot s_instance;
    return s_instance;
}

SysfsRoot::SysfsRoot() {
    const QString envRoot = qEnvironmentVariable("ACC_SYSFS_ROOT").trimmed();
    const QString envMode = qEnvironmentVariable("ACC_HARDWARE_MODE").trimmed().toLower();
    if (!envRoot.isEmpty()) {
        setCustomRootPath(envRoot, QStringLiteral("ACC_SYSFS_ROOT Fixture"));
    } else if (envMode == QLatin1String("mock_full")) {
        setMode(HardwareMode::MockFullCapabilities);
    } else if (envMode == QLatin1String("mock_mirror")) {
        setMode(HardwareMode::MockPhysicalMirror);
    }
}

HardwareMode SysfsRoot::mode() const {
    QMutexLocker locker(&m_mutex);
    return m_mode;
}

bool SysfsRoot::isMockActive() const {
    QMutexLocker locker(&m_mutex);
    return m_mode != HardwareMode::PhysicalSystem;
}

QString SysfsRoot::scenarioLabel() const {
    QMutexLocker locker(&m_mutex);
    return m_scenarioLabel;
}

void SysfsRoot::setMode(HardwareMode newMode) {
    QMutexLocker locker(&m_mutex);
    m_mode = newMode;
    if (newMode == HardwareMode::PhysicalSystem) {
        m_tempDir.reset();
        m_customRoot.clear();
        m_scenarioLabel = QStringLiteral("Physical Hardware (Acer Aspire A715-59G)");
        return;
    }

    if (newMode == HardwareMode::MockPhysicalMirror || newMode == HardwareMode::MockFullCapabilities) {
        m_tempDir = std::make_unique<QTemporaryDir>();
        if (m_tempDir->isValid()) {
            const bool includeOptional = (newMode == HardwareMode::MockFullCapabilities);
            populateMockTree(m_tempDir->path(), includeOptional);
            m_customRoot = m_tempDir->path();
            m_scenarioLabel = includeOptional
                                  ? QStringLiteral("Mock Hardware: Full Capabilities (RGB + PWM + Charge Limit)")
                                  : QStringLiteral("Mock Hardware: A715-59G / Kelilu_RTH Physical Mirror");
        }
    }
}

void SysfsRoot::setCustomRootPath(const QString& rootDir, const QString& label) {
    QMutexLocker locker(&m_mutex);
    m_mode = HardwareMode::CustomRootPath;
    m_tempDir.reset();
    m_customRoot = rootDir;
    m_scenarioLabel = label;
}

QString SysfsRoot::resolvePath(const QString& absoluteLinuxPath) const {
    QMutexLocker locker(&m_mutex);
    if (m_mode == HardwareMode::PhysicalSystem || m_customRoot.isEmpty()) {
        return absoluteLinuxPath;
    }
    const QString rel = absoluteLinuxPath.startsWith(QLatin1Char('/'))
                            ? absoluteLinuxPath.mid(1)
                            : absoluteLinuxPath;
    return QDir(m_customRoot).filePath(rel);
}

QString SysfsRoot::readFileTrimmed(const QString& fullPath, bool* outOk) {
    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (outOk) {
            *outOk = false;
        }
        return QString();
    }
    if (outOk) {
        *outOk = true;
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

bool SysfsRoot::writeFileVerified(const QString& fullPath, const QString& value, QString* outReadback, QString* outError) {
    QFile outFile(fullPath);
    if (!outFile.exists()) {
        if (outError) {
            *outError = QStringLiteral("Target sysfs path does not exist: %1").arg(fullPath);
        }
        return false;
    }
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) {
            *outError = QStringLiteral("Permission denied or open failed on '%1': %2").arg(fullPath, outFile.errorString());
        }
        return false;
    }
    const QByteArray payload = (value.trimmed() + QStringLiteral("\n")).toUtf8();
    if (outFile.write(payload) != payload.size()) {
        if (outError) {
            *outError = QStringLiteral("Incomplete write on '%1': %2").arg(fullPath, outFile.errorString());
        }
        return false;
    }
    outFile.flush();
    outFile.close();

    bool readOk = false;
    const QString readback = readFileTrimmed(fullPath, &readOk);
    if (outReadback) {
        *outReadback = readback;
    }
    if (!readOk) {
        if (outError) {
            *outError = QStringLiteral("Write succeeded but readback failed on '%1'").arg(fullPath);
        }
        return false;
    }
    if (readback.trimmed() != value.trimmed()) {
        if (outError) {
            *outError = QStringLiteral("Readback mismatch on '%1': wrote '%2', read '%3'")
                            .arg(fullPath, value.trimmed(), readback.trimmed());
        }
        return false;
    }
    return true;
}

bool SysfsRoot::populateMockTree(const QString& rootPath, bool includeOptionalControls) {
    // 1. DMI identity (A715-59G / Kelilu_RTH)
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/sys_vendor"), QStringLiteral("Acer\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/product_name"), QStringLiteral("Aspire A715-59G\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/product_version"), QStringLiteral("1.17\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/board_name"), QStringLiteral("RTH\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/board_vendor"), QStringLiteral("Kelilu_RTH\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/bios_vendor"), QStringLiteral("INSYDE Corp.\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/bios_version"), QStringLiteral("V1.17\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/bios_date"), QStringLiteral("06/30/2026\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/dmi/id/chassis_type"), QStringLiteral("10\n"));

    // 2. WMI GUIDs
    writeMockFile(rootPath, QStringLiteral("/sys/bus/wmi/devices/ABBC0F6B-8EA1-11D1-00A0-C90629100000-2/modalias"),
                  QStringLiteral("wmi:ABBC0F6B-8EA1-11D1-00A0-C90629100000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/wmi/devices/ABBC0F6C-8EA1-11D1-00A0-C90629100000-3/modalias"),
                  QStringLiteral("wmi:ABBC0F6C-8EA1-11D1-00A0-C90629100000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/wmi/devices/ABBC0F6D-8EA1-11D1-00A0-C90629100000-1/modalias"),
                  QStringLiteral("wmi:ABBC0F6D-8EA1-11D1-00A0-C90629100000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/wmi/devices/F6CB5C3C-9CAE-4EBD-B577-931EA32A2CC0-0/modalias"),
                  QStringLiteral("wmi:F6CB5C3C-9CAE-4EBD-B577-931EA32A2CC0\n"));

    // 3. /proc/cpuinfo, /proc/stat, /proc/meminfo, /proc/pressure
    writeMockFile(rootPath, QStringLiteral("/proc/cpuinfo"),
                  QStringLiteral("processor\t: 0\nmodel name\t: 13th Gen Intel(R) Core(TM) i7-13620H\ncpu MHz\t\t: 2850.000\n"));
    writeMockFile(rootPath, QStringLiteral("/proc/stat"),
                  QStringLiteral("cpu  142000 1200 48000 910000 4200 1800 900 0 0 0\n"
                                 "cpu0 18000 100 6000 90000 400 200 100 0 0 0\n"
                                 "cpu1 16000 100 5000 92000 400 200 100 0 0 0\n"
                                 "cpu2 14000 100 4500 94000 400 200 100 0 0 0\n"
                                 "cpu3 12000 100 4000 96000 400 200 100 0 0 0\n"));
    writeMockFile(rootPath, QStringLiteral("/proc/meminfo"),
                  QStringLiteral("MemTotal:       16040568 kB\n"
                                 "MemFree:         2441232 kB\n"
                                 "MemAvailable:    9642848 kB\n"
                                 "Buffers:          125176 kB\n"
                                 "Cached:          6825992 kB\n"));
    writeMockFile(rootPath, QStringLiteral("/proc/pressure/cpu"),
                  QStringLiteral("some avg10=1.25 avg60=1.10 avg300=0.95 total=8987412\nfull avg10=0.00 avg60=0.00 avg300=0.00 total=0\n"));
    writeMockFile(rootPath, QStringLiteral("/proc/pressure/memory"),
                  QStringLiteral("some avg10=0.05 avg60=0.02 avg300=0.01 total=9401\nfull avg10=0.00 avg60=0.00 avg300=0.00 total=9395\n"));
    writeMockFile(rootPath, QStringLiteral("/proc/pressure/io"),
                  QStringLiteral("some avg10=0.35 avg60=0.25 avg300=0.31 total=38852597\nfull avg10=0.21 avg60=0.19 avg300=0.24 total=34893607\n"));

    // 4. cpufreq & intel_pstate & RAPL
    for (int i = 0; i < 10; ++i) {
        const QString cpuBase = QStringLiteral("/sys/devices/system/cpu/cpu%1/cpufreq").arg(i);
        writeMockFile(rootPath, cpuBase + QStringLiteral("/scaling_driver"), QStringLiteral("intel_pstate\n"));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/scaling_governor"), QStringLiteral("powersave\n"));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/scaling_available_governors"), QStringLiteral("performance powersave\n"));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/energy_performance_preference"), QStringLiteral("balance_performance\n"));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/energy_performance_available_preferences"),
                      QStringLiteral("default performance balance_performance balance_power power\n"));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/scaling_cur_freq"), QStringLiteral("%1\n").arg(2400000 + i * 110000));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/cpuinfo_max_freq"), QStringLiteral("4900000\n"));
        writeMockFile(rootPath, cpuBase + QStringLiteral("/cpuinfo_min_freq"), QStringLiteral("400000\n"));
    }
    writeMockFile(rootPath, QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"), QStringLiteral("0\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/devices/system/cpu/intel_pstate/min_perf_pct"), QStringLiteral("15\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/devices/system/cpu/intel_pstate/max_perf_pct"), QStringLiteral("100\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/devices/system/cpu/intel_pstate/status"), QStringLiteral("active\n"));

    writeMockFile(rootPath, QStringLiteral("/sys/class/powercap/intel-rapl:0/name"), QStringLiteral("package-0\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/powercap/intel-rapl:0/energy_uj"), QStringLiteral("245000000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/powercap/intel-rapl:0:0/name"), QStringLiteral("core\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/powercap/intel-rapl:0:0/energy_uj"), QStringLiteral("185000000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/powercap/intel-rapl:1/name"), QStringLiteral("psys\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/powercap/intel-rapl:1/energy_uj"), QStringLiteral("320000000\n"));

    // 5. hwmon & thermal_zone
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon1/name"), QStringLiteral("acpitz\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon1/temp1_input"), QStringLiteral("48000\n"));

    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon3/name"), QStringLiteral("nvme\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon3/temp1_label"), QStringLiteral("Composite\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon3/temp1_input"), QStringLiteral("36850\n"));

    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon4/name"), QStringLiteral("nvme\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon4/temp1_label"), QStringLiteral("Composite\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon4/temp1_input"), QStringLiteral("39850\n"));

    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon5/name"), QStringLiteral("coretemp\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon5/temp1_label"), QStringLiteral("Package id 0\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon5/temp1_input"), QStringLiteral("51000\n"));
    for (int c = 0; c < 10; ++c) {
        const int idx = c + 2;
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon5/temp%1_label").arg(idx),
                      QStringLiteral("Core %1\n").arg(c));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon5/temp%1_input").arg(idx),
                      QStringLiteral("%1\n").arg((46 + (c % 5)) * 1000));
    }

    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon6/name"), QStringLiteral("iwlwifi_1\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon6/temp1_input"), QStringLiteral("41000\n"));

    writeMockFile(rootPath, QStringLiteral("/sys/class/thermal/thermal_zone2/type"), QStringLiteral("SEN2\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/thermal/thermal_zone2/temp"), QStringLiteral("42050\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/thermal/thermal_zone3/type"), QStringLiteral("SEN3\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/thermal/thermal_zone3/temp"), QStringLiteral("41050\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/thermal/thermal_zone4/type"), QStringLiteral("TCPU\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/thermal/thermal_zone4/temp"), QStringLiteral("51050\n"));

    // 6. Power supply (BAT0 & AC)
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/AC/type"), QStringLiteral("Mains\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/AC/online"), QStringLiteral("1\n"));

    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/type"), QStringLiteral("Battery\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/present"), QStringLiteral("1\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/status"), QStringLiteral("Charging\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/technology"), QStringLiteral("Li-ion\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/capacity"), QStringLiteral("72\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/voltage_now"), QStringLiteral("15420000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/current_now"), QStringLiteral("1850000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/charge_now"), QStringLiteral("1408000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/charge_full"), QStringLiteral("1956000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/charge_full_design"), QStringLiteral("3410000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/cycle_count"), QStringLiteral("42\n"));

    // 7. Display backlight
    writeMockFile(rootPath, QStringLiteral("/sys/class/backlight/intel_backlight/brightness"), QStringLiteral("24000\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/class/backlight/intel_backlight/max_brightness"), QStringLiteral("48000\n"));

    // 8. Optional simulated hardware controls for full-capability mock testing
    if (includeOptionalControls) {
        writeMockFile(rootPath, QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold"), QStringLiteral("80\n"));

        writeMockFile(rootPath, QStringLiteral("/sys/class/leds/acer::kbd_backlight/brightness"), QStringLiteral("80\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/leds/acer::kbd_backlight/max_brightness"), QStringLiteral("100\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/leds/acer::kbd_backlight/multi_index"), QStringLiteral("red green blue\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/leds/acer::kbd_backlight/multi_intensity"), QStringLiteral("59 158 255\n"));

        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/name"), QStringLiteral("acer_ec_fans\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/fan1_label"), QStringLiteral("CPU Blower\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/fan1_input"), QStringLiteral("2450\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/fan2_label"), QStringLiteral("GPU Blower\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/fan2_input"), QStringLiteral("2600\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/pwm1"), QStringLiteral("128\n"));
        writeMockFile(rootPath, QStringLiteral("/sys/class/hwmon/hwmon7/pwm1_enable"), QStringLiteral("2\n"));
    }

    return true;
}

} // namespace acc::hal
