// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QMutex>
#include <QString>
#include <QTemporaryDir>
#include <memory>

namespace acc::hal {

enum class HardwareMode {
    PhysicalSystem,
    MockPhysicalMirror,   // Mirrors exact observed A715-59G / Kelilu_RTH sysfs
    MockFullCapabilities, // Adds simulated RGB LED, PWM fan, and charge threshold sysfs
    CustomRootPath        // Driven by ACC_SYSFS_ROOT or unit test fixture path
};

class SysfsRoot {
public:
    static SysfsRoot& instance();

    HardwareMode mode() const;
    bool isMockActive() const;
    QString scenarioLabel() const;

    void setMode(HardwareMode newMode);
    void setCustomRootPath(const QString& rootDir, const QString& label = QStringLiteral("Custom Test Root"));

    // Resolves an absolute Linux path like "/sys/class/hwmon" or "/proc/meminfo"
    // against either the physical root "/" or the active mock root directory.
    QString resolvePath(const QString& absoluteLinuxPath) const;

    // Helper for reading/writing text files safely
    static QString readFileTrimmed(const QString& fullPath, bool* outOk = nullptr);
    static bool writeFileVerified(const QString& fullPath, const QString& value, QString* outReadback = nullptr, QString* outError = nullptr);

    // Populates a directory with mock sysfs/procfs trees
    static bool populateMockTree(const QString& rootPath, bool includeOptionalControls);

private:
    SysfsRoot();

    mutable QMutex m_mutex;
    HardwareMode m_mode = HardwareMode::PhysicalSystem;
    QString m_customRoot;
    QString m_scenarioLabel = QStringLiteral("Physical Hardware");
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

} // namespace acc::hal
