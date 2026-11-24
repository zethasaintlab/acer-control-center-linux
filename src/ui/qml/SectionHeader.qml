// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string indexCode: "01"
    property string title: "SECTION"
    property string badgeText: ""
    property color badgeColor: "transparent"
    property color accentStripeColor: theme.brandYellow
    property bool hazardStripe: false

    height: 34
    color: theme.surfaceHeader
    border.width: 1
    border.color: theme.borderSubtle

    // Left vertical industrial indicator bar
    Rectangle {
        id: leftAccentBar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        color: root.hazardStripe ? theme.industrialAmber : root.accentStripeColor
    }

    // Bottom crisp hairline rule
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: theme.borderStrong
    }

    Row {
        anchors.left: leftAccentBar.right
        anchors.leftMargin: 10
        anchors.right: badgeCtrl.visible ? badgeCtrl.left : parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Text {
            id: hazardLabel
            visible: root.hazardStripe
            anchors.verticalCenter: parent.verticalCenter
            text: "///"
            font.family: theme.fontMono
            font.pixelSize: 11
            font.weight: Font.Bold
            color: theme.industrialAmber
        }

        Text {
            id: indexLabel
            anchors.verticalCenter: parent.verticalCenter
            text: root.hazardStripe ? root.indexCode : ("// " + root.indexCode)
            font.family: theme.fontMono
            font.pixelSize: 11
            font.weight: Font.Bold
            color: theme.accentReadable
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(60, parent.width - indexLabel.width - (hazardLabel.visible ? hazardLabel.width + 18 : 10))
            elide: Text.ElideRight
            text: root.title
            font.family: theme.fontMono
            font.pixelSize: 11
            font.weight: Font.Bold
            font.letterSpacing: 0.7
            color: theme.textPrimary
        }
    }

    CapabilityBadge {
        id: badgeCtrl
        visible: root.badgeText.length > 0
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        badgeText: root.badgeText
        customColor: root.badgeColor
    }
}
