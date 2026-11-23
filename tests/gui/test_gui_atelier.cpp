// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include <QtTest/QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSize>
#include <QTemporaryDir>
#include <array>

#include "app/ControlCenterController.h"
#include "app/ThemeEngine.h"
#include "hal/SysfsRoot.h"
#include "ui/FanCurveEditorItem.h"
#include "ui/HsvColorWheelItem.h"
#include "ui/KeyboardVisualizerItem.h"
#include "ui/TelemetryStripChart.h"

using namespace acc;

class TestGuiAtelier : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempConfigDir;

private slots:
    void initTestCase() {
        QVERIFY(m_tempConfigDir.isValid());
        qputenv("ACC_CONFIG_DIR", m_tempConfigDir.path().toUtf8());
        Q_INIT_RESOURCE(resources);
        qmlRegisterType<ui::TelemetryStripChart>("AcerControl.Ui", 1, 0, "TelemetryStripChart");
        qmlRegisterType<ui::HsvColorWheelItem>("AcerControl.Ui", 1, 0, "HsvColorWheel");
        qmlRegisterType<ui::FanCurveEditorItem>("AcerControl.Ui", 1, 0, "FanCurveEditor");
        qmlRegisterType<ui::KeyboardVisualizerItem>("AcerControl.Ui", 1, 0, "KeyboardVisualizer");
    }

    void testThemeEngineContrastAndIndependence() {
        app::ThemeEngine theme;
        QCOMPARE(theme.darkTheme(), true);
        QCOMPARE(theme.accentHex(), QStringLiteral("#FFEA00"));
        QVERIFY(theme.contrastOnSurface() >= 4.5);
        QVERIFY(theme.contrastOnAccent() >= 4.5);
        QVERIFY(theme.wcagAaPass());

        QVERIFY(theme.setAccentFromHex(QStringLiteral("#3B82F6"))); // Blueprint Blue
        QVERIFY(theme.contrastOnSurface() >= 4.5);

        theme.setDarkTheme(false); // Light CAD Surface (#D8DCDF)
        QCOMPARE(theme.darkTheme(), false);
        QVERIFY(theme.setAccentFromHex(QStringLiteral("#FFEA00")));
        QVERIFY(theme.contrastOnSurface() >= 4.5);
        QVERIFY(theme.contrastOnAccent() >= 4.5);
        theme.setDarkTheme(true);
    }

    void testControllerWorkspacesAndHardwareIndependence() {
        app::ThemeEngine theme;
        QVERIFY(theme.setAccentFromHex(QStringLiteral("#10B981"))); // Emerald

        app::ControlCenterController controller(&theme);
        controller.switchHardwareMode(2); // MockFullCapabilities
        QCOMPARE(controller.hardwareModeIndex(), 2);
        QCOMPARE(controller.manualFanSupported(), true);
        QCOMPARE(controller.kbdHardwareConnected(), true);
        QCOMPARE(controller.kbdRgbSupported(), true);
        QCOMPARE(controller.batChargeLimitSupported(), true);

        // Navigate across all 8 views
        for (int i = 0; i < 8; ++i) {
            controller.setCurrentTabIndex(i);
            QCOMPARE(controller.currentTabIndex(), i);
        }

        // Test keyboard color updates via HSV, HSL, RGB, Hex and verify App Theme Accent is UNCHANGED
        const QString initialThemeAccent = theme.accentHex();
        controller.setKbdColorFromHsv(280.0, 0.9, 1.0);
        QCOMPARE(theme.accentHex(), initialThemeAccent);

        controller.setKbdColorFromHsl(120.0, 0.8, 0.5);
        QCOMPARE(theme.accentHex(), initialThemeAccent);

        controller.setKbdColorFromRgb(255, 64, 32);
        QCOMPARE(controller.kbdPickerR(), 255);
        QCOMPARE(controller.kbdPickerG(), 64);
        QCOMPARE(controller.kbdPickerB(), 32);
        QCOMPARE(theme.accentHex(), initialThemeAccent);

        QVERIFY(controller.setKbdColorFromHex(QStringLiteral("#00E5FF")));
        QCOMPARE(controller.kbdPickerHex(), QStringLiteral("#00E5FF"));
        QCOMPARE(theme.accentHex(), initialThemeAccent);

        // Apply keyboard state and verify readback status
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);

        // Apply performance profile
        controller.selectOperatingProfile(QStringLiteral("performance"));
        QCOMPARE(controller.activeProfileId(), QStringLiteral("performance"));

        // Apply fan mode and restore auto
        controller.setFanMode(QStringLiteral("manual"), 75);
        QCOMPARE(controller.fanControlModeLabel(), QStringLiteral("Manual PWM Override"));
        controller.setFanMode(QStringLiteral("auto"), 50);
        QCOMPARE(controller.fanControlModeLabel(), QStringLiteral("Firmware Automatic (PWM)"));

        // Apply battery limit and test restorePreviousBatteryChargeLimit() rollback
        QCOMPARE(controller.batCycleCountReported(), true);
        QCOMPARE(controller.batCycleCountText(), QStringLiteral("42"));
        QCOMPARE(controller.fanCapabilityBadge(), QStringLiteral("SIMULATED"));
        QCOMPARE(controller.batChargeThresholdPct(), 80);
        controller.applyBatteryChargeLimit(65);
        QCOMPARE(controller.batChargeThresholdPct(), 65);
        QCOMPARE(controller.batPreviousThresholdPct(), 80);
        controller.restorePreviousBatteryChargeLimit();
        QCOMPARE(controller.batChargeThresholdPct(), 80);
        QCOMPARE(controller.batCapabilityBadge(), QStringLiteral("SIMULATED"));

        // Suspend / resume cycle
        controller.onPrepareForSleep(true);
        controller.onPrepareForSleep(false);
        QCOMPARE(controller.statusBannerIsError(), false);

        // Switch to MockPhysicalMirror (A715-59G default kernel posture)
        controller.switchHardwareMode(1);
        QCOMPARE(controller.manualFanSupported(), false);
        QCOMPARE(controller.fanCapabilityBadge(), QStringLiteral("FIRMWARE AUTOMATIC"));
        QCOMPARE(controller.kbdHardwareConnected(), false);
        QCOMPARE(controller.batChargeLimitSupported(), false);
        QCOMPARE(controller.batCycleCountReported(), false);
        QCOMPARE(controller.batCycleCountText(), QStringLiteral("NOT REPORTED"));
        QCOMPARE(controller.batCapabilityBadge(), QStringLiteral("BATTERY TELEMETRY ONLY — CHARGE LIMIT NOT DETECTED"));

        // Switch to MockClv0001DriverActive (Stage 18 CLV0001 bound via clean-room driver)
        controller.switchHardwareMode(3);
        QCOMPARE(controller.hardwareModeIndex(), 3);
        QCOMPARE(controller.kbdBackendKindId(), QStringLiteral("clv0001_acpi_dsm"));
        QCOMPARE(controller.kbdRgbSupported(), false);
        QCOMPARE(controller.kbdSupportsPresetColors(), true);
        QCOMPARE(controller.kbdSupportsArbitraryRgb(), true);
        QCOMPARE(controller.kbdPhysicalReadbackSupported(), false);
        QCOMPARE(controller.kbdPresetsModel().size(), 17);

        // Single-zone CLV0001 hardware must refuse per-zone split selection
        controller.setKbdSelectedZone(2);
        QCOMPARE(controller.kbdSelectedZone(), -1);

        const QString themeBeforeClv = theme.accentHex();
        controller.selectKbdPreset(QStringLiteral("red"));
        controller.setKbdBrightnessLevel(2);
        QCOMPARE(controller.kbdPresetName(), QStringLiteral("red"));
        QCOMPARE(controller.kbdSelectionSource(), QStringLiteral("preset"));
        QCOMPARE(controller.kbdBrightnessLevel(), 2);
        QCOMPARE(controller.kbdFirmwarePacketHex(), QStringLiteral("00 7E 00 F0"));
        QCOMPARE(theme.accentHex(), themeBeforeClv);

        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("physical hardware readback not exposed")));

        // Verify strict #RRGGBB HEX validation rejects malformed values without snapping to a preset or altering color
        QVERIFY(!controller.setKbdColorFromHex(QStringLiteral("123456")));
        QCOMPARE(controller.statusBannerIsError(), true);
        QVERIFY(!controller.setKbdColorFromHex(QStringLiteral("#123")));
        QVERIFY(!controller.setKbdColorFromHex(QStringLiteral("#12345G")));
        QVERIFY(!controller.setKbdColorFromHex(QStringLiteral("#1234567")));
        QVERIFY(!controller.applyCustomKbdRgb(256, 0, 0, 4));
        QCOMPARE(controller.statusBannerIsError(), true);
        QVERIFY(!controller.applyCustomKbdRgb(18, 52, 86, 5));
        QCOMPARE(controller.statusBannerIsError(), true);

        // Selecting custom HEX (#12A7E3) updates preview only (does NOT write to hardware before Apply)
        const QString clvStatePath = hal::SysfsRoot::instance().resolvePath(
            QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_state")
        );
        const QString stateBeforePreview = hal::SysfsRoot::readFileTrimmed(clvStatePath);
        QVERIFY(controller.setKbdColorFromHex(QStringLiteral("#12A7E3")));
        controller.setKbdBrightnessLevel(4);
        QCOMPARE(controller.kbdCustomPreviewDirty(), true);
        QCOMPARE(controller.kbdSelectionSource(), QStringLiteral("custom_rgb"));
        QCOMPARE(controller.kbdFirmwarePacketHex(), QStringLiteral("A7 12 E3 F0"));
        QCOMPARE(hal::SysfsRoot::readFileTrimmed(clvStatePath), stateBeforePreview);

        // Apply Stage 17 hero color via explicit Apply action after cooldown
        QTest::qWait(50);
        QVERIFY(controller.applyCustomKbdRgb(18, 167, 227, 4));
        QCOMPARE(controller.statusBannerIsError(), false);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("A7 12 E3 F0")));
        QVERIFY(hal::SysfsRoot::readFileTrimmed(clvStatePath).contains(QStringLiteral("preset=custom_rgb")));

        // Random static color picks one uniform RGB triplet and sends through standard Apply
        controller.selectRandomKbdColor();
        QCOMPARE(controller.kbdSelectionSource(), QStringLiteral("random_rgb"));
        QCOMPARE(controller.kbdCustomPreviewDirty(), true);
        QTest::qWait(50);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);

        // Rapid second apply within 40 ms cooldown hits real rate limiter; after 50 ms cooldown preset apply succeeds
        controller.selectKbdPreset(QStringLiteral("green"));
        QCOMPARE(controller.kbdCustomPreviewDirty(), false);
        QCOMPARE(controller.kbdSelectionSource(), QStringLiteral("preset"));
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), true);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("rate-limited")));

        QTest::qWait(50);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);

        // Verify legacy preset-only module fallback disables arbitrary RGB cleanly in controller
        const QString clvPresetsPath = hal::SysfsRoot::instance().resolvePath(
            QStringLiteral("/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_presets")
        );
        {
            QFile pf(clvPresetsPath);
            QVERIFY(pf.open(QIODevice::WriteOnly | QIODevice::Truncate));
            pf.write("presets=off,blue,cyan,white\nbrightness_levels=0,1,2,3,4\n");
        }
        controller.refreshNow();
        QCOMPARE(controller.kbdSupportsPresetColors(), true);
        QCOMPARE(controller.kbdSupportsArbitraryRgb(), false);
        QVERIFY(controller.setKbdColorFromHex(QStringLiteral("#0000B0")));
        QCOMPARE(controller.kbdCustomPreviewDirty(), true);
        QCOMPARE(controller.kbdNearestPresetName(), QStringLiteral("blue"));
        QTest::qWait(50);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), true);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("17 verified named presets")));
        controller.snapToNearestKbdPreset();
        QCOMPARE(controller.kbdCustomPreviewDirty(), false);
        QCOMPARE(controller.kbdPresetName(), QStringLiteral("blue"));
        QTest::qWait(50);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);

        // Restore full mock mode and check profile brightness persistence + off
        controller.switchHardwareMode(3);
        const QString prevProfId = controller.activeKbdProfileId();
        controller.setKbdBrightnessLevel(3);
        QCOMPARE(controller.kbdBrightnessLevel(), 3);
        controller.selectKbdProfile(QStringLiteral("workstation_warm"));
        QCOMPARE(controller.kbdPresetName(), QStringLiteral("white"));
        controller.selectKbdProfile(prevProfId);
        QCOMPARE(controller.kbdBrightnessLevel(), 3);
        QTest::qWait(50);
        controller.selectKbdPreset(QStringLiteral("off"));
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);
    }

    void testQmlMainWindowInstantiationOffscreen() {
        app::ThemeEngine theme;
        app::ControlCenterController controller(&theme);
        controller.switchHardwareMode(2);

        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
        engine.rootContext()->setContextProperty(QStringLiteral("ctrl"), &controller);
        engine.load(QUrl(QStringLiteral("qrc:/qml/qml/Main.qml")));

        QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window != nullptr);

        for (int nav = 0; nav < 8; ++nav) {
            controller.setCurrentTabIndex(nav);
            QCoreApplication::processEvents();
            QImage img = window->grabWindow();
            QVERIFY(!img.isNull());
            QVERIFY(img.width() >= 1100);
            QVERIFY(img.height() >= 700);
        }

        // Verify CLV0001 Keyboard Studio mode (preset + custom RGB + random RGB) renders cleanly offscreen
        controller.switchHardwareMode(3);
        controller.setCurrentTabIndex(3);
        controller.selectKbdPreset(QStringLiteral("cyan"));
        QCoreApplication::processEvents();
        QImage clvImg = window->grabWindow();
        QVERIFY(!clvImg.isNull());
        QVERIFY(clvImg.width() >= 1100);

        QVERIFY(controller.setKbdColorFromHex(QStringLiteral("#12A7E3")));
        controller.setKbdBrightnessLevel(4);
        QCoreApplication::processEvents();
        QImage customImg = window->grabWindow();
        QVERIFY(!customImg.isNull());
        QVERIFY(customImg.width() >= 1100);

        controller.selectRandomKbdColor();
        QCoreApplication::processEvents();
        QImage randImg = window->grabWindow();
        QVERIFY(!randImg.isNull());

        // Multi-resolution responsive checks (1366x768, 1600x900, 1920x1080, 2560x1440) + Light CAD polarity
        const std::array<QSize, 4> resSizes = {
            QSize(1366, 768),
            QSize(1600, 900),
            QSize(1920, 1080),
            QSize(2560, 1440)
        };
        for (const QSize& sz : resSizes) {
            window->resize(sz);
            controller.setCurrentTabIndex(0);
            QCoreApplication::processEvents();
            const QImage szImg = window->grabWindow();
            QVERIFY(!szImg.isNull());
        }
        theme.setDarkTheme(false);
        controller.setCurrentTabIndex(6);
        QCoreApplication::processEvents();
        QImage lightCadImg = window->grabWindow();
        QVERIFY(!lightCadImg.isNull());
    }
};

QTEST_MAIN(TestGuiAtelier)
#include "test_gui_atelier.moc"
