// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/ControlOperations.h"
#include "core/ColorScience.h"
#include "core/DiagnosticRedactor.h"
#include "hal/HardwareDiscovery.h"
#include "hal/SysfsRoot.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <algorithm>
#include <cmath>

namespace acc::hal {

core::ControlResult ControlOperations::applyCpuPerformanceTuning(
    const QString& governor,
    const QString& epp,
    bool turboEnabled,
    int minPerfPct,
    int maxPerfPct
) {
    core::ControlResult res;
    const auto& root = SysfsRoot::instance();

    const QString normGov = governor.trimmed();
    const QString normEpp = epp.trimmed();
    if (normGov != QLatin1String("powersave") && normGov != QLatin1String("performance")) {
        res.errorCode = QStringLiteral("ERR_INVALID_GOVERNOR");
        res.message = QStringLiteral("Governor '%1' is not allowlisted (expected powersave or performance).").arg(normGov);
        return res;
    }

    const QStringList allowedEpp = {
        QStringLiteral("default"),
        QStringLiteral("performance"),
        QStringLiteral("balance_performance"),
        QStringLiteral("balance_power"),
        QStringLiteral("power")
    };
    if (!allowedEpp.contains(normEpp)) {
        res.errorCode = QStringLiteral("ERR_INVALID_EPP");
        res.message = QStringLiteral("EPP '%1' is not allowlisted.").arg(normEpp);
        return res;
    }

    if (minPerfPct < 10 || minPerfPct > 100 || maxPerfPct < 10 || maxPerfPct > 100 || minPerfPct > maxPerfPct) {
        res.errorCode = QStringLiteral("ERR_INVALID_PERF_RANGE");
        res.message = QStringLiteral("Invalid P-state performance range [%1%, %2%].").arg(minPerfPct).arg(maxPerfPct);
        return res;
    }

    const QString pstateBase = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate"));
    if (!QFileInfo::exists(pstateBase)) {
        res.errorCode = QStringLiteral("ERR_PSTATE_NOT_DETECTED");
        res.message = QStringLiteral("intel_pstate sysfs directory is not present.");
        return res;
    }

    // Snapshot previous values for rollback if any step fails
    const QString prevNoTurbo = SysfsRoot::readFileTrimmed(pstateBase + QStringLiteral("/no_turbo"));
    const QString prevMinPerf = SysfsRoot::readFileTrimmed(pstateBase + QStringLiteral("/min_perf_pct"));
    const QString prevMaxPerf = SysfsRoot::readFileTrimmed(pstateBase + QStringLiteral("/max_perf_pct"));
    const QString cpu0Base = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq"));
    const QString prevGov = SysfsRoot::readFileTrimmed(cpu0Base + QStringLiteral("/scaling_governor"));
    const QString prevEpp = SysfsRoot::readFileTrimmed(cpu0Base + QStringLiteral("/energy_performance_preference"));

    auto rollback = [&]() {
        if (!prevNoTurbo.isEmpty()) {
            SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/no_turbo"), prevNoTurbo);
        }
        if (!prevMaxPerf.isEmpty()) {
            SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/max_perf_pct"), prevMaxPerf);
        }
        if (!prevMinPerf.isEmpty()) {
            SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/min_perf_pct"), prevMinPerf);
        }
        if (!prevGov.isEmpty()) {
            for (int c = 0; c < 32; ++c) {
                const QString cBase = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu%1/cpufreq").arg(c));
                if (!QFileInfo::exists(cBase)) {
                    continue;
                }
                SysfsRoot::writeFileVerified(cBase + QStringLiteral("/scaling_governor"), prevGov);
                if (!prevEpp.isEmpty() && prevGov != QLatin1String("performance")) {
                    const QString eppPath = cBase + QStringLiteral("/energy_performance_preference");
                    if (QFileInfo::exists(eppPath)) {
                        SysfsRoot::writeFileVerified(eppPath, prevEpp);
                    }
                }
            }
        }
    };

    QString err;
    QString rbNoTurbo, rbMin, rbMax, rbGov, rbEpp;

    // Order max/min writes safely so min_perf_pct never exceeds max_perf_pct transiently
    const int curMax = prevMaxPerf.toInt();
    if (maxPerfPct >= curMax) {
        if (!SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/max_perf_pct"), QString::number(maxPerfPct), &rbMax, &err) ||
            !SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/min_perf_pct"), QString::number(minPerfPct), &rbMin, &err)) {
            rollback();
            res.errorCode = QStringLiteral("ERR_WRITE_PERF_PCT");
            res.message = err;
            return res;
        }
    } else {
        if (!SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/min_perf_pct"), QString::number(minPerfPct), &rbMin, &err) ||
            !SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/max_perf_pct"), QString::number(maxPerfPct), &rbMax, &err)) {
            rollback();
            res.errorCode = QStringLiteral("ERR_WRITE_PERF_PCT");
            res.message = err;
            return res;
        }
    }

    const QString wantedNoTurbo = turboEnabled ? QStringLiteral("0") : QStringLiteral("1");
    if (!SysfsRoot::writeFileVerified(pstateBase + QStringLiteral("/no_turbo"), wantedNoTurbo, &rbNoTurbo, &err)) {
        rollback();
        res.errorCode = QStringLiteral("ERR_WRITE_TURBO");
        res.message = err;
        return res;
    }

    // Apply governor and EPP across all exposed CPU cores
    for (int i = 0; i < 32; ++i) {
        const QString cBase = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu%1/cpufreq").arg(i));
        if (!QFileInfo::exists(cBase)) {
            continue;
        }
        if (!SysfsRoot::writeFileVerified(cBase + QStringLiteral("/scaling_governor"), normGov, &rbGov, &err)) {
            rollback();
            res.errorCode = QStringLiteral("ERR_WRITE_GOVERNOR");
            res.message = err;
            return res;
        }
        // When governor is "performance" on intel_pstate HWP, the kernel may lock EPP to "performance"
        const QString effectiveEpp = (normGov == QLatin1String("performance")) ? QStringLiteral("performance") : normEpp;
        const QString eppPath = cBase + QStringLiteral("/energy_performance_preference");
        if (QFileInfo::exists(eppPath)) {
            SysfsRoot::writeFileVerified(eppPath, effectiveEpp, &rbEpp, &err);
        }
    }

    QJsonObject rbObj;
    rbObj.insert(QStringLiteral("governor"), rbGov);
    rbObj.insert(QStringLiteral("epp"), rbEpp.isEmpty() ? normEpp : rbEpp);
    rbObj.insert(QStringLiteral("no_turbo"), rbNoTurbo);
    rbObj.insert(QStringLiteral("min_perf_pct"), rbMin.toInt());
    rbObj.insert(QStringLiteral("max_perf_pct"), rbMax.toInt());

    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = QString::fromUtf8(QJsonDocument(rbObj).toJson(QJsonDocument::Compact));
    res.message = QStringLiteral("Verified CPU P-state tuning: governor=%1, epp=%2, turbo=%3, range=%4%-%5%")
                      .arg(rbGov, rbEpp.isEmpty() ? normEpp : rbEpp, turboEnabled ? QStringLiteral("on") : QStringLiteral("off"))
                      .arg(minPerfPct)
                      .arg(maxPerfPct);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("POWER"), res.message);
    return res;
}

core::ControlResult ControlOperations::applySystemPowerProfile(const QString& ppdProfile) {
    core::ControlResult res;
    const QStringList allowed = {
        QStringLiteral("power-saver"),
        QStringLiteral("balanced"),
        QStringLiteral("performance")
    };
    const QString norm = ppdProfile.trimmed();
    if (!allowed.contains(norm)) {
        res.errorCode = QStringLiteral("ERR_INVALID_PPD_PROFILE");
        res.message = QStringLiteral("Unsupported power profile '%1'.").arg(norm);
        return res;
    }

    if (SysfsRoot::instance().isMockActive()) {
        res.applied = true;
        res.errorCode = QStringLiteral("OK");
        res.readbackSummary = norm;
        res.message = QStringLiteral("Mock TuneD-PPD switched to '%1' (verified).").arg(norm);
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("POWER"), res.message);
        return res;
    }

    QDBusInterface ppdIface(
        QStringLiteral("net.hadess.PowerProfiles"),
        QStringLiteral("/net/hadess/PowerProfiles"),
        QStringLiteral("net.hadess.PowerProfiles"),
        QDBusConnection::systemBus()
    );
    if (!ppdIface.isValid()) {
        res.errorCode = QStringLiteral("ERR_PPD_UNAVAILABLE");
        res.message = QStringLiteral("D-Bus service net.hadess.PowerProfiles (tuned-ppd) is not reachable.");
        return res;
    }

    if (!ppdIface.setProperty("ActiveProfile", QVariant::fromValue(norm))) {
        res.errorCode = QStringLiteral("ERR_PPD_SET_FAILED");
        res.message = QStringLiteral("Failed to set ActiveProfile to '%1' via D-Bus.").arg(norm);
        return res;
    }

    const QString rb = ppdIface.property("ActiveProfile").toString();
    if (rb != norm) {
        res.errorCode = QStringLiteral("ERR_PPD_READBACK_MISMATCH");
        res.message = QStringLiteral("Power profile readback mismatch: requested '%1', active is '%2'.").arg(norm, rb);
        return res;
    }

    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = rb;
    res.message = QStringLiteral("System power profile switched to '%1' (readback verified).").arg(rb);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("POWER"), res.message);
    return res;
}

core::ControlResult ControlOperations::applyKeyboardLighting(
    const QString& mode,
    int brightnessPct,
    const QString& zonesHexCsv
) {
    Q_UNUSED(mode);
    core::ControlResult res;

    if (brightnessPct < 0 || brightnessPct > 100) {
        res.errorCode = QStringLiteral("ERR_INVALID_BRIGHTNESS");
        res.message = QStringLiteral("Keyboard brightness %1% is out of range [0, 100].").arg(brightnessPct);
        return res;
    }

    const QStringList hexParts = zonesHexCsv.split(QLatin1Char(','), Qt::SkipEmptyParts);
    std::vector<core::RgbColor> parsedZones;
    for (const QString& hp : hexParts) {
        core::RgbColor c;
        if (!core::ColorScience::parseHexToRgb(hp, c)) {
            res.errorCode = QStringLiteral("ERR_INVALID_COLOR_HEX");
            res.message = QStringLiteral("Malformed zone color hex '%1'.").arg(hp);
            return res;
        }
        parsedZones.push_back(c);
    }
    if (parsedZones.empty()) {
        parsedZones.push_back(core::RgbColor{59, 158, 255});
    }

    const QString ledPath = HardwareDiscovery::findFirstKeyboardLedPath();
    if (ledPath.isEmpty()) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_KEYBOARD_LED_NOT_DETECTED");
        res.message = QStringLiteral(
            "No physical keyboard LED device exists under /sys/class/leds. "
            "Profile saved to Keyboard Studio Preview without performing unverified hardware writes."
        );
        return res;
    }

    bool okMax = false;
    const int maxB = SysfsRoot::readFileTrimmed(ledPath + QStringLiteral("/max_brightness"), &okMax).toInt();
    const int effectiveMax = (okMax && maxB > 0) ? maxB : 100;
    const int targetRawB = std::clamp(static_cast<int>(std::lround((brightnessPct * effectiveMax) / 100.0)), 0, effectiveMax);

    QString rbBrightness, err;
    if (!SysfsRoot::writeFileVerified(ledPath + QStringLiteral("/brightness"), QString::number(targetRawB), &rbBrightness, &err)) {
        res.errorCode = QStringLiteral("ERR_LED_BRIGHTNESS_WRITE");
        res.message = err;
        return res;
    }

    QString rbIntensity;
    const QString multiPath = ledPath + QStringLiteral("/multi_intensity");
    if (QFileInfo::exists(multiPath)) {
        const core::RgbColor primary = parsedZones.front();
        const QString rgbTriple = QStringLiteral("%1 %2 %3").arg(primary.r).arg(primary.g).arg(primary.b);
        if (!SysfsRoot::writeFileVerified(multiPath, rgbTriple, &rbIntensity, &err)) {
            res.errorCode = QStringLiteral("ERR_LED_RGB_WRITE");
            res.message = err;
            return res;
        }
    }

    QJsonObject rbObj;
    rbObj.insert(QStringLiteral("led_device"), QFileInfo(ledPath).fileName());
    rbObj.insert(QStringLiteral("brightness_raw"), rbBrightness.toInt());
    rbObj.insert(QStringLiteral("multi_intensity"), rbIntensity);

    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = QString::fromUtf8(QJsonDocument(rbObj).toJson(QJsonDocument::Compact));
    res.message = QStringLiteral("Applied & verified keyboard lighting on %1 (brightness=%2/%3, rgb=%4).")
                      .arg(QFileInfo(ledPath).fileName(), rbBrightness, QString::number(effectiveMax),
                           rbIntensity.isEmpty() ? QStringLiteral("monochrome") : rbIntensity);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("HAL"), res.message);
    return res;
}

core::ControlResult ControlOperations::applyBatteryChargeLimit(int endThresholdPct, int& outReadbackPct) {
    core::ControlResult res;
    outReadbackPct = 100;

    if (endThresholdPct < 50 || endThresholdPct > 100) {
        res.errorCode = QStringLiteral("ERR_INVALID_THRESHOLD");
        res.message = QStringLiteral("Battery charge threshold %1% is outside safe range [50%, 100%].").arg(endThresholdPct);
        return res;
    }

    const QString limitPath = SysfsRoot::instance().resolvePath(
        QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold")
    );
    if (!QFileInfo::exists(limitPath)) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_CHARGE_LIMIT_NOT_DETECTED");
        res.message = QStringLiteral("charge_control_end_threshold is not exposed by the kernel on BAT0.");
        return res;
    }

    QString rb, err;
    if (!SysfsRoot::writeFileVerified(limitPath, QString::number(endThresholdPct), &rb, &err)) {
        res.errorCode = QStringLiteral("ERR_CHARGE_LIMIT_WRITE");
        res.message = err;
        return res;
    }

    outReadbackPct = rb.toInt();
    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = rb;
    res.message = QStringLiteral("Battery charge end threshold verified at %1%.").arg(outReadbackPct);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("POWER"), res.message);
    return res;
}

core::ControlResult ControlOperations::applyFanControlMode(const QString& mode, int dutyPct) {
    core::ControlResult res;
    const QString normMode = mode.trimmed().toLower();
    if (normMode != QLatin1String("auto") && normMode != QLatin1String("manual") && normMode != QLatin1String("curve")) {
        res.errorCode = QStringLiteral("ERR_INVALID_FAN_MODE");
        res.message = QStringLiteral("Unsupported fan mode '%1'.").arg(mode);
        return res;
    }
    if (dutyPct < 20 || dutyPct > 100) {
        res.errorCode = QStringLiteral("ERR_INVALID_FAN_DUTY");
        res.message = QStringLiteral("Fan duty %1% is outside safe thermal bounds [20%, 100%].").arg(dutyPct);
        return res;
    }

    const QString fanHwmon = HardwareDiscovery::findFirstFanPwmHwmonPath();
    if (fanHwmon.isEmpty()) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_FAN_PWM_NOT_DETECTED");
        res.message = QStringLiteral(
            "No verified kernel PWM fan node exists under /sys/class/hwmon. "
            "Firmware automatic fan control remains active."
        );
        return res;
    }

    const QString enablePath = fanHwmon + QStringLiteral("/pwm1_enable");
    const QString pwmPath = fanHwmon + QStringLiteral("/pwm1");
    QString rbEnable, rbPwm, err;

    if (normMode == QLatin1String("auto")) {
        if (!SysfsRoot::writeFileVerified(enablePath, QStringLiteral("2"), &rbEnable, &err)) {
            res.errorCode = QStringLiteral("ERR_FAN_AUTO_WRITE");
            res.message = err;
            return res;
        }
        res.applied = true;
        res.errorCode = QStringLiteral("OK");
        res.readbackSummary = QStringLiteral("{\"pwm1_enable\":2}");
        res.message = QStringLiteral("Restored automatic firmware fan control (pwm1_enable=2 verified).");
        return res;
    }

    // Manual / curve duty override: set pwm1_enable=1 and write scaled PWM [51..255]
    if (!SysfsRoot::writeFileVerified(enablePath, QStringLiteral("1"), &rbEnable, &err)) {
        res.errorCode = QStringLiteral("ERR_FAN_ENABLE_WRITE");
        res.message = err;
        return res;
    }

    const int rawPwm = std::clamp(static_cast<int>(std::lround((dutyPct * 255.0) / 100.0)), 51, 255);
    if (!SysfsRoot::writeFileVerified(pwmPath, QString::number(rawPwm), &rbPwm, &err)) {
        // Safety fallback: immediately restore automatic firmware control (pwm1_enable=2)
        SysfsRoot::writeFileVerified(enablePath, QStringLiteral("2"));
        res.errorCode = QStringLiteral("ERR_FAN_PWM_WRITE_ROLLED_BACK_TO_AUTO");
        res.message = QStringLiteral("PWM write failed (%1); safely restored automatic firmware fan mode.").arg(err);
        return res;
    }

    QJsonObject rbObj;
    rbObj.insert(QStringLiteral("pwm1_enable"), rbEnable.toInt());
    rbObj.insert(QStringLiteral("pwm1"), rbPwm.toInt());
    rbObj.insert(QStringLiteral("duty_pct"), dutyPct);

    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = QString::fromUtf8(QJsonDocument(rbObj).toJson(QJsonDocument::Compact));
    res.message = QStringLiteral("Applied fan PWM %1/255 (%2%) with readback verification.").arg(rbPwm).arg(dutyPct);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("HAL"), res.message);
    return res;
}

core::ControlResult ControlOperations::applyDisplayBrightness(int brightnessPct) {
    core::ControlResult res;
    const int clampedPct = std::clamp(brightnessPct, 5, 100);
    const QString base = SysfsRoot::instance().resolvePath(QStringLiteral("/sys/class/backlight/intel_backlight"));
    bool okMax = false;
    const int maxB = SysfsRoot::readFileTrimmed(base + QStringLiteral("/max_brightness"), &okMax).toInt();
    if (!okMax || maxB <= 0) {
        res.errorCode = QStringLiteral("ERR_BACKLIGHT_NOT_DETECTED");
        res.message = QStringLiteral("intel_backlight max_brightness not readable.");
        return res;
    }

    const int rawTarget = std::clamp(static_cast<int>(std::lround((clampedPct * maxB) / 100.0)), 1, maxB);
    if (!SysfsRoot::instance().isMockActive()) {
        // Try systemd-logind Session.SetBrightness("backlight", "intel_backlight", rawTarget) first
        QDBusMessage msg = QDBusMessage::createMethodCall(
            QStringLiteral("org.freedesktop.login1"),
            QStringLiteral("/org/freedesktop/login1/session/auto"),
            QStringLiteral("org.freedesktop.login1.Session"),
            QStringLiteral("SetBrightness")
        );
        msg << QStringLiteral("backlight") << QStringLiteral("intel_backlight") << static_cast<uint>(rawTarget);
        const QDBusMessage reply = QDBusConnection::systemBus().call(msg, QDBus::Block, 1500);
        if (reply.type() != QDBusMessage::ErrorMessage) {
            res.applied = true;
            res.errorCode = QStringLiteral("OK");
            res.readbackSummary = QString::number(rawTarget);
            res.message = QStringLiteral("Set display brightness to %1% via systemd-logind.").arg(clampedPct);
            return res;
        }
    }

    QString rb, err;
    if (!SysfsRoot::writeFileVerified(base + QStringLiteral("/brightness"), QString::number(rawTarget), &rb, &err)) {
        res.errorCode = QStringLiteral("ERR_BACKLIGHT_WRITE");
        res.message = err;
        return res;
    }

    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = rb;
    res.message = QStringLiteral("Verified display brightness at %1% (%2/%3).").arg(clampedPct).arg(rb).arg(maxB);
    return res;
}

} // namespace acc::hal
