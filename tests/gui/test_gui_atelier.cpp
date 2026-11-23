// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include <QtTest/QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QElapsedTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QSize>
#include <QTemporaryDir>
#include <QTimer>
#include <array>

#include "app/AsyncControlClient.h"
#include "app/ControlCenterController.h"
#include "app/ThemeEngine.h"
#include "daemon/DaemonServiceObject.h"
#include "hal/SysfsRoot.h"
#include "ui/FanCurveEditorItem.h"
#include "ui/HsvColorWheelItem.h"
#include "ui/KeyboardVisualizerItem.h"
#include "ui/TelemetryStripChart.h"

using namespace acc;

namespace {

class MockAsyncControlTransport : public app::IAsyncControlTransport {
public:
    explicit MockAsyncControlTransport(QObject* parent = nullptr)
        : app::IAsyncControlTransport(parent) {}

    app::AsyncControlOutcome nextOutcome;
    bool holdCallbackManually = false;
    int dispatchDelayMs = 0;
    app::AsyncControlCallback heldCallback;
    int callCount = 0;
    QString lastOperation;

    void setKeyboardLighting(
        const QString&,
        uint,
        const QString&,
        bool,
        app::AsyncControlCallback callback
    ) override {
        finishOrHold(QStringLiteral("keyboard_lighting"), std::move(callback));
    }

    void setCpuPerformanceTuning(
        const QString&,
        const QString&,
        bool,
        int,
        int,
        app::AsyncControlCallback callback
    ) override {
        finishOrHold(QStringLiteral("cpu_tuning"), std::move(callback));
    }

    void setFanControlMode(
        const QString&,
        int,
        app::AsyncControlCallback callback
    ) override {
        finishOrHold(QStringLiteral("fan_mode"), std::move(callback));
    }

    void setBatteryChargeLimit(
        int,
        int,
        app::AsyncControlCallback callback
    ) override {
        finishOrHold(QStringLiteral("battery_limit"), std::move(callback));
    }

    void setSystemPowerProfile(
        const QString&,
        app::AsyncControlCallback callback
    ) override {
        finishOrHold(QStringLiteral("power_profile"), std::move(callback));
    }

    void setDisplayBrightness(
        int,
        app::AsyncControlCallback callback
    ) override {
        finishOrHold(QStringLiteral("display_brightness"), std::move(callback));
    }

    void triggerHeldCallback(const app::AsyncControlOutcome& outcome) {
        if (heldCallback) {
            auto cb = std::move(heldCallback);
            heldCallback = nullptr;
            cb(outcome);
        }
    }

private:
    void finishOrHold(const QString& op, app::AsyncControlCallback callback) {
        ++callCount;
        lastOperation = op;
        if (holdCallbackManually) {
            heldCallback = std::move(callback);
            return;
        }
        const app::AsyncControlOutcome out = nextOutcome;
        if (dispatchDelayMs > 0) {
            QTimer::singleShot(dispatchDelayMs, this, [callback = std::move(callback), out]() {
                if (callback) {
                    callback(out);
                }
            });
        } else if (callback) {
            callback(out);
        }
    }
};

} // namespace

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
        // Constructor must NOT block GUI thread with synchronous telemetry collection
        QCOMPARE(controller.telemetryFreshness(), QStringLiteral("unavailable"));
        QCOMPARE(controller.telemetryFreshnessState(), app::ControlCenterController::FreshnessState::Unavailable);

        controller.switchHardwareMode(2); // MockFullCapabilities
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.telemetryFreshness(), QStringLiteral("fresh"));
        QCOMPARE(controller.telemetryAvailable(), true);
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
        QVERIFY(controller.waitForNextSnapshot(3000));

        // Apply performance profile
        controller.selectOperatingProfile(QStringLiteral("performance"));
        QCOMPARE(controller.activeProfileId(), QStringLiteral("performance"));
        QVERIFY(controller.waitForNextSnapshot(3000));

        // Apply fan mode and restore auto
        controller.setFanMode(QStringLiteral("manual"), 75);
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.fanControlModeLabel(), QStringLiteral("Manual PWM Override"));
        controller.setFanMode(QStringLiteral("auto"), 50);
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.fanControlModeLabel(), QStringLiteral("Firmware Automatic (PWM)"));

        // Apply battery limit and test restorePreviousBatteryChargeLimit() rollback
        QCOMPARE(controller.batCycleCountReported(), true);
        QCOMPARE(controller.batCycleCountText(), QStringLiteral("42"));
        QCOMPARE(controller.fanCapabilityBadge(), QStringLiteral("SIMULATED"));
        QCOMPARE(controller.batChargeThresholdPct(), 80);
        controller.applyBatteryChargeLimit(65);
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.batChargeThresholdPct(), 65);
        QCOMPARE(controller.batPreviousThresholdPct(), 80);
        controller.restorePreviousBatteryChargeLimit();
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.batChargeThresholdPct(), 80);
        QCOMPARE(controller.batCapabilityBadge(), QStringLiteral("SIMULATED"));

        // Suspend / resume cycle
        controller.onPrepareForSleep(true);
        QCOMPARE(controller.telemetrySuspended(), true);
        QCOMPARE(controller.telemetryFreshness(), QStringLiteral("stale"));
        controller.onPrepareForSleep(false);
        QCOMPARE(controller.telemetrySuspended(), false);
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.telemetryFreshness(), QStringLiteral("fresh"));
        QCOMPARE(controller.statusBannerIsError(), false);

        // Switch to MockPhysicalMirror (A715-59G default kernel posture)
        controller.switchHardwareMode(1);
        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.manualFanSupported(), false);
        QCOMPARE(controller.fanCapabilityBadge(), QStringLiteral("FIRMWARE AUTOMATIC"));
        QCOMPARE(controller.kbdHardwareConnected(), false);
        QCOMPARE(controller.batChargeLimitSupported(), false);
        QCOMPARE(controller.batCycleCountReported(), false);
        QCOMPARE(controller.batCycleCountText(), QStringLiteral("NOT REPORTED"));
        QCOMPARE(controller.batCapabilityBadge(), QStringLiteral("BATTERY TELEMETRY ONLY — CHARGE LIMIT NOT DETECTED"));

        // Switch to MockClv0001DriverActive (Stage 18 CLV0001 bound via clean-room driver)
        controller.switchHardwareMode(3);
        QVERIFY(controller.waitForNextSnapshot(3000));
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
        QVERIFY(controller.waitForNextSnapshot(3000));
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
        QVERIFY(controller.waitForNextSnapshot(3000));
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

    void testAsyncTelemetryFreshnessStaleAndFailureTransitions() {
        app::ThemeEngine theme;
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockPhysicalMirror);
        app::ControlCenterController controller(&theme);

        // Before worker thread posts first result, freshness must be explicitly Unavailable
        QCOMPARE(controller.telemetryFreshness(), QStringLiteral("unavailable"));
        QCOMPARE(controller.telemetryAvailable(), false);
        QCOMPARE(controller.telemetryStale(), false);
        QCOMPARE(controller.lastTelemetryTimestampMs(), qint64(0));

        QVERIFY(controller.waitForNextSnapshot(3000));
        QCOMPARE(controller.telemetryFreshness(), QStringLiteral("fresh"));
        QCOMPARE(controller.telemetryAvailable(), true);
        QCOMPARE(controller.telemetryStale(), false);
        QVERIFY(controller.lastTelemetryTimestampMs() > 0);
        const QString recordedProduct = controller.productTitle();
        QVERIFY(!recordedProduct.isEmpty());

        // Worker failure transitions previously Fresh telemetry -> Stale (never silently Fresh)
        controller.telemetryCoordinator()->setSimulatedWorkerFailureForTest(QStringLiteral("ERR_SIMULATED_IO"));
        controller.refreshNow();
        QTRY_COMPARE_WITH_TIMEOUT(controller.telemetryFreshness(), QStringLiteral("stale"), 3000);
        QCOMPARE(controller.telemetryStale(), true);
        QCOMPARE(controller.telemetryAvailable(), true);
        QCOMPARE(controller.telemetryErrorMessage(), QStringLiteral("ERR_SIMULATED_IO"));
        QCOMPARE(controller.productTitle(), recordedProduct);

        // Clear failure -> recovers to Fresh
        controller.telemetryCoordinator()->setSimulatedWorkerFailureForTest(QString());
        controller.refreshNow();
        QTRY_COMPARE_WITH_TIMEOUT(controller.telemetryFreshness(), QStringLiteral("fresh"), 3000);
        QCOMPARE(controller.telemetryErrorMessage(), QString());

        // Age-out threshold marks snapshot Stale cleanly
        controller.setFreshnessTimeoutOverrideMsForTest(40);
        QTest::qWait(70);
        QTRY_COMPARE_WITH_TIMEOUT(controller.telemetryFreshness(), QStringLiteral("stale"), 1500);
        QVERIFY(controller.telemetryAgeMs() >= 40);
        controller.setFreshnessTimeoutOverrideMsForTest(0);
    }

    void testAsyncControlPendingAuthStatesAndGuiResponsiveness() {
        app::ThemeEngine theme;
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockClv0001DriverActive);
        MockAsyncControlTransport mockTransport;
        app::ControlCenterController controller(
            &theme,
            &mockTransport,
            QDBusConnection(QStringLiteral("stage24_unused_bus"))
        );
        QVERIFY(controller.waitForNextSnapshot(3000));

        // 1. Slow async control call must NOT block GUI event loop and must expose AuthPending
        mockTransport.dispatchDelayMs = 120;
        mockTransport.nextOutcome = app::AsyncControlOutcome{
            core::ControlAuthState::Denied,
            false,
            QStringLiteral("ERR_POLKIT_UNAUTHORIZED"),
            QString(),
            QStringLiteral("Polkit denied authorization for keyboard lighting."),
            false,
            -1
        };

        int guiTicks = 0;
        QTimer responsivenessTimer;
        responsivenessTimer.setInterval(10);
        connect(&responsivenessTimer, &QTimer::timeout, this, [&guiTicks]() { ++guiTicks; });
        responsivenessTimer.start();

        QElapsedTimer callTime;
        callTime.start();
        controller.applyKeyboardLightingToHardware();
        QVERIFY(callTime.elapsed() < 50); // Dispatch returned immediately without blocking
        QCOMPARE(controller.controlOperationPending(), true);
        QCOMPARE(controller.controlAuthState(), QStringLiteral("pending"));
        QCOMPARE(controller.controlAuthStateEnum(), app::ControlCenterController::AuthState::Pending);

        // Overlapping privileged call while pending must fail safely without dispatching duplicate call
        const int countBeforeOverlap = mockTransport.callCount;
        controller.applyCustomCpuTuning(QStringLiteral("powersave"), QStringLiteral("power"), true, 10, 80);
        QCOMPARE(mockTransport.callCount, countBeforeOverlap);

        // Wait for async denial completion while verifying GUI timer kept ticking
        QTRY_COMPARE_WITH_TIMEOUT(controller.controlOperationPending(), false, 3000);
        responsivenessTimer.stop();
        QVERIFY2(guiTicks >= 4, qPrintable(QStringLiteral("GUI loop ticked only %1 times during pending op").arg(guiTicks)));

        // Polkit denial is distinct from hardware/validation failure
        QCOMPARE(controller.controlAuthState(), QStringLiteral("denied"));
        QCOMPARE(controller.controlAuthStateEnum(), app::ControlCenterController::AuthState::Denied);
        QCOMPARE(controller.controlLastErrorCode(), QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));
        QCOMPARE(controller.statusBannerIsError(), true);

        // 2. Verify DaemonUnavailable and Hardware Failed classifications via DbusControlTransport::classifyDaemonError
        const auto deniedOutcome = app::DbusControlTransport::classifyDaemonError(
            QStringLiteral("fan mode"),
            QStringLiteral("org.freedesktop.PolicyKit1.Error.NotAuthorized"),
            QStringLiteral("Not authorized")
        );
        QCOMPARE(deniedOutcome.authState, core::ControlAuthState::Denied);

        const auto unavailOutcome = app::DbusControlTransport::classifyDaemonError(
            QStringLiteral("fan mode"),
            QStringLiteral("org.freedesktop.DBus.Error.ServiceUnknown"),
            QStringLiteral("Service not found")
        );
        QCOMPARE(unavailOutcome.authState, core::ControlAuthState::DaemonUnavailable);

        const auto hwFailOutcome = app::DbusControlTransport::classifyDaemonError(
            QStringLiteral("fan mode"),
            QStringLiteral("org.freedesktop.DBus.Error.Failed"),
            QStringLiteral("Hardware write error")
        );
        QCOMPARE(hwFailOutcome.authState, core::ControlAuthState::Failed);
    }

    void testAsyncCancellationAndLifetimeSafety() {
        app::ThemeEngine theme;
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);
        MockAsyncControlTransport mockTransport;
        mockTransport.holdCallbackManually = true;

        auto* controller = new app::ControlCenterController(
            &theme,
            &mockTransport,
            QDBusConnection(QStringLiteral("stage24_unused_bus"))
        );
        QVERIFY(controller->waitForNextSnapshot(3000));

        // 1. Mode switch cancels pending control callback & discards delayed worker snapshot from old mode
        controller->applyBatteryChargeLimit(75);
        QCOMPARE(controller->controlOperationPending(), true);
        QCOMPARE(controller->controlAuthState(), QStringLiteral("pending"));

        controller->telemetryCoordinator()->setSimulatedCollectionDelayMsForTest(70);
        controller->refreshNow(); // in-flight FullCapabilities poll
        controller->telemetryCoordinator()->setSimulatedCollectionDelayMsForTest(0);
        controller->switchHardwareMode(1); // immediately switch to MockPhysicalMirror

        // Stale control callback arriving after mode switch must be ignored
        mockTransport.triggerHeldCallback(app::AsyncControlOutcome{
            core::ControlAuthState::Denied,
            false,
            QStringLiteral("ERR_POLKIT_UNAUTHORIZED"),
            QString(),
            QStringLiteral("Stale denial"),
            false,
            -1
        });
        QCOMPARE(controller->controlAuthState(), QStringLiteral("idle"));
        QVERIFY(controller->waitForNextSnapshot(3000));
        QCOMPARE(controller->hardwareModeIndex(), 1);
        QCOMPARE(controller->batChargeLimitSupported(), false);

        // 2. Destroying controller while BOTH worker poll and async transport callback are in flight is safe
        controller->telemetryCoordinator()->setSimulatedCollectionDelayMsForTest(80);
        controller->refreshNow();
        controller->applyCustomCpuTuning(QStringLiteral("powersave"), QStringLiteral("power"), true, 20, 90);
        QVERIFY(mockTransport.heldCallback != nullptr);

        QPointer<app::ControlCenterController> guarded(controller);
        delete controller;
        QVERIFY(guarded.isNull());

        // Triggering held callback after controller destruction must safely no-op
        mockTransport.triggerHeldCallback(app::AsyncControlOutcome{
            core::ControlAuthState::Authorized,
            true,
            QStringLiteral("OK"),
            QStringLiteral("{}"),
            QStringLiteral("Completed late"),
            true,
            -1
        });
        QTest::qWait(120);
    }

    void testRealDbusPendingCallWatcherSessionRoundTrip() {
        QDBusConnection sessionBus = QDBusConnection::sessionBus();
        if (!sessionBus.isConnected()) {
            QSKIP("No D-Bus session bus available in isolated build environment.");
        }

        hal::SysfsRoot::instance().setMode(hal::HardwareMode::MockFullCapabilities);
        daemon::DaemonServiceObject daemonObj(daemon::AuthPolicyMode::TestDenyAll);
        const QString objPath = QStringLiteral("/io/github/zethasaintlab/Stage24Test_%1")
                                    .arg(QCoreApplication::applicationPid());
        QVERIFY(sessionBus.registerObject(objPath, &daemonObj, QDBusConnection::ExportScriptableSlots));

        app::DbusControlTransport transport(
            sessionBus,
            sessionBus.baseService(),
            objPath,
            QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
            2000
        );

        // Exercise real non-blocking QDBusPendingCallWatcher against DaemonServiceObject (temporarily clear mock check via direct QDBusMessage path or verify classify)
        // Call via QDBusPendingCallWatcher directly against sessionBus object to test real async D-Bus Polkit denial reply
        QDBusMessage call = QDBusMessage::createMethodCall(
            sessionBus.baseService(),
            objPath,
            QStringLiteral("io.github.zethasaintlab.AcerControlCenter1"),
            QStringLiteral("SetBatteryChargeLimit")
        );
        call << static_cast<uint>(80);

        bool callbackFired = false;
        bool replyApplied = true;
        QString replyErrCode;
        auto* watcher = new QDBusPendingCallWatcher(sessionBus.asyncCall(call, 2000), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [&](QDBusPendingCallWatcher* w) {
            w->deleteLater();
            const QDBusMessage reply = w->reply();
            if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 3) {
                replyApplied = reply.arguments().at(0).toBool();
                replyErrCode = reply.arguments().at(2).toString();
            }
            callbackFired = true;
        });

        QTRY_VERIFY_WITH_TIMEOUT(callbackFired, 2000);
        QCOMPARE(replyApplied, false);
        QCOMPARE(replyErrCode, QStringLiteral("ERR_POLKIT_UNAUTHORIZED"));

        sessionBus.unregisterObject(objPath);
        hal::SysfsRoot::instance().setMode(hal::HardwareMode::PhysicalSystem);
    }

    void testQmlMainWindowInstantiationOffscreenAndLazyViews() {
        app::ThemeEngine theme;
        app::ControlCenterController controller(&theme);
        controller.switchHardwareMode(2);
        QVERIFY(controller.waitForNextSnapshot(3000));

        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
        engine.rootContext()->setContextProperty(QStringLiteral("ctrl"), &controller);
        engine.load(QUrl(QStringLiteral("qrc:/qml/qml/Main.qml")));

        QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window != nullptr);

        // Verify lazy single-view Loader instantiates only OverviewView at startup (not all 8 views)
        auto* loader = window->findChild<QObject*>(QStringLiteral("workspaceLoader"));
        QVERIFY(loader != nullptr);
        QCOMPARE(loader->property("source").toUrl().fileName(), QStringLiteral("OverviewView.qml"));
        auto* freshnessLbl = window->findChild<QObject*>(QStringLiteral("telemetryFreshnessLbl"));
        QVERIFY(freshnessLbl != nullptr);
        QVERIFY(freshnessLbl->property("text").toString().contains(QStringLiteral("FRESH")));

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
        QVERIFY(controller.waitForNextSnapshot(3000));
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

        // Exact responsive breakpoint checks (680x540 min, 819 vs 820, 1159 vs 1160, 1920x1080, 2560x1440)
        QCOMPARE(window->minimumWidth(), 680);
        QCOMPARE(window->minimumHeight(), 540);

        struct BreakpointCase {
            int w;
            int h;
            bool narrow;
            bool wide;
            int railW;
        };
        const std::array<BreakpointCase, 6> bpCases = {{
            {680, 540, true, false, 0},
            {819, 700, true, false, 0},
            {820, 700, false, false, 64},
            {1159, 760, false, false, 64},
            {1160, 760, false, true, 222},
            {1920, 1080, false, true, 222}
        }};
        for (const auto& bp : bpCases) {
            window->resize(bp.w, bp.h);
            QCoreApplication::processEvents();
            QCOMPARE(window->property("isNarrowLayout").toBool(), bp.narrow);
            QCOMPARE(window->property("isWideRail").toBool(), bp.wide);
            QCOMPARE(window->property("railWidth").toInt(), bp.railW);
        }

        // Verify all 8 views across min (680x540), compact (820x700), and desktop (1920x1080) without horizontal overflow
        const std::array<QSize, 4> viewCheckSizes = {
            QSize(680, 540),
            QSize(820, 700),
            QSize(1366, 768),
            QSize(1920, 1080)
        };
        for (const QSize& sz : viewCheckSizes) {
            window->resize(sz);
            for (int nav = 0; nav < 8; ++nav) {
                controller.setCurrentTabIndex(nav);
                QCoreApplication::processEvents();
                QObject* pageItem = loader->property("item").value<QObject*>();
                QVERIFY(pageItem != nullptr);
                const qreal pageW = pageItem->property("width").toReal();
                const qreal pageContentW = pageItem->property("contentWidth").toReal();
                QVERIFY2(pageW > 400.0, qPrintable(QStringLiteral("nav=%1 narrow pageW=%2").arg(nav).arg(pageW)));
                QVERIFY2(pageContentW <= pageW + 1.0, qPrintable(QStringLiteral("Horizontal overflow nav=%1 %2 > %3").arg(nav).arg(pageContentW).arg(pageW)));
            }
            const QImage szImg = window->grabWindow();
            QVERIFY(!szImg.isNull());
            QCOMPARE(szImg.width(), sz.width());
        }

        // Verify Stage 24 async state banner surfaces stale telemetry inside QML
        auto* stateBanner = window->findChild<QObject*>(QStringLiteral("telemetryStateBanner"));
        QVERIFY(stateBanner != nullptr);
        QCOMPARE(stateBanner->property("visible").toBool(), false);
        controller.onPrepareForSleep(true);
        QCoreApplication::processEvents();
        QCOMPARE(stateBanner->property("visible").toBool(), true);
        QCOMPARE(freshnessLbl->property("text").toString().contains(QStringLiteral("STALE")), true);
        controller.onPrepareForSleep(false);
        QVERIFY(controller.waitForNextSnapshot(3000));

        theme.setDarkTheme(false);
        controller.setCurrentTabIndex(6);
        QCoreApplication::processEvents();
        QImage lightCadImg = window->grabWindow();
        QVERIFY(!lightCadImg.isNull());
    }
};

QTEST_MAIN(TestGuiAtelier)
#include "test_gui_atelier.moc"
