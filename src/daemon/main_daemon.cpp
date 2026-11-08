// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "daemon/DaemonServiceObject.h"
#include "hal/SysfsRoot.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QTextStream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("acer-control-daemon"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Privileged D-Bus Hardware Control Service for Acer Control Center Linux Edition"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption sessionMockOpt(
        QStringLiteral("session-mock"),
        QStringLiteral("Run on the D-Bus session bus using mock hardware fixtures (for unprivileged testing).")
    );
    parser.addOption(sessionMockOpt);

    parser.process(app);

    const bool useSessionMock = parser.isSet(sessionMockOpt);
    acc::daemon::AuthPolicyMode authMode = acc::daemon::AuthPolicyMode::SystemPolkit;
    if (useSessionMock) {
        authMode = acc::daemon::AuthPolicyMode::TestAllowAll;
        acc::hal::SysfsRoot::instance().setMode(acc::hal::HardwareMode::MockFullCapabilities);
    }

    acc::daemon::DaemonServiceObject serviceObj(authMode);

    QDBusConnection bus = useSessionMock ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        QTextStream(stderr) << "acer-control-daemon: Cannot connect to D-Bus ("
                            << bus.lastError().message() << ")\n";
        return 1;
    }

    const QString serviceName = QStringLiteral("io.github.zethasaintlab.AcerControlCenter1");
    const QString objectPath = QStringLiteral("/io/github/zethasaintlab/AcerControlCenter1");

    if (!bus.registerObject(objectPath, &serviceObj, QDBusConnection::ExportAllSlots)) {
        QTextStream(stderr) << "acer-control-daemon: Failed to register object path " << objectPath << "\n";
        return 2;
    }

    if (!bus.registerService(serviceName)) {
        QTextStream(stderr) << "acer-control-daemon: Failed to register service name " << serviceName
                            << " (" << bus.lastError().message() << ")\n";
        return 3;
    }

    return app.exec();
}
