// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include "hal/NvmlDynamicLoader.h"
#include <QMap>
#include <cstdint>

namespace acc::hal {

struct CpuStatSample {
    uint64_t activeJiffies = 0;
    uint64_t totalJiffies = 0;
};

class TelemetryCollector {
public:
    TelemetryCollector();

    core::SystemSnapshot collectSnapshot(const QString& activeProfileId);

    // Allows feeding RAPL microjoule counters obtained from the privileged D-Bus daemon
    void updateRaplCounters(uint64_t pkgUj, uint64_t coreUj, uint64_t psysUj, int64_t timestampUs, bool available);

    bool isNvmlLoaded() const;

private:
    core::CpuTelemetry readCpuTelemetry();
    core::MemoryTelemetry readMemoryTelemetry() const;
    core::ThermalTelemetry readThermalTelemetry(double cpuPkgTemp, double gpuTemp) const;
    core::BatteryTelemetry readBatteryTelemetry() const;
    int readDisplayBrightnessPct(bool& outSupported) const;
    QString readActiveTunedOrPpdProfile() const;

    NvmlDynamicLoader m_nvml;
    CpuStatSample m_prevTotalCpu;
    QMap<int, CpuStatSample> m_prevCoreCpu;

    uint64_t m_prevPkgUj = 0;
    uint64_t m_prevCoreUj = 0;
    uint64_t m_prevPsysUj = 0;
    int64_t m_prevRaplTsUs = 0;
    double m_lastPkgWatts = 0.0;
    double m_lastCoreWatts = 0.0;
    double m_lastPsysWatts = 0.0;
    bool m_raplAvailable = false;
};

} // namespace acc::hal
