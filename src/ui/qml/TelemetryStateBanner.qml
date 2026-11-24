// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    objectName: "telemetryStateBanner"

    readonly property bool hasAuthAlert: ctrl.controlOperationPending
                                         || ctrl.controlAuthState === "denied"
                                         || ctrl.controlAuthState === "daemon_unavailable"
                                         || ctrl.controlAuthState === "failed"
    readonly property bool hasTelemetryAlert: !ctrl.telemetryAvailable || ctrl.telemetryStale
    readonly property bool activeVisible: hasAuthAlert || hasTelemetryAlert

    readonly property color stateColor: {
        if (ctrl.controlAuthState === "denied" || ctrl.controlAuthState === "failed")
            return theme.statusCritical;
        if (ctrl.controlOperationPending)
            return theme.industrialAmber;
        if (ctrl.controlAuthState === "daemon_unavailable" || ctrl.telemetryStale)
            return theme.statusWarning;
        return theme.statusInfo;
    }

    readonly property string stateBadge: {
        if (ctrl.controlOperationPending) return "POLKIT // AUTH PENDING";
        if (ctrl.controlAuthState === "denied") return "AUTH // POLKIT DENIED";
        if (ctrl.controlAuthState === "daemon_unavailable") return "DAEMON // UNREACHABLE";
        if (ctrl.controlAuthState === "failed") return "CONTROL // HW FAILED";
        if (!ctrl.telemetryAvailable) return "TELEMETRY // INITIALIZING";
        if (ctrl.telemetrySuspended) return "SYSTEM // SUSPEND STALE";
        if (ctrl.telemetryStale) return "TELEMETRY // STALE";
        return "NOMINAL";
    }

    readonly property string detailMessage: {
        if (ctrl.controlOperationPending)
            return "Awaiting asynchronous Polkit / D-Bus authorization for [" + ctrl.controlOperationName + "]. UI remains non-blocking.";
        if (ctrl.controlAuthState === "denied")
            return "Authorization denied for [" + ctrl.controlOperationName + "] (" + ctrl.controlLastErrorCode + "): " + ctrl.controlLastErrorMessage;
        if (ctrl.controlAuthState === "daemon_unavailable")
            return "Privileged helper unavailable (" + ctrl.controlLastErrorCode + "): " + ctrl.controlLastErrorMessage;
        if (ctrl.controlAuthState === "failed")
            return "Control operation failed (" + ctrl.controlLastErrorCode + "): " + ctrl.controlLastErrorMessage;
        if (!ctrl.telemetryAvailable)
            return ctrl.telemetryRefreshing
                   ? "Async worker collecting initial hardware telemetry snapshot..."
                   : (ctrl.telemetryErrorMessage.length > 0 ? ctrl.telemetryErrorMessage : "Waiting for initial telemetry snapshot.");
        if (ctrl.telemetrySuspended)
            return "System suspended; telemetry marked stale until resume refresh completes.";
        if (ctrl.telemetryStale) {
            var ageSec = (ctrl.telemetryAgeMs / 1000.0).toFixed(1);
            var errPart = ctrl.telemetryErrorMessage.length > 0 ? (" — " + ctrl.telemetryErrorMessage) : "";
            return "Displayed readings are " + ageSec + "s old and may not reflect live hardware state" + errPart;
        }
        return "";
    }

    visible: activeVisible
    height: visible ? Math.max(34, bannerRow.implicitHeight + 12) : 0
    radius: theme.radiusStructural
    color: Qt.rgba(stateColor.r, stateColor.g, stateColor.b, theme.darkTheme ? 0.14 : 0.11)
    border.width: 1
    border.color: stateColor
    Accessible.role: Accessible.StaticText
    Accessible.name: root.stateBadge + ": " + root.detailMessage

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        color: root.stateColor
    }

    Row {
        id: bannerRow
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: syncAction.visible ? syncAction.left : parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Rectangle {
            width: tagLbl.implicitWidth + 12
            height: 20
            anchors.verticalCenter: parent.verticalCenter
            radius: theme.radiusStructural
            color: root.stateColor
            Text {
                id: tagLbl
                anchors.centerIn: parent
                text: root.stateBadge
                font.family: theme.fontMono
                font.pixelSize: 10
                font.weight: Font.Bold
                color: theme.darkTheme ? theme.darkInk : "#FFFFFF"
            }
        }

        Text {
            width: Math.max(80, parent.width - tagLbl.implicitWidth - 26)
            anchors.verticalCenter: parent.verticalCenter
            wrapMode: Text.WordWrap
            text: root.detailMessage
            font.family: theme.fontMono
            font.pixelSize: 10
            color: theme.textPrimary
        }
    }

    AtelierButton {
        id: syncAction
        visible: root.hasTelemetryAlert && !ctrl.controlOperationPending
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        height: 24
        pillShape: false
        text: ctrl.telemetryRefreshing ? "SYNCING..." : "RETRY SYNC"
        enabledState: !ctrl.telemetryRefreshing
        onClicked: ctrl.refreshNow()
    }
}
