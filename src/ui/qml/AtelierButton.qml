// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string text: "ACTION"
    property bool activeState: false
    property bool primaryAccent: false
    property bool enabledState: true
    property bool pendingState: false
    property bool pillShape: true
    signal clicked()

    readonly property bool interactive: root.enabledState && !root.pendingState

    implicitWidth: Math.max(82, labelItem.implicitWidth + 26)
    implicitHeight: 32
    radius: pillShape ? (height / 2) : theme.radiusStructural
    clip: true
    activeFocusOnTab: root.interactive
    opacity: root.enabledState ? (root.pendingState ? 0.85 : 1.0) : 0.48
    Accessible.role: Accessible.Button
    Accessible.name: root.pendingState ? (root.text + " (pending)") : root.text
    Accessible.pressed: mouseArea.pressed
    Accessible.checkable: true
    Accessible.checked: root.activeState

    color: {
        if (!root.enabledState) return theme.surfaceHeader;
        if (root.pendingState) return theme.surfaceElevated;
        if (root.primaryAccent)
            return mouseArea.containsMouse ? Qt.lighter(theme.accentPrimary, 1.07) : theme.accentPrimary;
        if (root.activeState) {
            if (!pillShape) return theme.darkTheme ? theme.surfaceOverlay : theme.surfaceSelectedDark;
            return theme.accentSurfaceTint;
        }
        return mouseArea.containsMouse ? theme.surfaceOverlay : theme.surfaceElevated;
    }

    border.width: root.activeFocus ? 2 : 1
    border.color: {
        if (!root.enabledState) return theme.hairlineRule;
        if (root.pendingState) return theme.industrialAmber;
        if (root.activeFocus || root.activeState || root.primaryAccent) return theme.accentBorder;
        return mouseArea.containsMouse ? theme.borderStrong : theme.borderSubtle;
    }

    Behavior on color {
        ColorAnimation { duration: theme.transitionDurationMs }
    }

    Rectangle {
        visible: (root.activeState || root.pendingState) && !root.primaryAccent
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: root.pendingState ? theme.industrialAmber : theme.brandYellow
    }

    Text {
        id: labelItem
        anchors.centerIn: parent
        width: Math.min(implicitWidth, Math.max(24, root.width - 18))
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: root.pendingState
              ? ("... " + root.text)
              : ((root.activeState && !root.primaryAccent ? "● " : "") + root.text)
        font.family: pillShape ? theme.fontSans : theme.fontMono
        font.pixelSize: pillShape ? 12 : 11
        font.weight: (root.primaryAccent || root.activeState || root.pendingState) ? Font.Bold : Font.Medium
        color: {
            if (!root.enabledState) return theme.textMuted;
            if (root.pendingState) return theme.statusWarning;
            if (root.primaryAccent) return theme.accentOnText;
            if (root.activeState) {
                if (!pillShape && !theme.darkTheme) return theme.brandYellow;
                return pillShape ? theme.accentReadable : theme.brandYellow;
            }
            return theme.textPrimary;
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.interactive
        enabled: root.interactive
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ForbiddenCursor
        onClicked: {
            root.forceActiveFocus();
            root.clicked();
        }
    }

    Keys.onReturnPressed: if (root.interactive) root.clicked()
    Keys.onSpacePressed: if (root.interactive) root.clicked()
}
