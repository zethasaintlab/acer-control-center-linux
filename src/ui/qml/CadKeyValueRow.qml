// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Item {
    id: root
    property string keyText: "KEY"
    property string valueText: "--"
    property color valueColor: theme.textPrimary
    property bool showDivider: true

    implicitHeight: 28
    height: 28

    Row {
        anchors.fill: parent
        spacing: 8

        Text {
            width: Math.max(90, parent.width * 0.42)
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            text: root.keyText
            font.family: theme.fontMono
            font.pixelSize: 10
            color: theme.textMuted
        }

        Text {
            width: Math.max(90, parent.width * 0.58 - 8)
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideMiddle
            text: root.valueText
            font.family: theme.fontMono
            font.pixelSize: 11
            font.weight: Font.DemiBold
            color: root.valueColor
        }
    }

    Rectangle {
        visible: root.showDivider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: theme.hairlineRule
    }
}
