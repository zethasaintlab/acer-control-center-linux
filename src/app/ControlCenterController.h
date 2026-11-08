// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "app/ThemeEngine.h"
#include "core/ConfigStore.h"
#include "core/DomainModels.h"
#include "hal/TelemetryCollector.h"

#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <deque>
#include <memory>

namespace acc::app {

class ControlCenterController : public QObject {
    Q_OBJECT

    // Navigation & Global Banner
    Q_PROPERTY(int currentTabIndex READ currentTabIndex WRITE setCurrentTabIndex NOTIFY navigationChanged)
    Q_PROPERTY(QString statusBannerText READ statusBannerText NOTIFY statusBannerChanged)
    Q_PROPERTY(bool statusBannerIsError READ statusBannerIsError NOTIFY statusBannerChanged)
    Q_PROPERTY(bool mockModeActive READ mockModeActive NOTIFY telemetryUpdated)
    Q_PROPERTY(QString hardwareModeLabel READ hardwareModeLabel NOTIFY telemetryUpdated)
    Q_PROPERTY(int hardwareModeIndex READ hardwareModeIndex NOTIFY telemetryUpdated)
    Q_PROPERTY(int pollingIntervalMs READ pollingIntervalMs WRITE setPollingIntervalMs NOTIFY settingsUpdated)

    // DMI & Platform Identity
    Q_PROPERTY(QString productTitle READ productTitle NOTIFY telemetryUpdated)
    Q_PROPERTY(QString boardTitle READ boardTitle NOTIFY telemetryUpdated)
    Q_PROPERTY(QString biosTitle READ biosTitle NOTIFY telemetryUpdated)
    Q_PROPERTY(QString odmPlatformNote READ odmPlatformNote NOTIFY telemetryUpdated)
    Q_PROPERTY(QString wmiGuidsJoined READ wmiGuidsJoined NOTIFY telemetryUpdated)

    // 01 // OVERVIEW & 02 // PERFORMANCE (CPU & Memory)
    Q_PROPERTY(QString cpuModelName READ cpuModelName NOTIFY telemetryUpdated)
    Q_PROPERTY(double cpuUtilizationPct READ cpuUtilizationPct NOTIFY telemetryUpdated)
    Q_PROPERTY(double cpuPackageTempC READ cpuPackageTempC NOTIFY telemetryUpdated)
    Q_PROPERTY(int cpuAvgFreqMhz READ cpuAvgFreqMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(int cpuMaxFreqMhz READ cpuMaxFreqMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(double cpuPackagePowerW READ cpuPackagePowerW NOTIFY telemetryUpdated)
    Q_PROPERTY(bool cpuPowerAvailable READ cpuPowerAvailable NOTIFY telemetryUpdated)
    Q_PROPERTY(QString cpuPowerSource READ cpuPowerSource NOTIFY telemetryUpdated)
    Q_PROPERTY(QString cpuGovernor READ cpuGovernor NOTIFY telemetryUpdated)
    Q_PROPERTY(QString cpuEpp READ cpuEpp NOTIFY telemetryUpdated)
    Q_PROPERTY(bool cpuTurboEnabled READ cpuTurboEnabled NOTIFY telemetryUpdated)
    Q_PROPERTY(int cpuMinPerfPct READ cpuMinPerfPct NOTIFY telemetryUpdated)
    Q_PROPERTY(int cpuMaxPerfPct READ cpuMaxPerfPct NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList cpuCoresModel READ cpuCoresModel NOTIFY telemetryUpdated)

    Q_PROPERTY(int memTotalMiB READ memTotalMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(int memUsedMiB READ memUsedMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(int memAvailableMiB READ memAvailableMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(int memCachedMiB READ memCachedMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(double memUtilizationPct READ memUtilizationPct NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiCpuSome10 READ psiCpuSome10 NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiMemSome10 READ psiMemSome10 NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiIoSome10 READ psiIoSome10 NOTIFY telemetryUpdated)

    // Rolling Time-Series Arrays (60 samples)
    Q_PROPERTY(QVariantList historyCpuTemp READ historyCpuTemp NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList historyGpuTemp READ historyGpuTemp NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList historyCpuLoad READ historyCpuLoad NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList historyGpuLoad READ historyGpuLoad NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList historyBatteryWatts READ historyBatteryWatts NOTIFY telemetryUpdated)

    // Operating Profiles
    Q_PROPERTY(QString activeProfileId READ activeProfileId NOTIFY settingsUpdated)
    Q_PROPERTY(QString activeTunedProfile READ activeTunedProfile NOTIFY telemetryUpdated)
    Q_PROPERTY(bool autoSwitchOnPower READ autoSwitchOnPower WRITE setAutoSwitchOnPower NOTIFY settingsUpdated)
    Q_PROPERTY(QString acProfileId READ acProfileId WRITE setAcProfileId NOTIFY settingsUpdated)
    Q_PROPERTY(QString batteryProfileId READ batteryProfileId WRITE setBatteryProfileId NOTIFY settingsUpdated)
    Q_PROPERTY(QVariantList operatingProfilesModel READ operatingProfilesModel NOTIFY settingsUpdated)

    // 03 // THERMAL & FAN CENTER
    Q_PROPERTY(QString thermalStateLabel READ thermalStateLabel NOTIFY telemetryUpdated)
    Q_PROPERTY(QString fanControlModeLabel READ fanControlModeLabel NOTIFY telemetryUpdated)
    Q_PROPERTY(bool manualFanSupported READ manualFanSupported NOTIFY telemetryUpdated)
    Q_PROPERTY(QString fanStatusExplanation READ fanStatusExplanation NOTIFY telemetryUpdated)
    Q_PROPERTY(double maxNvmeTempC READ maxNvmeTempC NOTIFY telemetryUpdated)
    Q_PROPERTY(double maxBoardTempC READ maxBoardTempC NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList thermalSensorsModel READ thermalSensorsModel NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList fansModel READ fansModel NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList fanCurvePoints READ fanCurvePoints NOTIFY settingsUpdated)

    // 04 // KEYBOARD LIGHTING STUDIO (Independent Physical Keyboard Color System)
    Q_PROPERTY(QString kbdCapabilityBadge READ kbdCapabilityBadge NOTIFY telemetryUpdated)
    Q_PROPERTY(QString kbdRoutingBadge READ kbdRoutingBadge NOTIFY telemetryUpdated)
    Q_PROPERTY(QString kbdHardwareExplanation READ kbdHardwareExplanation NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdHardwareConnected READ kbdHardwareConnected NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdRgbSupported READ kbdRgbSupported NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdEnabled READ kbdEnabled WRITE setKbdEnabled NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QString kbdMode READ kbdMode WRITE setKbdMode NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdBrightnessPct READ kbdBrightnessPct WRITE setKbdBrightnessPct NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdTimeoutSec READ kbdTimeoutSec WRITE setKbdTimeoutSec NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdSelectedZone READ kbdSelectedZone WRITE setKbdSelectedZone NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QVariantList kbdZoneHexList READ kbdZoneHexList NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QString kbdPickerHex READ kbdPickerHex NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdPickerR READ kbdPickerR NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdPickerG READ kbdPickerG NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdPickerB READ kbdPickerB NOTIFY keyboardStudioChanged)
    Q_PROPERTY(double kbdPickerHue READ kbdPickerHue NOTIFY keyboardStudioChanged)
    Q_PROPERTY(double kbdPickerSat READ kbdPickerSat NOTIFY keyboardStudioChanged)
    Q_PROPERTY(double kbdPickerVal READ kbdPickerVal NOTIFY keyboardStudioChanged)
    Q_PROPERTY(double kbdPickerHslLight READ kbdPickerHslLight NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QStringList kbdCustomSwatches READ kbdCustomSwatches NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QString activeKbdProfileId READ activeKbdProfileId NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QVariantList kbdProfilesModel READ kbdProfilesModel NOTIFY keyboardStudioChanged)

    // 05 // BATTERY & POWER
    Q_PROPERTY(bool batPresent READ batPresent NOTIFY telemetryUpdated)
    Q_PROPERTY(bool acOnline READ acOnline NOTIFY telemetryUpdated)
    Q_PROPERTY(QString batStatus READ batStatus NOTIFY telemetryUpdated)
    Q_PROPERTY(QString batTechnology READ batTechnology NOTIFY telemetryUpdated)
    Q_PROPERTY(int batCapacityPct READ batCapacityPct NOTIFY telemetryUpdated)
    Q_PROPERTY(double batVoltageV READ batVoltageV NOTIFY telemetryUpdated)
    Q_PROPERTY(double batCurrentA READ batCurrentA NOTIFY telemetryUpdated)
    Q_PROPERTY(double batPowerW READ batPowerW NOTIFY telemetryUpdated)
    Q_PROPERTY(double batChargeFullAh READ batChargeFullAh NOTIFY telemetryUpdated)
    Q_PROPERTY(double batChargeDesignAh READ batChargeDesignAh NOTIFY telemetryUpdated)
    Q_PROPERTY(double batWearHealthPct READ batWearHealthPct NOTIFY telemetryUpdated)
    Q_PROPERTY(int batCycleCount READ batCycleCount NOTIFY telemetryUpdated)
    Q_PROPERTY(bool batChargeLimitSupported READ batChargeLimitSupported NOTIFY telemetryUpdated)
    Q_PROPERTY(int batChargeThresholdPct READ batChargeThresholdPct NOTIFY telemetryUpdated)
    Q_PROPERTY(QString batStatusExplanation READ batStatusExplanation NOTIFY telemetryUpdated)
    Q_PROPERTY(int displayBrightnessPct READ displayBrightnessPct NOTIFY telemetryUpdated)
    Q_PROPERTY(bool displayBrightnessSupported READ displayBrightnessSupported NOTIFY telemetryUpdated)

    // 06 // NVIDIA GPU CENTER
    Q_PROPERTY(bool gpuAvailable READ gpuAvailable NOTIFY telemetryUpdated)
    Q_PROPERTY(QString gpuSourceLabel READ gpuSourceLabel NOTIFY telemetryUpdated)
    Q_PROPERTY(QString gpuName READ gpuName NOTIFY telemetryUpdated)
    Q_PROPERTY(QString gpuDriverVersion READ gpuDriverVersion NOTIFY telemetryUpdated)
    Q_PROPERTY(QString gpuPciBusId READ gpuPciBusId NOTIFY telemetryUpdated)
    Q_PROPERTY(QString gpuSubsystemId READ gpuSubsystemId NOTIFY telemetryUpdated)
    Q_PROPERTY(double gpuTempC READ gpuTempC NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuUtilizationPct READ gpuUtilizationPct NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuMemUtilizationPct READ gpuMemUtilizationPct NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuVramTotalMiB READ gpuVramTotalMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuVramUsedMiB READ gpuVramUsedMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuGraphicsClockMhz READ gpuGraphicsClockMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuSmClockMhz READ gpuSmClockMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuMemClockMhz READ gpuMemClockMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuMaxGraphicsClockMhz READ gpuMaxGraphicsClockMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(int gpuMaxMemClockMhz READ gpuMaxMemClockMhz NOTIFY telemetryUpdated)
    Q_PROPERTY(double gpuPowerDrawW READ gpuPowerDrawW NOTIFY telemetryUpdated)
    Q_PROPERTY(double gpuRawPowerW READ gpuRawPowerW NOTIFY telemetryUpdated)
    Q_PROPERTY(bool gpuPowerRailGated READ gpuPowerRailGated NOTIFY telemetryUpdated)
    Q_PROPERTY(double gpuDefaultLimitW READ gpuDefaultLimitW NOTIFY telemetryUpdated)
    Q_PROPERTY(double gpuMaxLimitW READ gpuMaxLimitW NOTIFY telemetryUpdated)
    Q_PROPERTY(QString gpuPstate READ gpuPstate NOTIFY telemetryUpdated)
    Q_PROPERTY(QStringList gpuThrottleReasons READ gpuThrottleReasons NOTIFY telemetryUpdated)

    // 07 // DIAGNOSTICS
    Q_PROPERTY(QVariantList capabilitiesModel READ capabilitiesModel NOTIFY telemetryUpdated)
    Q_PROPERTY(QVariantList diagnosticLogsModel READ diagnosticLogsModel NOTIFY logsUpdated)

public:
    explicit ControlCenterController(ThemeEngine* themeEngine, QObject* parent = nullptr);

    int currentTabIndex() const { return m_currentTabIndex; }
    void setCurrentTabIndex(int idx);

    QString statusBannerText() const { return m_statusBannerText; }
    bool statusBannerIsError() const { return m_statusBannerIsError; }

    bool mockModeActive() const { return m_snapshot.mockModeActive; }
    QString hardwareModeLabel() const { return m_snapshot.mockScenarioName; }
    int hardwareModeIndex() const;

    int pollingIntervalMs() const { return m_settings.pollingIntervalMs; }
    void setPollingIntervalMs(int ms);

    // DMI
    QString productTitle() const;
    QString boardTitle() const;
    QString biosTitle() const;
    QString odmPlatformNote() const { return m_snapshot.dmi.odmSubsystemNote; }
    QString wmiGuidsJoined() const { return m_snapshot.dmi.wmiGuids.join(QStringLiteral("  |  ")); }

    // CPU & Memory
    QString cpuModelName() const { return m_snapshot.cpu.modelName; }
    double cpuUtilizationPct() const { return m_snapshot.cpu.totalUtilizationPct; }
    double cpuPackageTempC() const { return m_snapshot.cpu.packageTempC; }
    int cpuAvgFreqMhz() const { return m_snapshot.cpu.avgFreqMhz; }
    int cpuMaxFreqMhz() const { return m_snapshot.cpu.maxFreqMhz; }
    double cpuPackagePowerW() const { return m_snapshot.cpu.packagePowerWatts; }
    bool cpuPowerAvailable() const { return m_snapshot.cpu.powerSourceAvailable; }
    QString cpuPowerSource() const { return m_snapshot.cpu.powerSourceLabel; }
    QString cpuGovernor() const { return m_snapshot.cpu.governor; }
    QString cpuEpp() const { return m_snapshot.cpu.epp; }
    bool cpuTurboEnabled() const { return m_snapshot.cpu.turboEnabled; }
    int cpuMinPerfPct() const { return m_snapshot.cpu.minPerfPct; }
    int cpuMaxPerfPct() const { return m_snapshot.cpu.maxPerfPct; }
    QVariantList cpuCoresModel() const;

    int memTotalMiB() const { return static_cast<int>(m_snapshot.memory.totalMiB); }
    int memUsedMiB() const { return static_cast<int>(m_snapshot.memory.usedMiB); }
    int memAvailableMiB() const { return static_cast<int>(m_snapshot.memory.availableMiB); }
    int memCachedMiB() const { return static_cast<int>(m_snapshot.memory.cachedMiB); }
    double memUtilizationPct() const { return m_snapshot.memory.utilizationPct; }
    double psiCpuSome10() const { return m_snapshot.memory.psiCpuSomeAvg10; }
    double psiMemSome10() const { return m_snapshot.memory.psiMemSomeAvg10; }
    double psiIoSome10() const { return m_snapshot.memory.psiIoSomeAvg10; }

    QVariantList historyCpuTemp() const { return m_histCpuTemp; }
    QVariantList historyGpuTemp() const { return m_histGpuTemp; }
    QVariantList historyCpuLoad() const { return m_histCpuLoad; }
    QVariantList historyGpuLoad() const { return m_histGpuLoad; }
    QVariantList historyBatteryWatts() const { return m_histBatteryWatts; }

    // Operating Profiles
    QString activeProfileId() const { return m_settings.activeOperatingProfileId; }
    QString activeTunedProfile() const { return m_snapshot.activeTunedProfile; }
    bool autoSwitchOnPower() const { return m_settings.autoSwitchProfileOnPower; }
    void setAutoSwitchOnPower(bool b);
    QString acProfileId() const { return m_settings.acOperatingProfileId; }
    void setAcProfileId(const QString& id);
    QString batteryProfileId() const { return m_settings.batteryOperatingProfileId; }
    void setBatteryProfileId(const QString& id);
    QVariantList operatingProfilesModel() const;

    // Thermal & Fans
    QString thermalStateLabel() const { return m_snapshot.thermal.thermalStateLabel; }
    QString fanControlModeLabel() const { return m_snapshot.thermal.fanControlMode; }
    bool manualFanSupported() const { return m_snapshot.thermal.manualFanSupported; }
    QString fanStatusExplanation() const { return m_snapshot.thermal.fanStatusExplanation; }
    double maxNvmeTempC() const { return m_snapshot.thermal.maxNvmeTempC; }
    double maxBoardTempC() const { return m_snapshot.thermal.maxBoardTempC; }
    QVariantList thermalSensorsModel() const;
    QVariantList fansModel() const;
    QVariantList fanCurvePoints() const;

    // Keyboard Studio
    QString kbdCapabilityBadge() const;
    QString kbdRoutingBadge() const { return m_snapshot.keyboardHw.hardwareRoutingBadge; }
    QString kbdHardwareExplanation() const { return m_snapshot.keyboardHw.hardwareExplanation; }
    bool kbdHardwareConnected() const { return m_snapshot.keyboardHw.supportsBacklightBrightness; }
    bool kbdRgbSupported() const { return m_snapshot.keyboardHw.supportsRgbZones; }
    bool kbdEnabled() const { return m_currentKbdProfile.enabled; }
    void setKbdEnabled(bool e);
    QString kbdMode() const { return m_currentKbdProfile.mode; }
    void setKbdMode(const QString& m);
    int kbdBrightnessPct() const { return m_currentKbdProfile.brightnessPct; }
    void setKbdBrightnessPct(int b);
    int kbdTimeoutSec() const { return m_currentKbdProfile.timeoutSeconds; }
    void setKbdTimeoutSec(int sec);
    int kbdSelectedZone() const { return m_kbdSelectedZone; }
    void setKbdSelectedZone(int z);
    QVariantList kbdZoneHexList() const;
    QString kbdPickerHex() const { return m_kbdPickerHex; }
    int kbdPickerR() const { return m_kbdRgb.r; }
    int kbdPickerG() const { return m_kbdRgb.g; }
    int kbdPickerB() const { return m_kbdRgb.b; }
    double kbdPickerHue() const { return m_kbdHsv.h; }
    double kbdPickerSat() const { return m_kbdHsv.s; }
    double kbdPickerVal() const { return m_kbdHsv.v; }
    double kbdPickerHslLight() const { return m_kbdHsl.l; }
    QStringList kbdCustomSwatches() const { return m_settings.customKeyboardSwatches; }
    QString activeKbdProfileId() const { return m_settings.activeKeyboardProfileId; }
    QVariantList kbdProfilesModel() const;

    // Battery
    bool batPresent() const { return m_snapshot.battery.present; }
    bool acOnline() const { return m_snapshot.battery.acOnline; }
    QString batStatus() const { return m_snapshot.battery.status; }
    QString batTechnology() const { return m_snapshot.battery.technology; }
    int batCapacityPct() const { return m_snapshot.battery.capacityPct; }
    double batVoltageV() const { return m_snapshot.battery.voltageVolts; }
    double batCurrentA() const { return m_snapshot.battery.currentAmps; }
    double batPowerW() const { return m_snapshot.battery.powerWatts; }
    double batChargeFullAh() const { return m_snapshot.battery.chargeFullAh; }
    double batChargeDesignAh() const { return m_snapshot.battery.chargeFullDesignAh; }
    double batWearHealthPct() const { return m_snapshot.battery.wearHealthPct; }
    int batCycleCount() const { return m_snapshot.battery.cycleCount; }
    bool batChargeLimitSupported() const { return m_snapshot.battery.chargeLimitSupported; }
    int batChargeThresholdPct() const { return m_snapshot.battery.chargeEndThresholdPct; }
    QString batStatusExplanation() const { return m_snapshot.battery.statusExplanation; }
    int displayBrightnessPct() const { return m_snapshot.displayBrightnessPct; }
    bool displayBrightnessSupported() const { return m_snapshot.displayBrightnessSupported; }

    // GPU
    bool gpuAvailable() const { return m_snapshot.gpu.available; }
    QString gpuSourceLabel() const { return m_snapshot.gpu.sourceLabel; }
    QString gpuName() const { return m_snapshot.gpu.name; }
    QString gpuDriverVersion() const { return m_snapshot.gpu.driverVersion; }
    QString gpuPciBusId() const { return m_snapshot.gpu.pciBusId; }
    QString gpuSubsystemId() const { return m_snapshot.gpu.subsystemId; }
    double gpuTempC() const { return m_snapshot.gpu.tempC; }
    int gpuUtilizationPct() const { return m_snapshot.gpu.gpuUtilizationPct; }
    int gpuMemUtilizationPct() const { return m_snapshot.gpu.memUtilizationPct; }
    int gpuVramTotalMiB() const { return static_cast<int>(m_snapshot.gpu.vramTotalMiB); }
    int gpuVramUsedMiB() const { return static_cast<int>(m_snapshot.gpu.vramUsedMiB); }
    int gpuGraphicsClockMhz() const { return m_snapshot.gpu.graphicsClockMhz; }
    int gpuSmClockMhz() const { return m_snapshot.gpu.smClockMhz; }
    int gpuMemClockMhz() const { return m_snapshot.gpu.memClockMhz; }
    int gpuMaxGraphicsClockMhz() const { return m_snapshot.gpu.maxGraphicsClockMhz; }
    int gpuMaxMemClockMhz() const { return m_snapshot.gpu.maxMemClockMhz; }
    double gpuPowerDrawW() const { return m_snapshot.gpu.powerDrawWatts; }
    double gpuRawPowerW() const { return m_snapshot.gpu.rawPowerWatts; }
    bool gpuPowerRailGated() const { return m_snapshot.gpu.powerRailGated; }
    double gpuDefaultLimitW() const { return m_snapshot.gpu.powerDefaultLimitWatts; }
    double gpuMaxLimitW() const { return m_snapshot.gpu.powerMaxLimitWatts; }
    QString gpuPstate() const { return m_snapshot.gpu.pstate; }
    QStringList gpuThrottleReasons() const { return m_snapshot.gpu.throttleReasonsDecoded; }

    // Diagnostics
    QVariantList capabilitiesModel() const;
    QVariantList diagnosticLogsModel() const;

    // Invokable User Commands
    Q_INVOKABLE void refreshNow();
    Q_INVOKABLE void selectOperatingProfile(const QString& profileId);
    Q_INVOKABLE void applyCustomCpuTuning(const QString& governor, const QString& epp, bool turbo, int minPerfPct, int maxPerfPct);

    // Keyboard Studio Unrestricted Color Picker Actions
    Q_INVOKABLE bool setKbdColorFromHex(const QString& hexStr);
    Q_INVOKABLE void setKbdColorFromRgb(int r, int g, int b);
    Q_INVOKABLE void setKbdColorFromHsv(double h, double s, double v);
    Q_INVOKABLE void setKbdColorFromHsl(double h, double s, double l);
    Q_INVOKABLE void saveCurrentKbdColorToSwatches();
    Q_INVOKABLE void selectKbdProfile(const QString& profileId);
    Q_INVOKABLE void saveNewKbdProfile(const QString& profileName);
    Q_INVOKABLE void applyKeyboardLightingToHardware();

    // Thermal & Fan Actions
    Q_INVOKABLE void updateFanCurvePoints(const QVariantList& points);
    Q_INVOKABLE void setFanMode(const QString& mode, int manualDutyPct);

    // Battery & Display Actions
    Q_INVOKABLE void applyBatteryChargeLimit(int thresholdPct);
    Q_INVOKABLE void applyDisplayBrightness(int brightnessPct);

    // Diagnostics & Hardware Mode Actions
    Q_INVOKABLE void switchHardwareMode(int modeIndex);
    Q_INVOKABLE QString exportDiagnosticReport(const QString& targetDir);
    Q_INVOKABLE bool exportProfileBundle(const QString& filePath);
    Q_INVOKABLE bool importProfileBundle(const QString& filePath);

public Q_SLOTS:
    void onPrepareForSleep(bool goingToSleep);

Q_SIGNALS:
    void navigationChanged();
    void statusBannerChanged();
    void telemetryUpdated();
    void settingsUpdated();
    void keyboardStudioChanged();
    void logsUpdated();

private:
    void syncThemeToSettings();
    void applySelectedKbdColorToActiveZones();
    void setStatusBanner(const QString& text, bool isError);
    void persistSettings();
    void pollRaplFromDaemonIfAvailable();
    void setupSystemTray();

    ThemeEngine* m_theme = nullptr;
    core::ConfigStore m_configStore;
    core::AppSettings m_settings;
    hal::TelemetryCollector m_collector;
    core::SystemSnapshot m_snapshot;
    QTimer m_pollTimer;

    int m_currentTabIndex = 0;
    QString m_statusBannerText = QStringLiteral("Ready — Hardware Telemetry Active");
    bool m_statusBannerIsError = false;
    bool m_lastAcOnlineState = true;
    bool m_firstPoll = true;

    // 60-sample rolling history buffers
    QVariantList m_histCpuTemp;
    QVariantList m_histGpuTemp;
    QVariantList m_histCpuLoad;
    QVariantList m_histGpuLoad;
    QVariantList m_histBatteryWatts;

    // Keyboard Studio state
    core::KeyboardLightingProfile m_currentKbdProfile;
    int m_kbdSelectedZone = -1; // -1 = All 4 Zones
    core::RgbColor m_kbdRgb{59, 158, 255};
    core::HsvColor m_kbdHsv{210.0, 0.77, 1.0};
    core::HslColor m_kbdHsl{210.0, 1.0, 0.62};
    QString m_kbdPickerHex = QStringLiteral("#3B9EFF");

    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
};

} // namespace acc::app
