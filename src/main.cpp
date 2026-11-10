// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/ControlCenterController.h"
#include "app/ThemeEngine.h"
#include "hal/SysfsRoot.h"
#include "ui/FanCurveEditorItem.h"
#include "ui/HsvColorWheelItem.h"
#include "ui/KeyboardVisualizerItem.h"
#include "ui/TelemetryStripChart.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <unistd.h>

int main(int argc, char* argv[]) {
    // Early non-negotiable security guard BEFORE QApplication / Qt platform plugin initialization.
    bool allowRootTest = false;
    for (int i = 1; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == QLatin1String("--allow-root-test")) {
            allowRootTest = true;
            break;
        }
    }
    if (::geteuid() == 0 && !allowRootTest) {
        QTextStream(stderr) << "SECURITY ERROR: acer-control-center GUI must never be executed as root.\n"
                            << "Run as an unprivileged desktop user; privileged operations are isolated behind acer-control-daemon and Polkit.\n";
        return 1;
    }

    Q_INIT_RESOURCE(resources);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("acer-control-center"));
    QApplication::setOrganizationName(QStringLiteral("zethasaintlab"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Acer Control Center — Linux Edition (Precision Hardware Atelier)"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption mockModeOpt(
        QStringLiteral("mock-hardware"),
        QStringLiteral("Launch with simulated hardware fixtures (mirror or full)."),
        QStringLiteral("scenario"),
        QStringLiteral("mirror")
    );
    QCommandLineOption captureScreenshotsOpt(
        QStringLiteral("capture-screenshots"),
        QStringLiteral("Render and save PNG screenshots of all 8 views (Dark, Light, Custom Accent) into <dir> and exit."),
        QStringLiteral("dir")
    );
    QCommandLineOption allowRootTestOpt(
        QStringLiteral("allow-root-test"),
        QStringLiteral("Allow running in isolated root container test environments.")
    );
    QCommandLineOption dumpTelemetryJsonOpt(
        QStringLiteral("dump-telemetry-json"),
        QStringLiteral("Perform a read-only telemetry & capability snapshot and output JSON to stdout without modifying hardware.")
    );

    parser.addOption(mockModeOpt);
    parser.addOption(captureScreenshotsOpt);
    parser.addOption(allowRootTestOpt);
    parser.addOption(dumpTelemetryJsonOpt);
    parser.process(app);

    if (::geteuid() == 0) {
        // When --allow-root-test is used in isolated container CI, force MockPhysicalMirror so root never touches physical sysfs.
        acc::hal::SysfsRoot::instance().setMode(acc::hal::HardwareMode::MockPhysicalMirror);
    }

    if (parser.isSet(mockModeOpt)) {
        const QString scen = parser.value(mockModeOpt).trimmed().toLower();
        if (scen == QLatin1String("full")) {
            acc::hal::SysfsRoot::instance().setMode(acc::hal::HardwareMode::MockFullCapabilities);
        } else {
            acc::hal::SysfsRoot::instance().setMode(acc::hal::HardwareMode::MockPhysicalMirror);
        }
    }

    qmlRegisterType<acc::ui::TelemetryStripChart>("AcerControl.Ui", 1, 0, "TelemetryStripChart");
    qmlRegisterType<acc::ui::HsvColorWheelItem>("AcerControl.Ui", 1, 0, "HsvColorWheel");
    qmlRegisterType<acc::ui::FanCurveEditorItem>("AcerControl.Ui", 1, 0, "FanCurveEditor");
    qmlRegisterType<acc::ui::KeyboardVisualizerItem>("AcerControl.Ui", 1, 0, "KeyboardVisualizer");

    acc::app::ThemeEngine themeEngine;
    acc::app::ControlCenterController controller(&themeEngine);

    if (parser.isSet(dumpTelemetryJsonOpt)) {
        QThread::msleep(120);
        controller.refreshNow();

        QJsonObject root;
        root.insert(QStringLiteral("hardwareMode"), controller.hardwareModeLabel());
        root.insert(QStringLiteral("mockActive"), controller.mockModeActive());
        root.insert(QStringLiteral("productTitle"), controller.productTitle());
        root.insert(QStringLiteral("boardTitle"), controller.boardTitle());
        root.insert(QStringLiteral("biosTitle"), controller.biosTitle());
        root.insert(QStringLiteral("odmPlatformNote"), controller.odmPlatformNote());
        root.insert(QStringLiteral("wmiGuids"), controller.wmiGuidsJoined());
        root.insert(QStringLiteral("activeTunedProfile"), controller.activeTunedProfile());

        QJsonObject cpuObj;
        cpuObj.insert(QStringLiteral("modelName"), controller.cpuModelName());
        cpuObj.insert(QStringLiteral("packageTempC"), controller.cpuPackageTempC());
        cpuObj.insert(QStringLiteral("totalUtilizationPct"), controller.cpuUtilizationPct());
        cpuObj.insert(QStringLiteral("avgFreqMhz"), controller.cpuAvgFreqMhz());
        cpuObj.insert(QStringLiteral("maxFreqMhz"), controller.cpuMaxFreqMhz());
        cpuObj.insert(QStringLiteral("governor"), controller.cpuGovernor());
        cpuObj.insert(QStringLiteral("epp"), controller.cpuEpp());
        cpuObj.insert(QStringLiteral("turboEnabled"), controller.cpuTurboEnabled());
        cpuObj.insert(QStringLiteral("minPerfPct"), controller.cpuMinPerfPct());
        cpuObj.insert(QStringLiteral("maxPerfPct"), controller.cpuMaxPerfPct());
        cpuObj.insert(QStringLiteral("powerAvailable"), controller.cpuPowerAvailable());
        cpuObj.insert(QStringLiteral("powerSourceLabel"), controller.cpuPowerSource());
        cpuObj.insert(QStringLiteral("packagePowerWatts"), controller.cpuPackagePowerW());
        cpuObj.insert(QStringLiteral("cores"), QJsonArray::fromVariantList(controller.cpuCoresModel()));
        root.insert(QStringLiteral("cpu"), cpuObj);

        QJsonObject memObj;
        memObj.insert(QStringLiteral("totalMiB"), controller.memTotalMiB());
        memObj.insert(QStringLiteral("usedMiB"), controller.memUsedMiB());
        memObj.insert(QStringLiteral("availableMiB"), controller.memAvailableMiB());
        memObj.insert(QStringLiteral("cachedMiB"), controller.memCachedMiB());
        memObj.insert(QStringLiteral("utilizationPct"), controller.memUtilizationPct());
        memObj.insert(QStringLiteral("psiCpuSome10"), controller.psiCpuSome10());
        memObj.insert(QStringLiteral("psiMemSome10"), controller.psiMemSome10());
        memObj.insert(QStringLiteral("psiIoSome10"), controller.psiIoSome10());
        root.insert(QStringLiteral("memory"), memObj);

        QJsonObject gpuObj;
        gpuObj.insert(QStringLiteral("available"), controller.gpuAvailable());
        gpuObj.insert(QStringLiteral("sourceLabel"), controller.gpuSourceLabel());
        gpuObj.insert(QStringLiteral("name"), controller.gpuName());
        gpuObj.insert(QStringLiteral("driverVersion"), controller.gpuDriverVersion());
        gpuObj.insert(QStringLiteral("pciBusId"), controller.gpuPciBusId());
        gpuObj.insert(QStringLiteral("tempC"), controller.gpuTempC());
        gpuObj.insert(QStringLiteral("utilizationPct"), controller.gpuUtilizationPct());
        gpuObj.insert(QStringLiteral("memUtilizationPct"), controller.gpuMemUtilizationPct());
        gpuObj.insert(QStringLiteral("vramUsedMiB"), controller.gpuVramUsedMiB());
        gpuObj.insert(QStringLiteral("vramTotalMiB"), controller.gpuVramTotalMiB());
        gpuObj.insert(QStringLiteral("graphicsClockMhz"), controller.gpuGraphicsClockMhz());
        gpuObj.insert(QStringLiteral("memClockMhz"), controller.gpuMemClockMhz());
        gpuObj.insert(QStringLiteral("pstate"), controller.gpuPstate());
        gpuObj.insert(QStringLiteral("powerDrawWatts"), controller.gpuPowerDrawW());
        gpuObj.insert(QStringLiteral("powerMaxLimitWatts"), controller.gpuMaxLimitW());
        gpuObj.insert(QStringLiteral("powerRailGated"), controller.gpuPowerRailGated());
        gpuObj.insert(QStringLiteral("throttleReasons"), QJsonArray::fromStringList(controller.gpuThrottleReasons()));
        root.insert(QStringLiteral("gpu"), gpuObj);

        QJsonObject batObj;
        batObj.insert(QStringLiteral("acOnline"), controller.acOnline());
        batObj.insert(QStringLiteral("status"), controller.batStatus());
        batObj.insert(QStringLiteral("capacityPct"), controller.batCapacityPct());
        batObj.insert(QStringLiteral("voltageVolts"), controller.batVoltageV());
        batObj.insert(QStringLiteral("currentAmps"), controller.batCurrentA());
        batObj.insert(QStringLiteral("powerWatts"), controller.batPowerW());
        batObj.insert(QStringLiteral("chargeFullAh"), controller.batChargeFullAh());
        batObj.insert(QStringLiteral("chargeDesignAh"), controller.batChargeDesignAh());
        batObj.insert(QStringLiteral("wearHealthPct"), controller.batWearHealthPct());
        batObj.insert(QStringLiteral("chargeLimitSupported"), controller.batChargeLimitSupported());
        batObj.insert(QStringLiteral("explanation"), controller.batStatusExplanation());
        root.insert(QStringLiteral("battery"), batObj);

        QJsonObject thObj;
        thObj.insert(QStringLiteral("stateLabel"), controller.thermalStateLabel());
        thObj.insert(QStringLiteral("maxNvmeTempC"), controller.maxNvmeTempC());
        thObj.insert(QStringLiteral("maxBoardTempC"), controller.maxBoardTempC());
        thObj.insert(QStringLiteral("manualFanSupported"), controller.manualFanSupported());
        thObj.insert(QStringLiteral("fanControlMode"), controller.fanControlModeLabel());
        thObj.insert(QStringLiteral("fanStatusExplanation"), controller.fanStatusExplanation());
        thObj.insert(QStringLiteral("sensors"), QJsonArray::fromVariantList(controller.thermalSensorsModel()));
        thObj.insert(QStringLiteral("fans"), QJsonArray::fromVariantList(controller.fansModel()));
        root.insert(QStringLiteral("thermal"), thObj);

        QJsonObject dispObj;
        dispObj.insert(QStringLiteral("brightnessSupported"), controller.displayBrightnessSupported());
        dispObj.insert(QStringLiteral("brightnessPct"), controller.displayBrightnessPct());
        root.insert(QStringLiteral("display"), dispObj);

        root.insert(QStringLiteral("capabilities"), QJsonArray::fromVariantList(controller.capabilitiesModel()));

        QTextStream(stdout) << QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
        return 0;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), &themeEngine);
    engine.rootContext()->setContextProperty(QStringLiteral("ctrl"), &controller);

    const QUrl mainUrl(QStringLiteral("qrc:/qml/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [mainUrl](QObject* obj, const QUrl& objUrl) {
            if (!obj && mainUrl == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );
    engine.load(mainUrl);

    if (engine.rootObjects().isEmpty()) {
        return 2;
    }

    if (parser.isSet(captureScreenshotsOpt)) {
        const QString outDir = parser.value(captureScreenshotsOpt);
        QDir dir(outDir);
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }

        auto* topWin = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        if (!topWin) {
            return 3;
        }

        const QStringList viewNames = {
            QStringLiteral("01-overview"),
            QStringLiteral("02-performance"),
            QStringLiteral("03-thermal"),
            QStringLiteral("04-keyboard-studio"),
            QStringLiteral("05-battery-power"),
            QStringLiteral("06-nvidia-gpu"),
            QStringLiteral("07-diagnostics"),
            QStringLiteral("08-settings")
        };

        themeEngine.setReducedMotion(true);
        themeEngine.setDarkTheme(true);
        themeEngine.setAccentFromHex(QStringLiteral("#3B9EFF"));

        QTimer::singleShot(250, &app, [dir, viewNames, topWin, &controller, &themeEngine]() {
            for (int i = 0; i < viewNames.size(); ++i) {
                controller.setCurrentTabIndex(i);
                QCoreApplication::processEvents();
                const QImage img = topWin->grabWindow();
                img.save(dir.filePath(viewNames.at(i) + QStringLiteral("-dark.png")));
            }

            // Capture Light Theme ("Anodized Aluminum") + Custom Emerald Accent on Overview, Keyboard Studio, and Settings
            themeEngine.setDarkTheme(false);
            themeEngine.setAccentFromHex(QStringLiteral("#168A56"));
            controller.setCurrentTabIndex(0);
            QCoreApplication::processEvents();
            topWin->grabWindow().save(dir.filePath(QStringLiteral("09-overview-light-custom-accent.png")));

            controller.setCurrentTabIndex(3);
            QCoreApplication::processEvents();
            topWin->grabWindow().save(dir.filePath(QStringLiteral("10-keyboard-studio-light.png")));

            controller.setCurrentTabIndex(7);
            QCoreApplication::processEvents();
            topWin->grabWindow().save(dir.filePath(QStringLiteral("11-settings-light-wcag.png")));

            // Restore Dark theme default before exit
            themeEngine.setDarkTheme(true);
            themeEngine.setAccentFromHex(QStringLiteral("#3B9EFF"));
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
