// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "core/CapabilityState.h"
#include "core/ColorScience.h"
#include "core/ConfigStore.h"
#include "core/DiagnosticRedactor.h"
#include "daemon/DaemonServiceObject.h"
#include "hal/ControlOperations.h"
#include "hal/HardwareDiscovery.h"
#include "hal/NvmlDynamicLoader.h"
#include "hal/SysfsRoot.h"
#include "hal/TelemetryCollector.h"

#include <QFile>
#include <QMap>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest/QtTest>

using namespace acc;

class TestCoreAndHal : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testColorScienceConversionsAndWcag() {
        // 1. RGB <-> Hex roundtrip
        core::RgbColor parsed;
        QVERIFY(core::ColorScience::parseHexToRgb(QStringLiteral("#3B9EFF"), parsed));
        QCOMPARE(parsed.r, 59);
        QCOMPARE(parsed.g, 158);
        QCOMPARE(parsed.b, 255);
        QCOMPARE(core::ColorScience::rgbToHex(parsed), QStringLiteral("#3B9EFF"));

        // 3-digit shorthand hex
        QVERIFY(core::ColorScience::parseHexToRgb(QStringLiteral("#F0A"), parsed));
        QCOMPARE(parsed.r, 255);
        QCOMPARE(parsed.g, 0);
        QCOMPARE(parsed.b, 170);

        // Invalid hex rejected
        QVERIFY(!core::ColorScience::parseHexToRgb(QStringLiteral("#12345G"), parsed));

        // 2. RGB <-> HSV roundtrip
        const core::HsvColor hsv = core::ColorScience::rgbToHsv(core::RgbColor{59, 158, 255});
        const core::RgbColor rgbFromHsv = core::ColorScience::hsvToRgb(hsv);
        QVERIFY(std::abs(rgbFromHsv.r - 59) <= 1);
        QVERIFY(std::abs(rgbFromHsv.g - 158) <= 1);
        QVERIFY(std::abs(rgbFromHsv.b - 255) <= 1);

        // 3. RGB <-> HSL roundtrip
        const core::HslColor hsl = core::ColorScience::rgbToHsl(core::RgbColor{46, 190, 126});
        const core::RgbColor rgbFromHsl = core::ColorScience::hslToRgb(hsl);
        QVERIFY(std::abs(rgbFromHsl.r - 46) <= 1);
        QVERIFY(std::abs(rgbFromHsl.g - 190) <= 1);
        QVERIFY(std::abs(rgbFromHsl.b - 126) <= 1);

        // 4. WCAG contrast guardrails even for extreme dark/bright user accent choices
        const core::AccentTokenSet darkThemeDarkAccent =
            core::ColorScience::deriveAccessibleAccentTokens(QColor(QStringLiteral("#050510")), true);
        QVERIFY(darkThemeDarkAccent.contrastOnSurface >= 4.5);
        QVERIFY(darkThemeDarkAccent.contrastOnAccent >= 4.5);
        QVERIFY(darkThemeDarkAccent.wcagAaNormal);

        const core::AccentTokenSet lightThemeBrightAccent =
            core::ColorScience::deriveAccessibleAccentTokens(QColor(QStringLiteral("#FFFFAA")), false);
        QVERIFY(lightThemeBrightAccent.contrastOnSurface >= 4.5);
        QVERIFY(lightThemeBrightAccent.contrastOnAccent >= 4.5);
        QVERIFY(lightThemeBrightAccent.wcagAaNormal);
    }

    void testNvmlPowerRailGateFilterAndThrottleDecode() {
        bool gated = false;
        // Normal 45W reading in P0 with 75W max limit
        double filtered = hal::NvmlDynamicLoader::filterPowerReading(45.2, 75.0, 0, gated);
        QVERIFY(!gated);
        QCOMPARE(filtered, 45.2);

        // Anomalous 312.23W reading in P8 on GA107BM (observed on physical A715-59G)
        filtered = hal::NvmlDynamicLoader::filterPowerReading(312.23, 75.0, 8, gated);
        QVERIFY(gated);
        QCOMPARE(filtered, 0.0);

        const QStringList reasons = hal::NvmlDynamicLoader::decodeThrottleReasons(0x01ULL | 0x04ULL);
        QCOMPARE(reasons.size(), 2);
        QVERIFY(reasons.at(0).contains(QLatin1String("GpuIdle")));
        QVERIFY(reasons.at(1).contains(QLatin1String("SwPowerCap")));
    }

    void testDiagnosticRedactorStripsSensitiveIdentifiers() {
        const QString dirty = QStringLiteral(
            "User=/home/zeth/secret Serial_Number=SN99887766 MAC=aa:bb:cc:dd:ee:ff "
            "RandomUUID=123e4567-e89b-12d3-a456-426614174000 "
            "PublicWMI=ABBC0F6D-8EA1-11D1-00A0-C90629100000"
        );
        const QString clean = core::DiagnosticRedactor::redactSensitiveText(dirty);
        QVERIFY(!clean.contains(QLatin1String("/home/zeth")));
        QVERIFY(clean.contains(QLatin1String("/home/<user>")));
        QVERIFY(!clean.contains(QLatin1String("SN99887766")));
        QVERIFY(!clean.contains(QLatin1String("aa:bb:cc:dd:ee:ff")));
        QVERIFY(!clean.contains(QLatin1String("123e4567-e89b-12d3-a456-426614174000")));
        // Public Clevo WMI GUID must remain intact for diagnostics
        QVERIFY(clean.contains(QLatin1String("ABBC0F6D-8EA1-11D1-00A0-C90629100000")));
    }

    void testConfigStorePersistenceAndValidation() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        core::ConfigStore store(tempDir.path());
        core::AppSettings settings = core::ConfigStore::defaultSettings();
        settings.darkTheme = false;
        settings.accentColorHex = QStringLiteral("#2EBE7E");
        settings.pollingIntervalMs = 2000;
        settings.customOperatingProfile.minPerfPct = 80;
        settings.customOperatingProfile.maxPerfPct = 40; // Intentionally inverted to test validator

        QString err;
        QVERIFY(store.saveSettings(settings, &err));

        QString warn;
        const core::AppSettings loaded = store.loadSettings(&warn);
        QCOMPARE(loaded.darkTheme, false);
        QCOMPARE(loaded.accentColorHex, QStringLiteral("#2EBE7E"));
        QCOMPARE(loaded.pollingIntervalMs, 2000);
        // Validator must auto-correct inverted min/max perf pct
        QCOMPARE(loaded.customOperatingProfile.minPerfPct, 40);
        QCOMPARE(loaded.customOperatingProfile.maxPerfPct, 80);

        // Export & import bundle roundtrip
        const QString bundlePath = tempDir.filePath(QStringLiteral("bundle.json"));
        QVERIFY(store.exportProfileBundleToFile(loaded, bundlePath, &err));

        core::AppSettings imported;
        QVERIFY(store.importProfileBundleFromFile(bundlePath, imported, &err));
        QCOMPARE(imported.accentColorHex, QStringLiteral("#2EBE7E"));

        // Corrupted JSON recovery
        QFile badFile(store.settingsFilePath());
        QVERIFY(badFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        badFile.write("{ corrupted json ");
        badFile.close();

        const core::AppSettings recovered = store.loadSettings(&warn);
        QVERIFY(!warn.isEmpty());
        QCOMPARE(recovered.schemaVersion, 1);
    }

    void testMockPhysicalMirrorDiscoveryAndSafeUnsupportedControls() {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockPhysicalMirror);
        hal::TelemetryCollector collector;
        const core::SystemSnapshot snap = collector.collectSnapshot(QStringLiteral("balanced"));

        QVERIFY(snap.mockModeActive);
        QCOMPARE(snap.dmi.sysVendor, QStringLiteral("Acer"));
        QCOMPARE(snap.dmi.productName, QStringLiteral("Aspire A715-59G"));
        QCOMPARE(snap.dmi.boardVendor, QStringLiteral("Kelilu_RTH"));
        QVERIFY(!snap.dmi.acerWmiLoaded);
        QVERIFY(snap.dmi.wmiGuids.join(QLatin1Char(',')).contains(QLatin1String("ABBC0F6D")));

        // On Physical Mirror, keyboard LED, fan PWM, and battery charge threshold must be NotDetected
        QCOMPARE(snap.keyboardHw.capabilityState, core::CapabilityState::NotDetected);
        QVERIFY(!snap.keyboardHw.supportsBacklightBrightness);
        QVERIFY(!snap.thermal.manualFanSupported);
        QVERIFY(!snap.battery.chargeLimitSupported);

        // Attempting writes on unexposed interfaces must fail safely without fabricating success
        int rbPct = 0;
        const core::ControlResult batRes = hal::ControlOperations::applyBatteryChargeLimit(80, rbPct);
        QVERIFY(!batRes.applied);
        QCOMPARE(batRes.errorCode, QStringLiteral("ERR_CHARGE_LIMIT_NOT_DETECTED"));

        const core::ControlResult kbdRes = hal::ControlOperations::applyKeyboardLighting(
            QStringLiteral("static"), 80, QStringLiteral("#3B9EFF")
        );
        QVERIFY(!kbdRes.applied);
        QCOMPARE(kbdRes.errorCode, QStringLiteral("ERR_KEYBOARD_LED_NOT_DETECTED"));

        const core::ControlResult fanRes = hal::ControlOperations::applyFanControlMode(QStringLiteral("manual"), 75);
        QVERIFY(!fanRes.applied);
        QCOMPARE(fanRes.errorCode, QStringLiteral("ERR_FAN_PWM_NOT_DETECTED"));
    }

    void testMockFullCapabilitiesVerifiedWritesAndDaemonPolkitGate() {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);

        // 1. Authorized daemon operations
        daemon::DaemonServiceObject daemonObj(daemon::AuthPolicyMode::TestAllowAll);
        bool applied = false;
        QString rbJson, errCode;

        daemonObj.SetCpuPerformanceTuning(
            QStringLiteral("powersave"),
            QStringLiteral("balance_power"),
            false,
            20,
            85,
            applied,
            rbJson,
            errCode
        );
        QVERIFY(applied);
        QCOMPARE(errCode, QStringLiteral("OK"));
        QVERIFY(rbJson.contains(QLatin1String("\"min_perf_pct\":20")));
        QVERIFY(rbJson.contains(QLatin1String("\"max_perf_pct\":85")));

        // 2. Reject invalid governor input
        daemonObj.SetCpuPerformanceTuning(
            QStringLiteral("overclock_unsafe"),
            QStringLiteral("performance"),
            true,
            10,
            100,
            applied,
            rbJson,
            errCode
        );
        QVERIFY(!applied);
        QCOMPARE(errCode, QStringLiteral("ERR_INVALID_GOVERNOR"));

        // 3. Keyboard RGB + Brightness verified write
        daemonObj.SetKeyboardLighting(
            QStringLiteral("static"),
            75,
            QStringLiteral("#E04F5F,#3B9EFF,#E5A024,#2EBE7E"),
            applied,
            rbJson,
            errCode
        );
        QVERIFY(applied);
        QCOMPARE(errCode, QStringLiteral("OK"));
        QVERIFY(rbJson.contains(QLatin1String("224 79 95")));

        // 4. Battery charge threshold verified write
        uint rbPct = 0;
        daemonObj.SetBatteryChargeLimit(80, applied, rbPct, errCode);
        QVERIFY(applied);
        QCOMPARE(rbPct, 80U);
        QCOMPARE(errCode, QStringLiteral("OK"));

        // 5. Fan PWM control & auto restoration
        daemonObj.SetFanControlMode(QStringLiteral("manual"), 60, applied, rbJson, errCode);
        QVERIFY(applied);
        QCOMPARE(errCode, QStringLiteral("OK"));

        daemonObj.SetFanControlMode(QStringLiteral("auto"), 60, applied, rbJson, errCode);
        QVERIFY(applied);
        QVERIFY(rbJson.contains(QLatin1String("\"pwm1_enable\":2")));

        // 6. Unauthorized Polkit rejection test
        daemonObj.setAuthMode(daemon::AuthPolicyMode::TestDenyAll);
        daemonObj.SetBatteryChargeLimit(80, applied, rbPct, errCode);
        QVERIFY(!applied);
        QCOMPARE(errCode, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));

        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testPolkitPolicyXmlBijectionAndPrivilegeBoundaries() {
        const QString policyPath = QStringLiteral(QT_TESTCASE_SOURCEDIR "/data/polkit/io.github.zethasaintlab.acercontrol.policy");
        QFile xmlFile(policyPath);
        QVERIFY2(xmlFile.open(QIODevice::ReadOnly | QIODevice::Text), "Failed to open data/polkit/io.github.zethasaintlab.acercontrol.policy");

        struct PolicyActionEntry {
            int declarationCount = 0;
            QString allowAny;
            QString allowInactive;
            QString allowActive;
        };
        QMap<QString, PolicyActionEntry> parsedActions;

        QXmlStreamReader xml(&xmlFile);
        QString currentActionId;
        bool insideDefaults = false;

        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                const QString elem = xml.name().toString();
                if (elem == QLatin1String("action")) {
                    currentActionId = xml.attributes().value(QLatin1String("id")).toString();
                    QVERIFY(!currentActionId.isEmpty());
                    parsedActions[currentActionId].declarationCount += 1;
                } else if (elem == QLatin1String("defaults") && !currentActionId.isEmpty()) {
                    insideDefaults = true;
                } else if (insideDefaults && !currentActionId.isEmpty()) {
                    const QString val = xml.readElementText().trimmed();
                    if (elem == QLatin1String("allow_any")) {
                        parsedActions[currentActionId].allowAny = val;
                    } else if (elem == QLatin1String("allow_inactive")) {
                        parsedActions[currentActionId].allowInactive = val;
                    } else if (elem == QLatin1String("allow_active")) {
                        parsedActions[currentActionId].allowActive = val;
                    }
                }
            } else if (xml.isEndElement()) {
                const QString elem = xml.name().toString();
                if (elem == QLatin1String("defaults")) {
                    insideDefaults = false;
                } else if (elem == QLatin1String("action")) {
                    currentActionId.clear();
                }
            }
        }
        QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));

        // Verify exact 1-to-1 bijection between PolkitActions constants and XML <action> declarations
        const QStringList expectedActions = daemon::PolkitActions::allActionIds();
        QCOMPARE(expectedActions.size(), 5);
        QCOMPARE(parsedActions.size(), expectedActions.size());

        for (const QString& actionId : expectedActions) {
            QVERIFY2(parsedActions.contains(actionId), qPrintable(QStringLiteral("Missing Polkit XML action: %1").arg(actionId)));
            const PolicyActionEntry& entry = parsedActions[actionId];
            QCOMPARE(entry.declarationCount, 1);
            QCOMPARE(entry.allowAny, QStringLiteral("no"));
            QCOMPARE(entry.allowInactive, QStringLiteral("no"));
        }

        // Verify read-only / non-destructive vs admin-gated write boundaries in policy defaults
        QCOMPARE(parsedActions[QString(daemon::PolkitActions::kReadRapl)].allowActive, QStringLiteral("yes"));
        QCOMPARE(parsedActions[QString(daemon::PolkitActions::kSetKeyboardLighting)].allowActive, QStringLiteral("yes"));
        QCOMPARE(parsedActions[QString(daemon::PolkitActions::kSetPowerProfile)].allowActive, QStringLiteral("auth_admin_keep"));
        QCOMPARE(parsedActions[QString(daemon::PolkitActions::kSetBatteryLimit)].allowActive, QStringLiteral("auth_admin_keep"));
        QCOMPARE(parsedActions[QString(daemon::PolkitActions::kSetFanMode)].allowActive, QStringLiteral("auth_admin_keep"));

        // Verify PolkitAuthorizer fails closed if an unregistered or legacy action ID is ever requested
        daemon::PolkitAuthorizer auth(daemon::AuthPolicyMode::TestAllowAll);
        QString reason;
        QVERIFY(!auth.checkCallerAuthorized(
            QStringLiteral(":1.99"),
            QStringLiteral("io.github.zethasaintlab.acercontrol.read-power"),
            false,
            &reason
        ));
        QVERIFY(!auth.checkCallerAuthorized(
            QStringLiteral(":1.99"),
            QStringLiteral("io.github.zethasaintlab.acercontrol.manage-performance"),
            true,
            &reason
        ));
    }

    void testAllFiveDaemonOperationsAuthorizationAndZeroWritesOnDeny() {
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);
        const auto& root = hal::SysfsRoot::instance();

        daemon::DaemonServiceObject daemonObj(daemon::AuthPolicyMode::TestAllowAll);
        daemonObj.clearAuthAuditTrail();

        // Phase A: Execute all 5 operations under TestAllowAll and verify exact action ID + interactivity flags
        qulonglong pkgUj = 0, coreUj = 0, uncoreUj = 0, psysUj = 0;
        qlonglong tsUs = 0;
        bool raplAvail = false;
        daemonObj.GetRaplPowerSnapshot(pkgUj, coreUj, uncoreUj, psysUj, tsUs, raplAvail);
        QVERIFY(raplAvail);
        QVERIFY(pkgUj > 0ULL);

        bool applied = false;
        QString rbJson, errCode;
        uint rbPct = 0;

        daemonObj.SetCpuPerformanceTuning(
            QStringLiteral("powersave"), QStringLiteral("balance_power"), false, 20, 85, applied, rbJson, errCode
        );
        QVERIFY(applied);

        daemonObj.SetKeyboardLighting(
            QStringLiteral("static"), 75, QStringLiteral("#3B9EFF,#3B9EFF,#3B9EFF,#3B9EFF"), applied, rbJson, errCode
        );
        QVERIFY(applied);

        daemonObj.SetBatteryChargeLimit(80, applied, rbPct, errCode);
        QVERIFY(applied);
        QCOMPARE(rbPct, 80U);

        daemonObj.SetFanControlMode(QStringLiteral("manual"), 60, applied, rbJson, errCode);
        QVERIFY(applied);

        const auto& allowTrail = daemonObj.authorizer().auditTrail();
        QCOMPARE(allowTrail.size(), static_cast<size_t>(5));

        // 1. GetRaplPowerSnapshot: read-only, non-interactive (allowUserInteraction == false)
        QCOMPARE(allowTrail[0].actionId, QString(daemon::PolkitActions::kReadRapl));
        QCOMPARE(allowTrail[0].allowUserInteraction, false);
        QCOMPARE(allowTrail[0].authorized, true);

        // 2. SetCpuPerformanceTuning: write, interactive (allowUserInteraction == true)
        QCOMPARE(allowTrail[1].actionId, QString(daemon::PolkitActions::kSetPowerProfile));
        QCOMPARE(allowTrail[1].allowUserInteraction, true);
        QCOMPARE(allowTrail[1].authorized, true);

        // 3. SetKeyboardLighting: write, interactive (allowUserInteraction == true)
        QCOMPARE(allowTrail[2].actionId, QString(daemon::PolkitActions::kSetKeyboardLighting));
        QCOMPARE(allowTrail[2].allowUserInteraction, true);
        QCOMPARE(allowTrail[2].authorized, true);

        // 4. SetBatteryChargeLimit: write, interactive (allowUserInteraction == true)
        QCOMPARE(allowTrail[3].actionId, QString(daemon::PolkitActions::kSetBatteryLimit));
        QCOMPARE(allowTrail[3].allowUserInteraction, true);
        QCOMPARE(allowTrail[3].authorized, true);

        // 5. SetFanControlMode: write, interactive (allowUserInteraction == true)
        QCOMPARE(allowTrail[4].actionId, QString(daemon::PolkitActions::kSetFanMode));
        QCOMPARE(allowTrail[4].allowUserInteraction, true);
        QCOMPARE(allowTrail[4].authorized, true);

        // Snapshot current sysfs state before testing denied operations
        const QString minPerfPath = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate/min_perf_pct"));
        const QString maxPerfPath = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate/max_perf_pct"));
        const QString noTurboPath = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate/no_turbo"));
        const QString kbdBriPath = root.resolvePath(QStringLiteral("/sys/class/leds/acer::kbd_backlight/brightness"));
        const QString kbdRgbPath = root.resolvePath(QStringLiteral("/sys/class/leds/acer::kbd_backlight/multi_intensity"));
        const QString batLimitPath = root.resolvePath(QStringLiteral("/sys/class/power_supply/BAT0/charge_control_end_threshold"));
        const QString fanPwmPath = root.resolvePath(QStringLiteral("/sys/class/hwmon/hwmon7/pwm1"));
        const QString fanEnPath = root.resolvePath(QStringLiteral("/sys/class/hwmon/hwmon7/pwm1_enable"));

        const QString preMinPerf = hal::SysfsRoot::readFileTrimmed(minPerfPath);
        const QString preMaxPerf = hal::SysfsRoot::readFileTrimmed(maxPerfPath);
        const QString preNoTurbo = hal::SysfsRoot::readFileTrimmed(noTurboPath);
        const QString preKbdBri = hal::SysfsRoot::readFileTrimmed(kbdBriPath);
        const QString preKbdRgb = hal::SysfsRoot::readFileTrimmed(kbdRgbPath);
        const QString preBatLimit = hal::SysfsRoot::readFileTrimmed(batLimitPath);
        const QString preFanPwm = hal::SysfsRoot::readFileTrimmed(fanPwmPath);
        const QString preFanEn = hal::SysfsRoot::readFileTrimmed(fanEnPath);

        // Phase B: Switch to TestDenyAll and verify all 5 operations fail closed with ZERO sysfs writes
        daemonObj.setAuthMode(daemon::AuthPolicyMode::TestDenyAll);
        daemonObj.clearAuthAuditTrail();

        daemonObj.GetRaplPowerSnapshot(pkgUj, coreUj, uncoreUj, psysUj, tsUs, raplAvail);
        QVERIFY(!raplAvail);
        QCOMPARE(pkgUj, 0ULL);

        daemonObj.SetCpuPerformanceTuning(
            QStringLiteral("performance"), QStringLiteral("performance"), true, 55, 100, applied, rbJson, errCode
        );
        QVERIFY(!applied);
        QCOMPARE(errCode, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));

        daemonObj.SetKeyboardLighting(
            QStringLiteral("static"), 15, QStringLiteral("#FF0000,#FF0000,#FF0000,#FF0000"), applied, rbJson, errCode
        );
        QVERIFY(!applied);
        QCOMPARE(errCode, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));

        daemonObj.SetBatteryChargeLimit(60, applied, rbPct, errCode);
        QVERIFY(!applied);
        QCOMPARE(errCode, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));

        daemonObj.SetFanControlMode(QStringLiteral("auto"), 95, applied, rbJson, errCode);
        QVERIFY(!applied);
        QCOMPARE(errCode, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));

        // Verify zero sysfs attributes were modified when Polkit denied authorization
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(minPerfPath), preMinPerf);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(maxPerfPath), preMaxPerf);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(noTurboPath), preNoTurbo);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(kbdBriPath), preKbdBri);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(kbdRgbPath), preKbdRgb);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(batLimitPath), preBatLimit);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(fanPwmPath), preFanPwm);
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(fanEnPath), preFanEn);

        const auto& denyTrail = daemonObj.authorizer().auditTrail();
        QCOMPARE(denyTrail.size(), static_cast<size_t>(5));
        for (size_t i = 0; i < 5; ++i) {
            QCOMPARE(denyTrail[i].authorized, false);
        }

        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }
};

QTEST_MAIN(TestCoreAndHal)
#include "test_core_and_hal.moc"
