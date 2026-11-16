// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/SysfsRoot.h"
#include "hal/Clv0001KeyboardBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <array>
#include <unistd.h>

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

bool validateSysfsWriteTarget(
    const QString& fullPath,
    const QString& requiredExactFile,
    QString* outCanonicalTarget,
    QString* outError
) {
    if (fullPath.contains(QStringLiteral(".."))) {
        if (outError) {
            *outError = QStringLiteral("Path traversal rejected on '%1'").arg(fullPath);
        }
        return false;
    }

    if (!QFileInfo::exists(fullPath)) {
        if (outError) {
            *outError = QStringLiteral("Target sysfs path does not exist: %1").arg(fullPath);
        }
        return false;
    }

    const QString canonicalTarget = QFileInfo(fullPath).canonicalFilePath();
    if (canonicalTarget.isEmpty()) {
        if (outError) {
            *outError = QStringLiteral("Cannot resolve canonical path for '%1'").arg(fullPath);
        }
        return false;
    }
    if (outCanonicalTarget) {
        *outCanonicalTarget = canonicalTarget;
    }

    const SysfsRoot& root = SysfsRoot::instance();
    QString logicalCheckPath = QDir::cleanPath(fullPath);
    QString canonicalCheckPath = canonicalTarget;
    if (root.isMockActive()) {
        const QString canonicalMockRoot = QFileInfo(root.resolvePath(QStringLiteral("/"))).canonicalFilePath();
        if (canonicalMockRoot.isEmpty() || !canonicalTarget.startsWith(canonicalMockRoot + QLatin1Char('/'))) {
            if (outError) {
                *outError = QStringLiteral("Refused write outside active mock sysfs root: '%1'").arg(canonicalTarget);
            }
            return false;
        }
        canonicalCheckPath = canonicalTarget.mid(canonicalMockRoot.size());
        const QString resolvedMockRoot = QDir::cleanPath(root.resolvePath(QStringLiteral("/")));
        if (logicalCheckPath.startsWith(resolvedMockRoot + QLatin1Char('/'))) {
            logicalCheckPath = logicalCheckPath.mid(resolvedMockRoot.size());
        } else if (logicalCheckPath.startsWith(canonicalMockRoot + QLatin1Char('/'))) {
            logicalCheckPath = logicalCheckPath.mid(canonicalMockRoot.size());
        }
    }

    if (!requiredExactFile.isEmpty()) {
        const bool exactLogicalMatch =
            (logicalCheckPath == QStringLiteral("/sys/bus/acpi/devices/CLV0001:00/") + requiredExactFile ||
             logicalCheckPath == QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/") + requiredExactFile);
        const QString expectedCanonical =
            QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/") + requiredExactFile;
        if (!exactLogicalMatch || canonicalCheckPath != expectedCanonical) {
            if (outError) {
                *outError = QStringLiteral("Refused sysfs command write outside CLV0001 allowlist: '%1' -> '%2'")
                                .arg(fullPath, canonicalTarget);
            }
            return false;
        }
        return true;
    }

    const bool allowedLogicalPrefix =
        logicalCheckPath.startsWith(QStringLiteral("/sys/devices/system/cpu/intel_pstate/")) ||
        logicalCheckPath.startsWith(QStringLiteral("/sys/devices/system/cpu/cpu")) ||
        logicalCheckPath.startsWith(QStringLiteral("/sys/devices/system/cpu/cpufreq/")) ||
        logicalCheckPath.startsWith(QStringLiteral("/sys/class/leds/")) ||
        logicalCheckPath == QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold") ||
        logicalCheckPath.startsWith(QStringLiteral("/sys/class/hwmon/")) ||
        logicalCheckPath == QStringLiteral("/sys/class/backlight/intel_backlight/brightness") ||
        logicalCheckPath == QStringLiteral("/sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_command") ||
        logicalCheckPath == QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command");
    const bool validCanonicalSys = root.isMockActive()
                                       ? canonicalCheckPath.startsWith(QStringLiteral("/sys/"))
                                       : canonicalCheckPath.startsWith(QStringLiteral("/sys/devices/"));
    if (!allowedLogicalPrefix || !validCanonicalSys) {
        if (outError) {
            *outError = QStringLiteral("Refused write to non-allowlisted physical path: '%1' (canonical '%2')")
                            .arg(fullPath, canonicalTarget);
        }
        return false;
    }
    return true;
}

} // namespace

SysfsRoot& SysfsRoot::instance() {
    static SysfsRoot s_instance;
    return s_instance;
}

SysfsRoot::SysfsRoot() {
    // Never honor environment variable root redirection when executing as root (e.g. acer-control-daemon).
    if (::geteuid() != 0) {
        const QString envRoot = qEnvironmentVariable("ACC_SYSFS_ROOT").trimmed();
        const QString envMode = qEnvironmentVariable("ACC_HARDWARE_MODE").trimmed().toLower();
        if (!envRoot.isEmpty()) {
            setCustomRootPath(envRoot, QStringLiteral("ACC_SYSFS_ROOT Fixture"));
        } else if (envMode == QLatin1String("mock_full")) {
            setMode(HardwareMode::MockFullCapabilities);
        } else if (envMode == QLatin1String("mock_clv0001")) {
            setMode(HardwareMode::MockClv0001DriverActive);
        } else if (envMode == QLatin1String("mock_mirror")) {
            setMode(HardwareMode::MockPhysicalMirror);
        }
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

    if (newMode == HardwareMode::MockPhysicalMirror ||
        newMode == HardwareMode::MockClv0001DriverActive ||
        newMode == HardwareMode::MockFullCapabilities) {
        m_tempDir = std::make_unique<QTemporaryDir>();
        if (m_tempDir->isValid()) {
            const bool includeOptional = (newMode == HardwareMode::MockFullCapabilities);
            const bool includeClvDriver = (newMode == HardwareMode::MockClv0001DriverActive);
            populateMockTree(m_tempDir->path(), includeOptional, includeClvDriver);
            m_customRoot = m_tempDir->path();
            if (newMode == HardwareMode::MockClv0001DriverActive) {
                m_scenarioLabel = QStringLiteral("Mock Hardware: A715-59G with CLV0001 Driver Bound");
            } else if (includeOptional) {
                m_scenarioLabel = QStringLiteral("Mock Hardware: Full Capabilities (RGB + PWM + Charge Limit)");
            } else {
                m_scenarioLabel = QStringLiteral("Mock Hardware: A715-59G / Kelilu_RTH Physical Mirror (CLV0001 Unbound)");
            }
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
    if (!validateSysfsWriteTarget(fullPath, QString(), nullptr, outError)) {
        return false;
    }

    QFile outFile(fullPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) {
            *outError = QStringLiteral("Permission denied or open failed on '%1': %2").arg(fullPath, outFile.errorString());
        }
        return false;
    }
    const QByteArray payload = (value.trimmed() + QStringLiteral("\n")).toUtf8();
    if (outFile.write(payload) != payload.size() || !outFile.flush()) {
        if (outError) {
            *outError = QStringLiteral("Incomplete write on '%1': %2").arg(fullPath, outFile.errorString());
        }
        return false;
    }
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

bool SysfsRoot::writeCommandAndReadState(
    const QString& commandPath,
    const QString& statePath,
    const QString& commandValue,
    QString* outStateText,
    QString* outError
) {
    QString canonicalCmd;
    if (!validateSysfsWriteTarget(commandPath, QStringLiteral("clv0001_kbd_command"), &canonicalCmd, outError)) {
        return false;
    }
    if (statePath.contains(QStringLiteral("..")) || !QFileInfo::exists(statePath)) {
        if (outError) {
            *outError = QStringLiteral("Invalid or missing state sysfs node '%1'").arg(statePath);
        }
        return false;
    }
    const QString canonicalState = QFileInfo(statePath).canonicalFilePath();
    if (canonicalState.isEmpty() ||
        QFileInfo(canonicalState).dir().canonicalPath() != QFileInfo(canonicalCmd).dir().canonicalPath() ||
        QFileInfo(canonicalState).fileName() != QLatin1String("clv0001_kbd_state")) {
        if (outError) {
            *outError = QStringLiteral("State sysfs path mismatch: '%1'").arg(statePath);
        }
        return false;
    }

    QFile cmdFile(commandPath);
    if (!cmdFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered)) {
        if (outError) {
            *outError = QStringLiteral("Permission denied or open failed on '%1': %2").arg(commandPath, cmdFile.errorString());
        }
        return false;
    }
    const QByteArray payload = (commandValue.trimmed() + QStringLiteral("\n")).toUtf8();
    const bool writtenOk = (cmdFile.write(payload) == payload.size()) && cmdFile.flush();
    const QString writeErrStr = cmdFile.errorString();
    cmdFile.close();

    bool simulatedFailure = false;
    if (SysfsRoot::instance().isMockActive()) {
        const QStringList parts = commandValue.trimmed().simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const QString preset = parts.isEmpty() ? QString() : parts.at(0).toLower();
        int level = (preset == QLatin1String("off")) ? 0 : 4;
        if (parts.size() >= 2) {
            bool okLvl = false;
            const int parsed = parts.at(1).toInt(&okLvl);
            if (!okLvl) {
                if (outError) {
                    *outError = QStringLiteral("Invalid brightness token in mock command '%1'").arg(commandValue);
                }
                return false;
            }
            level = parsed;
        }
        if (preset == QLatin1String("off")) {
            level = 0;
        }
        std::array<uint8_t, 4> pkt{};
        QString encErr;
        if (!Clv0001KeyboardBackend::buildFirmwarePayload(preset, level, pkt, &encErr)) {
            if (outError) {
                *outError = encErr;
            }
            return false;
        }
        const QString pktHex = Clv0001KeyboardBackend::formatPayloadHex(pkt);
        const QString devDir = QFileInfo(canonicalState).dir().absolutePath();
        const QString behavior = readFileTrimmed(devDir + QStringLiteral("/mock_eval_behavior"));

        QString status = QStringLiteral("accepted_no_hw_readback");
        QString retType = QStringLiteral("buffer");
        if (behavior == QLatin1String("eval_fail")) {
            status = QStringLiteral("error:acpi_eval_failed");
            retType = QStringLiteral("null");
            simulatedFailure = true;
            if (outError) {
                *outError = QStringLiteral("Simulated ACPI _DSM evaluation failure (-EIO)");
            }
        } else if (behavior == QLatin1String("unexpected_return")) {
            status = QStringLiteral("error:unexpected_return_type");
            retType = QStringLiteral("unexpected");
            simulatedFailure = true;
            if (outError) {
                *outError = QStringLiteral("Simulated ACPI _DSM unexpected return type (-EIO)");
            }
        } else if (behavior == QLatin1String("rate_limited")) {
            status = QStringLiteral("error:rate_limited");
            retType = QStringLiteral("none");
            simulatedFailure = true;
            if (outError) {
                *outError = QStringLiteral("Simulated kernel driver rate limit (-EBUSY)");
            }
        } else if (behavior == QLatin1String("state_mismatch")) {
            status = QStringLiteral("accepted_no_hw_readback");
            retType = QStringLiteral("buffer");
            QFile stFile(statePath);
            if (stFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                stFile.write("status=accepted_no_hw_readback\npreset=off\nbrightness_level=0\npayload_grb=00 00 00 F0\nacpi_return_type=buffer\nphysical_readback=unsupported\n");
                stFile.close();
            }
            bool readOk = false;
            if (outStateText) {
                *outStateText = readFileTrimmed(statePath, &readOk);
            }
            return writtenOk && readOk;
        }

        const QString mockState = QStringLiteral(
                                      "status=%1\n"
                                      "preset=%2\n"
                                      "brightness_level=%3\n"
                                      "payload_grb=%4\n"
                                      "acpi_return_type=%5\n"
                                      "physical_readback=unsupported\n")
                                      .arg(status, preset)
                                      .arg(level)
                                      .arg(pktHex, retType);
        QFile stFile(statePath);
        if (stFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            stFile.write(mockState.toUtf8());
            stFile.close();
        }
    }

    bool readOk = false;
    const QString stateText = readFileTrimmed(statePath, &readOk);
    if (outStateText) {
        *outStateText = stateText;
    }

    if (!writtenOk) {
        if (outError && outError->isEmpty()) {
            *outError = QStringLiteral("Write failed on '%1': %2").arg(commandPath, writeErrStr);
        }
        return false;
    }
    if (simulatedFailure) {
        return false;
    }
    if (!readOk) {
        if (outError) {
            *outError = QStringLiteral("Command written but state readback failed on '%1'").arg(statePath);
        }
        return false;
    }
    return true;
}

bool SysfsRoot::populateMockTree(const QString& rootPath, bool includeOptionalControls, bool includeClv0001Driver) {
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
        writeMockFile(rootPath, QStringLiteral("/sys/devices/system/cpu/cpu%1/topology/core_id").arg(i), QStringLiteral("%1\n").arg(i));
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

    // 9. Kernel / procfs identity and ACPI device tree (CLV0001:00 \_SB_.DCHU)
    writeMockFile(rootPath, QStringLiteral("/proc/version"),
                  QStringLiteral("Linux version 7.3.3-300.fc45.x86_64 (mockbuilder@fedora) #1 SMP PREEMPT_DYNAMIC\n"));
    writeMockFile(rootPath, QStringLiteral("/proc/modules"),
                  includeClv0001Driver
                      ? QStringLiteral("acer_clv0001_kbd 16384 0 - Live 0x0000000000000000\n"
                                       "nvidia 62000000 0 - Live 0x0000000000000000\n")
                      : QStringLiteral("nvidia 62000000 0 - Live 0x0000000000000000\n"));

    const QString clvDevRel = QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00");
    writeMockFile(rootPath, clvDevRel + QStringLiteral("/hid"), QStringLiteral("CLV0001\n"));
    writeMockFile(rootPath, clvDevRel + QStringLiteral("/path"), QStringLiteral("\\_SB_.DCHU\n"));
    writeMockFile(rootPath, clvDevRel + QStringLiteral("/status"), QStringLiteral("15\n"));
    writeMockFile(rootPath, clvDevRel + QStringLiteral("/uevent"),
                  includeClv0001Driver
                      ? QStringLiteral("DRIVER=acer_clv0001_kbd\nMODALIAS=acpi:CLV0001:\n")
                      : QStringLiteral("MODALIAS=acpi:CLV0001:\n"));

    if (includeClv0001Driver) {
        writeMockFile(rootPath, clvDevRel + QStringLiteral("/clv0001_kbd_command"), QString());
        writeMockFile(rootPath, clvDevRel + QStringLiteral("/clv0001_kbd_state"),
                      QStringLiteral("status=uninitialized\n"
                                     "preset=none\n"
                                     "brightness_level=0\n"
                                     "payload_grb=00 00 00 F0\n"
                                     "acpi_return_type=none\n"
                                     "physical_readback=unsupported\n"));
        QStringList presetNames;
        for (const auto& p : Clv0001KeyboardBackend::verifiedPresets()) {
            presetNames.append(p.name);
        }
        const QString presetsContent = QStringLiteral("presets=%1\nbrightness_levels=0,1,2,3,4\n")
                                           .arg(presetNames.join(QLatin1Char(',')));
        writeMockFile(rootPath, clvDevRel + QStringLiteral("/clv0001_kbd_presets"), presetsContent);
    }

    const QString acpiBusDir = QDir(rootPath).filePath(QStringLiteral("sys/bus/acpi/devices"));
    QDir().mkpath(acpiBusDir);
    const QString acpiLinkPath = QDir(acpiBusDir).filePath(QStringLiteral("CLV0001:00"));
    QFile::remove(acpiLinkPath);
    QFile::link(QStringLiteral("../../../devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00"), acpiLinkPath);

    writeMockFile(rootPath, QStringLiteral("/sys/bus/acpi/devices/CLV0002:00/hid"), QStringLiteral("CLV0002\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/acpi/devices/CLV0002:00/path"), QStringLiteral("\\_SB_.DCHV\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/acpi/devices/CLV0002:00/status"), QStringLiteral("15\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/acpi/devices/PNP0C09:00/hid"), QStringLiteral("PNP0C09\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/acpi/devices/PNP0C09:00/path"), QStringLiteral("\\_SB_.PC00.LPCB.EC0_\n"));
    writeMockFile(rootPath, QStringLiteral("/sys/bus/acpi/devices/PNP0C09:00/status"), QStringLiteral("15\n"));

    return true;
}

} // namespace acc::hal
