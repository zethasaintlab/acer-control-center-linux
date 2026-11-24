// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "core/CapabilityState.h"
#include "core/ColorScience.h"
#include "core/ConfigStore.h"
#include "core/DiagnosticRedactor.h"
#include "daemon/DaemonServiceObject.h"
#include "hal/Clv0001KeyboardBackend.h"
#include "hal/ControlOperations.h"
#include "hal/HardwareDiscovery.h"
#include "hal/NvmlDynamicLoader.h"
#include "hal/SysfsRoot.h"
#include "hal/TelemetryCollector.h"

#include <QFile>
#include <QMap>
#include <QTemporaryDir>
#include <QThread>
#include <QXmlStreamReader>
#include <QtTest/QtTest>
#include <cmath>
#include <errno.h>
#include <limits>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace clv0001_kmod_unit {
extern "C" {
#include "../../driver/acer_clv0001_kbd.c"
}
} // namespace clv0001_kmod_unit

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

        // Strict #RRGGBB parser (Stage 18): accepts 6 hex digits with '#', rejects shorthand/missing '#'/overflow
        QVERIFY(core::ColorScience::parseStrictHex6ToRgb(QStringLiteral("#12A7E3"), parsed));
        QCOMPARE(parsed.r, 18);
        QCOMPARE(parsed.g, 167);
        QCOMPARE(parsed.b, 227);
        QVERIFY(core::ColorScience::parseStrictHex6ToRgb(QStringLiteral(" #12a7e3 "), parsed));
        QVERIFY(!core::ColorScience::parseStrictHex6ToRgb(QStringLiteral("12A7E3"), parsed));
        QVERIFY(!core::ColorScience::parseStrictHex6ToRgb(QStringLiteral("#123"), parsed));
        QVERIFY(!core::ColorScience::parseStrictHex6ToRgb(QStringLiteral("#12345G"), parsed));
        QVERIFY(!core::ColorScience::parseStrictHex6ToRgb(QStringLiteral("#1234567"), parsed));

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

        // On Physical Mirror, keyboard LED & battery charge threshold must be NotDetected,
        // BAT0/cycle_count=0 must be reconciled as NOT REPORTED,
        // and fan control must truthfully report FirmwareAutomatic (PNP0C09:00 EC Auto)
        QCOMPARE(snap.keyboardHw.capabilityState, core::CapabilityState::NotDetected);
        QVERIFY(!snap.keyboardHw.supportsBacklightBrightness);
        QVERIFY(!snap.thermal.manualFanSupported);
        QVERIFY(!snap.battery.chargeLimitSupported);
        QCOMPARE(snap.battery.cycleCount, 0);
        QCOMPARE(snap.battery.cycleCountReported, false);
        QCOMPARE(snap.battery.cycleCountLabel, QStringLiteral("NOT REPORTED"));
        QVERIFY(snap.battery.statusExplanation.contains(QStringLiteral("BATTERY TELEMETRY ONLY — CHARGE LIMIT NOT DETECTED")));

        bool foundFanCap = false, foundBatCap = false;
        for (const auto& cap : snap.capabilities) {
            if (cap.id == QLatin1String("cap.thermal.fans")) {
                foundFanCap = true;
                QCOMPARE(cap.state, core::CapabilityState::FirmwareAutomatic);
                QCOMPARE(core::capabilityStateToBadge(cap.state), QStringLiteral("FIRMWARE AUTOMATIC"));
                QVERIFY(!cap.isWritable);
            } else if (cap.id == QLatin1String("cap.battery.limit")) {
                foundBatCap = true;
                QCOMPARE(cap.state, core::CapabilityState::NotDetected);
                QCOMPARE(core::capabilityStateToBadge(cap.state), QStringLiteral("NOT DETECTED"));
                QVERIFY(!cap.isWritable);
            }
        }
        QVERIFY(foundFanCap && foundBatCap);

        // Battery telemetry range validation, rounding, rollback & readback verification in MockFullCapabilities
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);
        const core::SystemSnapshot fullSnap = collector.collectSnapshot(QStringLiteral("balanced"));
        QCOMPARE(fullSnap.battery.cycleCount, 42);
        QCOMPARE(fullSnap.battery.cycleCountReported, true);
        QCOMPARE(fullSnap.battery.cycleCountLabel, QStringLiteral("42"));
        for (const auto& cap : fullSnap.capabilities) {
            if (cap.id == QLatin1String("cap.thermal.fans")) {
                QCOMPARE(cap.state, core::CapabilityState::Simulated);
                QCOMPARE(core::capabilityStateToBadge(cap.state), QStringLiteral("SIMULATED"));
                QVERIFY(cap.summary.contains(QStringLiteral("SIMULATED")));
            } else if (cap.id == QLatin1String("cap.battery.limit")) {
                QCOMPARE(cap.state, core::CapabilityState::Simulated);
                QCOMPARE(core::capabilityStateToBadge(cap.state), QStringLiteral("SIMULATED"));
                QVERIFY(cap.summary.contains(QStringLiteral("SIMULATED")));
            }
        }

        int rbLim = 0;
        const core::ControlResult batSetRes = hal::ControlOperations::applyBatteryChargeLimit(75, rbLim);
        QVERIFY(batSetRes.applied);
        QVERIFY(batSetRes.physicalStateVerified);
        QCOMPARE(batSetRes.verificationStatusId, QStringLiteral("verified_physical_readback"));
        QCOMPARE(rbLim, 75);
        QVERIFY(batSetRes.message.contains(QStringLiteral("previous=80%")));
        QCOMPARE(hal::ControlOperations::applyBatteryChargeLimit(45, rbLim).errorCode, QStringLiteral("ERR_INVALID_THRESHOLD"));
        QCOMPARE(hal::ControlOperations::applyBatteryChargeLimit(105, rbLim).errorCode, QStringLiteral("ERR_INVALID_THRESHOLD"));

        // Driver rounding handling (step=10 rounds requested 78% -> 80% with previous=75% logged)
        const QString stepPath = hal::SysfsRoot::instance().resolvePath(
            QStringLiteral("/sys/class/power_supply/BAT0/mock_charge_limit_step")
        );
        {
            QFile stepF(stepPath);
            QVERIFY(stepF.open(QIODevice::WriteOnly | QIODevice::Truncate));
            stepF.write("10\n");
        }
        int rbRounded = 0;
        const core::ControlResult roundedRes = hal::ControlOperations::applyBatteryChargeLimit(78, rbRounded);
        QVERIFY(roundedRes.applied);
        QVERIFY(roundedRes.physicalStateVerified);
        QCOMPARE(rbRounded, 80);
        QCOMPARE(roundedRes.verificationStatusId, QStringLiteral("verified_physical_readback_rounded"));
        QVERIFY(roundedRes.message.contains(QStringLiteral("driver rounded requested 78%; previous=75%")));

        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockPhysicalMirror);

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

    void testSystemdServiceHardeningAndRaptorLakeCoreIdMapping() {
        // 1. Audit systemd unit sandboxing directives
        const QString unitPath = QStringLiteral(QT_TESTCASE_SOURCEDIR "/data/systemd/acer-control-daemon.service");
        const QString unitText = hal::SysfsRoot::readFileTrimmed(unitPath);
        QVERIFY(!unitText.isEmpty());
        QVERIFY(unitText.contains(QStringLiteral("NoNewPrivileges=yes")));
        QVERIFY(unitText.contains(QStringLiteral("ProtectSystem=strict")));
        QVERIFY(unitText.contains(QStringLiteral("ProtectHome=yes")));
        QVERIFY(unitText.contains(QStringLiteral("ProtectKernelTunables=yes")));
        QVERIFY(unitText.contains(QStringLiteral("PrivateDevices=yes")));
        QVERIFY(unitText.contains(QStringLiteral("ProtectControlGroups=yes")));
        QVERIFY(unitText.contains(QStringLiteral("RestrictAddressFamilies=AF_UNIX")));
        QVERIFY(unitText.contains(QStringLiteral("CapabilityBoundingSet=\n")));
        QVERIFY(unitText.contains(QStringLiteral("AmbientCapabilities=\n")));
        QVERIFY(!unitText.contains(QStringLiteral("CAP_SYS_ADMIN")));
        QVERIFY(!unitText.contains(QStringLiteral("CAP_DAC_OVERRIDE")));
        QVERIFY(!unitText.contains(QStringLiteral("CAP_DAC_READ_SEARCH")));
        QVERIFY(unitText.contains(QStringLiteral("UMask=0077")));
        QVERIFY(unitText.contains(QStringLiteral("ReadWritePaths=-/sys/devices/system/cpu/intel_pstate/no_turbo -/sys/devices/system/cpu/intel_pstate/min_perf_pct -/sys/devices/system/cpu/intel_pstate/max_perf_pct -/sys/devices/system/cpu/cpufreq -/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00")));
        // Never allow unrestricted ReadWritePaths=/sys, /sys/devices/platform, or /sys/class/power_supply/BAT0
        QVERIFY(!unitText.contains(QStringLiteral("ReadWritePaths=/sys\n")));
        QVERIFY(!unitText.contains(QStringLiteral("ReadWritePaths=-/sys\n")));
        QVERIFY(!unitText.contains(QStringLiteral("/sys/devices/platform")));
        QVERIFY(!unitText.contains(QStringLiteral("/sys/class/power_supply/BAT0")));

        // 2. Verify Raptor Lake non-contiguous topology/core_id mapping to coretemp
        auto& root = hal::SysfsRoot::instance();
        root.setMode(hal::HardwareMode::MockPhysicalMirror);
        const QString topo2Path = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu2/topology/core_id"));
        QDir().mkpath(QFileInfo(topo2Path).absolutePath());
        QVERIFY(hal::SysfsRoot::writeFileVerified(topo2Path, QStringLiteral("4")));

        const QString coretempDir = hal::HardwareDiscovery::findHwmonByName(QStringLiteral("coretemp"));
        QVERIFY(!coretempDir.isEmpty());
        QVERIFY(hal::SysfsRoot::writeFileVerified(coretempDir + QStringLiteral("/temp6_label"), QStringLiteral("Core 4")));
        QVERIFY(hal::SysfsRoot::writeFileVerified(coretempDir + QStringLiteral("/temp6_input"), QStringLiteral("61000")));

        hal::TelemetryCollector collector;
        const auto snap = collector.collectSnapshot(QStringLiteral("balanced"));
        QVERIFY(snap.cpu.cores.size() > 2);
        QCOMPARE(snap.cpu.cores[2].coreId, 2);
        QCOMPARE(snap.cpu.cores[2].tempC, 61.0);

        // 3. Verify SysfsRoot::writeFileVerified rejects path traversal and out-of-root writes
        QString errMsg;
        QVERIFY(!hal::SysfsRoot::writeFileVerified(topo2Path + QStringLiteral("/../core_id"), QStringLiteral("5"), nullptr, &errMsg));
        QVERIFY(errMsg.contains(QStringLiteral("Path traversal rejected")));

        QTemporaryDir outsideDir;
        QVERIFY(outsideDir.isValid());
        const QString outsideFile = outsideDir.filePath(QStringLiteral("rogue.txt"));
        {
            QFile f(outsideFile);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("initial\n");
        }
        QVERIFY(!hal::SysfsRoot::writeFileVerified(outsideFile, QStringLiteral("hacked"), nullptr, &errMsg));
        QVERIFY(errMsg.contains(QStringLiteral("Refused write outside active mock sysfs root")));

        root.setMode(hal::HardwareMode::PhysicalSystem);
        QVERIFY(!hal::SysfsRoot::writeFileVerified(outsideFile, QStringLiteral("hacked"), nullptr, &errMsg));
        QVERIFY(errMsg.contains(QStringLiteral("Refused write to non-allowlisted physical path")));
    }

    void testClv0001PresetTableAndFirmwareEncoding() {
        const auto& presets = hal::Clv0001KeyboardBackend::verifiedPresets();
        QCOMPARE(presets.size(), static_cast<size_t>(17));

        // Verify GRB firmware byte ordering ((channel * scale) / 0xBF, level 4 => scale 0xBF, terminator 0xF0)
        std::array<uint8_t, 4> pkt{};
        QString err;
        QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("red"), 4, pkt, &err));
        QCOMPARE(hal::Clv0001KeyboardBackend::formatPayloadHex(pkt), QStringLiteral("00 FF 00 F0"));

        QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("green"), 4, pkt, &err));
        QCOMPARE(hal::Clv0001KeyboardBackend::formatPayloadHex(pkt), QStringLiteral("FF 00 00 F0"));

        QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("blue"), 4, pkt, &err));
        QCOMPARE(hal::Clv0001KeyboardBackend::formatPayloadHex(pkt), QStringLiteral("00 00 C8 F0"));

        QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("cyan"), 4, pkt, &err));
        QCOMPARE(hal::Clv0001KeyboardBackend::formatPayloadHex(pkt), QStringLiteral("FF 00 C8 F0"));

        QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("off"), 4, pkt, &err));
        QCOMPARE(hal::Clv0001KeyboardBackend::formatPayloadHex(pkt), QStringLiteral("00 00 00 F0"));

        // Verify all 5 discrete brightness levels on white (scale 0x00, 0x2F, 0x5F, 0x8F, 0xBF normalized over 0xBF)
        const std::array<QString, 5> expectedWhiteHex = {
            QStringLiteral("00 00 00 F0"),
            QStringLiteral("3E 3E 3E F0"),
            QStringLiteral("7E 7E 7E F0"),
            QStringLiteral("BE BE BE F0"),
            QStringLiteral("FF FF FF F0")
        };
        for (int lvl = 0; lvl <= 4; ++lvl) {
            QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("white"), lvl, pkt, &err));
            QCOMPARE(hal::Clv0001KeyboardBackend::formatPayloadHex(pkt), expectedWhiteHex[static_cast<size_t>(lvl)]);
        }

        // Reject invalid preset names, shell metacharacters, or out-of-range brightness
        QVERIFY(!hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("rgb"), 4, pkt, &err));
        QVERIFY(!hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("rainbow"), 4, pkt, &err));
        QVERIFY(!hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("../etc/passwd"), 4, pkt, &err));
        QVERIFY(!hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("cyan"), -1, pkt, &err));
        QVERIFY(!hal::Clv0001KeyboardBackend::buildFirmwarePayload(QStringLiteral("cyan"), 5, pkt, &err));

        // Reproduce fork alg-rgb-engine vs alg_rgb.c ABI mismatch (`"rgb 255 0 0 4"` rejected with -EINVAL)
        QString parsedColor, parseErr;
        int parsedBri = -1;
        QVERIFY(!hal::Clv0001KeyboardBackend::simulateLegacyAlgWriteParser(
            QStringLiteral("rgb 255 0 0 4\n"), &parsedColor, &parsedBri, &parseErr));
        QCOMPARE(parsedColor, QStringLiteral("rgb"));
        QVERIFY(parseErr.contains(QStringLiteral("-EINVAL")));

        QVERIFY(hal::Clv0001KeyboardBackend::simulateLegacyAlgWriteParser(
            QStringLiteral("cyan 3\n"), &parsedColor, &parsedBri, &parseErr));
        QCOMPARE(parsedColor, QStringLiteral("cyan"));
        QCOMPARE(parsedBri, 3);
    }

    void testClv0001HardwareDiscoveryAndControlPaths() {
        auto& root = hal::SysfsRoot::instance();
        hal::Clv0001KeyboardBackend::setMinCommandIntervalMsForTesting(0);
        hal::Clv0001KeyboardBackend::resetRateLimiterForTesting();

        // 1. MockPhysicalMirror: CLV0001 ACPI node detected, driver unbound -> preview only
        root.setMode(hal::HardwareMode::MockPhysicalMirror);
        const core::DmiIdentity dmi = hal::HardwareDiscovery::probeDmiAndWmi();
        QVERIFY(!dmi.kernelRelease.isEmpty());
        QVERIFY(dmi.acpiDevices.join(QLatin1Char(',')).contains(QStringLiteral("CLV0001:00")));

        const core::KeyboardHardwareState kbdUnbound = hal::HardwareDiscovery::probeKeyboardHardware(dmi);
        QCOMPARE(kbdUnbound.clv0001DevicePresent, true);
        QCOMPARE(kbdUnbound.clv0001AcpiPath, QStringLiteral("\\_SB_.DCHU"));
        QCOMPARE(kbdUnbound.clv0001DriverBound, false);
        QCOMPARE(kbdUnbound.backendKind, core::KeyboardBackendKind::Clv0001AcpiUnbound);
        QCOMPARE(kbdUnbound.supportsPresetColors, false);

        const core::ControlResult unboundRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:cyan"), 100, QStringLiteral("cyan"));
        QVERIFY(!unboundRes.applied);
        QVERIFY(!unboundRes.physicalStateVerified);
        QCOMPARE(unboundRes.errorCode, QStringLiteral("ERR_CLV0001_DRIVER_NOT_BOUND"));
        QCOMPARE(unboundRes.verificationStatusId, QStringLiteral("rejected_before_execution"));

        // 2. MockClv0001DriverActive: Stage 18 driver bound, 17 presets + static custom RGB accepted WITHOUT fake physical readback
        root.setMode(hal::HardwareMode::MockClv0001DriverActive);
        const core::DmiIdentity clvDmi = hal::HardwareDiscovery::probeDmiAndWmi();
        QVERIFY(clvDmi.loadedDrivers.contains(QStringLiteral("acer_clv0001_kbd")));

        const core::KeyboardHardwareState kbdBound = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(kbdBound.clv0001DevicePresent, true);
        QCOMPARE(kbdBound.clv0001DriverBound, true);
        QCOMPARE(kbdBound.backendKind, core::KeyboardBackendKind::Clv0001AcpiDsmPreset);
        QCOMPARE(kbdBound.supportsPresetColors, true);
        QCOMPARE(kbdBound.supportsArbitraryRgb, true);
        QCOMPARE(kbdBound.independentHwReadback, false);

        // Test Stage 18 static custom RGB vectors (0,0,0), (255,255,255), (18,167,227), (18,52,86) across all levels 0..4
        const core::ControlResult customHeroRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#12A7E3"));
        QVERIFY(customHeroRes.applied);
        QCOMPARE(customHeroRes.physicalStateVerified, false);
        QCOMPARE(customHeroRes.verificationStatusId, QStringLiteral("accepted_no_hw_readback"));
        QVERIFY(customHeroRes.readbackSummary.contains(QStringLiteral("\"preset\":\"custom_rgb\"")));
        QVERIFY(customHeroRes.readbackSummary.contains(QStringLiteral("\"payload_grb\":\"A7 12 E3 F0\"")));
        QVERIFY(customHeroRes.readbackSummary.contains(QStringLiteral("\"physical_readback\":false")));

        const core::ControlResult customZeroRes =
            hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 0, 0, 0, 0);
        QVERIFY(customZeroRes.applied);
        QVERIFY(customZeroRes.readbackSummary.contains(QStringLiteral("\"payload_grb\":\"00 00 00 F0\"")));

        const core::ControlResult customWhiteRes =
            hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 255, 255, 255, 4);
        QVERIFY(customWhiteRes.applied);
        QVERIFY(customWhiteRes.readbackSummary.contains(QStringLiteral("\"payload_grb\":\"FF FF FF F0\"")));

        const std::array<QString, 5> expected18_52_86 = {
            QStringLiteral("00 00 00 F0"),
            QStringLiteral("0C 04 15 F0"),
            QStringLiteral("19 08 2A F0"),
            QStringLiteral("26 0D 40 F0"),
            QStringLiteral("34 12 56 F0")
        };
        for (int lvl = 0; lvl <= 4; ++lvl) {
            const core::ControlResult lvlRes =
                hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 18, 52, 86, lvl);
            QVERIFY(lvlRes.applied);
            QVERIFY(lvlRes.readbackSummary.contains(QStringLiteral("\"payload_grb\":\"%1\"").arg(expected18_52_86[static_cast<size_t>(lvl)])));
        }

        // Invalid custom RGB / level / HEX / deferred animation mode rejected BEFORE touching sysfs
        QCOMPARE(hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, -1, 52, 86, 4).errorCode, QStringLiteral("ERR_INVALID_RGB_CHANNEL"));
        QCOMPARE(hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 256, 52, 86, 4).errorCode, QStringLiteral("ERR_INVALID_RGB_CHANNEL"));
        QCOMPARE(hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 18, 52, 86, -1).errorCode, QStringLiteral("ERR_INVALID_BRIGHTNESS"));
        QCOMPARE(hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 18, 52, 86, 5).errorCode, QStringLiteral("ERR_INVALID_BRIGHTNESS"));
        QCOMPARE(hal::ControlOperations::applyKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("123456")).errorCode, QStringLiteral("ERR_INVALID_COLOR_HEX"));
        QCOMPARE(hal::ControlOperations::applyKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#123")).errorCode, QStringLiteral("ERR_INVALID_COLOR_HEX"));
        QCOMPARE(hal::ControlOperations::applyKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#12345G")).errorCode, QStringLiteral("ERR_INVALID_COLOR_HEX"));
        QCOMPARE(hal::ControlOperations::applyKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#1234567")).errorCode, QStringLiteral("ERR_INVALID_COLOR_HEX"));
        QCOMPARE(hal::ControlOperations::applyKeyboardLighting(QStringLiteral("breathing"), 100, QStringLiteral("#12A7E3")).errorCode, QStringLiteral("ERR_UNSUPPORTED_KEYBOARD_MODE"));

        // Capability discovery: distinguish legacy preset-only module & malformed metadata from Stage 17/18 module
        const QString presetsPath =
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_presets"));
        const QString fullPresetsMetadata = hal::SysfsRoot::readFileTrimmed(presetsPath);
        QVERIFY(fullPresetsMetadata.contains(QStringLiteral("supports_custom_rgb=1")));
        {
            QFile pf(presetsPath);
            QVERIFY(pf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            pf.write("presets=off,red,cyan,white\nbrightness_levels=0,1,2,3,4\n");
        }
        const core::KeyboardHardwareState presetOnlyKbd = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(presetOnlyKbd.supportsPresetColors, true);
        QCOMPARE(presetOnlyKbd.supportsArbitraryRgb, false);
        const core::ControlResult oldModuleReject =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("static"), 100, QStringLiteral("#123456"));
        QVERIFY(!oldModuleReject.applied);
        QCOMPARE(oldModuleReject.errorCode, QStringLiteral("ERR_CLV0001_PRESET_REQUIRED"));
        QVERIFY(hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:cyan"), 100, QStringLiteral("cyan")).applied);

        // Malformed metadata also defaults supportsArbitraryRgb cleanly to false
        {
            QFile pf(presetsPath);
            QVERIFY(pf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            pf.write("corrupted_metadata\n");
        }
        QCOMPARE(hal::HardwareDiscovery::probeKeyboardHardware(clvDmi).supportsArbitraryRgb, false);
        {
            QFile pf(presetsPath);
            QVERIFY(pf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            pf.write(fullPresetsMetadata.toUtf8() + "\n");
        }

        // Permission failure on clv0001_kbd_command propagates cleanly without claiming success
        const QString cmdPathPerm =
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command"));
        QFile::setPermissions(cmdPathPerm, QFileDevice::ReadOwner);
        const core::ControlResult permFailRes =
            hal::Clv0001KeyboardBackend::applyCustomRgbCommand(clvDmi, 18, 167, 227, 4);
        QFile::setPermissions(cmdPathPerm, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QVERIFY(!permFailRes.applied);
        QCOMPARE(permFailRes.errorCode, QStringLiteral("ERR_CLV0001_WRITE_FAILED"));

        // Apply valid preset + brightness level (Stage 15 preset regression)
        const core::ControlResult applyRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:red"), 100, QStringLiteral("red"));
        QVERIFY(applyRes.applied);
        QCOMPARE(applyRes.physicalStateVerified, false); // Write-only _DSM 0x67 cannot independently verify physical LEDs
        QCOMPARE(applyRes.verificationStatusId, QStringLiteral("accepted_no_hw_readback"));
        QVERIFY(applyRes.readbackSummary.contains(QStringLiteral("\"payload_grb\":\"00 FF 00 F0\"")));
        QVERIFY(applyRes.readbackSummary.contains(QStringLiteral("\"physical_readback\":false")));

        // Inject ACPI _DSM eval failure (`eval_fail`) and unexpected return type (`unexpected_return`)
        const QString behaviorPath =
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/mock_eval_behavior"));
        {
            QFile f(behaviorPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("eval_fail\n");
        }
        const core::ControlResult evalFailRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:blue"), 75, QStringLiteral("blue"));
        QVERIFY(!evalFailRes.applied);
        QCOMPARE(evalFailRes.errorCode, QStringLiteral("ERR_ACPI_DSM_EVAL_FAILED"));
        QCOMPARE(evalFailRes.verificationStatusId, QStringLiteral("acpi_eval_failed"));

        {
            QFile f(behaviorPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("unexpected_return\n");
        }
        const core::ControlResult unexpRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:green"), 50, QStringLiteral("green"));
        QVERIFY(!unexpRes.applied);
        QCOMPARE(unexpRes.errorCode, QStringLiteral("ERR_ACPI_UNEXPECTED_RETURN_TYPE"));
        QCOMPARE(unexpRes.verificationStatusId, QStringLiteral("unexpected_acpi_return_type"));

        // Inject kernel rate_limited status (-EBUSY) and verify ERR_RATE_LIMITED propagation
        {
            QFile f(behaviorPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("rate_limited\n");
        }
        const core::ControlResult kRateRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:cyan"), 75, QStringLiteral("cyan"));
        QVERIFY(!kRateRes.applied);
        QCOMPARE(kRateRes.errorCode, QStringLiteral("ERR_RATE_LIMITED"));
        QCOMPARE(kRateRes.verificationStatusId, QStringLiteral("rejected_before_execution"));

        // Inject state mismatch (`state_mismatch`) and verify fail-closed ERR_CLV0001_STATE_MISMATCH
        {
            QFile f(behaviorPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("state_mismatch\n");
        }
        const core::ControlResult mismatchRes =
            hal::ControlOperations::applyKeyboardLighting(QStringLiteral("preset:cyan"), 100, QStringLiteral("cyan"));
        QVERIFY(!mismatchRes.applied);
        QCOMPARE(mismatchRes.errorCode, QStringLiteral("ERR_CLV0001_STATE_MISMATCH"));
        QCOMPARE(mismatchRes.verificationStatusId, QStringLiteral("rejected_before_execution"));
        QFile::remove(behaviorPath);

        // Concurrent calls across threads must remain serialized and deterministic under clvBackendMutex
        core::ControlResult t1Res, t2Res;
        std::unique_ptr<QThread> th1(QThread::create([&]() {
            t1Res = hal::Clv0001KeyboardBackend::applyPresetCommand(clvDmi, QStringLiteral("cyan"), 4);
        }));
        std::unique_ptr<QThread> th2(QThread::create([&]() {
            t2Res = hal::Clv0001KeyboardBackend::applyPresetCommand(clvDmi, QStringLiteral("white"), 3);
        }));
        th1->start();
        th2->start();
        QVERIFY(th1->wait(3000));
        QVERIFY(th2->wait(3000));
        QVERIFY(t1Res.applied && t2Res.applied);

        // Reject state mismatch when clv0001_kbd_state is corrupted/truncated to wrong preset
        const QString cmdPath =
            root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_command"));
        const QString statePath =
            root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state"));
        QString writeErr;
        QVERIFY(!hal::SysfsRoot::writeCommandAndReadState(
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_state")),
            statePath,
            QStringLiteral("cyan 4"),
            nullptr,
            &writeErr));
        QVERIFY(writeErr.contains(QStringLiteral("outside CLV0001 allowlist")));

        // Verify mock-mode symlink escape out of canonical CLV0001 device dir is blocked
        const QString fakeCmdPath =
            root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/CLV0002:00/clv0001_kbd_command"));
        {
            QFile f(fakeCmdPath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("\n");
        }
        QVERIFY(!hal::SysfsRoot::writeCommandAndReadState(fakeCmdPath, statePath, QStringLiteral("cyan 4"), nullptr, &writeErr));
        QVERIFY(writeErr.contains(QStringLiteral("outside CLV0001 allowlist")));
        Q_UNUSED(cmdPath);

        // Rate-limit enforcement on rapid consecutive _DSM calls
        hal::Clv0001KeyboardBackend::resetRateLimiterForTesting();
        hal::Clv0001KeyboardBackend::setMinCommandIntervalMsForTesting(500);
        const core::ControlResult firstOk =
            hal::Clv0001KeyboardBackend::applyPresetCommand(clvDmi, QStringLiteral("cyan"), 3);
        QVERIFY(firstOk.applied);
        const core::ControlResult secondLimited =
            hal::Clv0001KeyboardBackend::applyPresetCommand(clvDmi, QStringLiteral("violet"), 3);
        QVERIFY(!secondLimited.applied);
        QCOMPARE(secondLimited.errorCode, QStringLiteral("ERR_RATE_LIMITED"));
        hal::Clv0001KeyboardBackend::setMinCommandIntervalMsForTesting(0);

        // Strict DMI gate checks: reject empty board fields, unverified ALG model, and wrong laptop
        core::DmiIdentity wrongDmi = clvDmi;
        wrongDmi.productName = QStringLiteral("Nitro AN515-58");
        wrongDmi.boardName = QStringLiteral("OTHER");
        wrongDmi.boardVendor = QStringLiteral("OtherVendor");
        QVERIFY(!hal::Clv0001KeyboardBackend::isDmiEligible(wrongDmi));

        core::DmiIdentity missingBoardDmi = clvDmi;
        missingBoardDmi.boardName.clear();
        missingBoardDmi.boardVendor.clear();
        QVERIFY(!hal::Clv0001KeyboardBackend::isDmiEligible(missingBoardDmi));

        core::DmiIdentity unverifiedAlgDmi = clvDmi;
        unverifiedAlgDmi.productName = QStringLiteral("ALG AL15G-53");
        unverifiedAlgDmi.boardName = QStringLiteral("AL15G");
        QVERIFY(!hal::Clv0001KeyboardBackend::isDmiEligible(unverifiedAlgDmi));

        const core::ControlResult dmiFail =
            hal::Clv0001KeyboardBackend::applyPresetCommand(wrongDmi, QStringLiteral("cyan"), 4);
        QVERIFY(!dmiFail.applied);
        QCOMPARE(dmiFail.errorCode, QStringLiteral("ERR_UNSUPPORTED_MODEL"));

        // Verify disabled ACPI _STA ("0") and wrong HID fail topology detection closed
        const QString clvStatusPath =
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/status"));
        {
            QFile f(clvStatusPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("0\n");
        }
        QCOMPARE(hal::Clv0001KeyboardBackend::probeTopology(clvDmi).devicePresent, false);
        {
            QFile f(clvStatusPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("15\n");
        }
        const QString clvHidPath =
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/hid"));
        {
            QFile f(clvHidPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("CLV0002\n");
        }
        QCOMPARE(hal::Clv0001KeyboardBackend::probeTopology(clvDmi).devicePresent, false);
        {
            QFile f(clvHidPath);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            f.write("CLV0001\n");
        }
        QCOMPARE(hal::Clv0001KeyboardBackend::probeTopology(clvDmi).devicePresent, true);

        // Daemon D-Bus object dispatch with Polkit authorization check (Deny + Allow)
        daemon::DaemonServiceObject daemonObj;
        bool dbusApplied = false;
        QString dbusRb, dbusErr;

        // First set baseline state via allowed call so we verify denied call leaves state unchanged
        daemonObj.setAuthMode(daemon::AuthPolicyMode::TestAllowAll);
        daemonObj.SetKeyboardLighting(QStringLiteral("preset:magenta"), 75, QStringLiteral("magenta"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("OK"));
        QVERIFY(dbusRb.contains(QStringLiteral("accepted_no_hw_readback")));
        const QString savedStateBeforeDeny = hal::SysfsRoot::readFileTrimmed(statePath);

        daemonObj.setAuthMode(daemon::AuthPolicyMode::TestDenyAll);
        daemonObj.SetKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#12A7E3"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(!dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));
        QVERIFY(dbusRb.contains(QStringLiteral("ERR_POLKIT_UNAUTHORIZED")));
        QVERIFY(dbusRb.contains(QStringLiteral("rejected_before_execution")));
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(statePath), savedStateBeforeDeny);

        daemonObj.setAuthMode(daemon::AuthPolicyMode::TestAllowAll);
        daemonObj.SetKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#12A7E3"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("OK"));
        QVERIFY(dbusRb.contains(QStringLiteral("\"preset\":\"custom_rgb\"")));
        QVERIFY(dbusRb.contains(QStringLiteral("\"payload_grb\":\"A7 12 E3 F0\"")));
        QVERIFY(dbusRb.contains(QStringLiteral("accepted_no_hw_readback")));

        // Invalid custom RGB/HEX/mode over D-Bus rejected before driver write
        daemonObj.SetKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("256,0,0"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(!dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("ERR_INVALID_RGB_CHANNEL"));

        daemonObj.SetKeyboardLighting(QStringLiteral("custom_rgb"), 100, QStringLiteral("#12345G"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(!dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("ERR_INVALID_COLOR_HEX"));

        daemonObj.SetKeyboardLighting(QStringLiteral("wave"), 100, QStringLiteral("#12A7E3"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(!dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("ERR_UNSUPPORTED_KEYBOARD_MODE"));

        // Preset-only mock module rejection via D-Bus
        {
            QFile pf(presetsPath);
            QVERIFY(pf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            pf.write("presets=off,cyan,white\nbrightness_levels=0,1,2,3,4\nsupports_presets=1\nsupports_custom_rgb=0\n");
        }
        daemonObj.SetKeyboardLighting(QStringLiteral("static"), 100, QStringLiteral("#123456"), dbusApplied, dbusRb, dbusErr);
        QVERIFY(!dbusApplied);
        QCOMPARE(dbusErr, QStringLiteral("ERR_CLV0001_PRESET_REQUIRED"));
        QVERIFY(dbusRb.contains(QStringLiteral("nearest")));

        // Verify default keyboard profiles map cleanly to CLV0001 presets
        const core::AppSettings defSettings = core::ConfigStore::defaultSettings();
        QCOMPARE(defSettings.keyboardProfiles.size(), static_cast<size_t>(3));
        QCOMPARE(defSettings.keyboardProfiles[0].clv0001PresetName, QStringLiteral("cyan"));
        QCOMPARE(defSettings.keyboardProfiles[1].clv0001PresetName, QStringLiteral("white"));
        QCOMPARE(defSettings.keyboardProfiles[2].clv0001PresetName, QStringLiteral("red"));

        root.setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testNvmlProcFallbackNoFabricationAndRedaction() {
        auto& root = hal::SysfsRoot::instance();

        // 1. /proc/driver/nvidia fallback must not fabricate 6144 MiB VRAM or 60W/75W limits
        QTemporaryDir procFix;
        QVERIFY(procFix.isValid());
        const QString r = procFix.path();
        QDir().mkpath(r + QStringLiteral("/proc/driver/nvidia/gpus/0000:01:00.0"));
        QDir().mkpath(r + QStringLiteral("/sys/bus/pci/devices/0000:01:00.0"));
        {
            QFile f(r + QStringLiteral("/proc/driver/nvidia/gpus/0000:01:00.0/information"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("Model:\t\tTest Discrete GPU\n");
        }
        {
            QFile f(r + QStringLiteral("/proc/driver/nvidia/version"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("NVRM version: NVIDIA Open 620.70.06\n");
        }
        {
            QFile f(r + QStringLiteral("/sys/bus/pci/devices/0000:01:00.0/power_state"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("D3cold\n");
        }
        {
            QFile f(r + QStringLiteral("/sys/bus/pci/devices/0000:01:00.0/subsystem_vendor"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("0x1025\n");
        }
        {
            QFile f(r + QStringLiteral("/sys/bus/pci/devices/0000:01:00.0/subsystem_device"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("0x1900\n");
        }

        root.setCustomRootPath(r, QStringLiteral("Proc Fallback Unit Fixture"));
        const core::GpuTelemetry fallbackGpu = hal::NvmlDynamicLoader::readProcNvidiaFallback();
        QCOMPARE(fallbackGpu.available, true);
        QCOMPARE(fallbackGpu.telemetryMeasuredByNvml, false);
        QCOMPARE(fallbackGpu.powerLimitsMeasured, false);
        QCOMPARE(fallbackGpu.name, QStringLiteral("Test Discrete GPU"));
        QCOMPARE(fallbackGpu.vramTotalMiB, static_cast<uint64_t>(0));
        QCOMPARE(fallbackGpu.powerDefaultLimitWatts, 0.0);
        QCOMPARE(fallbackGpu.powerMaxLimitWatts, 0.0);
        QCOMPARE(fallbackGpu.pstate, QStringLiteral("PCI D3cold"));
        QCOMPARE(fallbackGpu.subsystemId, QStringLiteral("1025:1900"));

        // 2. P8 filterPowerReading edge cases: NaN, +Inf, negative limit, legitimate 21W in P8 (25W cap), spike
        bool railGated = false;
        QCOMPARE(hal::NvmlDynamicLoader::filterPowerReading(std::numeric_limits<double>::quiet_NaN(), 75.0, 8, railGated), 0.0);
        QCOMPARE(railGated, false);
        QCOMPARE(hal::NvmlDynamicLoader::filterPowerReading(std::numeric_limits<double>::infinity(), 75.0, 8, railGated), 0.0);
        QCOMPARE(railGated, false);
        QCOMPARE(hal::NvmlDynamicLoader::filterPowerReading(5.5, 0.0, 8, railGated), 5.5);
        QCOMPARE(railGated, false);
        QCOMPARE(hal::NvmlDynamicLoader::filterPowerReading(21.0, 25.0, 8, railGated), 21.0);
        QCOMPARE(railGated, false);
        QCOMPARE(hal::NvmlDynamicLoader::filterPowerReading(312.23, 75.0, 8, railGated), 0.0);
        QCOMPARE(railGated, true);

        // 3. ConfigStore sanitizes malformed/unknown clv0001_preset & mode values
        QJsonObject badCfgObj = core::ConfigStore::serializeSettings(core::ConfigStore::defaultSettings());
        QJsonArray kps = badCfgObj.value(QStringLiteral("keyboard_profiles")).toArray();
        QVERIFY(!kps.isEmpty());
        QJsonObject kp0 = kps.at(0).toObject();
        kp0.insert(QStringLiteral("mode"), QStringLiteral("strobe_hack"));
        kp0.insert(QStringLiteral("clv0001_preset"), QStringLiteral("../evil"));
        kp0.insert(QStringLiteral("clv0001_brightness_level"), 99);
        kps[0] = kp0;
        badCfgObj.insert(QStringLiteral("keyboard_profiles"), kps);
        core::AppSettings parsedCfg;
        QString cfgWarn;
        QVERIFY(core::ConfigStore::deserializeAndValidateSettings(badCfgObj, parsedCfg, &cfgWarn));
        QCOMPARE(parsedCfg.keyboardProfiles.front().mode, QStringLiteral("static"));
        QCOMPARE(parsedCfg.keyboardProfiles.front().clv0001PresetName, QStringLiteral("cyan"));
        QCOMPARE(parsedCfg.keyboardProfiles.front().clv0001BrightnessLevel, 4);

        // 3. Redaction keeps CLV0001 public DSM UUID while scrubbing NVIDIA GPU UUIDs
        const QString mixed = QStringLiteral("CLV=%1 GPU=GPU-11223344-5566-7788-99aa-bbccddeeff00")
                                  .arg(QStringLiteral("93f224e4-fbdc-4bbf-add6-db71bdc0afad"));
        const QString redacted = core::DiagnosticRedactor::redactSensitiveText(mixed);
        QVERIFY(redacted.contains(QStringLiteral("93f224e4-fbdc-4bbf-add6-db71bdc0afad")));
        QVERIFY(!redacted.contains(QStringLiteral("11223344")));
        QVERIFY(redacted.contains(QStringLiteral("<REDACTED-GPU-UUID>")));

        root.setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testCpuGovernorEppValidationAndRollback() {
        auto& root = hal::SysfsRoot::instance();
        root.setMode(hal::HardwareMode::MockPhysicalMirror);

        // Reject unsupported governor even if allowlisted string vs sysfs mismatch
        const QString availGovPath =
            root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"));
        QVERIFY(hal::SysfsRoot::writeFileVerified(availGovPath, QStringLiteral("powersave")));
        const core::ControlResult badGov =
            hal::ControlOperations::applyCpuPerformanceTuning(QStringLiteral("performance"), QStringLiteral("performance"), true, 20, 90);
        QVERIFY(!badGov.applied);
        QCOMPARE(badGov.errorCode, QStringLiteral("ERR_UNSUPPORTED_GOVERNOR"));

        QVERIFY(hal::SysfsRoot::writeFileVerified(availGovPath, QStringLiteral("performance powersave")));
        const QString availEppPath =
            root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_available_preferences"));
        QVERIFY(hal::SysfsRoot::writeFileVerified(availEppPath, QStringLiteral("balance_performance power")));
        const core::ControlResult badEpp =
            hal::ControlOperations::applyCpuPerformanceTuning(QStringLiteral("powersave"), QStringLiteral("balance_power"), true, 20, 90);
        QVERIFY(!badEpp.applied);
        QCOMPARE(badEpp.errorCode, QStringLiteral("ERR_UNSUPPORTED_EPP"));

        // Verify rollback restores min_perf_pct/max_perf_pct if cpu1 EPP becomes non-writable
        const QString maxPerfPath = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/intel_pstate/max_perf_pct"));
        QVERIFY(hal::SysfsRoot::writeFileVerified(availEppPath, QStringLiteral("default performance balance_performance balance_power power")));
        const QString cpu1Epp = root.resolvePath(QStringLiteral("/sys/devices/system/cpu/cpu1/cpufreq/energy_performance_preference"));
        QFile::setPermissions(cpu1Epp, QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        const core::ControlResult rollbackRes =
            hal::ControlOperations::applyCpuPerformanceTuning(QStringLiteral("powersave"), QStringLiteral("power"), false, 25, 70);
        QFile::setPermissions(cpu1Epp, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup);
        QVERIFY(!rollbackRes.applied);
        QCOMPARE(rollbackRes.errorCode, QStringLiteral("ERR_WRITE_EPP"));
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(maxPerfPath), QStringLiteral("100"));

        root.setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testStage17KernelDriverStaticCustomRgbAndPresetRegression() {
        auto formatKmodPayload = [](const clv0001_kmod_unit::u8 p[4]) -> QString {
            return QStringLiteral("%1 %2 %3 %4")
                .arg(p[0], 2, 16, QLatin1Char('0'))
                .arg(p[1], 2, 16, QLatin1Char('0'))
                .arg(p[2], 2, 16, QLatin1Char('0'))
                .arg(p[3], 2, 16, QLatin1Char('0'))
                .toUpper();
        };

        // 1. Existing preset regression vectors (byte-for-byte identical)
        struct PresetVector {
            const char* cmd;
            const char* expectedLabel;
            int expectedLevel;
            const char* expectedHex;
        };
        const PresetVector presetVectors[] = {
            { "cyan 2\n",         "cyan",         2, "7E 00 63 F0" },
            { "white 4\n",        "white",        4, "FF FF FF F0" },
            { "off 0\n",          "off",          0, "00 00 00 F0" },
            { "bluish-white 3\n", "bluish-white", 3, "5F 2F 5F F0" },
            { "cyan",             "cyan",         4, "FF 00 C8 F0" },
            { "off",              "off",          0, "00 00 00 F0" },
            { "off 4",            "off",          0, "00 00 00 F0" },
        };
        for (const auto& vec : presetVectors) {
            clv0001_kmod_unit::clv0001_parsed_cmd cmd{};
            clv0001_kmod_unit::u8 payload[4] = {};
            const int rc = clv0001_kmod_unit::clv0001_parse_and_build_payload(
                vec.cmd, strlen(vec.cmd), &cmd, payload
            );
            QCOMPARE(rc, 0);
            QCOMPARE(cmd.is_custom_rgb, false);
            QVERIFY(cmd.preset != nullptr);
            QCOMPARE(QString::fromLatin1(clv0001_kmod_unit::clv0001_parsed_cmd_preset_label(&cmd)),
                     QString::fromLatin1(vec.expectedLabel));
            QCOMPARE(cmd.brightness, vec.expectedLevel);
            QCOMPARE(formatKmodPayload(payload), QString::fromLatin1(vec.expectedHex));
        }

        // Cross-verify all 17 presets x levels 0..4 match HAL payload encoder identically
        for (const auto& preset : hal::Clv0001KeyboardBackend::verifiedPresets()) {
            for (int lvl = 0; lvl <= 4; ++lvl) {
                const QByteArray cmdBytes = QStringLiteral("%1 %2\n").arg(preset.name).arg(lvl).toLatin1();
                clv0001_kmod_unit::clv0001_parsed_cmd cmd{};
                clv0001_kmod_unit::u8 kmodPkt[4] = {};
                QCOMPARE(clv0001_kmod_unit::clv0001_parse_and_build_payload(
                    cmdBytes.constData(), static_cast<size_t>(cmdBytes.size()), &cmd, kmodPkt), 0);

                std::array<uint8_t, 4> halPkt{};
                QVERIFY(hal::Clv0001KeyboardBackend::buildFirmwarePayload(preset.name, lvl, halPkt));
                QCOMPARE(formatKmodPayload(kmodPkt), hal::Clv0001KeyboardBackend::formatPayloadHex(halPkt));
            }
        }

        // 2. Custom static RGB vectors (Stage 17 test table + Stage 17 physical test candidate)
        struct RgbVector {
            const char* cmd;
            int expectedLevel;
            const char* expectedHex;
        };
        const RgbVector rgbVectors[] = {
            { "rgb 18 52 86 4",       4, "34 12 56 F0" },
            { "rgb 18 52 86 2\n",     2, "19 08 2A F0" },
            { "rgb 0 255 200 2",      2, "7E 00 63 F0" },
            { "rgb 255 255 255 4",    4, "FF FF FF F0" },
            { "rgb 255 0 128 0",      0, "00 00 00 F0" },
            { "rgb 18 167 227 4\n",   4, "A7 12 E3 F0" },
            { "  rgb  18\t52\t86 4 \r\n", 4, "34 12 56 F0" },
        };
        for (const auto& vec : rgbVectors) {
            clv0001_kmod_unit::clv0001_parsed_cmd cmd{};
            clv0001_kmod_unit::u8 payload[4] = {};
            const int rc = clv0001_kmod_unit::clv0001_parse_and_build_payload(
                vec.cmd, strlen(vec.cmd), &cmd, payload
            );
            QCOMPARE(rc, 0);
            QCOMPARE(cmd.is_custom_rgb, true);
            QVERIFY(cmd.preset == nullptr);
            QCOMPARE(QString::fromLatin1(clv0001_kmod_unit::clv0001_parsed_cmd_preset_label(&cmd)),
                     QStringLiteral("custom_rgb"));
            QCOMPARE(cmd.brightness, vec.expectedLevel);
            QCOMPARE(formatKmodPayload(payload), QString::fromLatin1(vec.expectedHex));
        }

        // Uninitialized / NULL label safeguard
        clv0001_kmod_unit::clv0001_parsed_cmd emptyCmd{};
        QCOMPARE(QString::fromLatin1(clv0001_kmod_unit::clv0001_parsed_cmd_preset_label(nullptr)),
                 QStringLiteral("none"));
        QCOMPARE(QString::fromLatin1(clv0001_kmod_unit::clv0001_parsed_cmd_preset_label(&emptyCmd)),
                 QStringLiteral("none"));

        // 3. Invalid input validation: all malformed commands must return -EINVAL before ACPI invocation
        const char* invalidCommands[] = {
            "rgb",
            "rgb 18",
            "rgb 18 52",
            "rgb 18 52 86",
            "rgb 18 52 86 4 extra",
            "rgb -1 52 86 4",
            "rgb 256 52 86 4",
            "rgb 18 -1 86 4",
            "rgb 18 52 256 4",
            "rgb 18 52 86 -1",
            "rgb 18 52 86 5",
            "rgb abc 52 86 4",
            "rgb 18 def 86 4",
            "rgb 18 52 ghi 4",
            "rgb 18 52 86 xyz",
            "rgb 99999999999999999999 52 86 4",
            "rgb 18 52 86 4294967296",
            "rgb +18 52 86 4",
            "rgb -0 52 86 4",
            "rgb 0x12 52 86 4",
            "RGB 18 52 86 4",
            "rgb 18,52,86,4",
            "rgb18 52 86 4",
            "",
            "   \n",
            "\nrgb 18 52 86 4",
            "rgb 18 52\n86 4",
            "rgb 18 52 86\r4",
            "unknown 2",
            "cyan -1",
            "cyan 5",
            "cyan abc",
            "cyan 2 extra",
            "off 5",
        };
        for (const char* badCmd : invalidCommands) {
            clv0001_kmod_unit::clv0001_parsed_cmd cmd{};
            clv0001_kmod_unit::u8 payload[4] = {};
            const int rc = clv0001_kmod_unit::clv0001_parse_and_build_payload(
                badCmd, strlen(badCmd), &cmd, payload
            );
            QVERIFY2(rc == -EINVAL, qPrintable(QStringLiteral("Expected -EINVAL for '%1', got %2")
                                                   .arg(QString::fromLatin1(badCmd)).arg(rc)));
        }

        // Embedded NUL byte and oversized input (>= 64 bytes) must return -EINVAL
        const QByteArray nulEmbedded("rgb 18 52 86 4\0extra", 20);
        {
            clv0001_kmod_unit::clv0001_parsed_cmd cmd{};
            clv0001_kmod_unit::u8 payload[4] = {};
            QCOMPARE(clv0001_kmod_unit::clv0001_parse_and_build_payload(
                nulEmbedded.constData(), static_cast<size_t>(nulEmbedded.size()), &cmd, payload), -EINVAL);

            const QByteArray tooLong(64, '1');
            QCOMPARE(clv0001_kmod_unit::clv0001_parse_and_build_payload(
                tooLong.constData(), static_cast<size_t>(tooLong.size()), &cmd, payload), -EINVAL);

            QCOMPARE(clv0001_kmod_unit::clv0001_parse_and_build_payload(nullptr, 5, &cmd, payload), -EINVAL);
            QCOMPARE(clv0001_kmod_unit::clv0001_build_payload(18, 52, 86, 5, payload), -EINVAL);
            QCOMPARE(clv0001_kmod_unit::clv0001_build_payload(18, 52, 86, -1, payload), -EINVAL);
        }
    }

    void testFanCapabilitySafetyTransitions() {
        auto writeText = [](const QString& path, const QByteArray& content) {
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile f(path);
            QVERIFY2(f.open(QIODevice::WriteOnly | QIODevice::Truncate), qPrintable(path));
            f.write(content);
            f.close();
        };

        auto findFanCap = [](const core::SystemSnapshot& snap) -> core::CapabilityRecord {
            for (const auto& c : snap.capabilities) {
                if (c.id == QLatin1String("cap.thermal.fans")) {
                    return c;
                }
            }
            return core::CapabilityRecord{};
        };

        // Case 1: Only fan1_input (tachometer), missing pwm* -> ReadOnly & non-writable
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/name")), "acer_ec_fans\n");
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/fan1_input")), "2200\n");
            hal::SysfsRoot::instance().setCustomRootPath(tmp.path(), QStringLiteral("TachOnly"));
            hal::TelemetryCollector col;
            const core::SystemSnapshot snap = col.collectSnapshot(QStringLiteral("balanced"));
            QVERIFY(!snap.thermal.manualFanSupported);
            QCOMPARE(findFanCap(snap).state, core::CapabilityState::ReadOnly);
            QVERIFY(!findFanCap(snap).isWritable);
            const core::ControlResult r = hal::ControlOperations::applyFanControlMode(QStringLiteral("manual"), 70);
            QVERIFY(!r.applied);
            QCOMPARE(r.errorCode, QStringLiteral("ERR_FAN_PWM_NOT_DETECTED"));
        }

        // Case 2: pwm1 alone without pwm1_enable -> RequiresVerification (UNVERIFIED) & non-writable
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/name")), "acer_ec_fans\n");
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/pwm1")), "128\n");
            hal::SysfsRoot::instance().setCustomRootPath(tmp.path(), QStringLiteral("BarePwmOnly"));
            hal::TelemetryCollector col;
            const core::SystemSnapshot snap = col.collectSnapshot(QStringLiteral("balanced"));
            QVERIFY(!snap.thermal.manualFanSupported);
            QCOMPARE(findFanCap(snap).state, core::CapabilityState::RequiresVerification);
            QCOMPARE(core::capabilityStateToBadge(findFanCap(snap).state), QStringLiteral("UNVERIFIED"));
            QVERIFY(!findFanCap(snap).isWritable);
            const core::ControlResult r = hal::ControlOperations::applyFanControlMode(QStringLiteral("manual"), 70);
            QVERIFY(!r.applied);
            QCOMPARE(r.errorCode, QStringLiteral("ERR_FAN_PWM_NOT_DETECTED"));
        }

        // Case 3: pwm1 + pwm1_enable on unverified driver name -> RequiresVerification & blocked
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/name")), "unverified_fan_drv\n");
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/pwm1")), "128\n");
            writeText(tmp.filePath(QStringLiteral("sys/class/hwmon/hwmon7/pwm1_enable")), "2\n");
            hal::SysfsRoot::instance().setCustomRootPath(tmp.path(), QStringLiteral("UnverifiedDriverPwm"));
            hal::TelemetryCollector col;
            const core::SystemSnapshot snap = col.collectSnapshot(QStringLiteral("balanced"));
            QVERIFY(!snap.thermal.manualFanSupported);
            QCOMPARE(findFanCap(snap).state, core::CapabilityState::RequiresVerification);
            QVERIFY(!findFanCap(snap).isWritable);
            const core::ControlResult r = hal::ControlOperations::applyFanControlMode(QStringLiteral("manual"), 70);
            QVERIFY(!r.applied);
            QCOMPARE(r.errorCode, QStringLiteral("ERR_FAN_PWM_NOT_DETECTED"));
        }

        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testTelemetryCollectorExternalHintsAndReset() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        hal::SysfsRoot::instance().setCustomRootPath(tmp.path(), QStringLiteral("ExternalHintsTestRoot"));

        hal::TelemetryCollector col;
        hal::TelemetryExternalHints hints;
        hints.useExternalDbusHints = true;
        hints.daemonReachable = true;
        hints.activeTunedProfile = QStringLiteral("balanced");
        hints.hasRaplSample = true;
        hints.raplAvailable = true;
        hints.raplPkgUj = 50'000'000ULL;
        hints.raplTimestampUs = 1'000'000LL;

        const core::SystemSnapshot s1 = col.collectSnapshot(QStringLiteral("balanced"), &hints);
        QVERIFY(s1.valid);
        QVERIFY(s1.daemonReachable);
        QCOMPARE(s1.activeTunedProfile, QStringLiteral("balanced"));

        hints.activeTunedProfile = QStringLiteral("performance");
        hints.raplPkgUj = 65'000'000ULL;
        hints.raplTimestampUs = 2'000'000LL;
        const core::SystemSnapshot s2 = col.collectSnapshot(QStringLiteral("performance"), &hints);
        QVERIFY(s2.valid);
        QVERIFY(s2.cpu.powerSourceAvailable);
        QCOMPARE(s2.activeOperatingProfileId, QStringLiteral("performance"));
        QCOMPARE(s2.activeTunedProfile, QStringLiteral("performance"));

        col.resetDeltas();
        hints.daemonReachable = false;
        hints.activeTunedProfile.clear();
        hints.hasRaplSample = false;
        hints.raplAvailable = false;
        const core::SystemSnapshot s3 = col.collectSnapshot(QStringLiteral("balanced"), &hints);
        QVERIFY(s3.valid);
        // CustomRootPath is treated as a mock fixture root; activeTunedProfile falls back to mock label.
        QCOMPARE(s3.activeTunedProfile, QStringLiteral("balanced (Mock TuneD-PPD)"));

        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testClv0001DriverStateProvenanceAndFallback() {
        auto& root = hal::SysfsRoot::instance();

        // 1. Unbound CLV0001 (MockPhysicalMirror) must report unavailable/unknown, never white/cyan
        root.setMode(hal::HardwareMode::MockPhysicalMirror);
        const core::DmiIdentity mirrorDmi = hal::HardwareDiscovery::probeDmiAndWmi();
        const core::KeyboardHardwareState unboundState = hal::HardwareDiscovery::probeKeyboardHardware(mirrorDmi);
        QCOMPARE(unboundState.clv0001DriverBound, false);
        QCOMPARE(unboundState.driverLastStatus, QStringLiteral("unavailable"));
        QCOMPARE(unboundState.driverLastPreset, QStringLiteral("unknown"));
        QCOMPARE(unboundState.driverLastBrightnessLevel, -1);
        QCOMPARE(unboundState.driverLastPayloadHex, QStringLiteral("UNAVAILABLE"));
        QCOMPARE(unboundState.currentPresetName, QStringLiteral("unknown"));
        QCOMPARE(unboundState.currentBrightnessLevel, -1);
        QCOMPARE(unboundState.independentHwReadback, false);

        // 2. Bound but uninitialized CLV0001 driver preserves uninitialized/none/0 cleanly
        root.setMode(hal::HardwareMode::MockClv0001DriverActive);
        const core::DmiIdentity clvDmi = hal::HardwareDiscovery::probeDmiAndWmi();
        const core::KeyboardHardwareState initState = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(initState.clv0001DriverBound, true);
        QCOMPARE(initState.driverLastStatus, QStringLiteral("uninitialized"));
        QCOMPARE(initState.driverLastPreset, QStringLiteral("none"));
        QCOMPARE(initState.driverLastBrightnessLevel, 0);
        QCOMPARE(initState.driverLastPayloadHex, QStringLiteral("00 00 00 F0"));
        QCOMPARE(initState.currentPresetName, QStringLiteral("none"));
        QCOMPARE(initState.currentBrightnessLevel, 0);

        // 3. Accepted cyan 4 preserves exact driver-accepted provenance without claiming HW readback
        const QString statePath =
            root.resolvePath(QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_state"));
        {
            QFile sf(statePath);
            QVERIFY(sf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            sf.write("status=accepted_no_hw_readback\n"
                     "preset=cyan\n"
                     "brightness_level=4\n"
                     "payload_grb=FF 00 C8 F0\n"
                     "acpi_return_type=integer\n"
                     "physical_readback=unsupported\n");
        }
        const core::KeyboardHardwareState cyanState = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(cyanState.driverLastStatus, QStringLiteral("accepted_no_hw_readback"));
        QCOMPARE(cyanState.driverLastPreset, QStringLiteral("cyan"));
        QCOMPARE(cyanState.driverLastBrightnessLevel, 4);
        QCOMPARE(cyanState.driverLastPayloadHex, QStringLiteral("FF 00 C8 F0"));
        QCOMPARE(cyanState.driverLastAcpiReturnType, QStringLiteral("integer"));
        QCOMPARE(cyanState.currentPresetName, QStringLiteral("cyan"));
        QCOMPARE(cyanState.currentBrightnessLevel, 4);
        QCOMPARE(cyanState.independentHwReadback, false);

        // 4. custom_rgb state is preserved faithfully rather than rewritten to cyan
        {
            QFile sf(statePath);
            QVERIFY(sf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            sf.write("status=accepted_no_hw_readback\n"
                     "preset=custom_rgb\n"
                     "brightness_level=4\n"
                     "payload_grb=A7 12 E3 F0\n"
                     "acpi_return_type=integer\n"
                     "physical_readback=unsupported\n");
        }
        const core::KeyboardHardwareState rgbState = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(rgbState.driverLastPreset, QStringLiteral("custom_rgb"));
        QCOMPARE(rgbState.driverLastPayloadHex, QStringLiteral("A7 12 E3 F0"));

        // 5. Invalid/malformed/empty clv0001_kbd_state never leaks default cyan or white
        {
            QFile sf(statePath);
            QVERIFY(sf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            sf.write("status=accepted_no_hw_readback\n"
                     "preset=\n"
                     "brightness_level=99\n"
                     "payload_grb=BOGUS\n");
        }
        const core::KeyboardHardwareState badFieldsState = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(badFieldsState.driverLastStatus, QStringLiteral("accepted_no_hw_readback"));
        QCOMPARE(badFieldsState.driverLastPreset, QStringLiteral("unknown"));
        QCOMPARE(badFieldsState.driverLastBrightnessLevel, -1);
        QCOMPARE(badFieldsState.driverLastPayloadHex, QStringLiteral("INVALID"));

        {
            QFile sf(statePath);
            QVERIFY(sf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            sf.write("");
        }
        const core::KeyboardHardwareState emptyState = hal::HardwareDiscovery::probeKeyboardHardware(clvDmi);
        QCOMPARE(emptyState.driverLastStatus, QStringLiteral("unavailable"));
        QCOMPARE(emptyState.driverLastPreset, QStringLiteral("unknown"));
        QCOMPARE(emptyState.driverLastBrightnessLevel, -1);
        QCOMPARE(emptyState.driverLastPayloadHex, QStringLiteral("UNAVAILABLE"));

        root.setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testConfigStoreCustomSwatchesPersistenceAndValidation() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        core::ConfigStore store(tempDir.path());
        core::AppSettings settings = core::ConfigStore::defaultSettings();
        settings.customAccentSwatches = {
            QStringLiteral("#112233"),
            QStringLiteral("#aabbcc"),
            QStringLiteral("#AABBCC"),
            QStringLiteral("NOT_HEX"),
            QStringLiteral("#FFEA00")
        };
        settings.customKeyboardSwatches = {
            QStringLiteral("#00c8f0"),
            QStringLiteral("#123456"),
            QStringLiteral("#00C8F0")
        };

        QString err;
        QVERIFY(store.saveSettings(settings, &err));

        QString warn;
        const core::AppSettings loaded = store.loadSettings(&warn);
        QCOMPARE(loaded.customAccentSwatches, (QStringList{QStringLiteral("#112233"), QStringLiteral("#AABBCC"), QStringLiteral("#FFEA00")}));
        QCOMPARE(loaded.customKeyboardSwatches, (QStringList{QStringLiteral("#00C8F0"), QStringLiteral("#123456")}));

        // Bundle export/import must also preserve custom swatches
        const QString bundlePath = tempDir.filePath(QStringLiteral("swatch-bundle.json"));
        QVERIFY(store.exportProfileBundleToFile(loaded, bundlePath, &err));
        core::AppSettings imported;
        QVERIFY(store.importProfileBundleFromFile(bundlePath, imported, &err));
        QCOMPARE(imported.customAccentSwatches, loaded.customAccentSwatches);
        QCOMPARE(imported.customKeyboardSwatches, loaded.customKeyboardSwatches);

        // Invalid or legacy missing swatch arrays fall back safely to defaults without breaking schema
        QFile sf(store.settingsFilePath());
        QVERIFY(sf.open(QIODevice::WriteOnly | QIODevice::Truncate));
        sf.write("{\"schema_version\":1,\"accent_color_hex\":\"#2596FF\","
                 "\"custom_accent_swatches\":[\"BADHEX\",123],"
                 "\"custom_keyboard_swatches\":\"not-an-array\"}");
        sf.close();

        QString fallbackWarn;
        const core::AppSettings recovered = store.loadSettings(&fallbackWarn);
        const core::AppSettings defaults = core::ConfigStore::defaultSettings();
        QVERIFY(!fallbackWarn.isEmpty());
        QCOMPARE(recovered.accentColorHex, QStringLiteral("#2596FF"));
        QCOMPARE(recovered.customAccentSwatches, defaults.customAccentSwatches);
        QCOMPARE(recovered.customKeyboardSwatches, defaults.customKeyboardSwatches);

        // Verify ColorScience WCAG AA accent derivation against worst card surfaces in both modes
        const QStringList checkSwatches = {
            QStringLiteral("#FFEA00"), QStringLiteral("#A4E61D"), QStringLiteral("#F5D328"),
            QStringLiteral("#2596FF"), QStringLiteral("#F5B82E"), QStringLiteral("#9B6BFF"),
            QStringLiteral("#E06C65"), QStringLiteral("#68C938"), QStringLiteral("#3B82F6")
        };
        for (bool dark : {true, false}) {
            const QColor worstBg = dark ? QColor(QStringLiteral("#333837")) : QColor(QStringLiteral("#C5CDD2"));
            const QColor primaryBg = dark ? QColor(QStringLiteral("#222524")) : QColor(QStringLiteral("#D8DCDF"));
            for (const QString& hex : checkSwatches) {
                const auto tok = core::ColorScience::deriveAccessibleAccentTokens(QColor(hex), dark);
                QVERIFY2(tok.wcagAaNormal, qPrintable(hex));
                QVERIFY2(core::ColorScience::contrastRatio(tok.readableOnSurface, primaryBg) >= 4.5, qPrintable(hex));
                QVERIFY2(core::ColorScience::contrastRatio(tok.readableOnSurface, worstBg) >= 4.5, qPrintable(hex));
                QVERIFY2(core::ColorScience::contrastRatio(tok.onAccentText, tok.primary) >= 4.5, qPrintable(hex));
            }
        }
    }
};

QTEST_MAIN(TestCoreAndHal)
#include "test_core_and_hal.moc"
