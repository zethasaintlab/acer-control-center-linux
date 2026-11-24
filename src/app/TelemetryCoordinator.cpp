// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/TelemetryCoordinator.h"
#include "hal/SysfsRoot.h"

#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
#include <QMetaObject>
#include <QTimer>

namespace acc::app {

TelemetryWorker::TelemetryWorker(QObject* parent)
    : QObject(parent) {}

void TelemetryWorker::collect(
    uint64_t seq,
    const QString& activeProfileId,
    const hal::TelemetryExternalHints& hints
) {
    if (!m_simulatedFailure.isEmpty()) {
        Q_EMIT collectionErrored(seq, m_simulatedFailure);
        return;
    }
    const core::SystemSnapshot snap = m_collector.collectSnapshot(activeProfileId, &hints);
    Q_EMIT collectionCompleted(seq, snap);
}

void TelemetryWorker::resetCollectorDeltas() {
    m_collector.resetDeltas();
}

void TelemetryWorker::setSimulatedFailureForTest(const QString& errorMessage) {
    m_simulatedFailure = errorMessage;
}

TelemetryCoordinator::TelemetryCoordinator(
    QDBusConnection bus,
    QString daemonServiceName,
    QString daemonObjectPath,
    QString daemonInterfaceName,
    QObject* parent
)
    : QObject(parent),
      m_bus(std::move(bus)),
      m_daemonService(std::move(daemonServiceName)),
      m_daemonPath(std::move(daemonObjectPath)),
      m_daemonIface(std::move(daemonInterfaceName)) {
    qRegisterMetaType<acc::core::SystemSnapshot>("acc::core::SystemSnapshot");
    qRegisterMetaType<acc::hal::TelemetryExternalHints>("acc::hal::TelemetryExternalHints");
    qRegisterMetaType<uint64_t>("uint64_t");

    m_worker = new TelemetryWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &TelemetryWorker::collectionCompleted, this, &TelemetryCoordinator::onWorkerCompleted, Qt::QueuedConnection);
    connect(m_worker, &TelemetryWorker::collectionErrored, this, &TelemetryCoordinator::onWorkerErrored, Qt::QueuedConnection);

    m_workerThread.setObjectName(QStringLiteral("acc-telemetry-worker"));
    m_workerThread.start();
}

TelemetryCoordinator::~TelemetryCoordinator() {
    m_shuttingDown.store(true);
    m_workerThread.quit();
    m_workerThread.wait();
}

void TelemetryCoordinator::requestRefresh(const QString& activeProfileId) {
    if (m_shuttingDown.load()) {
        return;
    }
    m_pendingProfileId = activeProfileId;
    if (m_inFlight) {
        m_pendingRefresh = true;
        return;
    }

    m_inFlight = true;
    Q_EMIT inFlightChanged(true);

    // Kick off non-blocking async D-Bus probes on the GUI thread for next/current cycle
    refreshExternalDbusHintsAsync();
    dispatchNextWorkerCollection();
}

void TelemetryCoordinator::invalidateAndReset() {
    m_minValidSeq = m_nextSeq;
    m_pendingRefresh = false;
    m_hasCachedRapl = false;
    m_cachedRaplAvail = false;
    m_cachedDaemonReachable = false;
    m_cachedActiveTunedProfile.clear();
    if (m_worker && !m_shuttingDown.load()) {
        QMetaObject::invokeMethod(m_worker, &TelemetryWorker::resetCollectorDeltas, Qt::QueuedConnection);
    }
}

void TelemetryCoordinator::setSimulatedWorkerFailureForTest(const QString& errorMessage) {
    if (m_worker && !m_shuttingDown.load()) {
        QMetaObject::invokeMethod(
            m_worker,
            [w = m_worker, errorMessage]() { w->setSimulatedFailureForTest(errorMessage); },
            Qt::QueuedConnection
        );
    }
}

void TelemetryCoordinator::setSimulatedCollectionDelayMsForTest(int delayMs) {
    m_simulatedDelayMs = std::max(0, delayMs);
}

void TelemetryCoordinator::dispatchNextWorkerCollection() {
    if (m_shuttingDown.load() || !m_worker) {
        return;
    }

    const uint64_t seq = m_nextSeq++;
    const QString profId = m_pendingProfileId;
    hal::TelemetryExternalHints hints;
    hints.useExternalDbusHints = true;
    hints.daemonReachable = hal::SysfsRoot::instance().isMockActive() ? true : m_cachedDaemonReachable;
    hints.activeTunedProfile = m_cachedActiveTunedProfile;
    hints.hasRaplSample = m_hasCachedRapl;
    hints.raplPkgUj = m_cachedPkgUj;
    hints.raplCoreUj = m_cachedCoreUj;
    hints.raplPsysUj = m_cachedPsysUj;
    hints.raplTimestampUs = m_cachedRaplTsUs;
    hints.raplAvailable = m_cachedRaplAvail;

    auto invokeWorker = [this, seq, profId, hints]() {
        if (m_shuttingDown.load() || !m_worker) {
            return;
        }
        QMetaObject::invokeMethod(
            m_worker,
            [w = m_worker, seq, profId, hints]() { w->collect(seq, profId, hints); },
            Qt::QueuedConnection
        );
    };

    if (m_simulatedDelayMs > 0) {
        QTimer::singleShot(m_simulatedDelayMs, this, invokeWorker);
    } else {
        invokeWorker();
    }
}

void TelemetryCoordinator::refreshExternalDbusHintsAsync() {
    if (hal::SysfsRoot::instance().isMockActive()) {
        m_cachedDaemonReachable = true;
        m_cachedActiveTunedProfile = QStringLiteral("balanced (Mock TuneD-PPD)");
        m_hasCachedRapl = false;
        return;
    }

    if (!m_bus.isConnected() || m_hintsProbeActive) {
        return;
    }
    m_hintsProbeActive = true;

    QPointer<TelemetryCoordinator> self(this);

    // 1. Non-blocking check for daemon ownership
    QDBusMessage ownerCall = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.DBus"),
        QStringLiteral("/org/freedesktop/DBus"),
        QStringLiteral("org.freedesktop.DBus"),
        QStringLiteral("NameHasOwner")
    );
    ownerCall << m_daemonService;
    auto* ownerWatcher = new QDBusPendingCallWatcher(m_bus.asyncCall(ownerCall, 400), this);
    connect(ownerWatcher, &QDBusPendingCallWatcher::finished, this, [self](QDBusPendingCallWatcher* w) {
        w->deleteLater();
        if (!self || self->m_shuttingDown.load()) {
            return;
        }
        const QDBusMessage reply = w->reply();
        const bool registered = (reply.type() == QDBusMessage::ReplyMessage &&
                                 !reply.arguments().isEmpty() &&
                                 reply.arguments().at(0).toBool());
        self->m_cachedDaemonReachable = registered;

        if (!registered || hal::SysfsRoot::instance().isMockActive()) {
            self->m_hintsProbeActive = false;
            return;
        }

        // 2. Non-blocking RAPL snapshot request (allowUserInteraction=false on daemon side)
        QDBusMessage raplCall = QDBusMessage::createMethodCall(
            self->m_daemonService,
            self->m_daemonPath,
            self->m_daemonIface,
            QStringLiteral("GetRaplPowerSnapshot")
        );
        auto* raplWatcher = new QDBusPendingCallWatcher(self->m_bus.asyncCall(raplCall, 400), self.data());
        QObject::connect(raplWatcher, &QDBusPendingCallWatcher::finished, self.data(), [self](QDBusPendingCallWatcher* rw) {
            rw->deleteLater();
            if (!self || self->m_shuttingDown.load()) {
                return;
            }
            self->m_hintsProbeActive = false;
            const QDBusMessage rReply = rw->reply();
            if (rReply.type() == QDBusMessage::ReplyMessage && rReply.arguments().size() >= 6) {
                self->m_hasCachedRapl = true;
                self->m_cachedPkgUj = rReply.arguments().at(0).toULongLong();
                self->m_cachedCoreUj = rReply.arguments().at(1).toULongLong();
                self->m_cachedPsysUj = rReply.arguments().at(3).toULongLong();
                self->m_cachedRaplTsUs = rReply.arguments().at(4).toLongLong();
                self->m_cachedRaplAvail = rReply.arguments().at(5).toBool();
            }
        });
    });

    // 3. Non-blocking net.hadess.PowerProfiles ActiveProfile property read
    QDBusMessage ppdCall = QDBusMessage::createMethodCall(
        QStringLiteral("net.hadess.PowerProfiles"),
        QStringLiteral("/net/hadess/PowerProfiles"),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("Get")
    );
    ppdCall << QStringLiteral("net.hadess.PowerProfiles") << QStringLiteral("ActiveProfile");
    auto* ppdWatcher = new QDBusPendingCallWatcher(m_bus.asyncCall(ppdCall, 400), this);
    connect(ppdWatcher, &QDBusPendingCallWatcher::finished, this, [self](QDBusPendingCallWatcher* pw) {
        pw->deleteLater();
        if (!self || self->m_shuttingDown.load()) {
            return;
        }
        const QDBusMessage pReply = pw->reply();
        if (pReply.type() == QDBusMessage::ReplyMessage && !pReply.arguments().isEmpty()) {
            const QVariant outer = pReply.arguments().at(0);
            const QString val = outer.canConvert<QDBusVariant>()
                                    ? outer.value<QDBusVariant>().variant().toString()
                                    : outer.toString();
            if (!val.isEmpty()) {
                self->m_cachedActiveTunedProfile = val;
            }
        }
    });
}

void TelemetryCoordinator::onWorkerCompleted(uint64_t seq, const core::SystemSnapshot& snapshot) {
    if (m_shuttingDown.load()) {
        return;
    }

    if (seq >= m_minValidSeq) {
        Q_EMIT snapshotReady(snapshot, seq);
    }

    if (m_pendingRefresh) {
        m_pendingRefresh = false;
        dispatchNextWorkerCollection();
    } else {
        m_inFlight = false;
        Q_EMIT inFlightChanged(false);
    }
}

void TelemetryCoordinator::onWorkerErrored(uint64_t seq, const QString& errorMessage) {
    if (m_shuttingDown.load()) {
        return;
    }

    if (seq >= m_minValidSeq) {
        Q_EMIT collectionFailed(errorMessage, seq);
    }

    if (m_pendingRefresh) {
        m_pendingRefresh = false;
        dispatchNextWorkerCollection();
    } else {
        m_inFlight = false;
        Q_EMIT inFlightChanged(false);
    }
}

} // namespace acc::app
