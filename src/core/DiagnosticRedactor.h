// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/DomainModels.h"
#include <QJsonObject>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <deque>

namespace acc::core {

struct DiagnosticLogEntry {
    QString timestampIso;
    QString level;     // "INFO", "WARN", "ERROR"
    QString subsystem; // "HAL", "DAEMON", "NVML", "POWER", "UI"
    QString message;
};

class DiagnosticRedactor {
public:
    static DiagnosticRedactor& instance();

    void appendLog(const QString& level, const QString& subsystem, const QString& message);
    std::vector<DiagnosticLogEntry> recentLogs() const;
    void clearLogs();

    // Strips UUIDs, MAC addresses, serial_number patterns, and /home/<user> paths
    static QString redactSensitiveText(const QString& input);

    QString buildMarkdownReport(const SystemSnapshot& snapshot) const;
    QJsonObject buildJsonReport(const SystemSnapshot& snapshot) const;

private:
    DiagnosticRedactor() = default;
    mutable QMutex m_mutex;
    std::deque<DiagnosticLogEntry> m_logs;
    static constexpr size_t kMaxLogEntries = 300;
};

} // namespace acc::core
