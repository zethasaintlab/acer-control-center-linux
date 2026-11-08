// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "daemon/PolkitAuthorizer.h"
#include <QDBusContext>
#include <QObject>
#include <QString>
#include <cstdint>

namespace acc::daemon {

class DaemonServiceObject : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.zethasaintlab.AcerControlCenter1")

public:
    explicit DaemonServiceObject(AuthPolicyMode authMode = AuthPolicyMode::SystemPolkit, QObject* parent = nullptr);

    void setAuthMode(AuthPolicyMode mode);
    AuthPolicyMode authMode() const;

public Q_SLOTS:
    Q_SCRIPTABLE QString GetDaemonVersion();

    Q_SCRIPTABLE void GetRaplPowerSnapshot(
        qulonglong& pkg_uj,
        qulonglong& core_uj,
        qulonglong& uncore_uj,
        qulonglong& psys_uj,
        qlonglong& timestamp_us,
        bool& available
    );

    Q_SCRIPTABLE void SetCpuPerformanceTuning(
        const QString& governor,
        const QString& epp,
        bool turbo_enabled,
        uint min_perf_pct,
        uint max_perf_pct,
        bool& applied,
        QString& readback_json,
        QString& error_code
    );

    Q_SCRIPTABLE void SetKeyboardLighting(
        const QString& mode,
        uint brightness_pct,
        const QString& zones_hex_csv,
        bool& applied,
        QString& readback_json,
        QString& error_code
    );

    Q_SCRIPTABLE void SetBatteryChargeLimit(
        uint end_threshold_pct,
        bool& applied,
        uint& readback_pct,
        QString& error_code
    );

    Q_SCRIPTABLE void SetFanControlMode(
        const QString& mode,
        uint duty_pct,
        bool& applied,
        QString& readback_json,
        QString& error_code
    );

private:
    bool verifyCaller(const QString& actionId, bool interactive, QString& outReason) const;
    PolkitAuthorizer m_authorizer;
};

} // namespace acc::daemon
