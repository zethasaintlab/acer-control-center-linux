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
    QString odmSubsystemNote;
    QStringList wmiGuids;
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
    QString sourceLabel; // "NVML (libnvidia-ml.so.1)", "PCI/Sysfs Fallback", "Mock NVML"
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
    int maxGraphicsClockMhz = 2100;
    int maxMemClockMhz = 7001;
    double powerDrawWatts = 0.0;
    double rawPowerWatts = 0.0;
    bool powerRailGated = false;
    double powerDefaultLimitWatts = 60.0;
    double powerMinLimitWatts = 1.0;
    double powerMaxLimitWatts = 75.0;
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
    bool chargeLimitSupported = false;
    int chargeEndThresholdPct = 100;
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
    std::vector<KeyboardZoneColor> zones;
};

struct KeyboardHardwareState {
    CapabilityState capabilityState = CapabilityState::NotDetected;
    QString ledSysfsPath;
    bool supportsBacklightBrightness = false;
    bool supportsRgbZones = false;
    bool supportsTimeout = false;
    int zoneCount = 4;
    int maxBrightness = 100;
    int currentBrightnessPct = 0;
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
};

struct SystemSnapshot {
    int64_t timestampMs = 0;
    bool mockModeActive = false;
    QString mockScenarioName;
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
