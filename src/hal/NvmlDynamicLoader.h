// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include <QStringList>
#include <cstdint>

namespace acc::hal {

class NvmlDynamicLoader {
public:
    NvmlDynamicLoader();
    ~NvmlDynamicLoader();

    NvmlDynamicLoader(const NvmlDynamicLoader&) = delete;
    NvmlDynamicLoader& operator=(const NvmlDynamicLoader&) = delete;

    bool isLoaded() const;
    core::GpuTelemetry readTelemetry() const;

    // Pure helper for filtering bogus NVML power readouts when the dGPU power rail is gated in P8
    static double filterPowerReading(double rawWatts, double maxLimitWatts, int pstateNumber, bool& outRailGated);

    // Pure helper for decoding NVML clocks event reason bitmasks
    static QStringList decodeThrottleReasons(uint64_t bitmask);

private:
    void tryInitialize();
    void shutdown();

    void* m_libHandle = nullptr;
    void* m_deviceHandle = nullptr;
    bool m_initialized = false;
    QString m_driverVersion;
    QString m_deviceName;
    QString m_pciBusId;
    QString m_subsystemId;

    // Clean-room NVML C ABI function pointers (loaded dynamically at runtime)
    using FnInit = int (*)();
    using FnShutdown = int (*)();
    using FnGetDriverVersion = int (*)(char*, unsigned int);
    using FnGetCount = int (*)(unsigned int*);
    using FnGetHandleByIndex = int (*)(unsigned int, void**);
    using FnGetName = int (*)(void*, char*, unsigned int);
    using FnGetTemperature = int (*)(void*, int, unsigned int*);
    using FnGetUtilization = int (*)(void*, void*);
    using FnGetMemoryInfo = int (*)(void*, void*);
    using FnGetClockInfo = int (*)(void*, int, unsigned int*);
    using FnGetMaxClockInfo = int (*)(void*, int, unsigned int*);
    using FnGetPowerUsage = int (*)(void*, unsigned int*);
    using FnGetPowerLimits = int (*)(void*, unsigned int*, unsigned int*);
    using FnGetDefaultPowerLimit = int (*)(void*, unsigned int*);
    using FnGetPState = int (*)(void*, int*);
    using FnGetClocksEventReasons = int (*)(void*, unsigned long long*);
    using FnGetFanSpeed = int (*)(void*, unsigned int*);

    FnInit m_fnInit = nullptr;
    FnShutdown m_fnShutdown = nullptr;
    FnGetDriverVersion m_fnGetDriverVersion = nullptr;
    FnGetCount m_fnGetCount = nullptr;
    FnGetHandleByIndex m_fnGetHandleByIndex = nullptr;
    FnGetName m_fnGetName = nullptr;
    FnGetTemperature m_fnGetTemperature = nullptr;
    FnGetUtilization m_fnGetUtilization = nullptr;
    FnGetMemoryInfo m_fnGetMemoryInfo = nullptr;
    FnGetClockInfo m_fnGetClockInfo = nullptr;
    FnGetMaxClockInfo m_fnGetMaxClockInfo = nullptr;
    FnGetPowerUsage m_fnGetPowerUsage = nullptr;
    FnGetPowerLimits m_fnGetPowerLimits = nullptr;
    FnGetDefaultPowerLimit m_fnGetDefaultPowerLimit = nullptr;
    FnGetPState m_fnGetPState = nullptr;
    FnGetClocksEventReasons m_fnGetClocksEventReasons = nullptr;
    FnGetFanSpeed m_fnGetFanSpeed = nullptr;
};

} // namespace acc::hal
