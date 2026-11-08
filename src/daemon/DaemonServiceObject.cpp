// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "daemon/DaemonServiceObject.h"
#include "core/DiagnosticRedactor.h"
#include "hal/ControlOperations.h"
#include "hal/SysfsRoot.h"

#include <QDateTime>
#include <QDBusMessage>

namespace acc::daemon {

DaemonServiceObject::DaemonServiceObject(AuthPolicyMode authMode, QObject* parent)
    : QObject(parent), m_authorizer(authMode) {}

void DaemonServiceObject::setAuthMode(AuthPolicyMode mode) {
    m_authorizer.setMode(mode);
}

AuthPolicyMode DaemonServiceObject::authMode() const {
    return m_authorizer.mode();
}

bool DaemonServiceObject::verifyCaller(const QString& actionId, bool interactive, QString& outReason) const {
    QString callerName;
    if (calledFromDBus()) {
        callerName = message().service();
    } else {
        callerName = QStringLiteral(":local.inproc");
    }
    return m_authorizer.checkCallerAuthorized(callerName, actionId, interactive, &outReason);
}

QString DaemonServiceObject::GetDaemonVersion() {
    return QStringLiteral("1.0.0-fc45");
}

void DaemonServiceObject::GetRaplPowerSnapshot(
    qulonglong& pkg_uj,
    qulonglong& core_uj,
    qulonglong& uncore_uj,
    qulonglong& psys_uj,
    qlonglong& timestamp_us,
    bool& available
) {
    pkg_uj = 0;
    core_uj = 0;
    uncore_uj = 0;
    psys_uj = 0;
    timestamp_us = QDateTime::currentMSecsSinceEpoch() * 1000LL;
    available = false;

    QString reason;
    if (!verifyCaller(QString(PolkitActions::kReadRapl), false, reason)) {
        return;
    }

    const auto& root = hal::SysfsRoot::instance();
    bool okPkg = false;
    pkg_uj = hal::SysfsRoot::readFileTrimmed(
                 root.resolvePath(QStringLiteral("/sys/class/powercap/intel-rapl:0/energy_uj")), &okPkg)
                 .toULongLong();
    core_uj = hal::SysfsRoot::readFileTrimmed(
                  root.resolvePath(QStringLiteral("/sys/class/powercap/intel-rapl:0:0/energy_uj")))
                  .toULongLong();
    uncore_uj = hal::SysfsRoot::readFileTrimmed(
                    root.resolvePath(QStringLiteral("/sys/class/powercap/intel-rapl:0:1/energy_uj")))
                    .toULongLong();
    psys_uj = hal::SysfsRoot::readFileTrimmed(
                  root.resolvePath(QStringLiteral("/sys/class/powercap/intel-rapl:1/energy_uj")))
                  .toULongLong();
    available = okPkg;
}

void DaemonServiceObject::SetCpuPerformanceTuning(
    const QString& governor,
    const QString& epp,
    bool turbo_enabled,
    uint min_perf_pct,
    uint max_perf_pct,
    bool& applied,
    QString& readback_json,
    QString& error_code
) {
    QString authReason;
    if (!verifyCaller(QString(PolkitActions::kSetPowerProfile), true, authReason)) {
        applied = false;
        readback_json = QStringLiteral("{}");
        error_code = QStringLiteral("ERR_POLKIT_UNAUTHORIZED");
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DAEMON"), authReason);
        return;
    }

    const core::ControlResult res = hal::ControlOperations::applyCpuPerformanceTuning(
        governor,
        epp,
        turbo_enabled,
        static_cast<int>(min_perf_pct),
        static_cast<int>(max_perf_pct)
    );
    applied = res.applied;
    readback_json = res.readbackSummary;
    error_code = res.errorCode;
}

void DaemonServiceObject::SetKeyboardLighting(
    const QString& mode,
    uint brightness_pct,
    const QString& zones_hex_csv,
    bool& applied,
    QString& readback_json,
    QString& error_code
) {
    QString authReason;
    if (!verifyCaller(QString(PolkitActions::kSetKeyboardLighting), true, authReason)) {
        applied = false;
        readback_json = QStringLiteral("{}");
        error_code = QStringLiteral("ERR_POLKIT_UNAUTHORIZED");
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DAEMON"), authReason);
        return;
    }

    const core::ControlResult res = hal::ControlOperations::applyKeyboardLighting(
        mode,
        static_cast<int>(brightness_pct),
        zones_hex_csv
    );
    applied = res.applied;
    readback_json = res.readbackSummary;
    error_code = res.errorCode;
}

void DaemonServiceObject::SetBatteryChargeLimit(
    uint end_threshold_pct,
    bool& applied,
    uint& readback_pct,
    QString& error_code
) {
    QString authReason;
    if (!verifyCaller(QString(PolkitActions::kSetBatteryLimit), true, authReason)) {
        applied = false;
        readback_pct = 100;
        error_code = QStringLiteral("ERR_POLKIT_UNAUTHORIZED");
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DAEMON"), authReason);
        return;
    }

    int rbPct = 100;
    const core::ControlResult res = hal::ControlOperations::applyBatteryChargeLimit(
        static_cast<int>(end_threshold_pct),
        rbPct
    );
    applied = res.applied;
    readback_pct = static_cast<uint>(rbPct);
    error_code = res.errorCode;
}

void DaemonServiceObject::SetFanControlMode(
    const QString& mode,
    uint duty_pct,
    bool& applied,
    QString& readback_json,
    QString& error_code
) {
    QString authReason;
    if (!verifyCaller(QString(PolkitActions::kSetFanMode), true, authReason)) {
        applied = false;
        readback_json = QStringLiteral("{}");
        error_code = QStringLiteral("ERR_POLKIT_UNAUTHORIZED");
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DAEMON"), authReason);
        return;
    }

    const core::ControlResult res = hal::ControlOperations::applyFanControlMode(
        mode,
        static_cast<int>(duty_pct)
    );
    applied = res.applied;
    readback_json = res.readbackSummary;
    error_code = res.errorCode;
}

} // namespace acc::daemon
