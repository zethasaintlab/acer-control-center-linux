// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "app/AsyncControlClient.h"
#include "app/TelemetryCoordinator.h"
#include "app/ThemeEngine.h"
#include "core/ConfigStore.h"
#include "core/DomainModels.h"

#include <QDBusConnection>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <cstdint>
#include <memory>

namespace acc::app {

/**
 * Unprivileged GUI ViewModel/Controller for Acer Control Center.
 *
 * Stage 24 async & lifetime invariants (Stage 25 UI contract):
 * - All telemetry collection runs asynchronously on TelemetryCoordinator's
 *   dedicated worker QThread; constructor and QTimer polling never block the GUI.
 * - All D-Bus telemetry hints and privileged control operations use non-blocking
 *   QDBusPendingCallWatcher callbacks guarded by QPointer<ControlCenterController>
 *   and generation tokens (no QDBus::Block on the GUI thread).
 * - Telemetry freshness is explicitly exposed via `telemetryFreshness`
 *   ("unavailable" | "fresh" | "stale"), `telemetryFreshnessState` (FreshnessState),
 *   `telemetryRefreshing`, `telemetrySuspended`, `telemetryAgeMs`, and
 *   `telemetryErrorMessage`.
 * - Privileged authorization status is explicitly exposed via `controlAuthState`
 *   ("idle" | "pending" | "authorized" | "denied" | "daemon_unavailable" | "failed"),
 *   `controlAuthStateEnum` (AuthState), `controlOperationPending`,
 *   `controlOperationName`, `controlLastErrorCode`, `controlLastErrorMessage`,
 *   and the `controlOperationFinished` signal.
 * - Single-zone CLV0001 static RGB (`kbdSupportsArbitraryRgb`) remains strictly
 *   separate from spatial multi-zone hardware (`kbdRgbSupported`).
 */
class ControlCenterController : public QObject {
    Q_OBJECT

public:
    enum class FreshnessState {
        Unavailable = 0,
        Fresh = 1,
        Stale = 2
    };
    Q_ENUM(FreshnessState)

    enum class AuthState {
        Idle = 0,
        Pending = 1,
        Authorized = 2,
        Denied = 3,
        DaemonUnavailable = 4,
        Failed = 5
    };
    Q_ENUM(AuthState)

    // Navigation & Global Banner
    Q_PROPERTY(int currentTabIndex READ currentTabIndex WRITE setCurrentTabIndex NOTIFY navigationChanged)
    Q_PROPERTY(QString statusBannerText READ statusBannerText NOTIFY statusBannerChanged)
    Q_PROPERTY(bool statusBannerIsError READ statusBannerIsError NOTIFY statusBannerChanged)
    Q_PROPERTY(bool mockModeActive READ mockModeActive NOTIFY telemetryUpdated)
    Q_PROPERTY(QString hardwareModeLabel READ hardwareModeLabel NOTIFY telemetryUpdated)
    Q_PROPERTY(int hardwareModeIndex READ hardwareModeIndex NOTIFY telemetryUpdated)
    Q_PROPERTY(int pollingIntervalMs READ pollingIntervalMs WRITE setPollingIntervalMs NOTIFY settingsUpdated)

    // Explicit Telemetry Freshness & Availability (Stage 24/25 Contract)
    Q_PROPERTY(QString telemetryFreshness READ telemetryFreshness NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(FreshnessState telemetryFreshnessState READ telemetryFreshnessState NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(bool telemetryAvailable READ telemetryAvailable NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(bool telemetryStale READ telemetryStale NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(bool telemetryRefreshing READ telemetryRefreshing NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(bool telemetrySuspended READ telemetrySuspended NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(qint64 telemetryAgeMs READ telemetryAgeMs NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(qint64 lastTelemetryTimestampMs READ lastTelemetryTimestampMs NOTIFY telemetryFreshnessChanged)
    Q_PROPERTY(QString telemetryErrorMessage READ telemetryErrorMessage NOTIFY telemetryFreshnessChanged)

    // Explicit Async Control & Authorization State (Stage 24/25 Contract)
    Q_PROPERTY(bool controlOperationPending READ controlOperationPending NOTIFY controlOperationChanged)
    Q_PROPERTY(QString controlOperationName READ controlOperationName NOTIFY controlOperationChanged)
    Q_PROPERTY(QString controlAuthState READ controlAuthState NOTIFY controlOperationChanged)
    Q_PROPERTY(AuthState controlAuthStateEnum READ controlAuthStateEnum NOTIFY controlOperationChanged)
    Q_PROPERTY(QString controlLastErrorCode READ controlLastErrorCode NOTIFY controlOperationChanged)
    Q_PROPERTY(QString controlLastErrorMessage READ controlLastErrorMessage NOTIFY controlOperationChanged)

    // DMI & Platform Identity
    Q_PROPERTY(QString productTitle READ productTitle NOTIFY telemetryUpdated)
    Q_PROPERTY(QString boardTitle READ boardTitle NOTIFY telemetryUpdated)
    Q_PROPERTY(QString biosTitle READ biosTitle NOTIFY telemetryUpdated)
    Q_PROPERTY(QString odmPlatformNote READ odmPlatformNote NOTIFY telemetryUpdated)
    Q_PROPERTY(QString wmiGuidsJoined READ wmiGuidsJoined NOTIFY telemetryUpdated)
    Q_PROPERTY(QString kernelRelease READ kernelRelease NOTIFY telemetryUpdated)
    Q_PROPERTY(QString acpiDevicesJoined READ acpiDevicesJoined NOTIFY telemetryUpdated)
    Q_PROPERTY(QString loadedDriversJoined READ loadedDriversJoined NOTIFY telemetryUpdated)
    Q_PROPERTY(bool daemonReachable READ daemonReachable NOTIFY telemetryUpdated)

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
    Q_PROPERTY(int memBuffersMiB READ memBuffersMiB NOTIFY telemetryUpdated)
    Q_PROPERTY(double memUtilizationPct READ memUtilizationPct NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiCpuSome10 READ psiCpuSome10 NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiMemSome10 READ psiMemSome10 NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiMemFull10 READ psiMemFull10 NOTIFY telemetryUpdated)
    Q_PROPERTY(double psiIoSome10 READ psiIoSome10 NOTIFY telemetryUpdated)
    Q_PROPERTY(bool psiAvailable READ psiAvailable NOTIFY telemetryUpdated)

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
    Q_PROPERTY(QString fanCapabilityBadge READ fanCapabilityBadge NOTIFY telemetryUpdated)
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
    Q_PROPERTY(QString kbdBackendKindId READ kbdBackendKindId NOTIFY telemetryUpdated)
    Q_PROPERTY(QString kbdControlModeBadge READ kbdControlModeBadge NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdHardwareConnected READ kbdHardwareConnected NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdRgbSupported READ kbdRgbSupported NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdSupportsPresetColors READ kbdSupportsPresetColors NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdSupportsArbitraryRgb READ kbdSupportsArbitraryRgb NOTIFY telemetryUpdated)
    Q_PROPERTY(bool kbdPhysicalReadbackSupported READ kbdPhysicalReadbackSupported NOTIFY telemetryUpdated)
    Q_PROPERTY(QString kbdPresetName READ kbdPresetName NOTIFY keyboardStudioChanged)
    Q_PROPERTY(int kbdBrightnessLevel READ kbdBrightnessLevel NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QString kbdFirmwarePacketHex READ kbdFirmwarePacketHex NOTIFY keyboardStudioChanged)
    Q_PROPERTY(bool kbdCustomPreviewDirty READ kbdCustomPreviewDirty NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QString kbdSelectionSource READ kbdSelectionSource NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QString kbdNearestPresetName READ kbdNearestPresetName NOTIFY keyboardStudioChanged)
    Q_PROPERTY(QVariantList kbdPresetsModel READ kbdPresetsModel NOTIFY keyboardStudioChanged)
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
    Q_PROPERTY(double batChargeNowAh READ batChargeNowAh NOTIFY telemetryUpdated)
    Q_PROPERTY(double batWearHealthPct READ batWearHealthPct NOTIFY telemetryUpdated)
    Q_PROPERTY(int batCycleCount READ batCycleCount NOTIFY telemetryUpdated)
    Q_PROPERTY(bool batCycleCountReported READ batCycleCountReported NOTIFY telemetryUpdated)
    Q_PROPERTY(QString batCycleCountText READ batCycleCountText NOTIFY telemetryUpdated)
    Q_PROPERTY(bool batChargeLimitSupported READ batChargeLimitSupported NOTIFY telemetryUpdated)
    Q_PROPERTY(QString batCapabilityBadge READ batCapabilityBadge NOTIFY telemetryUpdated)
    Q_PROPERTY(int batChargeThresholdPct READ batChargeThresholdPct NOTIFY telemetryUpdated)
    Q_PROPERTY(int batPreviousThresholdPct READ batPreviousThresholdPct NOTIFY telemetryUpdated)
    Q_PROPERTY(QString batStatusExplanation READ batStatusExplanation NOTIFY telemetryUpdated)
    Q_PROPERTY(int displayBrightnessPct READ displayBrightnessPct NOTIFY telemetryUpdated)
    Q_PROPERTY(bool displayBrightnessSupported READ displayBrightnessSupported NOTIFY telemetryUpdated)

    // 06 // NVIDIA GPU CENTER
    Q_PROPERTY(bool gpuAvailable READ gpuAvailable NOTIFY telemetryUpdated)
    Q_PROPERTY(bool gpuTelemetryMeasuredByNvml READ gpuTelemetryMeasuredByNvml NOTIFY telemetryUpdated)
    Q_PROPERTY(bool gpuPowerLimitsMeasured READ gpuPowerLimitsMeasured NOTIFY telemetryUpdated)
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
    explicit ControlCenterController(
        ThemeEngine* themeEngine,
        QObject* parent = nullptr
    );
    ControlCenterController(
        ThemeEngine* themeEngine,
        IAsyncControlTransport* customTransport,
        QDBusConnection bus = QDBusConnection::systemBus(),
        QObject* parent = nullptr
    );
    ~ControlCenterController() override;

    int currentTabIndex() const { return m_currentTabIndex; }
    void setCurrentTabIndex(int idx);

    QString statusBannerText() const { return m_statusBannerText; }
    bool statusBannerIsError() const { return m_statusBannerIsError; }

    bool mockModeActive() const;
    QString hardwareModeLabel() const;
    int hardwareModeIndex() const;

    int pollingIntervalMs() const { return m_settings.pollingIntervalMs; }
    void setPollingIntervalMs(int ms);

    // Telemetry Freshness Accessors
    QString telemetryFreshness() const { return core::telemetryFreshnessToId(m_freshness); }
    FreshnessState telemetryFreshnessState() const { return static_cast<FreshnessState>(m_freshness); }
    bool telemetryAvailable() const { return m_hasValidSnapshot && m_freshness != core::TelemetryFreshness::Unavailable; }
    bool telemetryStale() const { return m_freshness == core::TelemetryFreshness::Stale; }
    bool telemetryRefreshing() const { return m_telemetryRefreshing; }
    bool telemetrySuspended() const { return m_telemetrySuspended; }
    qint64 telemetryAgeMs() const { return m_telemetryAgeMs; }
    qint64 lastTelemetryTimestampMs() const { return m_hasValidSnapshot ? m_snapshot.timestampMs : 0; }
    QString telemetryErrorMessage() const { return m_telemetryErrorMessage; }

    // Async Control & Auth Accessors
    bool controlOperationPending() const { return m_controlOperationPending; }
    QString controlOperationName() const { return m_controlOperationName; }
    QString controlAuthState() const { return core::controlAuthStateToId(m_controlAuthState); }
    AuthState controlAuthStateEnum() const { return static_cast<AuthState>(m_controlAuthState); }
    QString controlLastErrorCode() const { return m_controlLastErrorCode; }
    QString controlLastErrorMessage() const { return m_controlLastErrorMessage; }

    // Test & CLI hooks
    TelemetryCoordinator* telemetryCoordinator() const { return m_coordinator.get(); }
    bool waitForNextSnapshot(int timeoutMs = 3000);
    void setFreshnessTimeoutOverrideMsForTest(int timeoutMs);

    // DMI
    QString productTitle() const;
    QString boardTitle() const;
    QString biosTitle() const;
    QString odmPlatformNote() const { return m_snapshot.dmi.odmSubsystemNote; }
    QString wmiGuidsJoined() const { return m_snapshot.dmi.wmiGuids.join(QStringLiteral("  |  ")); }
    QString kernelRelease() const { return m_snapshot.dmi.kernelRelease; }
    QString acpiDevicesJoined() const { return m_snapshot.dmi.acpiDevices.join(QStringLiteral("  |  ")); }
    QString loadedDriversJoined() const { return m_snapshot.dmi.loadedDrivers.join(QStringLiteral(", ")); }
    bool daemonReachable() const { return m_snapshot.daemonReachable; }

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
    int memBuffersMiB() const { return static_cast<int>(m_snapshot.memory.buffersMiB); }
    double memUtilizationPct() const { return m_snapshot.memory.utilizationPct; }
    double psiCpuSome10() const { return m_snapshot.memory.psiCpuSomeAvg10; }
    double psiMemSome10() const { return m_snapshot.memory.psiMemSomeAvg10; }
    double psiMemFull10() const { return m_snapshot.memory.psiMemFullAvg10; }
    double psiIoSome10() const { return m_snapshot.memory.psiIoSomeAvg10; }
    bool psiAvailable() const { return m_snapshot.memory.psiAvailable; }

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
    QString fanCapabilityBadge() const;
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
    QString kbdBackendKindId() const { return m_snapshot.keyboardHw.backendKindId; }
    QString kbdControlModeBadge() const { return m_snapshot.keyboardHw.controlModeBadge; }
    bool kbdHardwareConnected() const { return m_snapshot.keyboardHw.supportsBacklightBrightness; }
    bool kbdRgbSupported() const { return m_snapshot.keyboardHw.supportsRgbZones; }
    bool kbdSupportsPresetColors() const { return m_snapshot.keyboardHw.supportsPresetColors; }
    bool kbdSupportsArbitraryRgb() const { return m_snapshot.keyboardHw.supportsArbitraryRgb; }
    bool kbdPhysicalReadbackSupported() const { return m_snapshot.keyboardHw.independentHwReadback; }
    QString kbdPresetName() const { return m_currentKbdProfile.clv0001PresetName; }
    int kbdBrightnessLevel() const { return m_currentKbdProfile.clv0001BrightnessLevel; }
    QString kbdFirmwarePacketHex() const;
    bool kbdCustomPreviewDirty() const { return m_kbdCustomPreviewDirty; }
    QString kbdSelectionSource() const { return m_kbdSelectionSource; }
    QString kbdNearestPresetName() const;
    QVariantList kbdPresetsModel() const;
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
    double batChargeNowAh() const { return m_snapshot.battery.chargeNowAh; }
    double batWearHealthPct() const { return m_snapshot.battery.wearHealthPct; }
    int batCycleCount() const { return m_snapshot.battery.cycleCount; }
    bool batCycleCountReported() const { return m_snapshot.battery.cycleCountReported; }
    QString batCycleCountText() const { return m_snapshot.battery.cycleCountLabel; }
    bool batChargeLimitSupported() const { return m_snapshot.battery.chargeLimitSupported; }
    QString batCapabilityBadge() const;
    int batChargeThresholdPct() const { return m_snapshot.battery.chargeEndThresholdPct; }
    int batPreviousThresholdPct() const { return m_batPreviousThresholdPct; }
    QString batStatusExplanation() const { return m_snapshot.battery.statusExplanation; }
    int displayBrightnessPct() const { return m_snapshot.displayBrightnessPct; }
    bool displayBrightnessSupported() const { return m_snapshot.displayBrightnessSupported; }

    // GPU
    bool gpuAvailable() const { return m_snapshot.gpu.available; }
    bool gpuTelemetryMeasuredByNvml() const { return m_snapshot.gpu.telemetryMeasuredByNvml; }
    bool gpuPowerLimitsMeasured() const { return m_snapshot.gpu.powerLimitsMeasured; }
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
    Q_INVOKABLE void selectKbdPreset(const QString& presetName);
    Q_INVOKABLE void snapToNearestKbdPreset();
    Q_INVOKABLE void setKbdBrightnessLevel(int level);
    Q_INVOKABLE void selectRandomKbdColor();
    Q_INVOKABLE bool applyCustomKbdRgb(int r, int g, int b, int brightnessLevel);
    Q_INVOKABLE void applyKeyboardLightingToHardware();

    // Thermal & Fan Actions
    Q_INVOKABLE void updateFanCurvePoints(const QVariantList& points);
    Q_INVOKABLE void setFanMode(const QString& mode, int manualDutyPct);

    // Battery & Display Actions
    Q_INVOKABLE void applyBatteryChargeLimit(int thresholdPct);
    Q_INVOKABLE void restorePreviousBatteryChargeLimit();
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
    void telemetryFreshnessChanged();
    void controlOperationChanged();
    void controlOperationFinished(
        const QString& operation,
        bool applied,
        const QString& authState,
        const QString& errorCode,
        const QString& message
    );
    void settingsUpdated();
    void keyboardStudioChanged();
    void logsUpdated();

private Q_SLOTS:
    void onSnapshotReady(const acc::core::SystemSnapshot& snapshot, uint64_t seq);
    void onCollectionFailed(const QString& errorMessage, uint64_t seq);
    void onCoordinatorInFlightChanged(bool inFlight);
    void onFreshnessTick();

private:
    void syncThemeToSettings();
    void syncCurrentKbdProfileToSettings();
    void loadActiveKeyboardProfileFromSettings();
    void applySelectedKbdColorToActiveZones();
    void setStatusBanner(const QString& text, bool isError);
    void persistSettings();
    void setupSystemTray();
    int64_t maxFreshAgeMs() const;
    bool beginControlOperation(const QString& operationName, uint64_t& outToken);
    void completeControlOperation(
        uint64_t token,
        const QString& operationName,
        const AsyncControlOutcome& outcome,
        bool triggerRefresh = true
    );

    ThemeEngine* m_theme = nullptr;
    core::ConfigStore m_configStore;
    core::AppSettings m_settings;
    QDBusConnection m_dbusConnection;
    std::unique_ptr<TelemetryCoordinator> m_coordinator;
    std::unique_ptr<DbusControlTransport> m_ownedTransport;
    IAsyncControlTransport* m_transport = nullptr;

    core::SystemSnapshot m_snapshot;
    bool m_hasValidSnapshot = false;
    core::TelemetryFreshness m_freshness = core::TelemetryFreshness::Unavailable;
    bool m_telemetryRefreshing = false;
    bool m_telemetrySuspended = false;
    qint64 m_telemetryAgeMs = -1;
    QString m_telemetryErrorMessage;
    int m_freshnessTimeoutOverrideMs = 0;

    bool m_controlOperationPending = false;
    QString m_controlOperationName;
    core::ControlAuthState m_controlAuthState = core::ControlAuthState::Idle;
    QString m_controlLastErrorCode;
    QString m_controlLastErrorMessage;
    uint64_t m_controlToken = 0;

    QTimer m_pollTimer;
    QTimer m_freshnessTimer;

    int m_currentTabIndex = 0;
    QString m_statusBannerText = QStringLiteral("Initializing Asynchronous Telemetry...");
    bool m_statusBannerIsError = false;
    bool m_lastAcOnlineState = true;
    bool m_firstPoll = true;
    bool m_initialLiveProfileSynced = false;
    int m_batPreviousThresholdPct = 100;

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
    bool m_kbdCustomPreviewDirty = false;
    QString m_kbdSelectionSource = QStringLiteral("preset");

    std::unique_ptr<QSystemTrayIcon> m_trayIcon;
};

} // namespace acc::app
