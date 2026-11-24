// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/ControlCenterController.h"
#include "core/ColorScience.h"
#include "core/DiagnosticRedactor.h"
#include "hal/Clv0001KeyboardBackend.h"
#include "hal/ControlOperations.h"
#include "hal/SysfsRoot.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QRandomGenerator>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>

namespace acc::app {

namespace {
constexpr int kMaxHistorySamples = 60;

void appendHistorySample(QVariantList& list, double value) {
    list.append(value);
    while (list.size() > kMaxHistorySamples) {
        list.removeFirst();
    }
}
} // namespace

ControlCenterController::ControlCenterController(ThemeEngine* themeEngine, QObject* parent)
    : ControlCenterController(themeEngine, nullptr, QDBusConnection::systemBus(), parent) {}

ControlCenterController::ControlCenterController(
    ThemeEngine* themeEngine,
    IAsyncControlTransport* customTransport,
    QDBusConnection bus,
    QObject* parent
)
    : QObject(parent),
      m_theme(themeEngine),
      m_dbusConnection(std::move(bus)) {
    QString loadWarn;
    m_settings = m_configStore.loadSettings(&loadWarn);
    if (!loadWarn.isEmpty()) {
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("UI"), loadWarn);
    }

    if (m_theme) {
        m_theme->setDarkTheme(m_settings.darkTheme);
        m_theme->setReducedMotion(m_settings.reducedMotion);
        m_theme->setCompactNavigation(m_settings.compactNavigation);
        m_theme->setAccentSwatches(m_settings.customAccentSwatches);
        m_theme->setAccentFromHex(m_settings.accentColorHex);

        connect(m_theme, &ThemeEngine::themeChanged, this, &ControlCenterController::syncThemeToSettings);
        connect(m_theme, &ThemeEngine::accentChanged, this, &ControlCenterController::syncThemeToSettings);
        connect(m_theme, &ThemeEngine::motionChanged, this, &ControlCenterController::syncThemeToSettings);
        connect(m_theme, &ThemeEngine::layoutChanged, this, &ControlCenterController::syncThemeToSettings);
        connect(m_theme, &ThemeEngine::swatchesChanged, this, &ControlCenterController::syncThemeToSettings);
    }

    loadActiveKeyboardProfileFromSettings();

    m_coordinator = std::make_unique<TelemetryCoordinator>(
        m_dbusConnection,
        QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        QStringLiteral("/io/github/zethasaintlab/AcerControlCenter1"),
        QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        this
    );
    connect(m_coordinator.get(), &TelemetryCoordinator::snapshotReady, this, &ControlCenterController::onSnapshotReady);
    connect(m_coordinator.get(), &TelemetryCoordinator::collectionFailed, this, &ControlCenterController::onCollectionFailed);
    connect(m_coordinator.get(), &TelemetryCoordinator::inFlightChanged, this, &ControlCenterController::onCoordinatorInFlightChanged);

    if (customTransport) {
        m_transport = customTransport;
    } else {
        m_ownedTransport = std::make_unique<DbusControlTransport>(m_dbusConnection);
        m_transport = m_ownedTransport.get();
    }

    // Subscribe to systemd-logind PrepareForSleep(bool) for suspend/resume recovery
    if (m_dbusConnection.isConnected()) {
        m_dbusConnection.connect(
            QStringLiteral("org.freedesktop.login1"),
            QStringLiteral("/org/freedesktop/login1"),
            QStringLiteral("org.freedesktop.login1.Manager"),
            QStringLiteral("PrepareForSleep"),
            this,
            SLOT(onPrepareForSleep(bool))
        );
    }

    connect(&m_pollTimer, &QTimer::timeout, this, &ControlCenterController::refreshNow);
    m_pollTimer.setInterval(m_settings.pollingIntervalMs);

    connect(&m_freshnessTimer, &QTimer::timeout, this, &ControlCenterController::onFreshnessTick);
    m_freshnessTimer.setInterval(1000);
    m_freshnessTimer.start();

    // Non-blocking initial async telemetry request (GUI event loop starts immediately)
    refreshNow();
    m_pollTimer.start();
    setupSystemTray();
}

ControlCenterController::~ControlCenterController() {
    m_pollTimer.stop();
    m_freshnessTimer.stop();
    ++m_controlToken;
    m_coordinator.reset();
    m_ownedTransport.reset();
    m_transport = nullptr;
}

bool ControlCenterController::mockModeActive() const {
    return m_hasValidSnapshot ? m_snapshot.mockModeActive : hal::SysfsRoot::instance().isMockActive();
}

QString ControlCenterController::hardwareModeLabel() const {
    return m_hasValidSnapshot ? m_snapshot.mockScenarioName : hal::SysfsRoot::instance().scenarioLabel();
}

void ControlCenterController::setupSystemTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(QStringLiteral("#FFEA00")), 2.0));
    p.setBrush(QColor(QStringLiteral("#222524")));
    p.drawRoundedRect(QRectF(2, 2, 28, 28), 2.0, 2.0);
    p.setPen(QColor(QStringLiteral("#F0F2F2")));
    QFont f(QStringLiteral("JetBrains Mono"));
    f.setPixelSize(13);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRectF(2, 2, 28, 28), Qt::AlignCenter, QStringLiteral("AC"));
    p.end();

    m_trayIcon = std::make_unique<QSystemTrayIcon>(QIcon(pix), this);
    m_trayIcon->setToolTip(QStringLiteral("Acer Control Center — Aspire A715-59G"));
    m_trayIcon->show();
}

int64_t ControlCenterController::maxFreshAgeMs() const {
    if (m_freshnessTimeoutOverrideMs > 0) {
        return m_freshnessTimeoutOverrideMs;
    }
    return std::max<int64_t>(3000, static_cast<int64_t>(m_settings.pollingIntervalMs) * 2 + 1500);
}

void ControlCenterController::setFreshnessTimeoutOverrideMsForTest(int timeoutMs) {
    m_freshnessTimeoutOverrideMs = std::max(0, timeoutMs);
    onFreshnessTick();
}

bool ControlCenterController::waitForNextSnapshot(int timeoutMs) {
    if (!m_coordinator) {
        return false;
    }
    if (!m_coordinator->isInFlight()) {
        refreshNow();
    }
    QEventLoop loop;
    bool finished = false;
    QTimer timer;
    timer.setSingleShot(true);

    const auto c1 = connect(this, &ControlCenterController::telemetryUpdated, &loop, [&]() {
        finished = true;
        loop.quit();
    });
    const auto c2 = connect(m_coordinator.get(), &TelemetryCoordinator::collectionFailed, &loop, [&]() {
        finished = true;
        loop.quit();
    });
    const auto c3 = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(std::max(50, timeoutMs));
    loop.exec();

    disconnect(c1);
    disconnect(c2);
    disconnect(c3);
    return finished && m_hasValidSnapshot;
}

void ControlCenterController::onFreshnessTick() {
    if (!m_hasValidSnapshot) {
        return;
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 age = std::max<qint64>(0, nowMs - m_snapshot.timestampMs);
    const core::TelemetryFreshness nextFreshness =
        (m_telemetrySuspended || !m_telemetryErrorMessage.isEmpty() || age > maxFreshAgeMs())
            ? core::TelemetryFreshness::Stale
            : core::TelemetryFreshness::Fresh;

    if (m_telemetryAgeMs != age || m_freshness != nextFreshness) {
        m_telemetryAgeMs = age;
        m_freshness = nextFreshness;
        Q_EMIT telemetryFreshnessChanged();
    }
}

void ControlCenterController::onCoordinatorInFlightChanged(bool inFlight) {
    if (m_telemetryRefreshing != inFlight) {
        m_telemetryRefreshing = inFlight;
        Q_EMIT telemetryFreshnessChanged();
    }
}

void ControlCenterController::onSnapshotReady(const core::SystemSnapshot& snapshot, uint64_t /*seq*/) {
    m_snapshot = snapshot;
    m_hasValidSnapshot = snapshot.valid;
    m_telemetryErrorMessage.clear();
    m_telemetryAgeMs = 0;
    m_freshness = m_hasValidSnapshot
                      ? (m_telemetrySuspended ? core::TelemetryFreshness::Stale : core::TelemetryFreshness::Fresh)
                      : core::TelemetryFreshness::Unavailable;

    if (m_hasValidSnapshot) {
        if (m_histCpuTemp.isEmpty()) {
            // Seed baseline history points once from the first valid real reading
            for (int i = 0; i < 12; ++i) {
                appendHistorySample(m_histCpuTemp, m_snapshot.cpu.packageTempC);
                appendHistorySample(m_histGpuTemp, m_snapshot.gpu.tempC);
                appendHistorySample(m_histCpuLoad, m_snapshot.cpu.totalUtilizationPct);
                appendHistorySample(m_histGpuLoad, m_snapshot.gpu.gpuUtilizationPct);
                appendHistorySample(m_histBatteryWatts, m_snapshot.battery.powerWatts);
            }
        } else {
            appendHistorySample(m_histCpuTemp, m_snapshot.cpu.packageTempC);
            appendHistorySample(m_histGpuTemp, m_snapshot.gpu.tempC);
            appendHistorySample(m_histCpuLoad, m_snapshot.cpu.totalUtilizationPct);
            appendHistorySample(m_histGpuLoad, m_snapshot.gpu.gpuUtilizationPct);
            appendHistorySample(m_histBatteryWatts, m_snapshot.battery.powerWatts);
        }
    }

    const bool prevAcOnline = m_lastAcOnlineState;
    const bool wasFirstPoll = m_firstPoll;
    m_lastAcOnlineState = m_snapshot.battery.acOnline;
    m_firstPoll = false;
    if (m_hasValidSnapshot && !m_snapshot.mockModeActive && !m_initialLiveProfileSynced &&
        m_settings.activeOperatingProfileId != QLatin1String("custom")) {
        const QString ppd = m_snapshot.activeTunedProfile.trimmed().toLower();
        QString liveMapped;
        if (ppd == QLatin1String("power-saver") || ppd == QLatin1String("powersave")) {
            liveMapped = QStringLiteral("quiet");
        } else if (ppd == QLatin1String("balanced")) {
            liveMapped = QStringLiteral("balanced");
        } else if (ppd == QLatin1String("performance") || ppd == QLatin1String("throughput-performance")) {
            liveMapped = QStringLiteral("performance");
        }
        if (!liveMapped.isEmpty()) {
            m_initialLiveProfileSynced = true;
            if (m_settings.activeOperatingProfileId != liveMapped) {
                m_settings.activeOperatingProfileId = liveMapped;
                Q_EMIT settingsUpdated();
            }
        }
    }
    if (wasFirstPoll && m_snapshot.battery.chargeLimitSupported) {
        m_batPreviousThresholdPct = std::clamp(m_snapshot.battery.chargeEndThresholdPct, 50, 100);
    }

    if (wasFirstPoll && !m_statusBannerIsError && m_controlAuthState == core::ControlAuthState::Idle) {
        setStatusBanner(QStringLiteral("Ready — Hardware Telemetry Active"), false);
    }

    // Check AC/Battery auto profile transition
    if (!wasFirstPoll && m_settings.autoSwitchProfileOnPower && m_snapshot.battery.acOnline != prevAcOnline) {
        const QString targetProf = m_snapshot.battery.acOnline ? m_settings.acOperatingProfileId : m_settings.batteryOperatingProfileId;
        selectOperatingProfile(targetProf);
    }

    if (m_trayIcon) {
        const QString gpuTip = m_snapshot.gpu.telemetryMeasuredByNvml
                                   ? QStringLiteral("%1°C").arg(m_snapshot.gpu.tempC, 0, 'f', 0)
                                   : (m_snapshot.gpu.available ? m_snapshot.gpu.pstate : QStringLiteral("N/A"));
        m_trayIcon->setToolTip(
            QStringLiteral("Acer Aspire A715-59G  |  CPU: %1°C (%2%)  |  GPU: %3  |  Profile: %4")
                .arg(m_snapshot.cpu.packageTempC, 0, 'f', 0)
                .arg(m_snapshot.cpu.totalUtilizationPct, 0, 'f', 0)
                .arg(gpuTip, m_settings.activeOperatingProfileId.toUpper())
        );
    }

    if (!m_snapshot.keyboardHw.supportsRgbZones && m_kbdSelectedZone != -1) {
        m_kbdSelectedZone = -1;
        Q_EMIT keyboardStudioChanged();
    }

    Q_EMIT telemetryFreshnessChanged();
    Q_EMIT telemetryUpdated();
}

void ControlCenterController::onCollectionFailed(const QString& errorMessage, uint64_t /*seq*/) {
    m_telemetryErrorMessage = errorMessage;
    m_freshness = m_hasValidSnapshot
                      ? core::TelemetryFreshness::Stale
                      : core::TelemetryFreshness::Unavailable;
    if (m_hasValidSnapshot) {
        m_telemetryAgeMs = std::max<qint64>(0, QDateTime::currentMSecsSinceEpoch() - m_snapshot.timestampMs);
    } else {
        m_telemetryAgeMs = -1;
    }

    core::DiagnosticRedactor::instance().appendLog(
        QStringLiteral("WARN"),
        QStringLiteral("HAL"),
        QStringLiteral("Telemetry collection failed (%1); state=%2")
            .arg(errorMessage, telemetryFreshness())
    );
    Q_EMIT telemetryFreshnessChanged();
    Q_EMIT telemetryUpdated();
}

void ControlCenterController::onPrepareForSleep(bool goingToSleep) {
    if (goingToSleep) {
        m_pollTimer.stop();
        m_telemetrySuspended = true;
        if (m_hasValidSnapshot) {
            m_freshness = core::TelemetryFreshness::Stale;
        }
        core::DiagnosticRedactor::instance().appendLog(
            QStringLiteral("INFO"),
            QStringLiteral("POWER"),
            QStringLiteral("System entering suspend (PrepareForSleep=true); paused telemetry polling.")
        );
        Q_EMIT telemetryFreshnessChanged();
    } else {
        m_telemetrySuspended = false;
        core::DiagnosticRedactor::instance().appendLog(
            QStringLiteral("INFO"),
            QStringLiteral("POWER"),
            QStringLiteral("System resumed from sleep (PrepareForSleep=false); re-verifying hardware state.")
        );
        refreshNow();
        m_pollTimer.start(m_settings.pollingIntervalMs);
        setStatusBanner(QStringLiteral("Resumed from sleep — re-verifying hardware telemetry & capabilities."), false);
    }
    Q_EMIT logsUpdated();
}

void ControlCenterController::setCurrentTabIndex(int idx) {
    const int clamped = std::clamp(idx, 0, 7);
    if (m_currentTabIndex != clamped) {
        m_currentTabIndex = clamped;
        Q_EMIT navigationChanged();
    }
}

int ControlCenterController::hardwareModeIndex() const {
    switch (hal::SysfsRoot::instance().mode()) {
        case hal::HardwareMode::PhysicalSystem:
            return 0;
        case hal::HardwareMode::MockPhysicalMirror:
            return 1;
        case hal::HardwareMode::MockFullCapabilities:
            return 2;
        case hal::HardwareMode::MockClv0001DriverActive:
            return 3;
        case hal::HardwareMode::CustomRootPath:
            return 1;
    }
    return 0;
}

void ControlCenterController::setPollingIntervalMs(int ms) {
    const int clamped = std::clamp(ms, 500, 5000);
    if (m_settings.pollingIntervalMs != clamped) {
        m_settings.pollingIntervalMs = clamped;
        m_pollTimer.setInterval(clamped);
        persistSettings();
        Q_EMIT settingsUpdated();
    }
}

void ControlCenterController::syncThemeToSettings() {
    if (!m_theme) {
        return;
    }
    m_settings.darkTheme = m_theme->darkTheme();
    m_settings.reducedMotion = m_theme->reducedMotion();
    m_settings.compactNavigation = m_theme->compactNavigation();
    m_settings.accentColorHex = m_theme->accentHex();
    m_settings.customAccentSwatches = m_theme->accentSwatches();
    persistSettings();
}

void ControlCenterController::persistSettings() {
    QString err;
    if (!m_configStore.saveSettings(m_settings, &err)) {
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("UI"), err);
    }
}

void ControlCenterController::setStatusBanner(const QString& text, bool isError) {
    m_statusBannerText = text;
    m_statusBannerIsError = isError;
    Q_EMIT statusBannerChanged();
    Q_EMIT logsUpdated();
}

void ControlCenterController::refreshNow() {
    if (m_coordinator) {
        m_coordinator->requestRefresh(m_settings.activeOperatingProfileId);
    }
}

bool ControlCenterController::beginControlOperation(const QString& operationName, uint64_t& outToken) {
    if (m_controlOperationPending) {
        setStatusBanner(
            QStringLiteral("Another control operation (%1) is still awaiting authorization or completion.")
                .arg(m_controlOperationName),
            true
        );
        return false;
    }

    outToken = ++m_controlToken;
    m_controlOperationPending = true;
    m_controlOperationName = operationName;
    m_controlAuthState = core::ControlAuthState::Pending;
    m_controlLastErrorCode.clear();
    m_controlLastErrorMessage.clear();
    Q_EMIT controlOperationChanged();
    setStatusBanner(QStringLiteral("Awaiting authorization / applying %1...").arg(operationName), false);
    return true;
}

void ControlCenterController::completeControlOperation(
    uint64_t token,
    const QString& operationName,
    const AsyncControlOutcome& outcome,
    bool triggerRefresh
) {
    if (token != m_controlToken) {
        return;
    }

    m_controlOperationPending = false;
    m_controlOperationName = operationName;
    m_controlAuthState = outcome.authState;
    m_controlLastErrorCode = outcome.errorCode;
    m_controlLastErrorMessage = outcome.applied ? QString() : outcome.message;
    Q_EMIT controlOperationChanged();
    Q_EMIT controlOperationFinished(
        operationName,
        outcome.applied,
        core::controlAuthStateToId(outcome.authState),
        outcome.errorCode,
        outcome.message
    );

    if (triggerRefresh) {
        refreshNow();
    }
    setStatusBanner(outcome.message, !outcome.applied);
}

QString ControlCenterController::productTitle() const {
    if (!m_hasValidSnapshot && m_snapshot.dmi.productName.isEmpty()) {
        return QStringLiteral("Detecting platform...");
    }
    return QStringLiteral("%1 %2 (v%3)")
        .arg(m_snapshot.dmi.sysVendor, m_snapshot.dmi.productName, m_snapshot.dmi.productVersion);
}

QString ControlCenterController::boardTitle() const {
    if (!m_hasValidSnapshot && m_snapshot.dmi.boardName.isEmpty()) {
        return QStringLiteral("--");
    }
    return QStringLiteral("%1 // %2").arg(m_snapshot.dmi.boardName, m_snapshot.dmi.boardVendor);
}

QString ControlCenterController::biosTitle() const {
    if (!m_hasValidSnapshot && m_snapshot.dmi.biosVersion.isEmpty()) {
        return QStringLiteral("--");
    }
    return QStringLiteral("%1 %2 (%3)")
        .arg(m_snapshot.dmi.biosVendor, m_snapshot.dmi.biosVersion, m_snapshot.dmi.biosDate);
}

QVariantList ControlCenterController::cpuCoresModel() const {
    QVariantList list;
    for (const auto& c : m_snapshot.cpu.cores) {
        QVariantMap m;
        m.insert(QStringLiteral("coreId"), c.coreId);
        m.insert(QStringLiteral("label"), c.label);
        m.insert(QStringLiteral("freqMhz"), c.freqMhz);
        m.insert(QStringLiteral("tempC"), c.tempC);
        m.insert(QStringLiteral("utilizationPct"), c.utilizationPct);
        list.append(m);
    }
    return list;
}

void ControlCenterController::setAutoSwitchOnPower(bool b) {
    if (m_settings.autoSwitchProfileOnPower != b) {
        m_settings.autoSwitchProfileOnPower = b;
        persistSettings();
        Q_EMIT settingsUpdated();
    }
}

void ControlCenterController::setAcProfileId(const QString& id) {
    if (m_settings.acOperatingProfileId != id) {
        m_settings.acOperatingProfileId = id;
        persistSettings();
        Q_EMIT settingsUpdated();
    }
}

void ControlCenterController::setBatteryProfileId(const QString& id) {
    if (m_settings.batteryOperatingProfileId != id) {
        m_settings.batteryOperatingProfileId = id;
        persistSettings();
        Q_EMIT settingsUpdated();
    }
}

QVariantList ControlCenterController::operatingProfilesModel() const {
    QVariantList list;
    const auto profiles = core::ConfigStore::standardOperatingProfiles(m_settings.customOperatingProfile);
    for (const auto& p : profiles) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), p.id);
        m.insert(QStringLiteral("name"), p.name);
        m.insert(QStringLiteral("subtitle"), p.subtitle);
        m.insert(QStringLiteral("tunedProfile"), p.tunedProfile);
        m.insert(QStringLiteral("ppdProfile"), p.ppdProfile);
        m.insert(QStringLiteral("governor"), p.governor);
        m.insert(QStringLiteral("epp"), p.epp);
        m.insert(QStringLiteral("turboEnabled"), p.turboEnabled);
        m.insert(QStringLiteral("minPerfPct"), p.minPerfPct);
        m.insert(QStringLiteral("maxPerfPct"), p.maxPerfPct);
        m.insert(QStringLiteral("active"), p.id == m_settings.activeOperatingProfileId);
        list.append(m);
    }
    return list;
}

QVariantList ControlCenterController::thermalSensorsModel() const {
    QVariantList list;
    for (const auto& s : m_snapshot.thermal.sensors) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), s.id);
        m.insert(QStringLiteral("label"), s.label);
        m.insert(QStringLiteral("sourceDriver"), s.sourceDriver);
        m.insert(QStringLiteral("sysfsPath"), s.sysfsPath);
        m.insert(QStringLiteral("tempC"), s.tempC);
        list.append(m);
    }
    return list;
}

QVariantList ControlCenterController::fansModel() const {
    QVariantList list;
    for (const auto& f : m_snapshot.thermal.fans) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), f.id);
        m.insert(QStringLiteral("label"), f.label);
        m.insert(QStringLiteral("sysfsPath"), f.sysfsPath);
        m.insert(QStringLiteral("rpm"), f.rpm);
        m.insert(QStringLiteral("pwmPct"), f.pwmPct);
        m.insert(QStringLiteral("writable"), f.isWritable);
        list.append(m);
    }
    return list;
}

QVariantList ControlCenterController::fanCurvePoints() const {
    QVariantList list;
    for (const auto& pt : m_settings.customFanCurve) {
        QVariantMap m;
        m.insert(QStringLiteral("tempC"), pt.tempC);
        m.insert(QStringLiteral("dutyPct"), pt.dutyPct);
        list.append(m);
    }
    return list;
}

QString ControlCenterController::kbdCapabilityBadge() const {
    return core::capabilityStateToBadge(m_snapshot.keyboardHw.capabilityState);
}

QString ControlCenterController::fanCapabilityBadge() const {
    for (const auto& cap : m_snapshot.capabilities) {
        if (cap.id == QLatin1String("cap.thermal.fans")) {
            return core::capabilityStateToBadge(cap.state);
        }
    }
    return m_snapshot.thermal.manualFanSupported
               ? QStringLiteral("FULL RW")
               : QStringLiteral("FIRMWARE AUTOMATIC");
}

QString ControlCenterController::batCapabilityBadge() const {
    if (!m_snapshot.battery.chargeLimitSupported) {
        return QStringLiteral("BATTERY TELEMETRY ONLY — CHARGE LIMIT NOT DETECTED");
    }
    for (const auto& cap : m_snapshot.capabilities) {
        if (cap.id == QLatin1String("cap.battery.limit")) {
            return core::capabilityStateToBadge(cap.state);
        }
    }
    return QStringLiteral("FULL RW");
}

void ControlCenterController::syncCurrentKbdProfileToSettings() {
    for (auto& kp : m_settings.keyboardProfiles) {
        if (kp.id == m_currentKbdProfile.id) {
            kp = m_currentKbdProfile;
            break;
        }
    }
    persistSettings();
}

void ControlCenterController::loadActiveKeyboardProfileFromSettings() {
    if (m_settings.keyboardProfiles.empty()) {
        return;
    }
    m_currentKbdProfile = m_settings.keyboardProfiles.front();
    for (const auto& kp : m_settings.keyboardProfiles) {
        if (kp.id == m_settings.activeKeyboardProfileId) {
            m_currentKbdProfile = kp;
            break;
        }
    }
    if (!m_currentKbdProfile.zones.empty()) {
        const auto& z0 = m_currentKbdProfile.zones.front();
        m_kbdRgb = core::ColorScience::clampRgb(z0.r, z0.g, z0.b);
        m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
        m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
        m_kbdPickerHex = core::ColorScience::rgbToHex(m_kbdRgb);
    }
    m_kbdCustomPreviewDirty = false;
    m_kbdSelectionSource = QStringLiteral("preset");
}

void ControlCenterController::setKbdEnabled(bool e) {
    if (m_currentKbdProfile.enabled != e) {
        m_currentKbdProfile.enabled = e;
        if (e) {
            if (!m_kbdCustomPreviewDirty && m_currentKbdProfile.clv0001PresetName == QLatin1String("off")) {
                m_currentKbdProfile.clv0001PresetName = QStringLiteral("cyan");
            }
            if (m_currentKbdProfile.clv0001BrightnessLevel <= 0) {
                m_currentKbdProfile.clv0001BrightnessLevel = 4;
            }
            if (m_currentKbdProfile.brightnessPct <= 0) {
                m_currentKbdProfile.brightnessPct = 100;
            }
        }
        syncCurrentKbdProfileToSettings();
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdMode(const QString& m) {
    if (m_currentKbdProfile.mode != m) {
        m_currentKbdProfile.mode = m;
        syncCurrentKbdProfileToSettings();
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdBrightnessPct(int b) {
    const int clamped = std::clamp(b, 0, 100);
    const int lvl = hal::Clv0001KeyboardBackend::brightnessPctToLevel(clamped);
    if (m_currentKbdProfile.brightnessPct != clamped || m_currentKbdProfile.clv0001BrightnessLevel != lvl) {
        m_currentKbdProfile.brightnessPct = clamped;
        m_currentKbdProfile.clv0001BrightnessLevel = lvl;
        syncCurrentKbdProfileToSettings();
        Q_EMIT keyboardStudioChanged();
    }
}

QString ControlCenterController::kbdFirmwarePacketHex() const {
    if (m_kbdCustomPreviewDirty) {
        const int lvl = m_currentKbdProfile.enabled ? m_currentKbdProfile.clv0001BrightnessLevel : 0;
        std::array<uint8_t, 4> payload{};
        if (hal::Clv0001KeyboardBackend::buildCustomRgbFirmwarePayload(
                m_kbdRgb.r, m_kbdRgb.g, m_kbdRgb.b, lvl, payload)) {
            return hal::Clv0001KeyboardBackend::formatPayloadHex(payload);
        }
        return QStringLiteral("00 00 00 F0");
    }

    const QString preset = m_currentKbdProfile.enabled
                               ? m_currentKbdProfile.clv0001PresetName
                               : QStringLiteral("off");
    const int lvl = m_currentKbdProfile.enabled ? m_currentKbdProfile.clv0001BrightnessLevel : 0;
    std::array<uint8_t, 4> payload{};
    if (hal::Clv0001KeyboardBackend::buildFirmwarePayload(preset, lvl, payload)) {
        return hal::Clv0001KeyboardBackend::formatPayloadHex(payload);
    }
    return QStringLiteral("00 00 00 F0");
}

QString ControlCenterController::kbdNearestPresetName() const {
    return hal::Clv0001KeyboardBackend::findNearestPresetName(m_kbdRgb);
}

QVariantList ControlCenterController::kbdPresetsModel() const {
    QVariantList list;
    for (const auto& p : hal::Clv0001KeyboardBackend::verifiedPresets()) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), p.name);
        m.insert(QStringLiteral("previewHex"), p.previewHex);
        m.insert(QStringLiteral("r"), static_cast<int>(p.r));
        m.insert(QStringLiteral("g"), static_cast<int>(p.g));
        m.insert(QStringLiteral("b"), static_cast<int>(p.b));
        m.insert(QStringLiteral("selected"),
                 !m_kbdCustomPreviewDirty && (p.name == m_currentKbdProfile.clv0001PresetName));
        list.append(m);
    }
    return list;
}

void ControlCenterController::selectKbdPreset(const QString& presetName) {
    core::Clv0001PresetInfo pInfo;
    if (!hal::Clv0001KeyboardBackend::findPreset(presetName, pInfo)) {
        setStatusBanner(QStringLiteral("Unknown CLV0001 preset '%1'.").arg(presetName), true);
        return;
    }
    m_currentKbdProfile.clv0001PresetName = pInfo.name;
    if (pInfo.name == QLatin1String("off")) {
        m_currentKbdProfile.enabled = false;
        m_currentKbdProfile.clv0001BrightnessLevel = 0;
        m_currentKbdProfile.brightnessPct = 0;
    } else {
        m_currentKbdProfile.enabled = true;
        if (m_currentKbdProfile.clv0001BrightnessLevel == 0) {
            m_currentKbdProfile.clv0001BrightnessLevel = 4;
            m_currentKbdProfile.brightnessPct = 100;
        }
    }
    m_kbdRgb = core::RgbColor{static_cast<int>(pInfo.r), static_cast<int>(pInfo.g), static_cast<int>(pInfo.b)};
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    m_kbdPickerHex = core::ColorScience::rgbToHex(m_kbdRgb);
    m_kbdSelectedZone = -1;
    m_kbdCustomPreviewDirty = false;
    m_kbdSelectionSource = QStringLiteral("preset");
    applySelectedKbdColorToActiveZones();
    setStatusBanner(
        QStringLiteral("Selected CLV0001 preset '%1' (GRB packet %2).")
            .arg(pInfo.name, kbdFirmwarePacketHex()),
        false
    );
}

void ControlCenterController::snapToNearestKbdPreset() {
    selectKbdPreset(kbdNearestPresetName());
}

void ControlCenterController::setKbdBrightnessLevel(int level) {
    const int clamped = std::clamp(level, 0, 4);
    m_currentKbdProfile.clv0001BrightnessLevel = clamped;
    m_currentKbdProfile.brightnessPct = hal::Clv0001KeyboardBackend::brightnessLevelToPct(clamped);
    if (m_kbdCustomPreviewDirty) {
        m_currentKbdProfile.enabled = true;
    } else {
        m_currentKbdProfile.enabled = (clamped > 0 && m_currentKbdProfile.clv0001PresetName != QLatin1String("off"));
    }
    syncCurrentKbdProfileToSettings();
    Q_EMIT keyboardStudioChanged();
}

void ControlCenterController::selectRandomKbdColor() {
    auto* rng = QRandomGenerator::global();
    int r = 0, g = 0, b = 0;
    do {
        r = rng->bounded(256);
        g = rng->bounded(256);
        b = rng->bounded(256);
    } while (r == 0 && g == 0 && b == 0);

    m_kbdRgb = core::RgbColor{r, g, b};
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    m_kbdCustomPreviewDirty = true;
    m_kbdSelectionSource = QStringLiteral("random_rgb");
    m_currentKbdProfile.enabled = true;
    if (m_currentKbdProfile.clv0001BrightnessLevel == 0) {
        m_currentKbdProfile.clv0001BrightnessLevel = 4;
        m_currentKbdProfile.brightnessPct = 100;
    }
    applySelectedKbdColorToActiveZones();
    setStatusBanner(
        QStringLiteral("Selected random static RGB %1 (%2, %3, %4) — click Apply to send to hardware.")
            .arg(m_kbdPickerHex)
            .arg(r)
            .arg(g)
            .arg(b),
        false
    );
}

bool ControlCenterController::applyCustomKbdRgb(int r, int g, int b, int brightnessLevel) {
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        setStatusBanner(
            QStringLiteral("RGB channels (%1, %2, %3) must be within [0, 255].").arg(r).arg(g).arg(b),
            true
        );
        return false;
    }
    if (brightnessLevel < 0 || brightnessLevel > 4) {
        setStatusBanner(
            QStringLiteral("CLV0001 brightness level %1 must be within [0, 4].").arg(brightnessLevel),
            true
        );
        return false;
    }
    setKbdColorFromRgb(r, g, b);
    setKbdBrightnessLevel(brightnessLevel);
    applyKeyboardLightingToHardware();
    return !m_statusBannerIsError;
}

void ControlCenterController::setKbdTimeoutSec(int sec) {
    const int clamped = std::clamp(sec, 0, 3600);
    if (m_currentKbdProfile.timeoutSeconds != clamped) {
        m_currentKbdProfile.timeoutSeconds = clamped;
        syncCurrentKbdProfileToSettings();
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdSelectedZone(int z) {
    const int clamped = m_snapshot.keyboardHw.supportsRgbZones ? std::clamp(z, -1, 3) : -1;
    if (m_kbdSelectedZone != clamped) {
        m_kbdSelectedZone = clamped;
        if (clamped >= 0 && static_cast<size_t>(clamped) < m_currentKbdProfile.zones.size()) {
            const auto& zc = m_currentKbdProfile.zones.at(static_cast<size_t>(clamped));
            m_kbdRgb = core::RgbColor{zc.r, zc.g, zc.b};
            m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
            m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
            m_kbdPickerHex = core::ColorScience::rgbToHex(m_kbdRgb);
        }
        Q_EMIT keyboardStudioChanged();
    }
}

QVariantList ControlCenterController::kbdZoneHexList() const {
    QVariantList list;
    for (const auto& z : m_currentKbdProfile.zones) {
        list.append(z.hex);
    }
    while (list.size() < 4) {
        list.append(m_kbdPickerHex);
    }
    return list;
}

QVariantList ControlCenterController::kbdProfilesModel() const {
    QVariantList list;
    for (const auto& kp : m_settings.keyboardProfiles) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), kp.id);
        m.insert(QStringLiteral("name"), kp.name);
        m.insert(QStringLiteral("mode"), kp.mode);
        m.insert(QStringLiteral("brightnessPct"), kp.brightnessPct);
        m.insert(QStringLiteral("active"), kp.id == m_settings.activeKeyboardProfileId);
        list.append(m);
    }
    return list;
}

void ControlCenterController::applySelectedKbdColorToActiveZones() {
    m_kbdPickerHex = core::ColorScience::rgbToHex(m_kbdRgb);
    if (m_currentKbdProfile.zones.size() < 4) {
        m_currentKbdProfile.zones.resize(4);
    }
    for (int i = 0; i < 4; ++i) {
        if (m_kbdSelectedZone == -1 || m_kbdSelectedZone == i) {
            m_currentKbdProfile.zones[static_cast<size_t>(i)].zoneIndex = i;
            m_currentKbdProfile.zones[static_cast<size_t>(i)].r = m_kbdRgb.r;
            m_currentKbdProfile.zones[static_cast<size_t>(i)].g = m_kbdRgb.g;
            m_currentKbdProfile.zones[static_cast<size_t>(i)].b = m_kbdRgb.b;
            m_currentKbdProfile.zones[static_cast<size_t>(i)].hex = m_kbdPickerHex;
        }
    }
    syncCurrentKbdProfileToSettings();
    Q_EMIT keyboardStudioChanged();
}

bool ControlCenterController::setKbdColorFromHex(const QString& hexStr) {
    core::RgbColor parsed;
    if (!core::ColorScience::parseStrictHex6ToRgb(hexStr, parsed)) {
        setStatusBanner(
            QStringLiteral("Invalid HEX color '%1' — expected #RRGGBB (six hexadecimal digits after '#').")
                .arg(hexStr.trimmed()),
            true
        );
        return false;
    }
    m_kbdRgb = parsed;
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    m_kbdCustomPreviewDirty = true;
    m_kbdSelectionSource = QStringLiteral("custom_rgb");
    m_currentKbdProfile.enabled = true;
    applySelectedKbdColorToActiveZones();
    return true;
}

void ControlCenterController::setKbdColorFromRgb(int r, int g, int b) {
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        setStatusBanner(
            QStringLiteral("RGB channels (%1, %2, %3) must be within [0, 255].").arg(r).arg(g).arg(b),
            true
        );
        return;
    }
    m_kbdRgb = core::RgbColor{r, g, b};
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    m_kbdCustomPreviewDirty = true;
    m_kbdSelectionSource = QStringLiteral("custom_rgb");
    m_currentKbdProfile.enabled = true;
    applySelectedKbdColorToActiveZones();
}

void ControlCenterController::setKbdColorFromHsv(double h, double s, double v) {
    m_kbdHsv = core::HsvColor{h, s, v};
    m_kbdRgb = core::ColorScience::hsvToRgb(m_kbdHsv);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    m_kbdCustomPreviewDirty = true;
    m_kbdSelectionSource = QStringLiteral("custom_rgb");
    m_currentKbdProfile.enabled = true;
    applySelectedKbdColorToActiveZones();
}

void ControlCenterController::setKbdColorFromHsl(double h, double s, double l) {
    m_kbdHsl = core::HslColor{h, s, l};
    m_kbdRgb = core::ColorScience::hslToRgb(m_kbdHsl);
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdCustomPreviewDirty = true;
    m_kbdSelectionSource = QStringLiteral("custom_rgb");
    m_currentKbdProfile.enabled = true;
    applySelectedKbdColorToActiveZones();
}

void ControlCenterController::saveCurrentKbdColorToSwatches() {
    if (!m_settings.customKeyboardSwatches.contains(m_kbdPickerHex)) {
        m_settings.customKeyboardSwatches.prepend(m_kbdPickerHex);
        while (m_settings.customKeyboardSwatches.size() > 12) {
            m_settings.customKeyboardSwatches.removeLast();
        }
        persistSettings();
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::selectKbdProfile(const QString& profileId) {
    for (const auto& kp : m_settings.keyboardProfiles) {
        if (kp.id == profileId) {
            m_currentKbdProfile = kp;
            m_settings.activeKeyboardProfileId = kp.id;
            if (!kp.zones.empty()) {
                const auto& z0 = kp.zones.front();
                m_kbdRgb = core::RgbColor{z0.r, z0.g, z0.b};
                m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
                m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
                m_kbdPickerHex = core::ColorScience::rgbToHex(m_kbdRgb);
            }
            m_kbdCustomPreviewDirty = false;
            m_kbdSelectionSource = QStringLiteral("preset");
            persistSettings();
            Q_EMIT keyboardStudioChanged();
            setStatusBanner(QStringLiteral("Loaded Keyboard Studio profile '%1'.").arg(kp.name), false);
            return;
        }
    }
}

void ControlCenterController::saveNewKbdProfile(const QString& profileName) {
    const QString cleanName = profileName.trimmed();
    if (cleanName.isEmpty()) {
        setStatusBanner(QStringLiteral("Enter a valid profile name before saving."), true);
        return;
    }
    core::KeyboardLightingProfile np = m_currentKbdProfile;
    np.id = QStringLiteral("custom_%1").arg(QDateTime::currentMSecsSinceEpoch());
    np.name = cleanName;
    m_settings.keyboardProfiles.push_back(np);
    m_settings.activeKeyboardProfileId = np.id;
    m_currentKbdProfile = np;
    persistSettings();
    Q_EMIT keyboardStudioChanged();
    setStatusBanner(QStringLiteral("Saved custom keyboard profile '%1'.").arg(cleanName), false);
}

void ControlCenterController::applyKeyboardLightingToHardware() {
    if (!m_transport) {
        return;
    }

    QStringList hexes;
    for (const auto& z : m_currentKbdProfile.zones) {
        hexes.append(z.hex);
    }

    const bool useClvPresetProtocol =
        (m_snapshot.keyboardHw.backendKind == core::KeyboardBackendKind::Clv0001AcpiDsmPreset ||
         m_snapshot.keyboardHw.backendKind == core::KeyboardBackendKind::Clv0001AcpiUnbound);

    QString hwMode = m_currentKbdProfile.mode;
    int effectiveBri = m_currentKbdProfile.enabled ? m_currentKbdProfile.brightnessPct : 0;
    QString zonesCsv = hexes.join(QLatin1Char(','));

    if (useClvPresetProtocol) {
        if (!m_currentKbdProfile.enabled) {
            hwMode = QStringLiteral("preset:off");
            effectiveBri = 0;
            zonesCsv = QStringLiteral("off");
        } else if (m_kbdCustomPreviewDirty) {
            hwMode = (m_currentKbdProfile.mode == QLatin1String("static"))
                         ? QStringLiteral("custom_rgb")
                         : m_currentKbdProfile.mode;
            effectiveBri = hal::Clv0001KeyboardBackend::brightnessLevelToPct(m_currentKbdProfile.clv0001BrightnessLevel);
            zonesCsv = m_kbdPickerHex;
        } else {
            const QString activePreset = m_currentKbdProfile.clv0001PresetName;
            hwMode = QStringLiteral("preset:%1").arg(activePreset);
            effectiveBri = (activePreset == QLatin1String("off"))
                               ? 0
                               : hal::Clv0001KeyboardBackend::brightnessLevelToPct(m_currentKbdProfile.clv0001BrightnessLevel);
            zonesCsv = activePreset;
        }
    }

    uint64_t token = 0;
    const QString opName = QStringLiteral("keyboard_lighting");
    if (!beginControlOperation(opName, token)) {
        return;
    }

    QPointer<ControlCenterController> self(this);
    m_transport->setKeyboardLighting(
        hwMode,
        static_cast<uint>(effectiveBri),
        zonesCsv,
        useClvPresetProtocol,
        [self, token, opName](const AsyncControlOutcome& outcome) {
            if (!self) {
                return;
            }
            self->completeControlOperation(token, opName, outcome, true);
        }
    );
}

void ControlCenterController::selectOperatingProfile(const QString& profileId) {
    if (!m_transport) {
        return;
    }
    const auto profiles = core::ConfigStore::standardOperatingProfiles(m_settings.customOperatingProfile);
    for (const auto& p : profiles) {
        if (p.id == profileId) {
            uint64_t token = 0;
            const QString opName = QStringLiteral("power_profile");
            if (!beginControlOperation(opName, token)) {
                return;
            }

            m_initialLiveProfileSynced = true;
            m_settings.activeOperatingProfileId = p.id;
            persistSettings();
            Q_EMIT settingsUpdated();

            const core::OperatingProfile chosen = p;
            QPointer<ControlCenterController> self(this);
            m_transport->setSystemPowerProfile(
                chosen.ppdProfile,
                [self, token, opName, chosen](const AsyncControlOutcome& outcome) {
                    if (!self) {
                        return;
                    }
                    if (outcome.applied && hal::SysfsRoot::instance().isMockActive()) {
                        hal::ControlOperations::applyCpuPerformanceTuning(
                            chosen.governor,
                            chosen.epp,
                            chosen.turboEnabled,
                            chosen.minPerfPct,
                            chosen.maxPerfPct
                        );
                    }
                    self->completeControlOperation(token, opName, outcome, true);
                }
            );
            return;
        }
    }
}

void ControlCenterController::applyCustomCpuTuning(
    const QString& governor,
    const QString& epp,
    bool turbo,
    int minPerfPct,
    int maxPerfPct
) {
    if (!m_transport) {
        return;
    }
    uint64_t token = 0;
    const QString opName = QStringLiteral("cpu_tuning");
    if (!beginControlOperation(opName, token)) {
        return;
    }

    m_settings.customOperatingProfile.governor = governor;
    m_settings.customOperatingProfile.epp = epp;
    m_settings.customOperatingProfile.turboEnabled = turbo;
    m_settings.customOperatingProfile.minPerfPct = std::min(minPerfPct, maxPerfPct);
    m_settings.customOperatingProfile.maxPerfPct = std::max(minPerfPct, maxPerfPct);
    m_initialLiveProfileSynced = true;
    m_settings.activeOperatingProfileId = QStringLiteral("custom");
    persistSettings();
    Q_EMIT settingsUpdated();

    QPointer<ControlCenterController> self(this);
    m_transport->setCpuPerformanceTuning(
        governor,
        epp,
        turbo,
        m_settings.customOperatingProfile.minPerfPct,
        m_settings.customOperatingProfile.maxPerfPct,
        [self, token, opName](const AsyncControlOutcome& outcome) {
            if (!self) {
                return;
            }
            self->completeControlOperation(token, opName, outcome, true);
        }
    );
}

void ControlCenterController::updateFanCurvePoints(const QVariantList& points) {
    std::vector<core::FanCurvePoint> pts;
    for (const QVariant& v : points) {
        const QVariantMap m = v.toMap();
        pts.push_back(core::FanCurvePoint{
            m.value(QStringLiteral("tempC")).toInt(),
            m.value(QStringLiteral("dutyPct")).toInt()
        });
    }
    if (pts.size() >= 2) {
        m_settings.customFanCurve = pts;
        persistSettings();
        Q_EMIT settingsUpdated();
    }
}

void ControlCenterController::setFanMode(const QString& mode, int manualDutyPct) {
    if (!m_transport) {
        return;
    }
    uint64_t token = 0;
    const QString opName = QStringLiteral("fan_mode");
    if (!beginControlOperation(opName, token)) {
        return;
    }

    QPointer<ControlCenterController> self(this);
    m_transport->setFanControlMode(
        mode,
        manualDutyPct,
        [self, token, opName](const AsyncControlOutcome& outcome) {
            if (!self) {
                return;
            }
            self->completeControlOperation(token, opName, outcome, true);
        }
    );
}

void ControlCenterController::applyBatteryChargeLimit(int thresholdPct) {
    if (!m_transport) {
        return;
    }
    const int clamped = std::clamp(thresholdPct, 50, 100);
    if (m_snapshot.battery.chargeLimitSupported &&
        m_snapshot.battery.chargeEndThresholdPct >= 50 &&
        m_snapshot.battery.chargeEndThresholdPct <= 100 &&
        m_snapshot.battery.chargeEndThresholdPct != clamped) {
        m_batPreviousThresholdPct = m_snapshot.battery.chargeEndThresholdPct;
    }

    uint64_t token = 0;
    const QString opName = QStringLiteral("battery_limit");
    if (!beginControlOperation(opName, token)) {
        return;
    }

    QPointer<ControlCenterController> self(this);
    m_transport->setBatteryChargeLimit(
        clamped,
        m_batPreviousThresholdPct,
        [self, token, opName](const AsyncControlOutcome& outcome) {
            if (!self) {
                return;
            }
            self->completeControlOperation(token, opName, outcome, true);
        }
    );
}

void ControlCenterController::restorePreviousBatteryChargeLimit() {
    applyBatteryChargeLimit(m_batPreviousThresholdPct);
}

void ControlCenterController::applyDisplayBrightness(int brightnessPct) {
    if (!m_transport) {
        return;
    }
    uint64_t token = 0;
    const QString opName = QStringLiteral("display_brightness");
    if (!beginControlOperation(opName, token)) {
        return;
    }

    QPointer<ControlCenterController> self(this);
    m_transport->setDisplayBrightness(
        brightnessPct,
        [self, token, opName](const AsyncControlOutcome& outcome) {
            if (!self) {
                return;
            }
            self->completeControlOperation(token, opName, outcome, true);
        }
    );
}

QVariantList ControlCenterController::capabilitiesModel() const {
    QVariantList list;
    for (const auto& cap : m_snapshot.capabilities) {
        QVariantMap m;
        m.insert(QStringLiteral("id"), cap.id);
        m.insert(QStringLiteral("title"), cap.title);
        m.insert(QStringLiteral("category"), cap.category);
        m.insert(QStringLiteral("interfacePath"), cap.interfacePath);
        m.insert(QStringLiteral("stateBadge"), core::capabilityStateToBadge(cap.state));
        m.insert(QStringLiteral("stateId"), core::capabilityStateToId(cap.state));
        m.insert(QStringLiteral("isWritable"), cap.isWritable);
        m.insert(QStringLiteral("summary"), cap.summary);
        m.insert(QStringLiteral("detailExplanation"), cap.detailExplanation);
        list.append(m);
    }
    return list;
}

QVariantList ControlCenterController::diagnosticLogsModel() const {
    QVariantList list;
    for (const auto& entry : core::DiagnosticRedactor::instance().recentLogs()) {
        QVariantMap m;
        m.insert(QStringLiteral("timestamp"), entry.timestampIso);
        m.insert(QStringLiteral("level"), entry.level);
        m.insert(QStringLiteral("subsystem"), entry.subsystem);
        m.insert(QStringLiteral("message"), entry.message);
        list.append(m);
    }
    return list;
}

void ControlCenterController::switchHardwareMode(int modeIndex) {
    if (m_coordinator) {
        m_coordinator->invalidateAndReset();
    }
    // Cancel/invalidate any pending control callback from the prior hardware mode
    ++m_controlToken;
    m_controlOperationPending = false;
    m_controlOperationName.clear();
    m_controlAuthState = core::ControlAuthState::Idle;
    m_controlLastErrorCode.clear();
    m_controlLastErrorMessage.clear();

    if (modeIndex == 1) {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockPhysicalMirror);
    } else if (modeIndex == 2) {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);
    } else if (modeIndex == 3) {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockClv0001DriverActive);
    } else {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }

    // Explicitly reset snapshot & mark Unavailable until async worker completes for new backend
    m_snapshot = core::SystemSnapshot{};
    m_hasValidSnapshot = false;
    m_freshness = core::TelemetryFreshness::Unavailable;
    m_telemetryAgeMs = -1;
    m_telemetryErrorMessage.clear();
    m_firstPoll = true;
    m_histCpuTemp.clear();
    m_histGpuTemp.clear();
    m_histCpuLoad.clear();
    m_histGpuLoad.clear();
    m_histBatteryWatts.clear();

    core::DiagnosticRedactor::instance().appendLog(
        QStringLiteral("INFO"),
        QStringLiteral("HAL"),
        QStringLiteral("Switched hardware backend mode to: %1").arg(hal::SysfsRoot::instance().scenarioLabel())
    );
    Q_EMIT controlOperationChanged();
    Q_EMIT telemetryFreshnessChanged();
    Q_EMIT telemetryUpdated();
    refreshNow();
    setStatusBanner(
        QStringLiteral("Active Backend Mode: %1").arg(hal::SysfsRoot::instance().scenarioLabel()),
        false
    );
}

QString ControlCenterController::exportDiagnosticReport(const QString& targetDir) {
    const QString dirPath = targetDir.trimmed().isEmpty() ? m_configStore.configDirectory() : targetDir.trimmed();
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    const QString mdPath = dir.filePath(QStringLiteral("acer-diagnostic-report.md"));
    const QString jsonPath = dir.filePath(QStringLiteral("acer-diagnostic-report.json"));

    QFile mdFile(mdPath);
    if (mdFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        mdFile.write(core::DiagnosticRedactor::instance().buildMarkdownReport(m_snapshot).toUtf8());
        mdFile.close();
    }

    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QJsonDocument doc(core::DiagnosticRedactor::instance().buildJsonReport(m_snapshot));
        jsonFile.write(doc.toJson(QJsonDocument::Indented));
        jsonFile.close();
    }

    setStatusBanner(QStringLiteral("Exported redacted diagnostic reports to %1").arg(mdPath), false);
    return mdPath;
}

bool ControlCenterController::exportProfileBundle(const QString& filePath) {
    QString err;
    const QString target = filePath.trimmed().isEmpty()
                               ? QDir(m_configStore.configDirectory()).filePath(QStringLiteral("acer-profiles-export.json"))
                               : filePath.trimmed();
    const bool ok = m_configStore.exportProfileBundleToFile(m_settings, target, &err);
    setStatusBanner(
        ok ? QStringLiteral("Exported profile bundle to %1").arg(target)
           : QStringLiteral("Profile export failed: %1").arg(err),
        !ok
    );
    return ok;
}

bool ControlCenterController::importProfileBundle(const QString& filePath) {
    QString err;
    const QString target = filePath.trimmed().isEmpty()
                               ? QDir(m_configStore.configDirectory()).filePath(QStringLiteral("acer-profiles-export.json"))
                               : filePath.trimmed();
    core::AppSettings imported;
    const bool ok = m_configStore.importProfileBundleFromFile(target, imported, &err);
    if (ok) {
        m_settings = imported;
        if (m_theme) {
            {
                QSignalBlocker blocker(m_theme);
                m_theme->setDarkTheme(imported.darkTheme);
                m_theme->setReducedMotion(imported.reducedMotion);
                m_theme->setCompactNavigation(imported.compactNavigation);
                m_theme->setAccentSwatches(imported.customAccentSwatches);
                m_theme->setAccentFromHex(imported.accentColorHex);
            }
            Q_EMIT m_theme->themeChanged();
            Q_EMIT m_theme->motionChanged();
            Q_EMIT m_theme->layoutChanged();
            Q_EMIT m_theme->swatchesChanged();
            Q_EMIT m_theme->accentChanged();
        }
        m_settings = imported;
        loadActiveKeyboardProfileFromSettings();
        persistSettings();
        Q_EMIT settingsUpdated();
        Q_EMIT keyboardStudioChanged();
    }
    setStatusBanner(
        ok ? QStringLiteral("Imported and validated profile bundle from %1").arg(target)
           : QStringLiteral("Profile import failed: %1").arg(err),
        !ok
    );
    return ok;
}

} // namespace acc::app
