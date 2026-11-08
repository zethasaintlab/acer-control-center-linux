// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "core/ConfigStore.h"
#include "core/ColorScience.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>

namespace acc::core {

ConfigStore::ConfigStore(const QString& configDirOverride) {
    if (!configDirOverride.trimmed().isEmpty()) {
        m_configDir = configDirOverride.trimmed();
    } else {
        const QString envDir = qEnvironmentVariable("ACC_CONFIG_DIR");
        if (!envDir.trimmed().isEmpty()) {
            m_configDir = envDir.trimmed();
        } else {
            const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
            m_configDir = base.isEmpty()
                              ? (QDir::homePath() + QStringLiteral("/.config/acer-control-center"))
                              : (QFileInfo(base).dir().filePath(QStringLiteral("acer-control-center")));
        }
    }
}

QString ConfigStore::configDirectory() const {
    return m_configDir;
}

QString ConfigStore::settingsFilePath() const {
    return QDir(m_configDir).filePath(QStringLiteral("settings.json"));
}

AppSettings ConfigStore::defaultSettings() {
    AppSettings s;
    s.schemaVersion = 1;
    s.darkTheme = true;
    s.accentColorHex = QStringLiteral("#3B9EFF");
    s.pollingIntervalMs = 1000;
    s.reducedMotion = false;
    s.compactNavigation = false;
    s.minimizeToTray = true;
    s.launchAtLogin = false;
    s.enableNotifications = true;

    s.activeOperatingProfileId = QStringLiteral("balanced");
    s.autoSwitchProfileOnPower = true;
    s.acOperatingProfileId = QStringLiteral("performance");
    s.batteryOperatingProfileId = QStringLiteral("balanced");

    s.customOperatingProfile.id = QStringLiteral("custom");
    s.customOperatingProfile.name = QStringLiteral("Custom Atelier");
    s.customOperatingProfile.subtitle = QStringLiteral("User-defined P-state bounds & EPP preference");
    s.customOperatingProfile.tunedProfile = QStringLiteral("balanced");
    s.customOperatingProfile.ppdProfile = QStringLiteral("balanced");
    s.customOperatingProfile.governor = QStringLiteral("powersave");
    s.customOperatingProfile.epp = QStringLiteral("balance_performance");
    s.customOperatingProfile.turboEnabled = true;
    s.customOperatingProfile.minPerfPct = 15;
    s.customOperatingProfile.maxPerfPct = 95;
    s.customOperatingProfile.isCustom = true;

    s.customAccentSwatches = {
        QStringLiteral("#3B9EFF"),
        QStringLiteral("#2EBE7E"),
        QStringLiteral("#E5A024"),
        QStringLiteral("#E04F5F"),
        QStringLiteral("#00B8D9"),
        QStringLiteral("#D97706"),
        QStringLiteral("#84CC16"),
        QStringLiteral("#94A3B8")
    };

    s.customKeyboardSwatches = {
        QStringLiteral("#3B9EFF"),
        QStringLiteral("#FFFFFF"),
        QStringLiteral("#00E5B0"),
        QStringLiteral("#FFB020"),
        QStringLiteral("#FF4D6D"),
        QStringLiteral("#6EE7B7"),
        QStringLiteral("#38BDF8"),
        QStringLiteral("#F43F5E")
    };

    KeyboardLightingProfile p1;
    p1.id = QStringLiteral("atelier_static");
    p1.name = QStringLiteral("Atelier Ice Blue");
    p1.enabled = true;
    p1.mode = QStringLiteral("static");
    p1.brightnessPct = 80;
    p1.timeoutSeconds = 30;
    p1.zones = {
        KeyboardZoneColor{0, QStringLiteral("Zone 1 (WASD / Left)"), 59, 158, 255, QStringLiteral("#3B9EFF")},
        KeyboardZoneColor{1, QStringLiteral("Zone 2 (Center Alpha)"), 59, 158, 255, QStringLiteral("#3B9EFF")},
        KeyboardZoneColor{2, QStringLiteral("Zone 3 (Arrows / Nav)"), 59, 158, 255, QStringLiteral("#3B9EFF")},
        KeyboardZoneColor{3, QStringLiteral("Zone 4 (Numeric Pad)"), 59, 158, 255, QStringLiteral("#3B9EFF")}
    };

    KeyboardLightingProfile p2;
    p2.id = QStringLiteral("workstation_warm");
    p2.name = QStringLiteral("Workstation Warm White");
    p2.enabled = true;
    p2.mode = QStringLiteral("static");
    p2.brightnessPct = 60;
    p2.timeoutSeconds = 60;
    p2.zones = {
        KeyboardZoneColor{0, QStringLiteral("Zone 1 (WASD / Left)"), 255, 236, 209, QStringLiteral("#FFECD1")},
        KeyboardZoneColor{1, QStringLiteral("Zone 2 (Center Alpha)"), 255, 244, 229, QStringLiteral("#FFF4E5")},
        KeyboardZoneColor{2, QStringLiteral("Zone 3 (Arrows / Nav)"), 255, 236, 209, QStringLiteral("#FFECD1")},
        KeyboardZoneColor{3, QStringLiteral("Zone 4 (Numeric Pad)"), 255, 236, 209, QStringLiteral("#FFECD1")}
    };

    KeyboardLightingProfile p3;
    p3.id = QStringLiteral("thermal_zone_split");
    p3.name = QStringLiteral("Tactical Zone Split");
    p3.enabled = true;
    p3.mode = QStringLiteral("static");
    p3.brightnessPct = 100;
    p3.timeoutSeconds = 0;
    p3.zones = {
        KeyboardZoneColor{0, QStringLiteral("Zone 1 (WASD / Left)"), 224, 79, 95, QStringLiteral("#E04F5F")},
        KeyboardZoneColor{1, QStringLiteral("Zone 2 (Center Alpha)"), 59, 158, 255, QStringLiteral("#3B9EFF")},
        KeyboardZoneColor{2, QStringLiteral("Zone 3 (Arrows / Nav)"), 229, 160, 36, QStringLiteral("#E5A024")},
        KeyboardZoneColor{3, QStringLiteral("Zone 4 (Numeric Pad)"), 46, 190, 126, QStringLiteral("#2EBE7E")}
    };

    s.keyboardProfiles = {p1, p2, p3};
    s.activeKeyboardProfileId = p1.id;

    s.customFanCurve = {
        FanCurvePoint{40, 25},
        FanCurvePoint{55, 40},
        FanCurvePoint{68, 60},
        FanCurvePoint{78, 82},
        FanCurvePoint{88, 100}
    };

    return s;
}

std::vector<OperatingProfile> ConfigStore::standardOperatingProfiles(const OperatingProfile& customProf) {
    OperatingProfile quiet;
    quiet.id = QStringLiteral("quiet");
    quiet.name = QStringLiteral("Quiet");
    quiet.subtitle = QStringLiteral("Low acoustic footprint, energy-biased EPP, conservative boost");
    quiet.tunedProfile = QStringLiteral("powersave");
    quiet.ppdProfile = QStringLiteral("power-saver");
    quiet.governor = QStringLiteral("powersave");
    quiet.epp = QStringLiteral("power");
    quiet.turboEnabled = false;
    quiet.minPerfPct = 10;
    quiet.maxPerfPct = 70;
    quiet.isCustom = false;

    OperatingProfile balanced;
    balanced.id = QStringLiteral("balanced");
    balanced.name = QStringLiteral("Balanced");
    balanced.subtitle = QStringLiteral("Default daily driver with dynamic Raptor Lake HWP scaling");
    balanced.tunedProfile = QStringLiteral("balanced");
    balanced.ppdProfile = QStringLiteral("balanced");
    balanced.governor = QStringLiteral("powersave");
    balanced.epp = QStringLiteral("balance_performance");
    balanced.turboEnabled = true;
    balanced.minPerfPct = 10;
    balanced.maxPerfPct = 100;
    balanced.isCustom = false;

    OperatingProfile perf;
    perf.id = QStringLiteral("performance");
    perf.name = QStringLiteral("Performance");
    perf.subtitle = QStringLiteral("Sustained peak clocks & throughput-performance system tuning");
    perf.tunedProfile = QStringLiteral("throughput-performance");
    perf.ppdProfile = QStringLiteral("performance");
    perf.governor = QStringLiteral("performance");
    perf.epp = QStringLiteral("performance");
    perf.turboEnabled = true;
    perf.minPerfPct = 25;
    perf.maxPerfPct = 100;
    perf.isCustom = false;

    OperatingProfile custom = customProf;
    custom.id = QStringLiteral("custom");
    custom.isCustom = true;
    if (custom.name.trimmed().isEmpty()) {
        custom.name = QStringLiteral("Custom Atelier");
    }

    return {quiet, balanced, perf, custom};
}

QJsonObject ConfigStore::serializeSettings(const AppSettings& s) {
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), s.schemaVersion);
    root.insert(QStringLiteral("dark_theme"), s.darkTheme);
    root.insert(QStringLiteral("accent_color_hex"), s.accentColorHex);
    root.insert(QStringLiteral("polling_interval_ms"), s.pollingIntervalMs);
    root.insert(QStringLiteral("reduced_motion"), s.reducedMotion);
    root.insert(QStringLiteral("compact_navigation"), s.compactNavigation);
    root.insert(QStringLiteral("minimize_to_tray"), s.minimizeToTray);
    root.insert(QStringLiteral("launch_at_login"), s.launchAtLogin);
    root.insert(QStringLiteral("enable_notifications"), s.enableNotifications);

    root.insert(QStringLiteral("active_operating_profile_id"), s.activeOperatingProfileId);
    root.insert(QStringLiteral("auto_switch_profile_on_power"), s.autoSwitchProfileOnPower);
    root.insert(QStringLiteral("ac_operating_profile_id"), s.acOperatingProfileId);
    root.insert(QStringLiteral("battery_operating_profile_id"), s.batteryOperatingProfileId);

    QJsonObject customOp;
    customOp.insert(QStringLiteral("name"), s.customOperatingProfile.name);
    customOp.insert(QStringLiteral("tuned_profile"), s.customOperatingProfile.tunedProfile);
    customOp.insert(QStringLiteral("ppd_profile"), s.customOperatingProfile.ppdProfile);
    customOp.insert(QStringLiteral("governor"), s.customOperatingProfile.governor);
    customOp.insert(QStringLiteral("epp"), s.customOperatingProfile.epp);
    customOp.insert(QStringLiteral("turbo_enabled"), s.customOperatingProfile.turboEnabled);
    customOp.insert(QStringLiteral("min_perf_pct"), s.customOperatingProfile.minPerfPct);
    customOp.insert(QStringLiteral("max_perf_pct"), s.customOperatingProfile.maxPerfPct);
    root.insert(QStringLiteral("custom_operating_profile"), customOp);

    QJsonArray accentArr;
    for (const QString& sw : s.customAccentSwatches) {
        accentArr.append(sw);
    }
    root.insert(QStringLiteral("custom_accent_swatches"), accentArr);

    QJsonArray kbdSwArr;
    for (const QString& sw : s.customKeyboardSwatches) {
        kbdSwArr.append(sw);
    }
    root.insert(QStringLiteral("custom_keyboard_swatches"), kbdSwArr);
    root.insert(QStringLiteral("active_keyboard_profile_id"), s.activeKeyboardProfileId);

    QJsonArray kbdProfilesArr;
    for (const KeyboardLightingProfile& kp : s.keyboardProfiles) {
        QJsonObject kpObj;
        kpObj.insert(QStringLiteral("id"), kp.id);
        kpObj.insert(QStringLiteral("name"), kp.name);
        kpObj.insert(QStringLiteral("enabled"), kp.enabled);
        kpObj.insert(QStringLiteral("mode"), kp.mode);
        kpObj.insert(QStringLiteral("brightness_pct"), kp.brightnessPct);
        kpObj.insert(QStringLiteral("timeout_seconds"), kp.timeoutSeconds);

        QJsonArray zonesArr;
        for (const KeyboardZoneColor& z : kp.zones) {
            QJsonObject zObj;
            zObj.insert(QStringLiteral("zone_index"), z.zoneIndex);
            zObj.insert(QStringLiteral("name"), z.name);
            zObj.insert(QStringLiteral("r"), z.r);
            zObj.insert(QStringLiteral("g"), z.g);
            zObj.insert(QStringLiteral("b"), z.b);
            zObj.insert(QStringLiteral("hex"), z.hex);
            zonesArr.append(zObj);
        }
        kpObj.insert(QStringLiteral("zones"), zonesArr);
        kbdProfilesArr.append(kpObj);
    }
    root.insert(QStringLiteral("keyboard_profiles"), kbdProfilesArr);

    QJsonArray fanCurveArr;
    for (const FanCurvePoint& pt : s.customFanCurve) {
        QJsonObject ptObj;
        ptObj.insert(QStringLiteral("temp_c"), pt.tempC);
        ptObj.insert(QStringLiteral("duty_pct"), pt.dutyPct);
        fanCurveArr.append(ptObj);
    }
    root.insert(QStringLiteral("custom_fan_curve"), fanCurveArr);

    return root;
}

bool ConfigStore::deserializeAndValidateSettings(const QJsonObject& obj, AppSettings& outSettings, QString* outWarning) {
    if (obj.isEmpty()) {
        if (outWarning) {
            *outWarning = QStringLiteral("Empty settings JSON object.");
        }
        return false;
    }

    const int version = obj.value(QStringLiteral("schema_version")).toInt(0);
    if (version < 1 || version > 1) {
        if (outWarning) {
            *outWarning = QStringLiteral("Unsupported settings schema_version: %1").arg(version);
        }
        return false;
    }

    AppSettings s = defaultSettings();
    s.schemaVersion = 1;
    s.darkTheme = obj.value(QStringLiteral("dark_theme")).toBool(s.darkTheme);

    RgbColor parsedAccent;
    const QString rawAccent = obj.value(QStringLiteral("accent_color_hex")).toString(s.accentColorHex);
    if (ColorScience::parseHexToRgb(rawAccent, parsedAccent)) {
        s.accentColorHex = ColorScience::rgbToHex(parsedAccent);
    } else if (outWarning) {
        *outWarning = QStringLiteral("Invalid accent_color_hex '%1'; reverted to default.").arg(rawAccent);
    }

    const int pollMs = obj.value(QStringLiteral("polling_interval_ms")).toInt(s.pollingIntervalMs);
    s.pollingIntervalMs = std::clamp(pollMs, 250, 10000);

    s.reducedMotion = obj.value(QStringLiteral("reduced_motion")).toBool(s.reducedMotion);
    s.compactNavigation = obj.value(QStringLiteral("compact_navigation")).toBool(s.compactNavigation);
    s.minimizeToTray = obj.value(QStringLiteral("minimize_to_tray")).toBool(s.minimizeToTray);
    s.launchAtLogin = obj.value(QStringLiteral("launch_at_login")).toBool(s.launchAtLogin);
    s.enableNotifications = obj.value(QStringLiteral("enable_notifications")).toBool(s.enableNotifications);

    const QStringList validProfileIds = {
        QStringLiteral("quiet"), QStringLiteral("balanced"),
        QStringLiteral("performance"), QStringLiteral("custom")
    };
    const QString actProf = obj.value(QStringLiteral("active_operating_profile_id")).toString(s.activeOperatingProfileId);
    if (validProfileIds.contains(actProf)) {
        s.activeOperatingProfileId = actProf;
    }

    s.autoSwitchProfileOnPower = obj.value(QStringLiteral("auto_switch_profile_on_power")).toBool(s.autoSwitchProfileOnPower);
    const QString acProf = obj.value(QStringLiteral("ac_operating_profile_id")).toString(s.acOperatingProfileId);
    if (validProfileIds.contains(acProf)) {
        s.acOperatingProfileId = acProf;
    }
    const QString batProf = obj.value(QStringLiteral("battery_operating_profile_id")).toString(s.batteryOperatingProfileId);
    if (validProfileIds.contains(batProf)) {
        s.batteryOperatingProfileId = batProf;
    }

    if (obj.contains(QStringLiteral("custom_operating_profile")) && obj.value(QStringLiteral("custom_operating_profile")).isObject()) {
        const QJsonObject cop = obj.value(QStringLiteral("custom_operating_profile")).toObject();
        s.customOperatingProfile.name = cop.value(QStringLiteral("name")).toString(s.customOperatingProfile.name).trimmed();
        if (s.customOperatingProfile.name.isEmpty()) {
            s.customOperatingProfile.name = QStringLiteral("Custom Atelier");
        }
        const QString gov = cop.value(QStringLiteral("governor")).toString(s.customOperatingProfile.governor);
        if (gov == QLatin1String("powersave") || gov == QLatin1String("performance")) {
            s.customOperatingProfile.governor = gov;
        }
        const QStringList allowedEpp = {
            QStringLiteral("default"), QStringLiteral("performance"),
            QStringLiteral("balance_performance"), QStringLiteral("balance_power"),
            QStringLiteral("power")
        };
        const QString epp = cop.value(QStringLiteral("epp")).toString(s.customOperatingProfile.epp);
        if (allowedEpp.contains(epp)) {
            s.customOperatingProfile.epp = epp;
        }
        s.customOperatingProfile.turboEnabled = cop.value(QStringLiteral("turbo_enabled")).toBool(s.customOperatingProfile.turboEnabled);
        int minP = std::clamp(cop.value(QStringLiteral("min_perf_pct")).toInt(s.customOperatingProfile.minPerfPct), 10, 100);
        int maxP = std::clamp(cop.value(QStringLiteral("max_perf_pct")).toInt(s.customOperatingProfile.maxPerfPct), 10, 100);
        if (minP > maxP) {
            std::swap(minP, maxP);
        }
        s.customOperatingProfile.minPerfPct = minP;
        s.customOperatingProfile.maxPerfPct = maxP;
    }

    if (obj.contains(QStringLiteral("keyboard_profiles")) && obj.value(QStringLiteral("keyboard_profiles")).isArray()) {
        std::vector<KeyboardLightingProfile> loadedProfiles;
        const QJsonArray kpArr = obj.value(QStringLiteral("keyboard_profiles")).toArray();
        for (const QJsonValue& val : kpArr) {
            if (!val.isObject()) {
                continue;
            }
            const QJsonObject kpObj = val.toObject();
            KeyboardLightingProfile kp;
            kp.id = kpObj.value(QStringLiteral("id")).toString().trimmed();
            kp.name = kpObj.value(QStringLiteral("name")).toString().trimmed();
            if (kp.id.isEmpty() || kp.name.isEmpty()) {
                continue;
            }
            kp.enabled = kpObj.value(QStringLiteral("enabled")).toBool(true);
            kp.mode = kpObj.value(QStringLiteral("mode")).toString(QStringLiteral("static"));
            kp.brightnessPct = std::clamp(kpObj.value(QStringLiteral("brightness_pct")).toInt(80), 0, 100);
            kp.timeoutSeconds = std::clamp(kpObj.value(QStringLiteral("timeout_seconds")).toInt(30), 0, 3600);

            const QJsonArray zArr = kpObj.value(QStringLiteral("zones")).toArray();
            for (const QJsonValue& zVal : zArr) {
                if (!zVal.isObject()) {
                    continue;
                }
                const QJsonObject zObj = zVal.toObject();
                KeyboardZoneColor z;
                z.zoneIndex = std::clamp(zObj.value(QStringLiteral("zone_index")).toInt(0), 0, 15);
                z.name = zObj.value(QStringLiteral("name")).toString(QStringLiteral("Zone %1").arg(z.zoneIndex + 1));
                const RgbColor c = ColorScience::clampRgb(
                    zObj.value(QStringLiteral("r")).toInt(59),
                    zObj.value(QStringLiteral("g")).toInt(158),
                    zObj.value(QStringLiteral("b")).toInt(255)
                );
                z.r = c.r;
                z.g = c.g;
                z.b = c.b;
                z.hex = ColorScience::rgbToHex(c);
                kp.zones.push_back(z);
            }
            if (!kp.zones.empty()) {
                loadedProfiles.push_back(kp);
            }
        }
        if (!loadedProfiles.empty()) {
            s.keyboardProfiles = loadedProfiles;
        }
    }

    const QString actKbdId = obj.value(QStringLiteral("active_keyboard_profile_id")).toString(s.activeKeyboardProfileId);
    for (const auto& kp : s.keyboardProfiles) {
        if (kp.id == actKbdId) {
            s.activeKeyboardProfileId = actKbdId;
            break;
        }
    }

    if (obj.contains(QStringLiteral("custom_fan_curve")) && obj.value(QStringLiteral("custom_fan_curve")).isArray()) {
        std::vector<FanCurvePoint> pts;
        const QJsonArray fcArr = obj.value(QStringLiteral("custom_fan_curve")).toArray();
        for (const QJsonValue& v : fcArr) {
            if (!v.isObject()) {
                continue;
            }
            const QJsonObject ptObj = v.toObject();
            FanCurvePoint pt;
            pt.tempC = std::clamp(ptObj.value(QStringLiteral("temp_c")).toInt(50), 25, 100);
            pt.dutyPct = std::clamp(ptObj.value(QStringLiteral("duty_pct")).toInt(50), 20, 100);
            pts.push_back(pt);
        }
        if (pts.size() >= 2) {
            std::sort(pts.begin(), pts.end(), [](const FanCurvePoint& a, const FanCurvePoint& b) {
                return a.tempC < b.tempC;
            });
            // Enforce monotonic non-decreasing fan duty for thermal safety
            int runningDuty = pts.front().dutyPct;
            for (auto& pt : pts) {
                if (pt.dutyPct < runningDuty) {
                    pt.dutyPct = runningDuty;
                }
                runningDuty = pt.dutyPct;
            }
            s.customFanCurve = pts;
        }
    }

    outSettings = s;
    return true;
}

AppSettings ConfigStore::loadSettings(QString* outWarning) const {
    QFile file(settingsFilePath());
    if (!file.exists()) {
        return defaultSettings();
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (outWarning) {
            *outWarning = QStringLiteral("Could not open settings file for reading; using defaults.");
        }
        return defaultSettings();
    }

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (outWarning) {
            *outWarning = QStringLiteral("Corrupted settings JSON (%1); restored safe defaults.").arg(err.errorString());
        }
        return defaultSettings();
    }

    AppSettings loaded;
    if (!deserializeAndValidateSettings(doc.object(), loaded, outWarning)) {
        return defaultSettings();
    }
    return loaded;
}

bool ConfigStore::saveSettings(const AppSettings& settings, QString* outError) const {
    QDir dir(m_configDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (outError) {
            *outError = QStringLiteral("Failed to create configuration directory: %1").arg(m_configDir);
        }
        return false;
    }

    QSaveFile saveFile(settingsFilePath());
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) {
            *outError = QStringLiteral("Failed to open settings file for atomic write: %1").arg(saveFile.errorString());
        }
        return false;
    }

    const QJsonDocument doc(serializeSettings(settings));
    saveFile.write(doc.toJson(QJsonDocument::Indented));
    if (!saveFile.commit()) {
        if (outError) {
            *outError = QStringLiteral("Failed to commit atomic settings file write.");
        }
        return false;
    }
    return true;
}

bool ConfigStore::exportProfileBundleToFile(const AppSettings& settings, const QString& filePath, QString* outError) const {
    QSaveFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) {
            *outError = QStringLiteral("Cannot open export file '%1': %2").arg(filePath, saveFile.errorString());
        }
        return false;
    }
    QJsonObject bundle = serializeSettings(settings);
    bundle.insert(QStringLiteral("bundle_type"), QStringLiteral("acer_control_center_profile_bundle"));
    const QJsonDocument doc(bundle);
    saveFile.write(doc.toJson(QJsonDocument::Indented));
    if (!saveFile.commit()) {
        if (outError) {
            *outError = QStringLiteral("Failed to commit exported profile bundle.");
        }
        return false;
    }
    return true;
}

bool ConfigStore::importProfileBundleFromFile(const QString& filePath, AppSettings& inOutSettings, QString* outError) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (outError) {
            *outError = QStringLiteral("Cannot read import file '%1': %2").arg(filePath, file.errorString());
        }
        return false;
    }
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (outError) {
            *outError = QStringLiteral("Invalid JSON in profile bundle: %1").arg(parseErr.errorString());
        }
        return false;
    }

    AppSettings validated;
    QString warning;
    if (!deserializeAndValidateSettings(doc.object(), validated, &warning)) {
        if (outError) {
            *outError = warning.isEmpty() ? QStringLiteral("Profile bundle failed schema validation.") : warning;
        }
        return false;
    }
    inOutSettings = validated;
    return true;
}

} // namespace acc::core
