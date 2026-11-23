// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string text: "ACTION"
    property bool activeState: false
    property bool primaryAccent: false
    property bool enabledState: true
    property bool pillShape: true
    signal clicked()

    implicitWidth: Math.max(88, labelItem.implicitWidth + 28)
    implicitHeight: 32
    radius: pillShape ? (height / 2) : theme.radiusStructural
    clip: true
    activeFocusOnTab: root.enabledState
    opacity: root.enabledState ? 1.0 : 0.55

    color: {
        if (!root.enabledState) return theme.surfaceHeader;
        if (root.primaryAccent) return mouseArea.containsMouse ? Qt.lighter(theme.accentPrimary, 1.08) : theme.accentPrimary;
        if (root.activeState) {
            if (!pillShape && !theme.darkTheme) return theme.surfaceSelectedDark;
            return theme.accentSurfaceTint;
        }
        if (pillShape && !theme.darkTheme) {
            return mouseArea.containsMouse ? theme.surfaceElevated : theme.surfacePillLight;
        }
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

    // Active status indicator marker (for tactical square pills & dark/light consistency)
    Rectangle {
        visible: root.activeState && !root.primaryAccent && !root.pillShape
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: theme.brandYellow
    }

    Text {
        id: labelItem
        anchors.centerIn: parent
        width: Math.min(implicitWidth, Math.max(24, root.width - 16))
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: (root.activeState && !root.primaryAccent ? "● " : "") + root.text
        font.family: theme.fontSans
        font.pixelSize: 12
        font.weight: (root.primaryAccent || root.activeState) ? Font.Bold : Font.Medium
        color: {
            if (!root.enabledState) return theme.textMuted;
            if (root.primaryAccent) return theme.accentOnText;
            if (root.activeState) {
                if (!pillShape && !theme.darkTheme) return theme.brandYellow;
                return theme.accentReadable;
            }
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
