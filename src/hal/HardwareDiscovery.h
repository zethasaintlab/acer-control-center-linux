// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include <QString>
#include <vector>

namespace acc::hal {

class HardwareDiscovery {
public:
    static core::DmiIdentity probeDmiAndWmi();
    static core::KeyboardHardwareState probeKeyboardHardware(const core::DmiIdentity& dmi);
    static std::vector<core::CapabilityRecord> probeAllCapabilities(bool nvmlLoaded);

    // Locates specific sysfs directories under the active SysfsRoot
    static QString findHwmonByName(const QString& driverName);
    static QString findFirstKeyboardLedPath();
    static QString findFirstFanPwmHwmonPath();
};

} // namespace acc::hal
