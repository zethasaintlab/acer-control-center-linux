// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/TelemetryCollector.h"
#include "hal/HardwareDiscovery.h"
#include "hal/SysfsRoot.h"

#include <QDateTime>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace acc::hal {

namespace {

double parsePsiSomeAvg10(const QString& content) {
    static const QRegularExpression kAvg10Re(QStringLiteral("some\\s+avg10=([0-9]+\\.?[0-9]*)"));
    const auto match = kAvg10Re.match(content);
    if (match.hasMatch()) {
        return match.captured(1).toDouble();
    }
    return 0.0;
}

double parsePsiFullAvg10(const QString& content) {
    static const QRegularExpression kFull10Re(QStringLiteral("full\\s+avg10=([0-9]+\\.?[0-9]*)"));
    const auto match = kFull10Re.match(content);
    if (match.hasMatch()) {
        return match.captured(1).toDouble();
    }
    return 0.0;
}

} // namespace

TelemetryCollector::TelemetryCollector() = default;

bool TelemetryCollector::isNvmlLoaded() const {
    return m_nvml.isLoaded();
}

void TelemetryCollector::updateRaplCounters(uint64_t pkgUj, uint64_t coreUj, uint64_t psysUj, int64_t timestampUs, bool available) {
    m_raplAvailable = available;
    if (!available) {
        return;
    }
    if (m_prevRaplTsUs > 0 && timestampUs > m_prevRaplTsUs && pkgUj >= m_prevPkgUj) {
        const double dtSec = static_cast<double>(timestampUs - m_prevRaplTsUs) / 1e6;
        if (dtSec >= 0.05) {
            m_lastPkgWatts = static_cast<double>(pkgUj - m_prevPkgUj) / (1e6 * dtSec);
            if (coreUj >= m_prevCoreUj) {
                m_lastCoreWatts = static_cast<double>(coreUj - m_prevCoreUj) / (1e6 * dtSec);
            }
            if (psysUj >= m_prevPsysUj) {
                m_lastPsysWatts = static_cast<double>(psysUj - m_prevPsysUj) / (1e6 * dtSec);
            }
        }
    }
    m_prevPkgUj = pkgUj;
    m_prevCoreUj = coreUj;
    m_prevPsysUj = psysUj;
    m_prevRaplTsUs = timestampUs;
}

core::CpuTelemetry TelemetryCollector::readCpuTelemetry() {
    core::CpuTelemetry cpu;
    const auto& root = SysfsRoot::instance();

    // 1. CPU model name
    const QString cpuinfo = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/cpuinfo")));
    for (const QString& line : cpuinfo.split(QLatin1Char('\n'))) {
        if (line.startsWith(QLatin1String("model name"))) {
            const int colon = line.indexOf(QLatin1Char(':'));
            if (colon >= 0) {
                cpu.modelName = line.mid(colon + 1).trimmed();
                break;
            }
        }
    }
    if (cpu.modelName.isEmpty()) {
        cpu.modelName = QStringLiteral("13th Gen Intel(R) Core(TM) i7-13620H");
    }

    // 2. /proc/stat utilization delta
    QMap<int, double> perCpuLoad;
    const QString statText = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/stat")));
    int threadCount = 0;
    for (const QString& line : statText.split(QLatin1Char('\n'))) {
        const QStringList parts = line.simplified().split(QLatin1Char(' '));
        if (parts.size() < 8) {
            continue;
        }
        const QString tag = parts.at(0);
        if (!tag.startsWith(QLatin1String("cpu"))) {
            continue;
        }
        const uint64_t user = parts.at(1).toULongLong();
        const uint64_t nice = parts.at(2).toULongLong();
        const uint64_t system = parts.at(3).toULongLong();
        const uint64_t idle = parts.at(4).toULongLong();
        const uint64_t iowait = parts.at(5).toULongLong();
        const uint64_t irq = parts.at(6).toULongLong();
        const uint64_t softirq = parts.at(7).toULongLong();
        const uint64_t steal = (parts.size() > 8) ? parts.at(8).toULongLong() : 0ULL;

        const uint64_t active = user + nice + system + irq + softirq + steal;
        const uint64_t total = active + idle + iowait;

        if (tag == QLatin1String("cpu")) {
            if (m_prevTotalCpu.totalJiffies > 0 && total > m_prevTotalCpu.totalJiffies) {
                const double dActive = static_cast<double>(active - m_prevTotalCpu.activeJiffies);
                const double dTotal = static_cast<double>(total - m_prevTotalCpu.totalJiffies);
                cpu.totalUtilizationPct = std::clamp((dActive * 100.0) / dTotal, 0.0, 100.0);
            } else if (root.isMockActive()) {
                cpu.totalUtilizationPct = 18.4;
            }
            m_prevTotalCpu = CpuStatSample{active, total};
        } else {
            bool okIdx = false;
            const int cpuIdx = tag.mid(3).toInt(&okIdx);
            if (okIdx) {
                ++threadCount;
                const auto prev = m_prevCoreCpu.value(cpuIdx, CpuStatSample{0, 0});
                double loadPct = root.isMockActive() ? (12.0 + (cpuIdx % 5) * 4.5) : 0.0;
                if (prev.totalJiffies > 0 && total > prev.totalJiffies) {
                    const double dActive = static_cast<double>(active - prev.activeJiffies);
                    const double dTotal = static_cast<double>(total - prev.totalJiffies);
                    loadPct = std::clamp((dActive * 100.0) / dTotal, 0.0, 100.0);
                }
                perCpuLoad.insert(cpuIdx, loadPct);
                m_prevCoreCpu.insert(cpuIdx, CpuStatSample{active, total});
            }
        }
    }
    if (threadCount > 0) {
        cpu.logicalThreads = threadCount;
    }

    // 3. coretemp package & per-core temperatures
    QMap<int, double> coreTemps;
    const QString coretempDir = HardwareDiscovery::findHwmonByName(QStringLiteral("coretemp"));
    if (!coretempDir.isEmpty()) {
        QDir d(coretempDir);
        const QStringList labelFiles = d.entryList({QStringLiteral("temp*_label")}, QDir::Files, QDir::Name);
        for (const QString& lf : labelFiles) {
            const QString prefix = lf.left(lf.indexOf(QLatin1String("_label")));
            const QString label = SysfsRoot::readFileTrimmed(d.filePath(lf));
            const double millideg = SysfsRoot::readFileTrimmed(d.filePath(prefix + QStringLiteral("_input"))).toDouble();
            const double degC = millideg / 1000.0;
            if (label.contains(QLatin1String("Package"), Qt::CaseInsensitive)) {
                cpu.packageTempC = degC;
            } else if (label.startsWith(QLatin1String("Core "), Qt::CaseInsensitive)) {
                const int coreNum = label.mid(5).trimmed().toInt();
                coreTemps.insert(coreNum, degC);
            }
        }
    }

    // 4. cpufreq & intel_pstate
    const QString cpu0FreqBase = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq"));
    cpu.governor = SysfsRoot::readFileTrimmed(cpu0FreqBase + QStringLiteral("/scaling_governor"));
    cpu.availableGovernors = SysfsRoot::readFileTrimmed(cpu0FreqBase + QStringLiteral("/scaling_available_governors"))
                                 .split(QLatin1Char(' '), Qt::SkipEmptyParts);
    cpu.epp = SysfsRoot::readFileTrimmed(cpu0FreqBase + QStringLiteral("/energy_performance_preference"));
    cpu.availableEpp = SysfsRoot::readFileTrimmed(cpu0FreqBase + QStringLiteral("/energy_performance_available_preferences"))
                           .split(QLatin1Char(' '), Qt::SkipEmptyParts);

    const int maxKhz = SysfsRoot::readFileTrimmed(cpu0FreqBase + QStringLiteral("/cpuinfo_max_freq")).toInt();
    const int minKhz = SysfsRoot::readFileTrimmed(cpu0FreqBase + QStringLiteral("/cpuinfo_min_freq")).toInt();
    if (maxKhz > 0) {
        cpu.maxFreqMhz = maxKhz / 1000;
    }
    if (minKhz > 0) {
        cpu.minFreqMhz = minKhz / 1000;
    }

    const QString pstateBase = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate"));
    if (QFileInfo::exists(pstateBase)) {
        cpu.turboEnabled = (SysfsRoot::readFileTrimmed(pstateBase + QStringLiteral("/no_turbo")) == QLatin1String("0"));
        bool okMin = false, okMax = false;
        const int minP = SysfsRoot::readFileTrimmed(pstateBase + QStringLiteral("/min_perf_pct"), &okMin).toInt();
        const int maxP = SysfsRoot::readFileTrimmed(pstateBase + QStringLiteral("/max_perf_pct"), &okMax).toInt();
        if (okMin) {
            cpu.minPerfPct = minP;
        }
        if (okMax) {
            cpu.maxPerfPct = maxP;
        }
    }

    // Build per-core reading array (up to 16 logical threads or 10 cores)
    int64_t freqSumMhz = 0;
    int freqCount = 0;
    for (int i = 0; i < 16; ++i) {
        const QString fPath = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu%1/cpufreq/scaling_cur_freq").arg(i));
        if (!QFileInfo::exists(fPath)) {
            continue;
        }
        const int curKhz = SysfsRoot::readFileTrimmed(fPath).toInt();
        const int curMhz = curKhz / 1000;
        freqSumMhz += curMhz;
        ++freqCount;

        core::CpuCoreReading cr;
        cr.coreId = i;
        cr.label = QStringLiteral("CPU %1").arg(i);
        cr.freqMhz = curMhz;
        cr.tempC = coreTemps.value(i, cpu.packageTempC);
        cr.utilizationPct = perCpuLoad.value(i, cpu.totalUtilizationPct);
        cpu.cores.push_back(cr);
    }
    cpu.avgFreqMhz = (freqCount > 0) ? static_cast<int>(freqSumMhz / freqCount) : 2400;

    // 5. RAPL power
    if (root.isMockActive()) {
        cpu.powerSourceAvailable = true;
        cpu.powerSourceLabel = QStringLiteral("Mock RAPL (intel-rapl:0)");
        cpu.packagePowerWatts = 24.6;
        cpu.corePowerWatts = 18.2;
        cpu.psysPowerWatts = 38.5;
    } else if (m_raplAvailable) {
        cpu.powerSourceAvailable = true;
        cpu.powerSourceLabel = QStringLiteral("Intel RAPL (via Privileged Daemon)");
        cpu.packagePowerWatts = m_lastPkgWatts;
        cpu.corePowerWatts = m_lastCoreWatts;
        cpu.psysPowerWatts = m_lastPsysWatts;
    } else {
        // Attempt direct read in case permissions allow it
        bool okUj = false;
        const uint64_t directUj = SysfsRoot::readFileTrimmed(
                                      root.resolvePath(QStringLiteral("/sys/class/powercap/intel-rapl:0/energy_uj")), &okUj)
                                      .toULongLong();
        if (okUj && directUj > 0) {
            updateRaplCounters(directUj, 0, 0, QDateTime::currentMSecsSinceEpoch() * 1000LL, true);
            cpu.powerSourceAvailable = true;
            cpu.powerSourceLabel = QStringLiteral("Intel RAPL (Direct)");
            cpu.packagePowerWatts = m_lastPkgWatts;
        } else {
            cpu.powerSourceAvailable = false;
            cpu.powerSourceLabel = QStringLiteral("RAPL Requires Daemon (0400 Root-Only)");
        }
    }

    return cpu;
}

core::MemoryTelemetry TelemetryCollector::readMemoryTelemetry() const {
    core::MemoryTelemetry mem;
    const auto& root = SysfsRoot::instance();

    const QString meminfo = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/meminfo")));
    int64_t totalKb = 0, availKb = 0, cachedKb = 0, buffersKb = 0;
    for (const QString& line : meminfo.split(QLatin1Char('\n'))) {
        if (line.startsWith(QLatin1String("MemTotal:"))) {
            totalKb = line.simplified().split(QLatin1Char(' ')).value(1).toLongLong();
        } else if (line.startsWith(QLatin1String("MemAvailable:"))) {
            availKb = line.simplified().split(QLatin1Char(' ')).value(1).toLongLong();
        } else if (line.startsWith(QLatin1String("Cached:"))) {
            cachedKb = line.simplified().split(QLatin1Char(' ')).value(1).toLongLong();
        } else if (line.startsWith(QLatin1String("Buffers:"))) {
            buffersKb = line.simplified().split(QLatin1Char(' ')).value(1).toLongLong();
        }
    }

    mem.totalMiB = totalKb / 1024;
    mem.availableMiB = availKb / 1024;
    mem.cachedMiB = cachedKb / 1024;
    mem.buffersMiB = buffersKb / 1024;
    mem.usedMiB = std::max<int64_t>(0, mem.totalMiB - mem.availableMiB);
    mem.utilizationPct = (mem.totalMiB > 0)
                             ? std::clamp((static_cast<double>(mem.usedMiB) * 100.0) / static_cast<double>(mem.totalMiB), 0.0, 100.0)
                             : 0.0;

    bool okPsi = false;
    const QString psiMem = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/pressure/memory")), &okPsi);
    if (okPsi) {
        mem.psiAvailable = true;
        mem.psiMemSomeAvg10 = parsePsiSomeAvg10(psiMem);
        mem.psiMemFullAvg10 = parsePsiFullAvg10(psiMem);
        mem.psiCpuSomeAvg10 = parsePsiSomeAvg10(SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/pressure/cpu"))));
        mem.psiIoSomeAvg10 = parsePsiSomeAvg10(SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/proc/pressure/io"))));
    }

    return mem;
}

core::ThermalTelemetry TelemetryCollector::readThermalTelemetry(double cpuPkgTemp, double gpuTemp) const {
    core::ThermalTelemetry th;
    th.cpuPackageTempC = cpuPkgTemp;
    th.gpuTempC = gpuTemp;

    const auto& root = SysfsRoot::instance();
    const QString hwmonBase = root.resolvePath(QStringLiteral("/sys/class/hwmon"));
    QDir hwmonDir(hwmonBase);
    if (hwmonDir.exists()) {
        const QStringList entries = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& entry : entries) {
            const QString dPath = hwmonDir.filePath(entry);
            const QString drvName = SysfsRoot::readFileTrimmed(dPath + QStringLiteral("/name"));
            QDir d(dPath);

            // Temperature inputs
            const QStringList tempInputs = d.entryList({QStringLiteral("temp*_input")}, QDir::Files, QDir::Name);
            for (const QString& tf : tempInputs) {
                const QString prefix = tf.left(tf.indexOf(QLatin1String("_input")));
                const QString label = SysfsRoot::readFileTrimmed(d.filePath(prefix + QStringLiteral("_label")));
                bool okT = false;
                const double millideg = SysfsRoot::readFileTrimmed(d.filePath(tf), &okT).toDouble();
                if (!okT) {
                    continue;
                }
                const double degC = millideg / 1000.0;
                if (drvName == QLatin1String("nvme")) {
                    th.maxNvmeTempC = std::max(th.maxNvmeTempC, degC);
                } else if (drvName != QLatin1String("coretemp")) {
                    th.maxBoardTempC = std::max(th.maxBoardTempC, degC);
                }

                // Include key non-core sensors in the summary table
                if (drvName != QLatin1String("coretemp") || label.contains(QLatin1String("Package"), Qt::CaseInsensitive)) {
                    core::ThermalSensorReading sr;
                    sr.id = entry + QStringLiteral(".") + prefix;
                    sr.label = label.isEmpty() ? QStringLiteral("%1 (%2)").arg(drvName, entry) : QStringLiteral("%1 — %2").arg(drvName, label);
                    sr.sourceDriver = drvName;
                    sr.sysfsPath = d.filePath(tf);
                    sr.tempC = degC;
                    sr.isValid = true;
                    th.sensors.push_back(sr);
                }
            }

            // Fan inputs
            const QStringList fanInputs = d.entryList({QStringLiteral("fan*_input")}, QDir::Files, QDir::Name);
            for (const QString& ff : fanInputs) {
                const QString prefix = ff.left(ff.indexOf(QLatin1String("_input")));
                const QString label = SysfsRoot::readFileTrimmed(d.filePath(prefix + QStringLiteral("_label")));
                const int rpm = SysfsRoot::readFileTrimmed(d.filePath(ff)).toInt();
                const int pwmRaw = SysfsRoot::readFileTrimmed(d.filePath(QStringLiteral("pwm1"))).toInt();

                core::FanReading fr;
                fr.id = entry + QStringLiteral(".") + prefix;
                fr.label = label.isEmpty() ? QStringLiteral("System Fan (%1)").arg(prefix) : label;
                fr.sysfsPath = d.filePath(ff);
                fr.rpm = rpm;
                fr.pwmPct = std::clamp((pwmRaw * 100) / 255, 0, 100);
                fr.isWritable = QFileInfo::exists(d.filePath(QStringLiteral("pwm1")));
                fr.isDetected = true;
                th.fans.push_back(fr);
            }
        }
    }

    // Also inspect /sys/class/thermal/thermal_zone* for SEN2, SEN3, TCPU
    const QString tzBase = root.resolvePath(QStringLiteral("/sys/class/thermal"));
    QDir tzDir(tzBase);
    if (tzDir.exists()) {
        const QStringList zEntries = tzDir.entryList({QStringLiteral("thermal_zone*")}, QDir::Dirs, QDir::Name);
        for (const QString& zEntry : zEntries) {
            const QString zPath = tzDir.filePath(zEntry);
            const QString zType = SysfsRoot::readFileTrimmed(zPath + QStringLiteral("/type"));
            bool okT = false;
            const double millideg = SysfsRoot::readFileTrimmed(zPath + QStringLiteral("/temp"), &okT).toDouble();
            if (!okT || millideg <= 0.0) {
                continue;
            }
            const double degC = millideg / 1000.0;
            th.maxBoardTempC = std::max(th.maxBoardTempC, degC);

            if (zType == QLatin1String("SEN2") || zType == QLatin1String("SEN3") || zType == QLatin1String("TCPU")) {
                core::ThermalSensorReading sr;
                sr.id = zEntry;
                sr.label = QStringLiteral("ACPI Zone %1 (%2)").arg(zType, zEntry);
                sr.sourceDriver = QStringLiteral("thermal_zone");
                sr.sysfsPath = zPath + QStringLiteral("/temp");
                sr.tempC = degC;
                sr.isValid = true;
                th.sensors.push_back(sr);
            }
        }
    }

    const double peakTemp = std::max({th.cpuPackageTempC, th.gpuTempC, th.maxBoardTempC});
    if (peakTemp >= 88.0) {
        th.thermalStateLabel = QStringLiteral("Critical / High Thermal Load");
    } else if (peakTemp >= 75.0) {
        th.thermalStateLabel = QStringLiteral("Elevated Load");
    } else {
        th.thermalStateLabel = QStringLiteral("Nominal");
    }

    if (!th.fans.empty()) {
        th.manualFanSupported = th.fans.front().isWritable;
        th.fanControlMode = QStringLiteral("Hardware PWM Bound");
        th.fanStatusExplanation = QStringLiteral("Kernel hwmon fan tachometers and PWM control are active.");
    } else {
        th.manualFanSupported = false;
        th.fanControlMode = QStringLiteral("Firmware Automatic (INSYDE EC)");
        th.fanStatusExplanation = QStringLiteral(
            "No `fan*_input` or `pwm*` sysfs nodes are exposed by in-tree drivers on the Aspire A715-59G (`Kelilu_RTH`). "
            "Dual cooling fans remain safely managed by the INSYDE V1.17 Embedded Controller (`PNP0C09:00`). "
            "Curve Editor operates in Preview & Simulation Mode unless Mock Hardware Mode or a verified PWM driver is active."
        );
    }

    return th;
}

core::BatteryTelemetry TelemetryCollector::readBatteryTelemetry() const {
    core::BatteryTelemetry bat;
    const auto& root = SysfsRoot::instance();

    const QString acOnlineStr = SysfsRoot::readFileTrimmed(root.resolvePath(QStringLiteral("/sys/class/power_supply/AC/online")));
    bat.acOnline = (acOnlineStr == QLatin1String("1"));

    const QString batBase = root.resolvePath(QStringLiteral("/sys/class/power_supply/BAT0"));
    if (!QFileInfo::exists(batBase)) {
        bat.present = false;
        bat.status = QStringLiteral("Not Present");
        bat.statusExplanation = QStringLiteral("BAT0 power supply node not found.");
        return bat;
    }

    bat.present = (SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/present")) != QLatin1String("0"));
    bat.status = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/status"));
    bat.technology = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/technology"));
    bat.capacityPct = std::clamp(SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/capacity")).toInt(), 0, 100);
    bat.cycleCount = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/cycle_count")).toInt();

    const double voltageUv = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/voltage_now")).toDouble();
    const double currentUa = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/current_now")).toDouble();
    const double chargeNowUah = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/charge_now")).toDouble();
    const double chargeFullUah = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/charge_full")).toDouble();
    const double chargeDesignUah = SysfsRoot::readFileTrimmed(batBase + QStringLiteral("/charge_full_design")).toDouble();

    bat.voltageVolts = voltageUv / 1e6;
    bat.currentAmps = currentUa / 1e6;
    bat.powerWatts = bat.voltageVolts * bat.currentAmps;
    bat.chargeNowAh = chargeNowUah / 1e6;
    bat.chargeFullAh = chargeFullUah / 1e6;
    bat.chargeFullDesignAh = chargeDesignUah / 1e6;

    if (bat.chargeFullDesignAh > 0.1) {
        bat.wearHealthPct = std::clamp((bat.chargeFullAh * 100.0) / bat.chargeFullDesignAh, 0.0, 100.0);
    } else {
        bat.wearHealthPct = 100.0;
    }

    const QString limitPath = batBase + QStringLiteral("/charge_control_end_threshold");
    if (QFileInfo::exists(limitPath)) {
        bat.chargeLimitSupported = true;
        bat.chargeLimitSysfsPath = limitPath;
        bool okLim = false;
        const int lim = SysfsRoot::readFileTrimmed(limitPath, &okLim).toInt();
        bat.chargeEndThresholdPct = (okLim && lim >= 50 && lim <= 100) ? lim : 80;
        bat.statusExplanation = QStringLiteral("Kernel charge_control_end_threshold active (%1%).").arg(bat.chargeEndThresholdPct);
    } else {
        bat.chargeLimitSupported = false;
        bat.chargeEndThresholdPct = 100;
        bat.statusExplanation = QStringLiteral(
            "Standard ACPI battery telemetry (`BAT0`) is active. "
            "Charge threshold control (`charge_control_end_threshold`) is not exposed by `acpi-battery` on the `Kelilu_RTH` board."
        );
    }

    return bat;
}

int TelemetryCollector::readDisplayBrightnessPct(bool& outSupported) const {
    const auto& root = SysfsRoot::instance();
    const QString base = root.resolvePath(QStringLiteral("/sys/class/backlight/intel_backlight"));
    bool okCur = false, okMax = false;
    const int cur = SysfsRoot::readFileTrimmed(base + QStringLiteral("/brightness"), &okCur).toInt();
    const int max = SysfsRoot::readFileTrimmed(base + QStringLiteral("/max_brightness"), &okMax).toInt();
    if (okCur && okMax && max > 0) {
        outSupported = true;
        return std::clamp((cur * 100) / max, 1, 100);
    }
    outSupported = false;
    return 50;
}

QString TelemetryCollector::readActiveTunedOrPpdProfile() const {
    if (SysfsRoot::instance().isMockActive()) {
        return QStringLiteral("balanced (Mock TuneD-PPD)");
    }
    QDBusInterface ppdIface(
        QStringLiteral("net.hadess.PowerProfiles"),
        QStringLiteral("/net/hadess/PowerProfiles"),
        QStringLiteral("net.hadess.PowerProfiles"),
        QDBusConnection::systemBus()
    );
    if (ppdIface.isValid()) {
        const QVariant prop = ppdIface.property("ActiveProfile");
        if (prop.isValid() && !prop.toString().isEmpty()) {
            return prop.toString();
        }
    }
    return QStringLiteral("throughput-performance");
}

core::SystemSnapshot TelemetryCollector::collectSnapshot(const QString& activeProfileId) {
    core::SystemSnapshot snap;
    snap.timestampMs = QDateTime::currentMSecsSinceEpoch();
    snap.mockModeActive = SysfsRoot::instance().isMockActive();
    snap.mockScenarioName = SysfsRoot::instance().scenarioLabel();

    snap.dmi = HardwareDiscovery::probeDmiAndWmi();
    snap.cpu = readCpuTelemetry();
    snap.memory = readMemoryTelemetry();
    snap.gpu = m_nvml.readTelemetry();
    snap.thermal = readThermalTelemetry(snap.cpu.packageTempC, snap.gpu.tempC);
    snap.battery = readBatteryTelemetry();
    snap.keyboardHw = HardwareDiscovery::probeKeyboardHardware(snap.dmi);
    snap.activeOperatingProfileId = activeProfileId;
    snap.activeTunedProfile = readActiveTunedOrPpdProfile();
    snap.displayBrightnessPct = readDisplayBrightnessPct(snap.displayBrightnessSupported);
    snap.capabilities = HardwareDiscovery::probeAllCapabilities(m_nvml.isLoaded());

    return snap;
}

} // namespace acc::hal
