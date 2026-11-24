// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/ControlCenterController.h"
#include "app/ThemeEngine.h"
#include "hal/Clv0001KeyboardBackend.h"
#include "hal/HardwareDiscovery.h"
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
#include <QPainter>
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
    parser.setApplicationDescription(QStringLiteral("Acer Control Center — Linux Edition (Industrial-Technical Hardware Command Center)"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption mockModeOpt(
        QStringLiteral("mock-hardware"),
        QStringLiteral("Launch with simulated hardware fixtures (mirror, clv0001, or full)."),
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
        } else if (scen == QLatin1String("clv0001")) {
            acc::hal::SysfsRoot::instance().setMode(acc::hal::HardwareMode::MockClv0001DriverActive);
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
        // Wait cleanly for initial async telemetry + second CPU/RAPL delta cycle without blocking GUI mode
        controller.waitForNextSnapshot(3000);
        QThread::msleep(120);
        controller.waitForNextSnapshot(3000);

        QJsonObject root;
        root.insert(QStringLiteral("hardwareMode"), controller.hardwareModeLabel());
        root.insert(QStringLiteral("mockActive"), controller.mockModeActive());
        root.insert(QStringLiteral("productTitle"), controller.productTitle());
        root.insert(QStringLiteral("boardTitle"), controller.boardTitle());
        root.insert(QStringLiteral("biosTitle"), controller.biosTitle());
        root.insert(QStringLiteral("odmPlatformNote"), controller.odmPlatformNote());
        root.insert(QStringLiteral("wmiGuids"), controller.wmiGuidsJoined());
        root.insert(QStringLiteral("kernelRelease"), controller.kernelRelease());
        root.insert(QStringLiteral("acpiDevices"), controller.acpiDevicesJoined());
        root.insert(QStringLiteral("loadedDrivers"), controller.loadedDriversJoined());
        root.insert(QStringLiteral("daemonReachable"), controller.daemonReachable());
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
        gpuObj.insert(QStringLiteral("telemetryMeasuredByNvml"), controller.gpuTelemetryMeasuredByNvml());
        gpuObj.insert(QStringLiteral("powerLimitsMeasured"), controller.gpuPowerLimitsMeasured());
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
        batObj.insert(QStringLiteral("cycleCount"), controller.batCycleCount());
        batObj.insert(QStringLiteral("cycleCountReported"), controller.batCycleCountReported());
        batObj.insert(QStringLiteral("cycleCountLabel"), controller.batCycleCountText());
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

        QJsonObject kbdObj;
        const auto clvTopo = acc::hal::Clv0001KeyboardBackend::probeTopology(
            acc::hal::HardwareDiscovery::probeDmiAndWmi()
        );
        kbdObj.insert(QStringLiteral("backendKind"), controller.kbdBackendKindId());
        kbdObj.insert(QStringLiteral("controlModeBadge"), controller.kbdControlModeBadge());
        kbdObj.insert(QStringLiteral("hardwareConnected"), controller.kbdHardwareConnected());
        kbdObj.insert(QStringLiteral("supportsPresetColors"), controller.kbdSupportsPresetColors());
        kbdObj.insert(QStringLiteral("supportsArbitraryRgb"), controller.kbdSupportsArbitraryRgb());
        kbdObj.insert(QStringLiteral("physicalReadbackSupported"), controller.kbdPhysicalReadbackSupported());
        kbdObj.insert(QStringLiteral("physicalLedStateVerified"), controller.kbdPhysicalReadbackSupported());
        kbdObj.insert(QStringLiteral("physicalLedStateStatus"),
                      controller.kbdPhysicalReadbackSupported()
                          ? QStringLiteral("verified_sysfs_readback")
                          : QStringLiteral("unsupported_no_hw_readback"));
        kbdObj.insert(QStringLiteral("driverAcceptedStatus"), controller.kbdDriverLastStatus());
        kbdObj.insert(QStringLiteral("driverAcceptedPreset"), controller.kbdDriverLastPreset());
        kbdObj.insert(QStringLiteral("driverAcceptedBrightnessLevel"), controller.kbdDriverLastBrightnessLevel());
        kbdObj.insert(QStringLiteral("driverAcceptedPayloadHex"), controller.kbdDriverLastPayloadHex());
        kbdObj.insert(QStringLiteral("driverAcceptedAcpiReturnType"), controller.kbdDriverLastAcpiReturnType());
        kbdObj.insert(QStringLiteral("requestedActiveProfileId"), controller.activeKbdProfileId());
        kbdObj.insert(QStringLiteral("requestedEnabled"), controller.kbdEnabled());
        kbdObj.insert(QStringLiteral("requestedPresetName"), controller.kbdPresetName());
        kbdObj.insert(QStringLiteral("requestedBrightnessLevel"), controller.kbdBrightnessLevel());
        kbdObj.insert(QStringLiteral("requestedPacketHex"), controller.kbdFirmwarePacketHex());
        kbdObj.insert(QStringLiteral("clv0001DevicePresent"), clvTopo.devicePresent);
        kbdObj.insert(QStringLiteral("clv0001AcpiPath"), clvTopo.acpiPath);
        kbdObj.insert(QStringLiteral("clv0001Status"), clvTopo.status);
        kbdObj.insert(QStringLiteral("clv0001DriverBound"), clvTopo.driverBound);
        kbdObj.insert(QStringLiteral("clv0001DmiEligible"), clvTopo.dmiEligible);
        kbdObj.insert(QStringLiteral("clv0001LastStatus"), controller.kbdDriverLastStatus());
        kbdObj.insert(QStringLiteral("clv0001LastAcpiReturnType"), controller.kbdDriverLastAcpiReturnType());
        root.insert(QStringLiteral("keyboard"), kbdObj);

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
        themeEngine.setAccentFromHex(QStringLiteral("#FFEA00"));

        QTimer::singleShot(250, &app, [dir, viewNames, topWin, &controller, &themeEngine]() {
            controller.waitForNextSnapshot(3000);
            QStringList canonicalShots;
            auto captureView = [&](const QString& fileName, bool recordInSheet = false) {
                QCoreApplication::processEvents();
                QCoreApplication::processEvents();
                topWin->grabWindow().save(dir.filePath(fileName));
                if (recordInSheet) {
                    canonicalShots.append(fileName);
                }
            };

            topWin->resize(1920, 1080);
            for (int i = 0; i < viewNames.size(); ++i) {
                controller.setCurrentTabIndex(i);
                captureView(viewNames.at(i) + QStringLiteral("-dark.png"), true);
            }

            // Capture full-resolution (1920x1080) Light CAD Schematic ("#D8DCDF") for all 8 views + canonical Light captures
            themeEngine.setDarkTheme(false);
            themeEngine.setAccentFromHex(QStringLiteral("#FFEA00"));
            for (int i = 0; i < viewNames.size(); ++i) {
                controller.setCurrentTabIndex(i);
                captureView(viewNames.at(i) + QStringLiteral("-light.png"), false);
            }

            controller.setCurrentTabIndex(0);
            captureView(QStringLiteral("09-overview-light-custom-accent.png"), true);

            controller.setCurrentTabIndex(3);
            captureView(QStringLiteral("10-keyboard-studio-light.png"), true);

            controller.setCurrentTabIndex(7);
            captureView(QStringLiteral("11-settings-light-wcag.png"), true);

            // Multi-resolution & state QA captures (1366x768, 1600x900, 1920x1080, 2560x1440)
            themeEngine.setDarkTheme(true);
            themeEngine.setAccentFromHex(QStringLiteral("#FFEA00"));

            topWin->resize(1366, 768);
            controller.setCurrentTabIndex(0);
            captureView(QStringLiteral("12-overview-1366x768.png"), true);

            topWin->resize(1600, 900);
            controller.setCurrentTabIndex(2);
            captureView(QStringLiteral("13-thermal-1600x900.png"), true);

            topWin->resize(1920, 1080);
            controller.setCurrentTabIndex(4);
            captureView(QStringLiteral("14-battery-1920x1080.png"), true);

            topWin->resize(2560, 1440);
            controller.setCurrentTabIndex(6);
            captureView(QStringLiteral("15-diagnostics-2560x1440.png"), true);

            topWin->resize(1920, 1080);
            controller.setCurrentTabIndex(3);
            controller.setKbdColorFromHex(QStringLiteral("#12A7E3"));
            controller.setKbdBrightnessLevel(4);
            captureView(QStringLiteral("16-keyboard-clv0001-rgb-1920x1080.png"), true);

            // Narrow tiled layout check (800x720 half-screen Hyprland/KDE tile)
            topWin->resize(800, 720);
            controller.setCurrentTabIndex(0);
            captureView(QStringLiteral("17-overview-tiled-800x720.png"), false);

            // Generate 4x4 readable contact sheet containing all 16 canonical QA images
            constexpr int kSheetW = 3200;
            constexpr int kSheetH = 2048;
            constexpr int kCols = 4;
            constexpr int kRows = 4;
            constexpr int kOuterPad = 24;
            constexpr int kHeaderH = 44;
            constexpr int kGap = 18;
            const int cellW = (kSheetW - 2 * kOuterPad - (kCols - 1) * kGap) / kCols;
            const int cellH = (kSheetH - 2 * kOuterPad - kHeaderH - (kRows - 1) * kGap) / kRows;

            QImage sheet(kSheetW, kSheetH, QImage::Format_ARGB32);
            sheet.fill(QColor(QStringLiteral("#161818")));
            QPainter sp(&sheet);
            sp.setRenderHint(QPainter::Antialiasing, true);
            sp.setRenderHint(QPainter::SmoothPixmapTransform, true);

            QFont hdrFont(QStringLiteral("JetBrains Mono"), 13, QFont::Bold);
            sp.setFont(hdrFont);
            sp.setPen(QColor(QStringLiteral("#FFEA00")));
            sp.drawText(QRect(kOuterPad, kOuterPad, kSheetW - 2 * kOuterPad, 30),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        QStringLiteral("ACER CONTROL CENTER // STAGE 25 ENDFIELD-INSPIRED STRUCTURAL UI CONTACT SHEET (16 CANONICAL VIEWS)"));

            QFont labelFont(QStringLiteral("JetBrains Mono"), 10, QFont::Bold);
            sp.setFont(labelFont);
            for (int idx = 0; idx < canonicalShots.size() && idx < kCols * kRows; ++idx) {
                const int col = idx % kCols;
                const int row = idx / kCols;
                const int cx = kOuterPad + col * (cellW + kGap);
                const int cy = kOuterPad + kHeaderH + row * (cellH + kGap);
                const QRect cellRect(cx, cy, cellW, cellH);

                sp.fillRect(cellRect, QColor(QStringLiteral("#222524")));
                sp.fillRect(QRect(cx, cy, cellW, 26), QColor(QStringLiteral("#1B1E1D")));

                const QRect badgeRect(cx + 6, cy + 4, 28, 18);
                sp.fillRect(badgeRect, QColor(QStringLiteral("#FFEA00")));
                sp.setPen(QColor(QStringLiteral("#141618")));
                sp.drawText(badgeRect, Qt::AlignCenter, QStringLiteral("%1").arg(idx + 1, 2, 10, QLatin1Char('0')));

                sp.setPen(QColor(QStringLiteral("#F0F2F2")));
                sp.drawText(QRect(cx + 42, cy + 2, cellW - 48, 22),
                            Qt::AlignLeft | Qt::AlignVCenter,
                            canonicalShots.at(idx));

                const QImage thumb(dir.filePath(canonicalShots.at(idx)));
                if (!thumb.isNull()) {
                    const QRect imgTarget(cx + 4, cy + 30, cellW - 8, cellH - 34);
                    const QSize scaled = thumb.size().scaled(imgTarget.size(), Qt::KeepAspectRatio);
                    const QRect aligned(
                        imgTarget.x() + (imgTarget.width() - scaled.width()) / 2,
                        imgTarget.y() + (imgTarget.height() - scaled.height()) / 2,
                        scaled.width(),
                        scaled.height()
                    );
                    sp.drawImage(aligned, thumb);
                }

                sp.setPen(QPen(QColor(QStringLiteral("#3E4443")), 1));
                sp.drawRect(cellRect.adjusted(0, 0, -1, -1));
            }
            sp.end();
            sheet.save(dir.filePath(QStringLiteral("00-contact-sheet-stage21.png")));

            QCoreApplication::quit();
        });
    }

    return app.exec();
}
