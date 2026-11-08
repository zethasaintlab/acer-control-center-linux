// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/ControlCenterController.h"
#include "core/ColorScience.h"
#include "core/DiagnosticRedactor.h"
#include "hal/ControlOperations.h"
#include "hal/SysfsRoot.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
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
    : QObject(parent), m_theme(themeEngine) {
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

    if (!m_settings.keyboardProfiles.empty()) {
        m_currentKbdProfile = m_settings.keyboardProfiles.front();
        for (const auto& kp : m_settings.keyboardProfiles) {
            if (kp.id == m_settings.activeKeyboardProfileId) {
                m_currentKbdProfile = kp;
                break;
            }
        }
        if (!m_currentKbdProfile.zones.empty()) {
            const auto& z0 = m_currentKbdProfile.zones.front();
            setKbdColorFromRgb(z0.r, z0.g, z0.b);
        }
    }

    // Subscribe to systemd-logind PrepareForSleep(bool) for suspend/resume recovery
    QDBusConnection::systemBus().connect(
        QStringLiteral("org.freedesktop.login1"),
        QStringLiteral("/org/freedesktop/login1"),
        QStringLiteral("org.freedesktop.login1.Manager"),
        QStringLiteral("PrepareForSleep"),
        this,
        SLOT(onPrepareForSleep(bool))
    );

    connect(&m_pollTimer, &QTimer::timeout, this, &ControlCenterController::refreshNow);
    m_pollTimer.setInterval(m_settings.pollingIntervalMs);

    refreshNow();
    // Seed at least 12 initial points from the real reading so strip charts render an immediate baseline
    for (int i = 0; i < 12; ++i) {
        appendHistorySample(m_histCpuTemp, m_snapshot.cpu.packageTempC);
        appendHistorySample(m_histGpuTemp, m_snapshot.gpu.tempC);
        appendHistorySample(m_histCpuLoad, m_snapshot.cpu.totalUtilizationPct);
        appendHistorySample(m_histGpuLoad, m_snapshot.gpu.gpuUtilizationPct);
        appendHistorySample(m_histBatteryWatts, m_snapshot.battery.powerWatts);
    }
    Q_EMIT telemetryUpdated();

    m_pollTimer.start();
    setupSystemTray();
}

void ControlCenterController::setupSystemTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(QStringLiteral("#3B9EFF")), 2.0));
    p.setBrush(QColor(QStringLiteral("#13161B")));
    p.drawRoundedRect(QRectF(2, 2, 28, 28), 5.0, 5.0);
    p.setPen(QColor(QStringLiteral("#ECEFF4")));
    QFont f(QStringLiteral("monospace"));
    f.setPixelSize(13);
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRectF(2, 2, 28, 28), Qt::AlignCenter, QStringLiteral("AC"));
    p.end();

    m_trayIcon = std::make_unique<QSystemTrayIcon>(QIcon(pix), this);
    m_trayIcon->setToolTip(QStringLiteral("Acer Control Center — Aspire A715-59G"));
    m_trayIcon->show();
}

void ControlCenterController::onPrepareForSleep(bool goingToSleep) {
    if (goingToSleep) {
        m_pollTimer.stop();
        core::DiagnosticRedactor::instance().appendLog(
            QStringLiteral("INFO"),
            QStringLiteral("POWER"),
            QStringLiteral("System entering suspend (PrepareForSleep=true); paused telemetry polling.")
        );
    } else {
        core::DiagnosticRedactor::instance().appendLog(
            QStringLiteral("INFO"),
            QStringLiteral("POWER"),
            QStringLiteral("System resumed from sleep (PrepareForSleep=false); re-verifying hardware state.")
        );
        refreshNow();
        m_pollTimer.start(m_settings.pollingIntervalMs);
        setStatusBanner(QStringLiteral("Resumed from sleep — hardware telemetry & capabilities re-verified."), false);
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

void ControlCenterController::pollRaplFromDaemonIfAvailable() {
    if (hal::SysfsRoot::instance().isMockActive()) {
        return;
    }
    QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        QStringLiteral("/io/github/zethasaintlab/AcerControlCenter1"),
        QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
        QStringLiteral("GetRaplPowerSnapshot")
    );
    const QDBusMessage reply = QDBusConnection::systemBus().call(call, QDBus::Block, 120);
    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 6) {
        const uint64_t pkgUj = reply.arguments().at(0).toULongLong();
        const uint64_t coreUj = reply.arguments().at(1).toULongLong();
        const uint64_t psysUj = reply.arguments().at(3).toULongLong();
        const int64_t tsUs = reply.arguments().at(4).toLongLong();
        const bool avail = reply.arguments().at(5).toBool();
        m_collector.updateRaplCounters(pkgUj, coreUj, psysUj, tsUs, avail);
    }
}

void ControlCenterController::refreshNow() {
    pollRaplFromDaemonIfAvailable();
    m_snapshot = m_collector.collectSnapshot(m_settings.activeOperatingProfileId);

    appendHistorySample(m_histCpuTemp, m_snapshot.cpu.packageTempC);
    appendHistorySample(m_histGpuTemp, m_snapshot.gpu.tempC);
    appendHistorySample(m_histCpuLoad, m_snapshot.cpu.totalUtilizationPct);
    appendHistorySample(m_histGpuLoad, m_snapshot.gpu.gpuUtilizationPct);
    appendHistorySample(m_histBatteryWatts, m_snapshot.battery.powerWatts);

    const bool prevAcOnline = m_lastAcOnlineState;
    const bool wasFirstPoll = m_firstPoll;
    m_lastAcOnlineState = m_snapshot.battery.acOnline;
    m_firstPoll = false;

    // Check AC/Battery auto profile transition
    if (!wasFirstPoll && m_settings.autoSwitchProfileOnPower && m_snapshot.battery.acOnline != prevAcOnline) {
        const QString targetProf = m_snapshot.battery.acOnline ? m_settings.acOperatingProfileId : m_settings.batteryOperatingProfileId;
        selectOperatingProfile(targetProf);
    }

    if (m_trayIcon) {
        m_trayIcon->setToolTip(
            QStringLiteral("Acer Aspire A715-59G  |  CPU: %1°C (%2%)  |  GPU: %3°C  |  Profile: %4")
                .arg(m_snapshot.cpu.packageTempC, 0, 'f', 0)
                .arg(m_snapshot.cpu.totalUtilizationPct, 0, 'f', 0)
                .arg(m_snapshot.gpu.tempC, 0, 'f', 0)
                .arg(m_settings.activeOperatingProfileId.toUpper())
        );
    }

    Q_EMIT telemetryUpdated();
}

QString ControlCenterController::productTitle() const {
    return QStringLiteral("%1 %2 (v%3)")
        .arg(m_snapshot.dmi.sysVendor, m_snapshot.dmi.productName, m_snapshot.dmi.productVersion);
}

QString ControlCenterController::boardTitle() const {
    return QStringLiteral("%1 // %2").arg(m_snapshot.dmi.boardName, m_snapshot.dmi.boardVendor);
}

QString ControlCenterController::biosTitle() const {
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

void ControlCenterController::setKbdEnabled(bool e) {
    if (m_currentKbdProfile.enabled != e) {
        m_currentKbdProfile.enabled = e;
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdMode(const QString& m) {
    if (m_currentKbdProfile.mode != m) {
        m_currentKbdProfile.mode = m;
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdBrightnessPct(int b) {
    const int clamped = std::clamp(b, 0, 100);
    if (m_currentKbdProfile.brightnessPct != clamped) {
        m_currentKbdProfile.brightnessPct = clamped;
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdTimeoutSec(int sec) {
    const int clamped = std::clamp(sec, 0, 3600);
    if (m_currentKbdProfile.timeoutSeconds != clamped) {
        m_currentKbdProfile.timeoutSeconds = clamped;
        Q_EMIT keyboardStudioChanged();
    }
}

void ControlCenterController::setKbdSelectedZone(int z) {
    const int clamped = std::clamp(z, -1, 3);
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
    // Sync back into settings profile vector
    for (auto& kp : m_settings.keyboardProfiles) {
        if (kp.id == m_currentKbdProfile.id) {
            kp = m_currentKbdProfile;
            break;
        }
    }
    persistSettings();
    Q_EMIT keyboardStudioChanged();
}

bool ControlCenterController::setKbdColorFromHex(const QString& hexStr) {
    core::RgbColor parsed;
    if (!core::ColorScience::parseHexToRgb(hexStr, parsed)) {
        return false;
    }
    m_kbdRgb = parsed;
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    applySelectedKbdColorToActiveZones();
    return true;
}

void ControlCenterController::setKbdColorFromRgb(int r, int g, int b) {
    m_kbdRgb = core::ColorScience::clampRgb(r, g, b);
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    applySelectedKbdColorToActiveZones();
}

void ControlCenterController::setKbdColorFromHsv(double h, double s, double v) {
    m_kbdHsv = core::HsvColor{h, s, v};
    m_kbdRgb = core::ColorScience::hsvToRgb(m_kbdHsv);
    m_kbdHsl = core::ColorScience::rgbToHsl(m_kbdRgb);
    applySelectedKbdColorToActiveZones();
}

void ControlCenterController::setKbdColorFromHsl(double h, double s, double l) {
    m_kbdHsl = core::HslColor{h, s, l};
    m_kbdRgb = core::ColorScience::hslToRgb(m_kbdHsl);
    m_kbdHsv = core::ColorScience::rgbToHsv(m_kbdRgb);
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
    QStringList hexes;
    for (const auto& z : m_currentKbdProfile.zones) {
        hexes.append(z.hex);
    }
    const int effectiveBri = m_currentKbdProfile.enabled ? m_currentKbdProfile.brightnessPct : 0;
    const core::ControlResult res = hal::ControlOperations::applyKeyboardLighting(
        m_currentKbdProfile.mode,
        effectiveBri,
        hexes.join(QLatin1Char(','))
    );
    refreshNow();
    setStatusBanner(res.message, !res.applied);
}

void ControlCenterController::selectOperatingProfile(const QString& profileId) {
    const auto profiles = core::ConfigStore::standardOperatingProfiles(m_settings.customOperatingProfile);
    for (const auto& p : profiles) {
        if (p.id == profileId) {
            m_settings.activeOperatingProfileId = p.id;
            persistSettings();
            Q_EMIT settingsUpdated();

            const core::ControlResult ppdRes = hal::ControlOperations::applySystemPowerProfile(p.ppdProfile);
            if (hal::SysfsRoot::instance().isMockActive()) {
                hal::ControlOperations::applyCpuPerformanceTuning(
                    p.governor, p.epp, p.turboEnabled, p.minPerfPct, p.maxPerfPct
                );
            }
            refreshNow();
            setStatusBanner(ppdRes.message, !ppdRes.applied);
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
    m_settings.customOperatingProfile.governor = governor;
    m_settings.customOperatingProfile.epp = epp;
    m_settings.customOperatingProfile.turboEnabled = turbo;
    m_settings.customOperatingProfile.minPerfPct = std::min(minPerfPct, maxPerfPct);
    m_settings.customOperatingProfile.maxPerfPct = std::max(minPerfPct, maxPerfPct);
    m_settings.activeOperatingProfileId = QStringLiteral("custom");
    persistSettings();
    Q_EMIT settingsUpdated();

    if (!hal::SysfsRoot::instance().isMockActive()) {
        // Call privileged daemon over D-Bus if running
        QDBusMessage call = QDBusMessage::createMethodCall(
            QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
            QStringLiteral("/io/github/zethasaintlab/AcerControlCenter1"),
            QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
            QStringLiteral("SetCpuPerformanceTuning")
        );
        call << governor << epp << turbo
             << static_cast<uint>(m_settings.customOperatingProfile.minPerfPct)
             << static_cast<uint>(m_settings.customOperatingProfile.maxPerfPct);
        const QDBusMessage reply = QDBusConnection::systemBus().call(call, QDBus::Block, 3000);
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 3) {
            const bool ok = reply.arguments().at(0).toBool();
            const QString rb = reply.arguments().at(1).toString();
            const QString errCode = reply.arguments().at(2).toString();
            refreshNow();
            setStatusBanner(
                ok ? QStringLiteral("Daemon applied & verified CPU P-state tuning (%1).").arg(rb)
                   : QStringLiteral("Daemon rejected CPU tuning (%1).").arg(errCode),
                !ok
            );
            return;
        }
    }

    const core::ControlResult res = hal::ControlOperations::applyCpuPerformanceTuning(
        governor,
        epp,
        turbo,
        m_settings.customOperatingProfile.minPerfPct,
        m_settings.customOperatingProfile.maxPerfPct
    );
    refreshNow();
    setStatusBanner(res.message, !res.applied);
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
    const core::ControlResult res = hal::ControlOperations::applyFanControlMode(mode, manualDutyPct);
    refreshNow();
    setStatusBanner(res.message, !res.applied);
}

void ControlCenterController::applyBatteryChargeLimit(int thresholdPct) {
    int rbPct = 100;
    const core::ControlResult res = hal::ControlOperations::applyBatteryChargeLimit(thresholdPct, rbPct);
    refreshNow();
    setStatusBanner(res.message, !res.applied);
}

void ControlCenterController::applyDisplayBrightness(int brightnessPct) {
    const core::ControlResult res = hal::ControlOperations::applyDisplayBrightness(brightnessPct);
    refreshNow();
    setStatusBanner(res.message, !res.applied);
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
    if (modeIndex == 1) {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockPhysicalMirror);
    } else if (modeIndex == 2) {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);
    } else {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }
    core::DiagnosticRedactor::instance().appendLog(
        QStringLiteral("INFO"),
        QStringLiteral("HAL"),
        QStringLiteral("Switched hardware backend mode to: %1").arg(hal::SysfsRoot::instance().scenarioLabel())
    );
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
    const bool ok = m_configStore.importProfileBundleFromFile(target, m_settings, &err);
    if (ok) {
        if (m_theme) {
            m_theme->setDarkTheme(m_settings.darkTheme);
            m_theme->setReducedMotion(m_settings.reducedMotion);
            m_theme->setCompactNavigation(m_settings.compactNavigation);
            m_theme->setAccentFromHex(m_settings.accentColorHex);
        }
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
