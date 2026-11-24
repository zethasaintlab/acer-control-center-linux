// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"

#include <QDBusConnection>
#include <QObject>
#include <QString>
#include <functional>

namespace acc::app {

struct AsyncControlOutcome {
    core::ControlAuthState authState = core::ControlAuthState::Idle;
    bool applied = false;
    QString errorCode;
    QString readbackSummary;
    QString message;
    bool physicalStateVerified = false;
    int readbackIntValue = -1;
};

using AsyncControlCallback = std::function<void(const AsyncControlOutcome&)>;

/**
 * Non-blocking control transport interface used by ControlCenterController.
 * Eliminates all QDBus::Block calls from the Qt GUI thread while preserving
 * explicit authorization-pending, authorization-denied, daemon-unavailable,
 * and hardware/validation failure classification.
 */
class IAsyncControlTransport : public QObject {
    Q_OBJECT
public:
    explicit IAsyncControlTransport(QObject* parent = nullptr) : QObject(parent) {}
    ~IAsyncControlTransport() override = default;

    virtual void setKeyboardLighting(
        const QString& mode,
        uint brightnessPct,
        const QString& zonesHexCsv,
        bool isClvPresetProtocol,
        AsyncControlCallback callback
    ) = 0;

    virtual void setCpuPerformanceTuning(
        const QString& governor,
        const QString& epp,
        bool turboEnabled,
        int minPerfPct,
        int maxPerfPct,
        AsyncControlCallback callback
    ) = 0;

    virtual void setFanControlMode(
        const QString& mode,
        int manualDutyPct,
        AsyncControlCallback callback
    ) = 0;

    virtual void setBatteryChargeLimit(
        int endThresholdPct,
        int previousThresholdPct,
        AsyncControlCallback callback
    ) = 0;

    virtual void setSystemPowerProfile(
        const QString& ppdProfile,
        AsyncControlCallback callback
    ) = 0;

    virtual void setDisplayBrightness(
        int brightnessPct,
        AsyncControlCallback callback
    ) = 0;
};

/**
 * Production QtDBus implementation using asyncCall() + QDBusPendingCallWatcher.
 * Supports custom QDBusConnection / service coordinates so tests can run real
 * async D-Bus round-trips against DaemonServiceObject over sessionBus().
 */
class DbusControlTransport : public IAsyncControlTransport {
    Q_OBJECT
public:
    explicit DbusControlTransport(
        QDBusConnection bus = QDBusConnection::systemBus(),
        QString serviceName = QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        QString objectPath = QStringLiteral("/io/github/zethasaintlab/AcerControlCenter1"),
        QString interfaceName = QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        int polkitTimeoutMs = 60000,
        QObject* parent = nullptr
    );

    void setKeyboardLighting(
        const QString& mode,
        uint brightnessPct,
        const QString& zonesHexCsv,
        bool isClvPresetProtocol,
        AsyncControlCallback callback
    ) override;

    void setCpuPerformanceTuning(
        const QString& governor,
        const QString& epp,
        bool turboEnabled,
        int minPerfPct,
        int maxPerfPct,
        AsyncControlCallback callback
    ) override;

    void setFanControlMode(
        const QString& mode,
        int manualDutyPct,
        AsyncControlCallback callback
    ) override;

    void setBatteryChargeLimit(
        int endThresholdPct,
        int previousThresholdPct,
        AsyncControlCallback callback
    ) override;

    void setSystemPowerProfile(
        const QString& ppdProfile,
        AsyncControlCallback callback
    ) override;

    void setDisplayBrightness(
        int brightnessPct,
        AsyncControlCallback callback
    ) override;

    static AsyncControlOutcome classifyDaemonError(
        const QString& operationLabel,
        const QString& errorName,
        const QString& errorMessage
    );

private:
    QDBusConnection m_bus;
    QString m_service;
    QString m_path;
    QString m_iface;
    int m_polkitTimeoutMs = 60000;
};

} // namespace acc::app
