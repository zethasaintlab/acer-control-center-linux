// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string leftLabel: "OFF"
    property string rightLabel: "ON"
    property bool rightActive: false
    property bool enabledState: true
    signal toggled(bool newValue)

    implicitWidth: 240
    implicitHeight: 34
    radius: height / 2
    color: theme.surfaceSunken
    border.width: root.activeFocus ? 2 : 1
    border.color: root.activeFocus ? theme.accentBorder : theme.borderSubtle
    activeFocusOnTab: root.enabledState
    opacity: root.enabledState ? 1.0 : 0.50
    Accessible.role: Accessible.Button
    Accessible.name: (root.rightActive ? root.rightLabel : root.leftLabel) + " (" + root.leftLabel + " / " + root.rightLabel + ")"
    Accessible.checkable: true
    Accessible.checked: root.rightActive

    Row {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 2

        Rectangle {
            width: (parent.width - 2) * 0.5
            height: parent.height
            radius: height / 2
            color: !root.rightActive
                   ? (root.enabledState ? theme.surfaceOverlay : theme.surfacePrimary)
                   : "transparent"
            border.width: !root.rightActive ? 1 : 0
            border.color: !root.rightActive ? theme.borderStrong : "transparent"

            Text {
                anchors.centerIn: parent
                width: parent.width - 10
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: root.leftLabel
                font.family: theme.fontMono
                font.pixelSize: 10
                font.weight: !root.rightActive ? Font.Bold : Font.Normal
                color: !root.rightActive ? theme.textPrimary : theme.textMuted
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.enabledState
                cursorShape: root.enabledState ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                onClicked: {
                    root.forceActiveFocus();
                    if (root.rightActive) root.toggled(false);
                }
            }
        }

        Rectangle {
            width: (parent.width - 2) * 0.5
            height: parent.height
            radius: height / 2
            color: root.rightActive
                   ? (root.enabledState ? theme.accentPrimary : theme.surfaceElevated)
                   : "transparent"

            Text {
                anchors.centerIn: parent
                width: parent.width - 10
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: root.rightLabel
                font.family: theme.fontMono
                font.pixelSize: 10
                font.weight: root.rightActive ? Font.Bold : Font.Normal
                color: root.rightActive
                       ? (root.enabledState ? theme.accentOnText : theme.textMuted)
                       : theme.textMuted
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.enabledState
                cursorShape: root.enabledState ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                onClicked: {
                    root.forceActiveFocus();
                    if (!root.rightActive) root.toggled(true);
                }
            }
        }
    }

    Keys.onSpacePressed: if (root.enabledState) root.toggled(!root.rightActive)
    Keys.onReturnPressed: if (root.enabledState) root.toggled(!root.rightActive)
}
