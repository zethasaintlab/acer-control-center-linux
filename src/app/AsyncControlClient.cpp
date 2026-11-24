// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/AsyncControlClient.h"
#include "core/DiagnosticRedactor.h"
#include "hal/ControlOperations.h"
#include "hal/SysfsRoot.h"

#include <QDBusError>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPointer>
#include <algorithm>
#include <cmath>

namespace acc::app {

namespace {

QString formatDaemonRejection(const QString& errCode, const QString& readbackJson) {
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(readbackJson.toUtf8(), &parseErr);
    if (parseErr.error == QJsonParseError::NoError && doc.isObject()) {
        const QString msg = doc.object().value(QStringLiteral("message")).toString().trimmed();
        if (!msg.isEmpty()) {
            return QStringLiteral("%1 — %2").arg(errCode, msg);
        }
    }
    return errCode;
}

} // namespace

DbusControlTransport::DbusControlTransport(
    QDBusConnection bus,
    QString serviceName,
    QString objectPath,
    QString interfaceName,
    int polkitTimeoutMs,
    QObject* parent
)
    : IAsyncControlTransport(parent),
      m_bus(std::move(bus)),
      m_service(std::move(serviceName)),
      m_path(std::move(objectPath)),
      m_iface(std::move(interfaceName)),
      m_polkitTimeoutMs(std::max(100, polkitTimeoutMs)) {}

AsyncControlOutcome DbusControlTransport::classifyDaemonError(
    const QString& operationLabel,
    const QString& errorName,
    const QString& errorMessage
) {
    AsyncControlOutcome out;
    out.applied = false;

    if (errorName.contains(QLatin1String("PolicyKit1"), Qt::CaseInsensitive) ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.AccessDenied") ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.InteractiveAuthorizationRequired")) {
        out.authState = core::ControlAuthState::Denied;
        out.errorCode = QStringLiteral("ERR_POLKIT_UNAUTHORIZED");
        out.message = QStringLiteral("Authorization denied for %1 (%2: %3)")
                          .arg(operationLabel, errorName, errorMessage);
        return out;
    }

    if (errorName.isEmpty() ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.ServiceUnknown") ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.NameHasNoOwner") ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.NoReply") ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.Timeout") ||
        errorName == QLatin1String("org.freedesktop.DBus.Error.Disconnected")) {
        out.authState = core::ControlAuthState::DaemonUnavailable;
        out.errorCode = QStringLiteral("ERR_DBUS_DAEMON_UNAVAILABLE");
        out.message = errorName.isEmpty()
                          ? QStringLiteral("Privileged daemon D-Bus service unavailable for %1.").arg(operationLabel)
                          : QStringLiteral("Privileged daemon unreachable for %1 (%2: %3)")
                                .arg(operationLabel, errorName, errorMessage);
        return out;
    }

    out.authState = core::ControlAuthState::Failed;
    out.errorCode = errorName;
    out.message = QStringLiteral("D-Bus call failed for %1 (%2: %3)")
                      .arg(operationLabel, errorName, errorMessage);
    return out;
}

void DbusControlTransport::setKeyboardLighting(
    const QString& mode,
    uint brightnessPct,
    const QString& zonesHexCsv,
    bool isClvPresetProtocol,
    AsyncControlCallback callback
) {
    if (hal::SysfsRoot::instance().isMockActive()) {
        const core::ControlResult res = hal::ControlOperations::applyKeyboardLighting(
            mode,
            static_cast<int>(brightnessPct),
            zonesHexCsv
        );
        AsyncControlOutcome out;
        out.applied = res.applied;
        out.errorCode = res.errorCode;
        out.readbackSummary = res.readbackSummary;
        out.message = res.message;
        out.physicalStateVerified = res.physicalStateVerified;
        out.authState = res.applied ? core::ControlAuthState::Authorized : core::ControlAuthState::Failed;
        if (callback) {
            callback(out);
        }
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        m_service,
        m_path,
        m_iface,
        QStringLiteral("SetKeyboardLighting")
    );
    call << mode << brightnessPct << zonesHexCsv;

    QPointer<DbusControlTransport> self(this);
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(call, m_polkitTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [self, isClvPresetProtocol, callback](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self) {
            return;
        }
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 3) {
            const bool ok = reply.arguments().at(0).toBool();
            const QString rb = reply.arguments().at(1).toString();
            const QString errCode = reply.arguments().at(2).toString();

            AsyncControlOutcome out;
            out.applied = ok;
            out.errorCode = errCode;
            out.readbackSummary = rb;
            out.physicalStateVerified = ok && !isClvPresetProtocol;
            if (ok) {
                out.authState = core::ControlAuthState::Authorized;
                out.message = isClvPresetProtocol
                                  ? QStringLiteral("Command accepted — physical readback unavailable (%1).").arg(rb)
                                  : QStringLiteral("Daemon applied & verified keyboard lighting (%1).").arg(rb);
            } else if (errCode == QLatin1String("ERR_POLKIT_UNAUTHORIZED")) {
                out.authState = core::ControlAuthState::Denied;
                out.message = QStringLiteral("Authorization denied for keyboard lighting (%1).")
                                  .arg(formatDaemonRejection(errCode, rb));
            } else {
                out.authState = core::ControlAuthState::Failed;
                out.message = QStringLiteral("Daemon rejected keyboard lighting (%1).")
                                  .arg(formatDaemonRejection(errCode, rb));
            }
            if (callback) {
                callback(out);
            }
            return;
        }

        const AsyncControlOutcome errOut = classifyDaemonError(
            QStringLiteral("keyboard lighting"),
            reply.errorName(),
            reply.errorMessage()
        );
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DBUS"), errOut.message);
        if (callback) {
            callback(errOut);
        }
    });
}

void DbusControlTransport::setCpuPerformanceTuning(
    const QString& governor,
    const QString& epp,
    bool turboEnabled,
    int minPerfPct,
    int maxPerfPct,
    AsyncControlCallback callback
) {
    const int minClamped = std::min(minPerfPct, maxPerfPct);
    const int maxClamped = std::max(minPerfPct, maxPerfPct);

    if (hal::SysfsRoot::instance().isMockActive()) {
        const core::ControlResult res = hal::ControlOperations::applyCpuPerformanceTuning(
            governor, epp, turboEnabled, minClamped, maxClamped
        );
        AsyncControlOutcome out;
        out.applied = res.applied;
        out.errorCode = res.errorCode;
        out.readbackSummary = res.readbackSummary;
        out.message = res.message;
        out.physicalStateVerified = res.physicalStateVerified;
        out.authState = res.applied ? core::ControlAuthState::Authorized : core::ControlAuthState::Failed;
        if (callback) {
            callback(out);
        }
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        m_service,
        m_path,
        m_iface,
        QStringLiteral("SetCpuPerformanceTuning")
    );
    call << governor << epp << turboEnabled
         << static_cast<uint>(minClamped) << static_cast<uint>(maxClamped);

    QPointer<DbusControlTransport> self(this);
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(call, m_polkitTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [self, callback](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self) {
            return;
        }
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 3) {
            const bool ok = reply.arguments().at(0).toBool();
            const QString rb = reply.arguments().at(1).toString();
            const QString errCode = reply.arguments().at(2).toString();

            AsyncControlOutcome out;
            out.applied = ok;
            out.errorCode = errCode;
            out.readbackSummary = rb;
            out.physicalStateVerified = ok;
            if (ok) {
                out.authState = core::ControlAuthState::Authorized;
                out.message = QStringLiteral("Daemon applied & verified CPU P-state tuning (%1).").arg(rb);
            } else if (errCode == QLatin1String("ERR_POLKIT_UNAUTHORIZED")) {
                out.authState = core::ControlAuthState::Denied;
                out.message = QStringLiteral("Authorization denied for CPU tuning (%1).")
                                  .arg(formatDaemonRejection(errCode, rb));
            } else {
                out.authState = core::ControlAuthState::Failed;
                out.message = QStringLiteral("Daemon rejected CPU tuning (%1).")
                                  .arg(formatDaemonRejection(errCode, rb));
            }
            if (callback) {
                callback(out);
            }
            return;
        }

        const AsyncControlOutcome errOut = classifyDaemonError(
            QStringLiteral("CPU tuning"),
            reply.errorName(),
            reply.errorMessage()
        );
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DBUS"), errOut.message);
        if (callback) {
            callback(errOut);
        }
    });
}

void DbusControlTransport::setFanControlMode(
    const QString& mode,
    int manualDutyPct,
    AsyncControlCallback callback
) {
    if (hal::SysfsRoot::instance().isMockActive()) {
        const core::ControlResult res = hal::ControlOperations::applyFanControlMode(mode, manualDutyPct);
        AsyncControlOutcome out;
        out.applied = res.applied;
        out.errorCode = res.errorCode;
        out.readbackSummary = res.readbackSummary;
        out.message = res.message;
        out.physicalStateVerified = res.physicalStateVerified;
        out.authState = res.applied ? core::ControlAuthState::Authorized : core::ControlAuthState::Failed;
        if (callback) {
            callback(out);
        }
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        m_service,
        m_path,
        m_iface,
        QStringLiteral("SetFanControlMode")
    );
    call << mode << static_cast<uint>(std::clamp(manualDutyPct, 20, 100));

    QPointer<DbusControlTransport> self(this);
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(call, m_polkitTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [self, callback](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self) {
            return;
        }
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 3) {
            const bool ok = reply.arguments().at(0).toBool();
            const QString rb = reply.arguments().at(1).toString();
            const QString errCode = reply.arguments().at(2).toString();

            AsyncControlOutcome out;
            out.applied = ok;
            out.errorCode = errCode;
            out.readbackSummary = rb;
            out.physicalStateVerified = ok;
            if (ok) {
                out.authState = core::ControlAuthState::Authorized;
                out.message = QStringLiteral("Daemon applied & verified fan PWM mode (%1).").arg(rb);
            } else if (errCode == QLatin1String("ERR_POLKIT_UNAUTHORIZED")) {
                out.authState = core::ControlAuthState::Denied;
                out.message = QStringLiteral("Authorization denied for fan mode (%1).")
                                  .arg(formatDaemonRejection(errCode, rb));
            } else {
                out.authState = core::ControlAuthState::Failed;
                out.message = QStringLiteral("Daemon rejected fan mode (%1).")
                                  .arg(formatDaemonRejection(errCode, rb));
            }
            if (callback) {
                callback(out);
            }
            return;
        }

        const AsyncControlOutcome errOut = classifyDaemonError(
            QStringLiteral("fan mode"),
            reply.errorName(),
            reply.errorMessage()
        );
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DBUS"), errOut.message);
        if (callback) {
            callback(errOut);
        }
    });
}

void DbusControlTransport::setBatteryChargeLimit(
    int endThresholdPct,
    int previousThresholdPct,
    AsyncControlCallback callback
) {
    const int clamped = std::clamp(endThresholdPct, 50, 100);

    if (hal::SysfsRoot::instance().isMockActive()) {
        int rbPct = 100;
        const core::ControlResult res = hal::ControlOperations::applyBatteryChargeLimit(clamped, rbPct);
        AsyncControlOutcome out;
        out.applied = res.applied;
        out.errorCode = res.errorCode;
        out.readbackSummary = res.readbackSummary;
        out.message = res.message;
        out.physicalStateVerified = res.physicalStateVerified;
        out.readbackIntValue = rbPct;
        out.authState = res.applied ? core::ControlAuthState::Authorized : core::ControlAuthState::Failed;
        if (callback) {
            callback(out);
        }
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        m_service,
        m_path,
        m_iface,
        QStringLiteral("SetBatteryChargeLimit")
    );
    call << static_cast<uint>(clamped);

    QPointer<DbusControlTransport> self(this);
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(call, m_polkitTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [self, previousThresholdPct, callback](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self) {
            return;
        }
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 3) {
            const bool ok = reply.arguments().at(0).toBool();
            const uint rbPct = reply.arguments().at(1).toUInt();
            const QString errCode = reply.arguments().at(2).toString();

            AsyncControlOutcome out;
            out.applied = ok;
            out.errorCode = errCode;
            out.readbackIntValue = static_cast<int>(rbPct);
            out.physicalStateVerified = ok;
            if (ok) {
                out.authState = core::ControlAuthState::Authorized;
                out.message = QStringLiteral("Daemon verified battery charge threshold at %1% (previous=%2%).")
                                  .arg(rbPct)
                                  .arg(previousThresholdPct);
            } else if (errCode == QLatin1String("ERR_POLKIT_UNAUTHORIZED")) {
                out.authState = core::ControlAuthState::Denied;
                out.message = QStringLiteral("Authorization denied for battery charge threshold (%1).").arg(errCode);
            } else {
                out.authState = core::ControlAuthState::Failed;
                out.message = QStringLiteral("Daemon rejected battery charge threshold (%1).").arg(errCode);
            }
            if (callback) {
                callback(out);
            }
            return;
        }

        const AsyncControlOutcome errOut = classifyDaemonError(
            QStringLiteral("battery charge limit"),
            reply.errorName(),
            reply.errorMessage()
        );
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("DBUS"), errOut.message);
        if (callback) {
            callback(errOut);
        }
    });
}

void DbusControlTransport::setSystemPowerProfile(
    const QString& ppdProfile,
    AsyncControlCallback callback
) {
    const QString norm = ppdProfile.trimmed();
    if (hal::SysfsRoot::instance().isMockActive()) {
        const core::ControlResult res = hal::ControlOperations::applySystemPowerProfile(norm);
        AsyncControlOutcome out;
        out.applied = res.applied;
        out.errorCode = res.errorCode;
        out.readbackSummary = res.readbackSummary;
        out.message = res.message;
        out.physicalStateVerified = res.applied;
        out.authState = res.applied ? core::ControlAuthState::Authorized : core::ControlAuthState::Failed;
        if (callback) {
            callback(out);
        }
        return;
    }

    const QStringList allowed = {
        QStringLiteral("power-saver"),
        QStringLiteral("balanced"),
        QStringLiteral("performance")
    };
    if (!allowed.contains(norm)) {
        AsyncControlOutcome out;
        out.authState = core::ControlAuthState::Failed;
        out.errorCode = QStringLiteral("ERR_INVALID_PPD_PROFILE");
        out.message = QStringLiteral("Unsupported power profile '%1'.").arg(norm);
        if (callback) {
            callback(out);
        }
        return;
    }

    QDBusMessage setMsg = QDBusMessage::createMethodCall(
        QStringLiteral("net.hadess.PowerProfiles"),
        QStringLiteral("/net/hadess/PowerProfiles"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("Set")
    );
    setMsg << QStringLiteral("net.hadess.PowerProfiles")
           << QStringLiteral("ActiveProfile")
           << QVariant::fromValue(QDBusVariant(norm));

    QPointer<DbusControlTransport> self(this);
    auto* setWatcher = new QDBusPendingCallWatcher(m_bus.asyncCall(setMsg, 2000), this);
    connect(setWatcher, &QDBusPendingCallWatcher::finished, this, [self, norm, callback](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self) {
            return;
        }
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ErrorMessage) {
            AsyncControlOutcome out = classifyDaemonError(
                QStringLiteral("power profile"),
                reply.errorName(),
                reply.errorMessage()
            );
            if (out.authState == core::ControlAuthState::DaemonUnavailable) {
                out.errorCode = QStringLiteral("ERR_PPD_UNAVAILABLE");
            }
            if (callback) {
                callback(out);
            }
            return;
        }

        // Non-blocking readback verification
        QDBusMessage getMsg = QDBusMessage::createMethodCall(
            QStringLiteral("net.hadess.PowerProfiles"),
            QStringLiteral("/net/hadess/PowerProfiles"),
            QStringLiteral("org.freedesktop.DBus.Properties"),
            QStringLiteral("Get")
        );
        getMsg << QStringLiteral("net.hadess.PowerProfiles") << QStringLiteral("ActiveProfile");
        auto* getWatcher = new QDBusPendingCallWatcher(self->m_bus.asyncCall(getMsg, 2000), self.data());
        connect(getWatcher, &QDBusPendingCallWatcher::finished, self.data(),
                [self, norm, callback](QDBusPendingCallWatcher* gw) {
            gw->deleteLater();
            if (!self) {
                return;
            }
            const QDBusMessage gReply = gw->reply();
            AsyncControlOutcome out;
            if (gReply.type() == QDBusMessage::ReplyMessage && !gReply.arguments().isEmpty()) {
                const QVariant outer = gReply.arguments().at(0);
                const QString rb = outer.canConvert<QDBusVariant>()
                                       ? outer.value<QDBusVariant>().variant().toString()
                                       : outer.toString();
                if (rb == norm) {
                    out.applied = true;
                    out.authState = core::ControlAuthState::Authorized;
                    out.physicalStateVerified = true;
                    out.errorCode = QStringLiteral("OK");
                    out.readbackSummary = rb;
                    out.message = QStringLiteral("Verified system power profile switched to '%1'.").arg(rb);
                } else {
                    out.authState = core::ControlAuthState::Failed;
                    out.errorCode = QStringLiteral("ERR_PPD_READBACK_MISMATCH");
                    out.message = QStringLiteral("Power profile readback mismatch: requested '%1', active is '%2'.")
                                      .arg(norm, rb);
                }
            } else {
                out.authState = core::ControlAuthState::Failed;
                out.errorCode = QStringLiteral("ERR_PPD_SET_FAILED");
                out.message = QStringLiteral("Failed to read back ActiveProfile after setting '%1'.").arg(norm);
            }
            if (callback) {
                callback(out);
            }
        });
    });
}

void DbusControlTransport::setDisplayBrightness(
    int brightnessPct,
    AsyncControlCallback callback
) {
    const int clampedPct = std::clamp(brightnessPct, 5, 100);
    if (hal::SysfsRoot::instance().isMockActive()) {
        const core::ControlResult res = hal::ControlOperations::applyDisplayBrightness(clampedPct);
        AsyncControlOutcome out;
        out.applied = res.applied;
        out.errorCode = res.errorCode;
        out.readbackSummary = res.readbackSummary;
        out.message = res.message;
        out.physicalStateVerified = res.applied;
        out.authState = res.applied ? core::ControlAuthState::Authorized : core::ControlAuthState::Failed;
        if (callback) {
            callback(out);
        }
        return;
    }

    const QString base = hal::SysfsRoot::instance().resolvePath(QStringLiteral("/sys/class/backlight/intel_backlight"));
    bool okMax = false;
    const int maxB = hal::SysfsRoot::readFileTrimmed(base + QStringLiteral("/max_brightness"), &okMax).toInt();
    if (!okMax || maxB <= 0) {
        AsyncControlOutcome out;
        out.authState = core::ControlAuthState::Failed;
        out.errorCode = QStringLiteral("ERR_BACKLIGHT_NOT_DETECTED");
        out.message = QStringLiteral("intel_backlight max_brightness not readable.");
        if (callback) {
            callback(out);
        }
        return;
    }

    const int rawTarget = std::clamp(static_cast<int>(std::lround((clampedPct * maxB) / 100.0)), 1, maxB);
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.login1"),
        QStringLiteral("/org/freedesktop/login1/session/auto"),
        QStringLiteral("org.freedesktop.login1.Session"),
        QStringLiteral("SetBrightness")
    );
    msg << QStringLiteral("backlight") << QStringLiteral("intel_backlight") << static_cast<uint>(rawTarget);

    QPointer<DbusControlTransport> self(this);
    auto* watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(msg, 1500), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [self, clampedPct, rawTarget, callback](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self) {
            return;
        }
        const QDBusMessage reply = w->reply();
        AsyncControlOutcome out;
        if (reply.type() != QDBusMessage::ErrorMessage) {
            out.applied = true;
            out.authState = core::ControlAuthState::Authorized;
            out.physicalStateVerified = true;
            out.errorCode = QStringLiteral("OK");
            out.readbackSummary = QString::number(rawTarget);
            out.message = QStringLiteral("Set display brightness to %1% via systemd-logind.").arg(clampedPct);
        } else {
            out = classifyDaemonError(QStringLiteral("display brightness"), reply.errorName(), reply.errorMessage());
        }
        if (callback) {
            callback(out);
        }
    });
}

} // namespace acc::app
