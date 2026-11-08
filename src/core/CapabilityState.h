// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QJsonObject>
#include <QString>
#include <vector>

namespace acc::core {

enum class CapabilityState {
    FullySupported,
    ReadOnly,
    PartiallySupported,
    Unsupported,
    NotDetected,
    RequiresVerification
};

QString capabilityStateToId(CapabilityState state);
QString capabilityStateToLabel(CapabilityState state);
QString capabilityStateToBadge(CapabilityState state);
CapabilityState capabilityStateFromId(const QString& id);

struct CapabilityRecord {
    QString id;
    QString title;
    QString category;
    QString interfacePath;
    CapabilityState state = CapabilityState::NotDetected;
    bool isWritable = false;
    QString summary;
    QString detailExplanation;

    QJsonObject toJson() const;
};

} // namespace acc::core
