// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string text: "ACTION"
    property bool activeState: false
    property bool primaryAccent: false
    property bool enabledState: true
    signal clicked()

    implicitWidth: Math.max(88, labelItem.implicitWidth + 24)
    implicitHeight: 32
    radius: 2
    clip: true
    activeFocusOnTab: root.enabledState

    color: {
        if (!root.enabledState) return theme.surfaceHeader;
        if (root.primaryAccent) return mouseArea.containsMouse ? Qt.lighter(theme.accentPrimary, 1.08) : theme.accentPrimary;
        if (root.activeState) return theme.accentSurfaceTint;
        return mouseArea.containsMouse ? theme.surfaceElevated : theme.surfacePrimary;
    }

    border.width: root.activeFocus ? 2 : 1
    border.color: {
        if (!root.enabledState) return theme.borderSubtle;
        if (root.activeFocus || root.activeState || root.primaryAccent) return theme.accentBorder;
        return mouseArea.containsMouse ? theme.borderStrong : theme.borderSubtle;
    }

    Behavior on color {
        ColorAnimation { duration: theme.transitionDurationMs }
    }

    Text {
        id: labelItem
        anchors.centerIn: parent
        width: Math.min(implicitWidth, Math.max(24, root.width - 12))
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: root.text
        font.family: "sans-serif"
        font.pixelSize: 12
        font.weight: (root.primaryAccent || root.activeState) ? Font.DemiBold : Font.Medium
        color: {
            if (!root.enabledState) return theme.textMuted;
            if (root.primaryAccent) return theme.accentOnText;
            if (root.activeState) return theme.accentReadable;
            return theme.textPrimary;
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.enabledState
        enabled: root.enabledState
        cursorShape: root.enabledState ? Qt.PointingHandCursor : Qt.ForbiddenCursor
        onClicked: {
            root.forceActiveFocus();
            root.clicked();
        }
    }

    Keys.onReturnPressed: {
        if (root.enabledState) root.clicked();
    }
    Keys.onSpacePressed: {
        if (root.enabledState) root.clicked();
    }
}
