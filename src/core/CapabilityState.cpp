// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "core/CapabilityState.h"

namespace acc::core {

QString capabilityStateToId(CapabilityState state) {
    switch (state) {
        case CapabilityState::FullySupported:
            return QStringLiteral("fully_supported");
        case CapabilityState::ReadOnly:
            return QStringLiteral("read_only");
        case CapabilityState::PartiallySupported:
            return QStringLiteral("partially_supported");
        case CapabilityState::Unsupported:
            return QStringLiteral("unsupported");
        case CapabilityState::NotDetected:
            return QStringLiteral("not_detected");
        case CapabilityState::FirmwareAutomatic:
            return QStringLiteral("firmware_automatic");
        case CapabilityState::PreviewOnly:
            return QStringLiteral("preview_only");
        case CapabilityState::Simulated:
            return QStringLiteral("simulated");
        case CapabilityState::RequiresVerification:
            return QStringLiteral("requires_verification");
        case CapabilityState::Error:
            return QStringLiteral("error");
    }
    return QStringLiteral("not_detected");
}

QString capabilityStateToLabel(CapabilityState state) {
    switch (state) {
        case CapabilityState::FullySupported:
            return QStringLiteral("Fully Supported (Read / Verified Write)");
        case CapabilityState::ReadOnly:
            return QStringLiteral("Read-Only Supported");
        case CapabilityState::PartiallySupported:
            return QStringLiteral("Partially Supported");
        case CapabilityState::Unsupported:
            return QStringLiteral("Unavailable / Unsupported by Hardware");
        case CapabilityState::NotDetected:
            return QStringLiteral("Not Detected in Kernel / Sysfs");
        case CapabilityState::FirmwareAutomatic:
            return QStringLiteral("Firmware Automatic (Hardware EC Managed)");
        case CapabilityState::PreviewOnly:
            return QStringLiteral("Simulation / Preview Only");
        case CapabilityState::Simulated:
            return QStringLiteral("Simulated (Mock Hardware Mode)");
        case CapabilityState::RequiresVerification:
            return QStringLiteral("Requires Firmware Verification");
        case CapabilityState::Error:
            return QStringLiteral("Hardware / Interface Error");
    }
    return QStringLiteral("Unknown");
}

QString capabilityStateToBadge(CapabilityState state) {
    switch (state) {
        case CapabilityState::FullySupported:
            return QStringLiteral("FULL RW");
        case CapabilityState::ReadOnly:
            return QStringLiteral("READ-ONLY");
        case CapabilityState::PartiallySupported:
            return QStringLiteral("PARTIAL");
        case CapabilityState::Unsupported:
            return QStringLiteral("UNAVAILABLE");
        case CapabilityState::NotDetected:
            return QStringLiteral("NOT DETECTED");
        case CapabilityState::FirmwareAutomatic:
            return QStringLiteral("FIRMWARE AUTOMATIC");
        case CapabilityState::PreviewOnly:
            return QStringLiteral("PREVIEW ONLY");
        case CapabilityState::Simulated:
            return QStringLiteral("SIMULATED");
        case CapabilityState::RequiresVerification:
            return QStringLiteral("UNVERIFIED");
        case CapabilityState::Error:
            return QStringLiteral("ERROR");
    }
    return QStringLiteral("UNKNOWN");
}

CapabilityState capabilityStateFromId(const QString& id) {
    const QString norm = id.trimmed().toLower();
    if (norm == QLatin1String("fully_supported")) {
        return CapabilityState::FullySupported;
    }
    if (norm == QLatin1String("read_only")) {
        return CapabilityState::ReadOnly;
    }
    if (norm == QLatin1String("partially_supported")) {
        return CapabilityState::PartiallySupported;
    }
    if (norm == QLatin1String("unsupported") || norm == QLatin1String("unavailable")) {
        return CapabilityState::Unsupported;
    }
    if (norm == QLatin1String("firmware_automatic")) {
        return CapabilityState::FirmwareAutomatic;
    }
    if (norm == QLatin1String("preview_only")) {
        return CapabilityState::PreviewOnly;
    }
    if (norm == QLatin1String("simulated")) {
        return CapabilityState::Simulated;
    }
    if (norm == QLatin1String("requires_verification")) {
        return CapabilityState::RequiresVerification;
    }
    if (norm == QLatin1String("error")) {
        return CapabilityState::Error;
    }
    return CapabilityState::NotDetected;
}

QJsonObject CapabilityRecord::toJson() const {
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), id);
    obj.insert(QStringLiteral("title"), title);
    obj.insert(QStringLiteral("category"), category);
    obj.insert(QStringLiteral("interface_path"), interfacePath);
    obj.insert(QStringLiteral("state_id"), capabilityStateToId(state));
    obj.insert(QStringLiteral("state_label"), capabilityStateToLabel(state));
    obj.insert(QStringLiteral("state_badge"), capabilityStateToBadge(state));
    obj.insert(QStringLiteral("is_writable"), isWritable);
    obj.insert(QStringLiteral("summary"), summary);
    obj.insert(QStringLiteral("detail_explanation"), detailExplanation);
    return obj;
}

} // namespace acc::core
