// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/NvmlDynamicLoader.h"
#include "core/DiagnosticRedactor.h"
#include "hal/SysfsRoot.h"

#include <dlfcn.h>
#include <algorithm>
#include <cmath>

namespace acc::hal {

namespace {

struct CleanRoomNvmlUtilization {
    unsigned int gpu;
    unsigned int memory;
};

struct CleanRoomNvmlMemory {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
};

template <typename FnPtr>
FnPtr resolveSym(void* handle, const char* name) {
    return reinterpret_cast<FnPtr>(dlsym(handle, name));
}

} // namespace

NvmlDynamicLoader::NvmlDynamicLoader() {
    tryInitialize();
}

NvmlDynamicLoader::~NvmlDynamicLoader() {
    shutdown();
}

void NvmlDynamicLoader::tryInitialize() {
    if (SysfsRoot::instance().isMockActive()) {
        return;
    }

    m_libHandle = dlopen("libnvidia-ml.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!m_libHandle) {
        core::DiagnosticRedactor::instance().appendLog(
            QStringLiteral("INFO"),
            QStringLiteral("NVML"),
            QStringLiteral("libnvidia-ml.so.1 not present; falling back to PCI/mock GPU telemetry.")
        );
        return;
    }

    m_fnInit = resolveSym<FnInit>(m_libHandle, "nvmlInit_v2");
    if (!m_fnInit) {
        m_fnInit = resolveSym<FnInit>(m_libHandle, "nvmlInit");
    }
    m_fnShutdown = resolveSym<FnShutdown>(m_libHandle, "nvmlShutdown");
    m_fnGetDriverVersion = resolveSym<FnGetDriverVersion>(m_libHandle, "nvmlSystemGetDriverVersion");
    m_fnGetCount = resolveSym<FnGetCount>(m_libHandle, "nvmlDeviceGetCount_v2");
    m_fnGetHandleByIndex = resolveSym<FnGetHandleByIndex>(m_libHandle, "nvmlDeviceGetHandleByIndex_v2");
    m_fnGetName = resolveSym<FnGetName>(m_libHandle, "nvmlDeviceGetName");
    m_fnGetTemperature = resolveSym<FnGetTemperature>(m_libHandle, "nvmlDeviceGetTemperature");
    m_fnGetUtilization = resolveSym<FnGetUtilization>(m_libHandle, "nvmlDeviceGetUtilizationRates");
    m_fnGetMemoryInfo = resolveSym<FnGetMemoryInfo>(m_libHandle, "nvmlDeviceGetMemoryInfo");
    m_fnGetClockInfo = resolveSym<FnGetClockInfo>(m_libHandle, "nvmlDeviceGetClockInfo");
    m_fnGetMaxClockInfo = resolveSym<FnGetMaxClockInfo>(m_libHandle, "nvmlDeviceGetMaxClockInfo");
    m_fnGetPowerUsage = resolveSym<FnGetPowerUsage>(m_libHandle, "nvmlDeviceGetPowerUsage");
    m_fnGetPowerLimits = resolveSym<FnGetPowerLimits>(m_libHandle, "nvmlDeviceGetPowerManagementLimitConstraints");
    m_fnGetDefaultPowerLimit = resolveSym<FnGetDefaultPowerLimit>(m_libHandle, "nvmlDeviceGetPowerManagementDefaultLimit");
    m_fnGetPState = resolveSym<FnGetPState>(m_libHandle, "nvmlDeviceGetPerformanceState");
    m_fnGetClocksEventReasons = resolveSym<FnGetClocksEventReasons>(m_libHandle, "nvmlDeviceGetCurrentClocksEventReasons");
    m_fnGetFanSpeed = resolveSym<FnGetFanSpeed>(m_libHandle, "nvmlDeviceGetFanSpeed");

    if (!m_fnInit || !m_fnGetCount || !m_fnGetHandleByIndex || m_fnInit() != 0) {
        dlclose(m_libHandle);
        m_libHandle = nullptr;
        return;
    }

    m_initialized = true;
    unsigned int count = 0;
    if (m_fnGetCount(&count) != 0 || count == 0) {
        return;
    }

    if (m_fnGetHandleByIndex(0, &m_deviceHandle) != 0) {
        m_deviceHandle = nullptr;
        return;
    }

    char buf[128] = {0};
    if (m_fnGetDriverVersion && m_fnGetDriverVersion(buf, sizeof(buf)) == 0) {
        m_driverVersion = QString::fromUtf8(buf);
    }
    char nameBuf[128] = {0};
    if (m_fnGetName && m_fnGetName(m_deviceHandle, nameBuf, sizeof(nameBuf)) == 0) {
        m_deviceName = QString::fromUtf8(nameBuf);
    }
    m_pciBusId = QStringLiteral("0000:01:00.0");
    m_subsystemId = QStringLiteral("1025:1900");
}

void NvmlDynamicLoader::shutdown() {
    if (m_initialized && m_fnShutdown) {
        m_fnShutdown();
    }
    m_initialized = false;
    m_deviceHandle = nullptr;
    if (m_libHandle) {
        dlclose(m_libHandle);
        m_libHandle = nullptr;
    }
}

bool NvmlDynamicLoader::isLoaded() const {
    return m_initialized && (m_deviceHandle != nullptr);
}

double NvmlDynamicLoader::filterPowerReading(double rawWatts, double maxLimitWatts, int pstateNumber, bool& outRailGated) {
    outRailGated = false;
    if (rawWatts < 0.0 || std::isnan(rawWatts)) {
        return 0.0;
    }
    const double ceiling = std::max(100.0, maxLimitWatts * 1.5);
    // On GA107BM (RTX 3050 Laptop GPU) in P8 low-power state, NVML can return ~312.23 W
    // when the sensor ADC rail is gated. Detect and flag this accurately.
    if (rawWatts > ceiling || (pstateNumber >= 8 && rawWatts > maxLimitWatts * 0.8)) {
        outRailGated = true;
        return 0.0;
    }
    return rawWatts;
}

QStringList NvmlDynamicLoader::decodeThrottleReasons(uint64_t bitmask) {
    QStringList reasons;
    if (bitmask == 0) {
        reasons.append(QStringLiteral("None (Unthrottled)"));
        return reasons;
    }
    if (bitmask & 0x0000000000000001ULL) {
        reasons.append(QStringLiteral("GpuIdle (P-State Idle Clock Downshift)"));
    }
    if (bitmask & 0x0000000000000002ULL) {
        reasons.append(QStringLiteral("ApplicationsClocksSetting"));
    }
    if (bitmask & 0x0000000000000004ULL) {
        reasons.append(QStringLiteral("SwPowerCap (TGP Power Limit Reached)"));
    }
    if (bitmask & 0x0000000000000008ULL) {
        reasons.append(QStringLiteral("HwSlowdown (Hardware Slowdown Active)"));
    }
    if (bitmask & 0x0000000000000010ULL) {
        reasons.append(QStringLiteral("SyncBoost"));
    }
    if (bitmask & 0x0000000000000020ULL) {
        reasons.append(QStringLiteral("SwThermalSlowdown (Driver Thermal Cap)"));
    }
    if (bitmask & 0x0000000000000040ULL) {
        reasons.append(QStringLiteral("HwThermalSlowdown (Hardware Thermal Threshold)"));
    }
    if (bitmask & 0x0000000000000080ULL) {
        reasons.append(QStringLiteral("HwPowerBrakeSlowdown (External Power Brake)"));
    }
    if (reasons.isEmpty()) {
        reasons.append(QStringLiteral("VendorMask (0x%1)").arg(bitmask, 0, 16));
    }
    return reasons;
}

core::GpuTelemetry NvmlDynamicLoader::readTelemetry() const {
    core::GpuTelemetry g;

    if (SysfsRoot::instance().isMockActive()) {
        g.available = true;
        g.sourceLabel = QStringLiteral("Mock NVML (RTX 3050 6GB Fixture)");
        g.name = QStringLiteral("NVIDIA GeForce RTX 3050 6GB Laptop GPU");
        g.driverVersion = QStringLiteral("615.78.08");
        g.pciBusId = QStringLiteral("0000:01:00.0");
        g.subsystemId = QStringLiteral("1025:1900 (Acer Kelilu_RTH)");
        g.tempC = 43.0;
        g.gpuUtilizationPct = 14;
        g.memUtilizationPct = 8;
        g.vramTotalMiB = 6144;
        g.vramUsedMiB = 612;
        g.vramFreeMiB = 5532;
        g.graphicsClockMhz = 1140;
        g.smClockMhz = 1140;
        g.memClockMhz = 6000;
        g.maxGraphicsClockMhz = 2100;
        g.maxMemClockMhz = 7001;
        g.powerDrawWatts = 18.4;
        g.rawPowerWatts = 18.4;
        g.powerRailGated = false;
        g.powerDefaultLimitWatts = 60.0;
        g.powerMinLimitWatts = 1.0;
        g.powerMaxLimitWatts = 75.0;
        g.pstate = QStringLiteral("P3");
        g.throttleReasonsBitmask = 0x0ULL;
        g.throttleReasonsDecoded = decodeThrottleReasons(0x0ULL);
        g.fanSpeedPct = 0;
        g.fanSpeedReported = false;
        return g;
    }

    if (!isLoaded()) {
        g.available = false;
        g.sourceLabel = QStringLiteral("NVML Not Loaded");
        return g;
    }

    g.available = true;
    g.sourceLabel = QStringLiteral("NVML (libnvidia-ml.so.1)");
    g.name = m_deviceName.isEmpty() ? QStringLiteral("NVIDIA GeForce RTX 3050 6GB Laptop GPU") : m_deviceName;
    g.driverVersion = m_driverVersion;
    g.pciBusId = m_pciBusId;
    g.subsystemId = m_subsystemId;

    unsigned int temp = 0;
    if (m_fnGetTemperature && m_fnGetTemperature(m_deviceHandle, 0, &temp) == 0) {
        g.tempC = static_cast<double>(temp);
    }

    CleanRoomNvmlUtilization util{0, 0};
    if (m_fnGetUtilization && m_fnGetUtilization(m_deviceHandle, &util) == 0) {
        g.gpuUtilizationPct = static_cast<int>(util.gpu);
        g.memUtilizationPct = static_cast<int>(util.memory);
    }

    CleanRoomNvmlMemory mem{0, 0, 0};
    if (m_fnGetMemoryInfo && m_fnGetMemoryInfo(m_deviceHandle, &mem) == 0) {
        g.vramTotalMiB = static_cast<int64_t>(mem.total / (1024ULL * 1024ULL));
        g.vramUsedMiB = static_cast<int64_t>(mem.used / (1024ULL * 1024ULL));
        g.vramFreeMiB = static_cast<int64_t>(mem.free / (1024ULL * 1024ULL));
    }

    unsigned int clk = 0;
    if (m_fnGetClockInfo && m_fnGetClockInfo(m_deviceHandle, 0, &clk) == 0) {
        g.graphicsClockMhz = static_cast<int>(clk);
    }
    if (m_fnGetClockInfo && m_fnGetClockInfo(m_deviceHandle, 1, &clk) == 0) {
        g.smClockMhz = static_cast<int>(clk);
    }
    if (m_fnGetClockInfo && m_fnGetClockInfo(m_deviceHandle, 2, &clk) == 0) {
        g.memClockMhz = static_cast<int>(clk);
    }

    if (m_fnGetMaxClockInfo && m_fnGetMaxClockInfo(m_deviceHandle, 0, &clk) == 0) {
        g.maxGraphicsClockMhz = static_cast<int>(clk);
    }
    if (m_fnGetMaxClockInfo && m_fnGetMaxClockInfo(m_deviceHandle, 2, &clk) == 0) {
        g.maxMemClockMhz = static_cast<int>(clk);
    }

    unsigned int minLimMw = 1000, maxLimMw = 75000, defLimMw = 60000;
    if (m_fnGetPowerLimits && m_fnGetPowerLimits(m_deviceHandle, &minLimMw, &maxLimMw) == 0) {
        g.powerMinLimitWatts = minLimMw / 1000.0;
        g.powerMaxLimitWatts = maxLimMw / 1000.0;
    }
    if (m_fnGetDefaultPowerLimit && m_fnGetDefaultPowerLimit(m_deviceHandle, &defLimMw) == 0) {
        g.powerDefaultLimitWatts = defLimMw / 1000.0;
    }

    int pstateNum = 8;
    if (m_fnGetPState && m_fnGetPState(m_deviceHandle, &pstateNum) == 0) {
        g.pstate = QStringLiteral("P%1").arg(pstateNum);
    } else {
        g.pstate = QStringLiteral("P8");
    }

    unsigned int powerMw = 0;
    if (m_fnGetPowerUsage && m_fnGetPowerUsage(m_deviceHandle, &powerMw) == 0) {
        g.rawPowerWatts = powerMw / 1000.0;
        g.powerDrawWatts = filterPowerReading(g.rawPowerWatts, g.powerMaxLimitWatts, pstateNum, g.powerRailGated);
    }

    unsigned long long reasons = 0;
    if (m_fnGetClocksEventReasons && m_fnGetClocksEventReasons(m_deviceHandle, &reasons) == 0) {
        g.throttleReasonsBitmask = static_cast<uint64_t>(reasons);
        g.throttleReasonsDecoded = decodeThrottleReasons(g.throttleReasonsBitmask);
    } else {
        g.throttleReasonsDecoded = {QStringLiteral("None")};
    }

    unsigned int fanPct = 0;
    if (m_fnGetFanSpeed && m_fnGetFanSpeed(m_deviceHandle, &fanPct) == 0) {
        g.fanSpeedPct = static_cast<int>(fanPct);
        g.fanSpeedReported = true;
    } else {
        g.fanSpeedPct = 0;
        g.fanSpeedReported = false;
    }

    return g;
}

} // namespace acc::hal
