// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "core/DiagnosticRedactor.h"

#include <QDateTime>
#include <QJsonArray>
#include <QMutexLocker>
#include <QRegularExpression>

namespace acc::core {

DiagnosticRedactor& DiagnosticRedactor::instance() {
    static DiagnosticRedactor s_instance;
    return s_instance;
}

void DiagnosticRedactor::appendLog(const QString& level, const QString& subsystem, const QString& message) {
    QMutexLocker locker(&m_mutex);
    DiagnosticLogEntry entry;
    entry.timestampIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    entry.level = level.trimmed().toUpper();
    entry.subsystem = subsystem.trimmed().toUpper();
    entry.message = redactSensitiveText(message);

    m_logs.push_back(entry);
    while (m_logs.size() > kMaxLogEntries) {
        m_logs.pop_front();
    }
}

std::vector<DiagnosticLogEntry> DiagnosticRedactor::recentLogs() const {
    QMutexLocker locker(&m_mutex);
    return std::vector<DiagnosticLogEntry>(m_logs.begin(), m_logs.end());
}

void DiagnosticRedactor::clearLogs() {
    QMutexLocker locker(&m_mutex);
    m_logs.clear();
}

QString DiagnosticRedactor::redactSensitiveText(const QString& input) {
    QString out = input;

    // 1. Redact standard UUIDs (but keep known public WMI class GUIDs ABBC0F6x, F6CB5C3C, 676AA15E, 6AF4F258, 67C3371D)
    static const QRegularExpression kUuidRegex(
        QStringLiteral("\\b(?!ABBC0F6[A-F]-8EA1-11D1-00A0-C90629100000|F6CB5C3C-9CAE-4EBD-B577-931EA32A2CC0|676AA15E-6A47-4D9F-A2CC-1E6D18D14026|6AF4F258-B401-42FD-BE91-3D4AC2D7C0D3|67C3371D-95A3-4C37-BB61-DD47B491DAAB)[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\b"),
        QRegularExpression::CaseInsensitiveOption
    );
    out.replace(kUuidRegex, QStringLiteral("<REDACTED-UUID>"));

    // 2. Redact MAC addresses
    static const QRegularExpression kMacRegex(
        QStringLiteral("\\b(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}\\b")
    );
    out.replace(kMacRegex, QStringLiteral("<REDACTED-MAC>"));

    // 3. Redact serial_number=... or serial: ...
    static const QRegularExpression kSerialRegex(
        QStringLiteral("(?i)(serial[_\\s-]*number|product[_\\s-]*serial|board[_\\s-]*serial|uuid)\\s*[:=]\\s*[^\\s,;]+")
    );
    out.replace(kSerialRegex, QStringLiteral("\\1=<REDACTED>"));

    // 4. Redact /home/<username>
    static const QRegularExpression kHomeRegex(
        QStringLiteral("/home/[A-Za-z0-9._-]+")
    );
    out.replace(kHomeRegex, QStringLiteral("/home/<user>"));

    return out;
}

QJsonObject DiagnosticRedactor::buildJsonReport(const SystemSnapshot& snapshot) const {
    QJsonObject root;
    root.insert(QStringLiteral("generated_at_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("mock_mode_active"), snapshot.mockModeActive);
    root.insert(QStringLiteral("mock_scenario"), snapshot.mockScenarioName);

    QJsonObject dmiObj;
    dmiObj.insert(QStringLiteral("sys_vendor"), snapshot.dmi.sysVendor);
    dmiObj.insert(QStringLiteral("product_name"), snapshot.dmi.productName);
    dmiObj.insert(QStringLiteral("product_version"), snapshot.dmi.productVersion);
    dmiObj.insert(QStringLiteral("board_name"), snapshot.dmi.boardName);
    dmiObj.insert(QStringLiteral("board_vendor"), snapshot.dmi.boardVendor);
    dmiObj.insert(QStringLiteral("bios_vendor"), snapshot.dmi.biosVendor);
    dmiObj.insert(QStringLiteral("bios_version"), snapshot.dmi.biosVersion);
    dmiObj.insert(QStringLiteral("bios_date"), snapshot.dmi.biosDate);
    dmiObj.insert(QStringLiteral("acer_wmi_loaded"), snapshot.dmi.acerWmiLoaded);
    QJsonArray wmiArr;
    for (const QString& g : snapshot.dmi.wmiGuids) {
        wmiArr.append(g);
    }
    dmiObj.insert(QStringLiteral("wmi_guids"), wmiArr);
    root.insert(QStringLiteral("dmi_identity"), dmiObj);

    QJsonArray capArr;
    for (const CapabilityRecord& cap : snapshot.capabilities) {
        capArr.append(cap.toJson());
    }
    root.insert(QStringLiteral("capabilities"), capArr);

    QJsonArray logsArr;
    for (const DiagnosticLogEntry& entry : recentLogs()) {
        QJsonObject lObj;
        lObj.insert(QStringLiteral("timestamp"), entry.timestampIso);
        lObj.insert(QStringLiteral("level"), entry.level);
        lObj.insert(QStringLiteral("subsystem"), entry.subsystem);
        lObj.insert(QStringLiteral("message"), entry.message);
        logsArr.append(lObj);
    }
    root.insert(QStringLiteral("recent_logs"), logsArr);
    return root;
}

QString DiagnosticRedactor::buildMarkdownReport(const SystemSnapshot& snapshot) const {
    QString md;
    md += QStringLiteral("# Acer Control Center Linux Edition — Hardware Diagnostic Report\n\n");
    md += QStringLiteral("- **Generated (UTC):** `%1`\n").arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    md += QStringLiteral("- **Operating Mode:** `%1`\n\n")
              .arg(snapshot.mockModeActive
                       ? QStringLiteral("MOCK HARDWARE FIXTURE (%1)").arg(snapshot.mockScenarioName)
                       : QStringLiteral("PHYSICAL HARDWARE (READ-VERIFIED)"));

    md += QStringLiteral("## 1. Redacted Platform Identity\n\n");
    md += QStringLiteral("| Field | Value |\n|---|---|\n");
    md += QStringLiteral("| Vendor | `%1` |\n").arg(snapshot.dmi.sysVendor);
    md += QStringLiteral("| Product | `%1` (`%2`) |\n").arg(snapshot.dmi.productName, snapshot.dmi.productVersion);
    md += QStringLiteral("| Board | `%1` (`%2`) |\n").arg(snapshot.dmi.boardName, snapshot.dmi.boardVendor);
    md += QStringLiteral("| BIOS | `%1` `%2` (`%3`) |\n").arg(snapshot.dmi.biosVendor, snapshot.dmi.biosVersion, snapshot.dmi.biosDate);
    md += QStringLiteral("| `acer_wmi` Loaded | `%1` |\n").arg(snapshot.dmi.acerWmiLoaded ? QStringLiteral("Yes") : QStringLiteral("No"));
    md += QStringLiteral("| Exposed WMI GUIDs | `%1` |\n\n").arg(snapshot.dmi.wmiGuids.join(QStringLiteral(", ")));

    md += QStringLiteral("## 2. Hardware Capability Matrix\n\n");
    md += QStringLiteral("| Capability | Category | State | Writable | Interface |\n|---|---|---|---|---|\n");
    for (const CapabilityRecord& cap : snapshot.capabilities) {
        md += QStringLiteral("| %1 | %2 | `%3` | `%4` | `%5` |\n")
                  .arg(cap.title,
                       cap.category,
                       capabilityStateToBadge(cap.state),
                       cap.isWritable ? QStringLiteral("Yes") : QStringLiteral("No"),
                       cap.interfacePath);
    }

    md += QStringLiteral("\n## 3. Recent Redacted Application Logs\n\n```text\n");
    for (const DiagnosticLogEntry& entry : recentLogs()) {
        md += QStringLiteral("[%1] [%2] [%3] %4\n")
                  .arg(entry.timestampIso, entry.level, entry.subsystem, entry.message);
    }
    md += QStringLiteral("```\n");

    return redactSensitiveText(md);
}

} // namespace acc::core
