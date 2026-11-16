// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include <QtTest/QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTemporaryDir>

#include "app/ControlCenterController.h"
#include "app/ThemeEngine.h"
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
        QVERIFY(theme.contrastOnSurface() >= 4.5);
        QVERIFY(theme.wcagAaPass());

        QVERIFY(theme.setAccentFromHex(QStringLiteral("#3B82F6"))); // Blueprint Blue
        QVERIFY(theme.contrastOnSurface() >= 4.5);

        theme.setDarkTheme(false);
        QCOMPARE(theme.darkTheme(), false);
        QVERIFY(theme.contrastOnSurface() >= 4.5);
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

        // Apply battery limit
        controller.applyBatteryChargeLimit(80);
        QCOMPARE(controller.batChargeThresholdPct(), 80);

        // Suspend / resume cycle
        controller.onPrepareForSleep(true);
        controller.onPrepareForSleep(false);
        QCOMPARE(controller.statusBannerIsError(), false);

        // Switch to MockPhysicalMirror (A715-59G default kernel posture)
        controller.switchHardwareMode(1);
        QCOMPARE(controller.manualFanSupported(), false);
        QCOMPARE(controller.kbdHardwareConnected(), false);
        QCOMPARE(controller.batChargeLimitSupported(), false);

        // Switch to MockClv0001DriverActive (CLV0001 bound via clean-room driver)
        controller.switchHardwareMode(3);
        QCOMPARE(controller.hardwareModeIndex(), 3);
        QCOMPARE(controller.kbdBackendKindId(), QStringLiteral("clv0001_acpi_dsm"));
        QCOMPARE(controller.kbdSupportsPresetColors(), true);
        QCOMPARE(controller.kbdSupportsArbitraryRgb(), false);
        QCOMPARE(controller.kbdPhysicalReadbackSupported(), false);
        QCOMPARE(controller.kbdPresetsModel().size(), 17);

        const QString themeBeforeClv = theme.accentHex();
        controller.selectKbdPreset(QStringLiteral("red"));
        controller.setKbdBrightnessLevel(2);
        QCOMPARE(controller.kbdPresetName(), QStringLiteral("red"));
        QCOMPARE(controller.kbdBrightnessLevel(), 2);
        QCOMPARE(controller.kbdFirmwarePacketHex(), QStringLiteral("00 7E 00 F0"));
        QCOMPARE(theme.accentHex(), themeBeforeClv);

        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("physical hardware readback not exposed")));

        // Verify custom non-preset RGB color in CLV0001 mode fails cleanly instead of silently substituting
        QVERIFY(controller.setKbdColorFromHex(QStringLiteral("#123456")));
        QCOMPARE(controller.kbdCustomPreviewDirty(), true);
        QCOMPARE(controller.kbdNearestPresetName(), QStringLiteral("bluish-white"));
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), true);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("17 verified named presets")));

        // Rapid second apply within 40 ms cooldown hits real rate limiter; after 50 ms cooldown preset apply succeeds
        controller.selectKbdPreset(QStringLiteral("green"));
        QCOMPARE(controller.kbdCustomPreviewDirty(), false);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), true);
        QVERIFY(controller.statusBannerText().contains(QStringLiteral("rate-limited")));

        QTest::qWait(50);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);

        // Snap from custom RGB preview (#0000B0 -> nearest 'blue') and verify profile brightness persistence
        QVERIFY(controller.setKbdColorFromHex(QStringLiteral("#0000B0")));
        QCOMPARE(controller.kbdCustomPreviewDirty(), true);
        QCOMPARE(controller.kbdNearestPresetName(), QStringLiteral("blue"));
        controller.snapToNearestKbdPreset();
        QCOMPARE(controller.kbdCustomPreviewDirty(), false);
        QCOMPARE(controller.kbdPresetName(), QStringLiteral("blue"));
        QTest::qWait(50);
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);

        const QString prevProfId = controller.activeKbdProfileId();
        controller.setKbdBrightnessLevel(3);
        QCOMPARE(controller.kbdBrightnessLevel(), 3);
        controller.selectKbdProfile(QStringLiteral("workstation_warm"));
        QCOMPARE(controller.kbdPresetName(), QStringLiteral("white"));
        controller.selectKbdProfile(prevProfId);
        QCOMPARE(controller.kbdBrightnessLevel(), 3);
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

        // Verify CLV0001 Keyboard Studio mode renders cleanly offscreen
        controller.switchHardwareMode(3);
        controller.setCurrentTabIndex(3);
        controller.selectKbdPreset(QStringLiteral("cyan"));
        QCoreApplication::processEvents();
        QImage clvImg = window->grabWindow();
        QVERIFY(!clvImg.isNull());
        QVERIFY(clvImg.width() >= 1100);
    }
};

QTEST_MAIN(TestGuiAtelier)
#include "test_gui_atelier.moc"
