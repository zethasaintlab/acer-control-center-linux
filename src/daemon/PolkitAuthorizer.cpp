// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "daemon/PolkitAuthorizer.h"

#undef signals
#include <polkit/polkit.h>
#define signals Q_SIGNALS

namespace acc::daemon {

PolkitAuthorizer::PolkitAuthorizer(AuthPolicyMode mode)
    : m_mode(mode) {}

void PolkitAuthorizer::setMode(AuthPolicyMode mode) {
    m_mode = mode;
}

AuthPolicyMode PolkitAuthorizer::mode() const {
    return m_mode;
}

bool PolkitAuthorizer::checkCallerAuthorized(
    const QString& callerBusName,
    const QString& actionId,
    bool allowUserInteraction,
    QString* outReason
) const {
    auto recordDecision = [&](bool auth) -> bool {
        if (m_auditTrail.size() >= 256) {
            m_auditTrail.erase(m_auditTrail.begin());
        }
        m_auditTrail.push_back(AuthCheckRecord{
            callerBusName,
            actionId,
            allowUserInteraction,
            auth
        });
        return auth;
    };

    // Fail closed if an unregistered Polkit action ID is ever requested.
    if (!PolkitActions::isRegisteredActionId(actionId)) {
        if (outReason) {
            *outReason = QStringLiteral("Rejected unregistered Polkit action ID '%1'.").arg(actionId);
        }
        return recordDecision(false);
    }

    if (m_mode == AuthPolicyMode::TestAllowAll) {
        if (outReason) {
            *outReason = QStringLiteral("Authorized by TestAllowAll policy for '%1'.").arg(actionId);
        }
        return recordDecision(true);
    }
    if (m_mode == AuthPolicyMode::TestDenyAll) {
        if (outReason) {
            *outReason = QStringLiteral("Denied by TestDenyAll policy for action '%1'.").arg(actionId);
        }
        return recordDecision(false);
    }

    if (callerBusName.trimmed().isEmpty() || callerBusName == QLatin1String(":local.inproc")) {
        if (outReason) {
            *outReason = QStringLiteral("Missing or non-D-Bus caller bus name ('%1').").arg(callerBusName);
        }
        return recordDecision(false);
    }

    GError* gerr = nullptr;
    PolkitAuthority* authority = polkit_authority_get_sync(nullptr, &gerr);
    if (!authority) {
        if (outReason) {
            *outReason = QStringLiteral("Failed to acquire PolkitAuthority: %1")
                             .arg(gerr ? QString::fromUtf8(gerr->message) : QStringLiteral("unknown"));
        }
        if (gerr) {
            g_error_free(gerr);
        }
        return recordDecision(false);
    }

    PolkitSubject* subject = polkit_system_bus_name_new(callerBusName.toUtf8().constData());
    const PolkitCheckAuthorizationFlags flags = allowUserInteraction
                                                    ? POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION
                                                    : POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE;

    PolkitAuthorizationResult* result = polkit_authority_check_authorization_sync(
        authority,
        subject,
        actionId.toUtf8().constData(),
        nullptr,
        flags,
        nullptr,
        &gerr
    );

    bool authorized = false;
    if (!result) {
        if (outReason) {
            *outReason = QStringLiteral("Polkit check error for '%1': %2")
                             .arg(actionId, gerr ? QString::fromUtf8(gerr->message) : QStringLiteral("unknown"));
        }
        if (gerr) {
            g_error_free(gerr);
        }
    } else {
        authorized = (polkit_authorization_result_get_is_authorized(result) != 0);
        if (!authorized && outReason) {
            *outReason = QStringLiteral("Polkit denied authorization for action '%1'.").arg(actionId);
        }
        g_object_unref(result);
    }

    g_object_unref(subject);
    g_object_unref(authority);
    return recordDecision(authorized);
}

} // namespace acc::daemon
