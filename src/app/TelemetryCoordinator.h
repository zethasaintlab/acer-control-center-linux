// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include "hal/TelemetryCollector.h"

#include <QDBusConnection>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QThread>
#include <atomic>
#include <cstdint>

class QDBusPendingCallWatcher;

namespace acc::app {

/**
 * Worker QObject living exclusively on TelemetryCoordinator's dedicated QThread.
 * Owns hal::TelemetryCollector so NVML and sysfs/proc delta state remain
 * thread-confined without cross-thread mutex locking inside the collector.
 */
class TelemetryWorker : public QObject {
    Q_OBJECT
public:
    explicit TelemetryWorker(QObject* parent = nullptr);

public Q_SLOTS:
    void collect(
        uint64_t seq,
        const QString& activeProfileId,
        const acc::hal::TelemetryExternalHints& hints
    );
    void resetCollectorDeltas();
    void setSimulatedFailureForTest(const QString& errorMessage);

Q_SIGNALS:
    void collectionCompleted(uint64_t seq, const acc::core::SystemSnapshot& snapshot);
    void collectionErrored(uint64_t seq, const QString& errorMessage);

private:
    hal::TelemetryCollector m_collector;
    QString m_simulatedFailure;
};

/**
 * GUI-thread coordinator that decouples all sysfs/proc/NVML reads (on a worker
 * QThread) and daemon/systemd D-Bus queries (via non-blocking QDBusPendingCallWatcher)
 * from ControlCenterController.
 *
 * Threading & lifetime invariants:
 * - Lives on the GUI thread; never exposes TelemetryWorker directly to QML.
 * - Uses monotonic sequence numbers (m_minValidSeq) to drop stale in-flight
 *   snapshots after mode switches or cancellations.
 * - Coalesces overlapping timer ticks instead of queuing unbounded worker jobs.
 * - Cleanly shuts down and joins its worker QThread in ~TelemetryCoordinator().
 */
class TelemetryCoordinator : public QObject {
    Q_OBJECT
public:
    explicit TelemetryCoordinator(
        QDBusConnection bus = QDBusConnection::systemBus(),
        QString daemonServiceName = QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        QString daemonObjectPath = QStringLiteral("/io/github/zethasaintlab/AcerControlCenter1"),
        QString daemonInterfaceName = QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        QObject* parent = nullptr
    );
    ~TelemetryCoordinator() override;

    bool isInFlight() const { return m_inFlight; }
    uint64_t currentMinValidSeq() const { return m_minValidSeq; }

    void requestRefresh(const QString& activeProfileId);
    void invalidateAndReset();
    void setSimulatedWorkerFailureForTest(const QString& errorMessage);
    void setSimulatedCollectionDelayMsForTest(int delayMs);

Q_SIGNALS:
    void snapshotReady(const acc::core::SystemSnapshot& snapshot, uint64_t seq);
    void collectionFailed(const QString& errorMessage, uint64_t seq);
    void inFlightChanged(bool inFlight);

private Q_SLOTS:
    void onWorkerCompleted(uint64_t seq, const acc::core::SystemSnapshot& snapshot);
    void onWorkerErrored(uint64_t seq, const QString& errorMessage);

private:
    void refreshExternalDbusHintsAsync();
    void dispatchNextWorkerCollection();

    QDBusConnection m_bus;
    QString m_daemonService;
    QString m_daemonPath;
    QString m_daemonIface;

    QThread m_workerThread;
    TelemetryWorker* m_worker = nullptr;

    uint64_t m_nextSeq = 1;
    uint64_t m_minValidSeq = 1;
    bool m_inFlight = false;
    bool m_pendingRefresh = false;
    QString m_pendingProfileId = QStringLiteral("balanced");
    std::atomic<bool> m_shuttingDown{false};
    int m_simulatedDelayMs = 0;

    // Non-blocking GUI-thread cached D-Bus telemetry hints
    bool m_hintsProbeActive = false;
    bool m_cachedDaemonReachable = false;
    QString m_cachedActiveTunedProfile;
    bool m_hasCachedRapl = false;
    uint64_t m_cachedPkgUj = 0;
    uint64_t m_cachedCoreUj = 0;
    uint64_t m_cachedPsysUj = 0;
    int64_t m_cachedRaplTsUs = 0;
    bool m_cachedRaplAvail = false;
};

} // namespace acc::app

Q_DECLARE_METATYPE(acc::core::SystemSnapshot)
Q_DECLARE_METATYPE(acc::hal::TelemetryExternalHints)
