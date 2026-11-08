// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QString>

namespace acc::daemon {

enum class AuthPolicyMode {
    SystemPolkit,
    TestAllowAll,
    TestDenyAll
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

private:
    AuthPolicyMode m_mode = AuthPolicyMode::SystemPolkit;
};

} // namespace acc::daemon
