// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/CapabilityState.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <vector>

namespace acc::core {

struct DmiIdentity {
    QString sysVendor;
    QString productName;
    QString productVersion;
    QString boardName;
    QString boardVendor;
    QString biosVendor;
    QString biosVersion;
    QString biosDate;
    QString chassisType;
    QString kernelRelease;
    QString odmSubsystemNote;
    QStringList wmiGuids;
    QStringList acpiDevices;
    QStringList loadedDrivers;
    bool acerWmiLoaded = false;
};

struct CpuCoreReading {
    int coreId = 0;
    QString label;
    int freqMhz = 0;
    double tempC = 0.0;
    double utilizationPct = 0.0;
};

struct CpuTelemetry {
    QString modelName;
    int physicalCores = 10;
    int logicalThreads = 16;
    double totalUtilizationPct = 0.0;
    double packageTempC = 0.0;
    int avgFreqMhz = 0;
    int maxFreqMhz = 4900;
    int minFreqMhz = 400;
    double packagePowerWatts = 0.0;
    double corePowerWatts = 0.0;
    double psysPowerWatts = 0.0;
    bool powerSourceAvailable = false;
    QString powerSourceLabel;
    QString governor;
    QStringList availableGovernors;
    QString epp;
    QStringList availableEpp;
    bool turboEnabled = true;
    int minPerfPct = 10;
    int maxPerfPct = 100;
    std::vector<CpuCoreReading> cores;
};

struct MemoryTelemetry {
    int64_t totalMiB = 0;
    int64_t usedMiB = 0;
    int64_t availableMiB = 0;
    int64_t cachedMiB = 0;
    int64_t buffersMiB = 0;
    double utilizationPct = 0.0;
    double psiCpuSomeAvg10 = 0.0;
    double psiMemSomeAvg10 = 0.0;
    double psiMemFullAvg10 = 0.0;
    double psiIoSomeAvg10 = 0.0;
    bool psiAvailable = false;
};

struct ThermalSensorReading {
    QString id;
    QString label;
    QString sourceDriver;
    QString sysfsPath;
    double tempC = 0.0;
    double highThresholdC = 85.0;
    bool isValid = false;
};

struct FanReading {
    QString id;
    QString label;
    QString sysfsPath;
    int rpm = 0;
    int pwmPct = 0;
    bool isWritable = false;
    bool isDetected = false;
};

struct FanCurvePoint {
    int tempC = 40;
    int dutyPct = 30;
};

struct ThermalTelemetry {
    double cpuPackageTempC = 0.0;
    double gpuTempC = 0.0;
    double maxNvmeTempC = 0.0;
    double maxBoardTempC = 0.0;
    QString thermalStateLabel; // "Nominal", "Elevated", "Critical / Throttling"
    QString fanControlMode;    // "Auto (Firmware EC)", "Manual", "Custom Curve"
    bool manualFanSupported = false;
    QString fanStatusExplanation;
    std::vector<ThermalSensorReading> sensors;
    std::vector<FanReading> fans;
};

struct GpuTelemetry {
    bool available = false;
    bool telemetryMeasuredByNvml = false;
    bool powerLimitsMeasured = false;
    QString sourceLabel; // "NVML (libnvidia-ml.so.1)", "/proc/driver/nvidia Fallback", "Mock NVML"
    QString name;
    QString driverVersion;
    QString pciBusId;
    QString subsystemId;
    double tempC = 0.0;
    int gpuUtilizationPct = 0;
    int memUtilizationPct = 0;
    int64_t vramTotalMiB = 0;
    int64_t vramUsedMiB = 0;
    int64_t vramFreeMiB = 0;
    int graphicsClockMhz = 0;
    int smClockMhz = 0;
    int memClockMhz = 0;
    int maxGraphicsClockMhz = 0;
    int maxMemClockMhz = 0;
    double powerDrawWatts = 0.0;
    double rawPowerWatts = 0.0;
    bool powerRailGated = false;
    double powerDefaultLimitWatts = 0.0;
    double powerMinLimitWatts = 0.0;
    double powerMaxLimitWatts = 0.0;
    QString pstate;
    uint64_t throttleReasonsBitmask = 0;
    QStringList throttleReasonsDecoded;
    int fanSpeedPct = 0;
    bool fanSpeedReported = false;
};

struct BatteryTelemetry {
    bool present = false;
    bool acOnline = false;
    QString status;     // "Charging", "Discharging", "Not charging", "Full"
    QString technology; // "Li-ion"
    int capacityPct = 0;
    double voltageVolts = 0.0;
    double currentAmps = 0.0;
    double powerWatts = 0.0;
    double chargeNowAh = 0.0;
    double chargeFullAh = 0.0;
    double chargeFullDesignAh = 0.0;
    double wearHealthPct = 0.0;
    int cycleCount = 0;
    bool cycleCountReported = false;
    QString cycleCountLabel = QStringLiteral("NOT REPORTED");
    bool chargeLimitSupported = false;
    int chargeEndThresholdPct = 100;
    int previousChargeEndThresholdPct = 100;
    QString chargeLimitSysfsPath;
    QString statusExplanation;
};

struct KeyboardZoneColor {
    int zoneIndex = 0;
    QString name;
    int r = 59;
    int g = 158;
    int b = 255;
    QString hex = QStringLiteral("#3B9EFF");
};

struct KeyboardLightingProfile {
    QString id;
    QString name;
    bool enabled = true;
    QString mode = QStringLiteral("static"); // "static", "breathing", "wave"
    int brightnessPct = 80;
    int timeoutSeconds = 30;
    QString clv0001PresetName = QStringLiteral("cyan");
    int clv0001BrightnessLevel = 4;
    std::vector<KeyboardZoneColor> zones;
};

enum class KeyboardBackendKind {
    None,
    LinuxLedsMulticolorRgb,
    LinuxLedsMonochrome,
    Clv0001AcpiDsmPreset,
    Clv0001AcpiUnbound
};

enum class KeyboardExecutionStatus {
    NotExecuted,
    RejectedBeforeExecution,
    AcpiEvalFailed,
    UnexpectedAcpiReturnType,
    AcceptedByAcpiNoReadback,
    VerifiedPhysicalReadback
};

struct Clv0001PresetInfo {
    QString name;
    uint8_t g = 0;
    uint8_t r = 0;
    uint8_t b = 0;
    QString previewHex;
};

struct KeyboardHardwareState {
    CapabilityState capabilityState = CapabilityState::NotDetected;
    KeyboardBackendKind backendKind = KeyboardBackendKind::None;
    QString backendKindId = QStringLiteral("none");
    QString controlModeBadge = QStringLiteral("UNSUPPORTED LIGHTING");
    QString ledSysfsPath;
    bool clv0001DevicePresent = false;
    QString clv0001AcpiPath;
    QString clv0001Status;
    bool clv0001DriverBound = false;
    bool supportsBacklightBrightness = false;
    bool supportsRgbZones = false;
    bool supportsPresetColors = false;
    bool supportsArbitraryRgb = false;
    bool supportsTimeout = false;
    bool independentHwReadback = false;
    int zoneCount = 4;
    int maxBrightness = 100;
    int currentBrightnessPct = 0;
    int discreteBrightnessLevels = 5;
    QString currentPresetName = QStringLiteral("unknown");
    int currentBrightnessLevel = -1;
    QString lastPayloadHex = QStringLiteral("UNAVAILABLE");
    QString driverLastStatus = QStringLiteral("unavailable");
    QString driverLastPreset = QStringLiteral("unknown");
    int driverLastBrightnessLevel = -1;
    QString driverLastPayloadHex = QStringLiteral("UNAVAILABLE");
    QString driverLastAcpiReturnType = QStringLiteral("none");
    QString hardwareRoutingBadge;
    QString hardwareExplanation;
};

struct OperatingProfile {
    QString id;          // "quiet", "balanced", "performance", "custom"
    QString name;        // "Quiet", "Balanced", "Performance", "Custom"
    QString subtitle;
    QString tunedProfile; // "powersave", "balanced", "throughput-performance"
    QString ppdProfile;   // "power-saver", "balanced", "performance"
    QString governor;     // "powersave", "performance"
    QString epp;          // "power", "balance_performance", "performance"
    bool turboEnabled = true;
    int minPerfPct = 10;
    int maxPerfPct = 100;
    bool isCustom = false;
};

struct ControlResult {
    bool applied = false;
    QString errorCode;
    QString message;
    QString readbackSummary;
    QString verificationStatusId;
    bool physicalStateVerified = false;
};

enum class TelemetryFreshness {
    Unavailable = 0,
    Fresh = 1,
    Stale = 2
};

enum class ControlAuthState {
    Idle = 0,
    Pending = 1,
    Authorized = 2,
    Denied = 3,
    DaemonUnavailable = 4,
    Failed = 5
};

inline QString telemetryFreshnessToId(TelemetryFreshness freshness) {
    switch (freshness) {
        case TelemetryFreshness::Fresh:
            return QStringLiteral("fresh");
        case TelemetryFreshness::Stale:
            return QStringLiteral("stale");
        case TelemetryFreshness::Unavailable:
            return QStringLiteral("unavailable");
    }
    return QStringLiteral("unavailable");
}

inline QString controlAuthStateToId(ControlAuthState state) {
    switch (state) {
        case ControlAuthState::Idle:
            return QStringLiteral("idle");
        case ControlAuthState::Pending:
            return QStringLiteral("pending");
        case ControlAuthState::Authorized:
            return QStringLiteral("authorized");
        case ControlAuthState::Denied:
            return QStringLiteral("denied");
        case ControlAuthState::DaemonUnavailable:
            return QStringLiteral("daemon_unavailable");
        case ControlAuthState::Failed:
            return QStringLiteral("failed");
    }
    return QStringLiteral("idle");
}

struct SystemSnapshot {
    bool valid = false;
    bool collectionFailed = false;
    QString collectionError;
    int64_t timestampMs = 0;
    bool mockModeActive = false;
    QString mockScenarioName;
    bool daemonReachable = false;
    DmiIdentity dmi;
    CpuTelemetry cpu;
    MemoryTelemetry memory;
    ThermalTelemetry thermal;
    GpuTelemetry gpu;
    BatteryTelemetry battery;
    KeyboardHardwareState keyboardHw;
    QString activeOperatingProfileId = QStringLiteral("balanced");
    QString activeTunedProfile;
    int displayBrightnessPct = 50;
    bool displayBrightnessSupported = false;
    std::vector<CapabilityRecord> capabilities;
};

} // namespace acc::core
