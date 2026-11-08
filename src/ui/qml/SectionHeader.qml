// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string indexCode: "01"
    property string title: "SECTION"
    property string badgeText: ""
    property color badgeColor: theme.statusInfo

    height: 32
    color: theme.surfaceHeader
    border.width: 1
    border.color: theme.borderSubtle

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Text {
            id: indexLabel
            text: root.indexCode + " //"
            font.family: "monospace"
            font.pixelSize: 11
            font.weight: Font.Bold
            color: theme.accentReadable
        }

        Text {
            width: Math.max(60, root.width - (badgeRect.visible ? badgeRect.width + 24 : 12) - indexLabel.width - 28)
            elide: Text.ElideRight
            text: root.title
            font.family: "monospace"
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 0.8
            color: theme.textPrimary
        }
    }

    Rectangle {
        id: badgeRect
        visible: root.badgeText.length > 0
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: 20
        width: badgeLabel.implicitWidth + 14
        radius: 2
        color: "transparent"
        border.width: 1
        border.color: root.badgeColor

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.badgeText
            font.family: "monospace"
            font.pixelSize: 10
            font.weight: Font.Bold
            color: root.badgeColor
        }
    }
}
