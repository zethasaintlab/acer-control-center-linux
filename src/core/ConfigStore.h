// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <vector>

namespace acc::core {

struct AppSettings {
    int schemaVersion = 1;
    bool darkTheme = true;
    QString accentColorHex = QStringLiteral("#3B9EFF");
    int pollingIntervalMs = 1000; // 500, 1000, 2000, 5000
    bool reducedMotion = false;
    bool compactNavigation = false;
    bool minimizeToTray = true;
    bool launchAtLogin = false;
    bool enableNotifications = true;

    // Performance & Power auto-switching
    QString activeOperatingProfileId = QStringLiteral("balanced");
    bool autoSwitchProfileOnPower = true;
    QString acOperatingProfileId = QStringLiteral("performance");
    QString batteryOperatingProfileId = QStringLiteral("balanced");
    OperatingProfile customOperatingProfile;

    // Saved application accent swatches
    QStringList customAccentSwatches;

    // Saved keyboard color swatches
    QStringList customKeyboardSwatches;
    QString activeKeyboardProfileId = QStringLiteral("atelier_static");
    std::vector<KeyboardLightingProfile> keyboardProfiles;

    // Custom fan curve points
    std::vector<FanCurvePoint> customFanCurve;
};

class ConfigStore {
public:
    explicit ConfigStore(const QString& configDirOverride = QString());

    QString configDirectory() const;
    QString settingsFilePath() const;

    AppSettings loadSettings(QString* outWarning = nullptr) const;
    bool saveSettings(const AppSettings& settings, QString* outError = nullptr) const;

    bool exportProfileBundleToFile(const AppSettings& settings, const QString& filePath, QString* outError = nullptr) const;
    bool importProfileBundleFromFile(const QString& filePath, AppSettings& inOutSettings, QString* outError = nullptr) const;

    static AppSettings defaultSettings();
    static std::vector<OperatingProfile> standardOperatingProfiles(const OperatingProfile& customProf);
    static QJsonObject serializeSettings(const AppSettings& settings);
    static bool deserializeAndValidateSettings(const QJsonObject& obj, AppSettings& outSettings, QString* outWarning = nullptr);

private:
    QString m_configDir;
};

} // namespace acc::core
