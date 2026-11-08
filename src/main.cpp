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
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTextStream>
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

    parser.addOption(mockModeOpt);
    parser.addOption(captureScreenshotsOpt);
    parser.addOption(allowRootTestOpt);
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
