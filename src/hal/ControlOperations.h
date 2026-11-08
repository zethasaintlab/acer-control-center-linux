// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include <QString>

namespace acc::hal {

class ControlOperations {
public:
    // Validates and writes CPU P-state / governor / EPP / turbo settings with readback and rollback
    static core::ControlResult applyCpuPerformanceTuning(
        const QString& governor,
        const QString& epp,
        bool turboEnabled,
        int minPerfPct,
        int maxPerfPct
    );

    // Switches Fedora system power profile via D-Bus net.hadess.PowerProfiles (tuned-ppd)
    static core::ControlResult applySystemPowerProfile(const QString& ppdProfile);

    // Validates and writes keyboard backlight brightness & RGB zones with readback verification
    static core::ControlResult applyKeyboardLighting(
        const QString& mode,
        int brightnessPct,
        const QString& zonesHexCsv
    );

    // Validates and writes battery charge control end threshold (50-100%) with readback verification
    static core::ControlResult applyBatteryChargeLimit(int endThresholdPct, int& outReadbackPct);

    // Validates and writes fan control mode ("auto", "manual", "curve") and PWM duty with automatic safety fallback
    static core::ControlResult applyFanControlMode(const QString& mode, int dutyPct);

    // Adjusts internal display brightness via systemd-logind or sysfs
    static core::ControlResult applyDisplayBrightness(int brightnessPct);
};

} // namespace acc::hal
