// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include <QtTest/QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTemporaryDir>

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

private slots:
    void initTestCase() {
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
        QCOMPARE(controller.kbdRgbSupported(), false);
        QCOMPARE(controller.kbdMonochromeOnly(), false);
        QCOMPARE(controller.kbdFirmwareHotkeyManaged(), true);
        QCOMPARE(controller.kbdBackendKindId(), QStringLiteral("firmware_hotkey_only"));
        QCOMPARE(controller.batChargeLimitSupported(), false);

        // Saving a software profile in Preview Mode must succeed locally without claiming a hardware write
        controller.saveNewKbdProfile(QStringLiteral("Stage 12 Studio Preview"));
        QCOMPARE(controller.statusBannerIsError(), false);
        QVERIFY(controller.statusBannerText().contains(QLatin1String("Saved custom keyboard profile")));
        QVERIFY(!controller.statusBannerText().contains(QLatin1String("verified keyboard lighting")));

        // Verify Monochrome-only LED posture in controller
        controller.switchHardwareMode(2);
        const QString multiPath = hal::SysfsRoot::instance().resolvePath(
            QStringLiteral("/sys/class/leds/acer::kbd_backlight/multi_intensity")
        );
        QVERIFY(QFile::remove(multiPath));
        controller.refreshNow();
        QCOMPARE(controller.kbdHardwareConnected(), true);
        QCOMPARE(controller.kbdRgbSupported(), false);
        QCOMPARE(controller.kbdMonochromeOnly(), true);
        QCOMPARE(controller.kbdBackendKindId(), QStringLiteral("physical_monochrome"));
        controller.applyKeyboardLightingToHardware();
        QCOMPARE(controller.statusBannerIsError(), false);
        QVERIFY(controller.statusBannerText().contains(QLatin1String("monochrome")));

        controller.switchHardwareMode(1);
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
    }
};

QTEST_MAIN(TestGuiAtelier)
#include "test_gui_atelier.moc"
