// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QLatin1StringView>
#include <QString>
#include <QStringList>
#include <vector>

namespace acc::daemon {

namespace PolkitActions {

inline constexpr QLatin1StringView kReadRapl{
    "io.github.zethasaintlab.acercontrol.read-rapl"
};
inline constexpr QLatin1StringView kSetPowerProfile{
    "io.github.zethasaintlab.acercontrol.set-power-profile"
};
inline constexpr QLatin1StringView kSetKeyboardLighting{
    "io.github.zethasaintlab.acercontrol.set-keyboard-lighting"
};
inline constexpr QLatin1StringView kSetBatteryLimit{
    "io.github.zethasaintlab.acercontrol.set-battery-limit"
};
inline constexpr QLatin1StringView kSetFanMode{
    "io.github.zethasaintlab.acercontrol.set-fan-mode"
};

inline QStringList allActionIds() {
    return {
        QString(kReadRapl),
        QString(kSetPowerProfile),
        QString(kSetKeyboardLighting),
        QString(kSetBatteryLimit),
        QString(kSetFanMode)
    };
}

inline bool isRegisteredActionId(const QString& actionId) {
    return allActionIds().contains(actionId);
}

} // namespace PolkitActions

enum class AuthPolicyMode {
    SystemPolkit,
    TestAllowAll,
    TestDenyAll
};

struct AuthCheckRecord {
    QString callerBusName;
    QString actionId;
    bool allowUserInteraction = false;
    bool authorized = false;
};

class PolkitAuthorizer {
public:
    explicit PolkitAuthorizer(AuthPolicyMode mode = AuthPolicyMode::SystemPolkit);

    void setMode(AuthPolicyMode mode);
    AuthPolicyMode mode() const;

    bool checkCallerAuthorized(
        const QString& callerBusName,
        const QString& actionId,
        bool allowUserInteraction,
        QString* outReason = nullptr
    ) const;

    const std::vector<AuthCheckRecord>& auditTrail() const { return m_auditTrail; }
    void clearAuditTrail() { m_auditTrail.clear(); }

private:
    AuthPolicyMode m_mode = AuthPolicyMode::SystemPolkit;
    mutable std::vector<AuthCheckRecord> m_auditTrail;
};

} // namespace acc::daemon
