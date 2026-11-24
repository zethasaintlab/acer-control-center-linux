// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root

    property string indexCode: "01"
    property string title: "SUBSYSTEM"
    property string provenanceText: ""
    property string badgeText: ""
    property color badgeColor: "transparent"
    property color stripeColor: theme.brandYellow
    property bool hazardMode: false
    property bool highlighted: false
    property bool showFiducial: true
    property int padding: 12
    property int contentSpacing: 10
    property int explicitContentHeight: -1

    default property alias bodyData: innerCol.data
    readonly property int headerHeight: hdrBar.height + (hazardRule.visible ? hazardRule.height : 0)

    implicitHeight: headerHeight + (explicitContentHeight >= 0 ? explicitContentHeight : innerCol.implicitHeight) + padding * 2
    radius: theme.radiusStructural
    color: theme.surfacePrimary
    border.width: 1
    border.color: highlighted
                  ? theme.accentBorder
                  : (hazardMode ? theme.industrialAmber : theme.borderSubtle)

    // Architectural Header Conduit
    Rectangle {
        id: hdrBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 34
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        // Left vertical register stripe
        Rectangle {
            id: regStripe
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 4
            color: root.hazardMode ? theme.industrialAmber : root.stripeColor
        }

        // Bottom hairline divider
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: theme.borderStrong
        }

        Row {
            anchors.left: regStripe.right
            anchors.leftMargin: 10
            anchors.right: hdrRightRow.visible ? hdrRightRow.left : parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Text {
                id: codeStamp
                anchors.verticalCenter: parent.verticalCenter
                text: root.hazardMode ? ("///" + root.indexCode) : ("[" + root.indexCode + "]")
                font.family: theme.fontMono
                font.pixelSize: 11
                font.weight: Font.Bold
                color: root.hazardMode ? theme.statusWarning : theme.accentReadable
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(60, parent.width - codeStamp.implicitWidth - 10)
                elide: Text.ElideRight
                text: root.title
                font.family: theme.fontMono
                font.pixelSize: 11
                font.weight: Font.Bold
                font.letterSpacing: 0.7
                color: theme.textPrimary
            }
        }

        Row {
            id: hdrRightRow
            visible: root.provenanceText.length > 0 || root.badgeText.length > 0
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Text {
                visible: root.provenanceText.length > 0 && root.width >= 620
                anchors.verticalCenter: parent.verticalCenter
                text: root.provenanceText
                font.family: theme.fontMono
                font.pixelSize: 9
                color: theme.textMuted
            }

            CapabilityBadge {
                visible: root.badgeText.length > 0
                anchors.verticalCenter: parent.verticalCenter
                badgeText: root.badgeText
                customColor: root.badgeColor
            }
        }
    }

    // Subtle Endfield hazard step indicator ONLY on unverified / preview-locked modules
    Rectangle {
        id: hazardRule
        visible: root.hazardMode
        anchors.top: hdrBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        color: Qt.rgba(theme.statusWarning.r, theme.statusWarning.g, theme.statusWarning.b, 0.28)
        Row {
            anchors.fill: parent
            spacing: 8
            clip: true
            Repeater {
                model: Math.max(8, Math.ceil(hazardRule.width / 22))
                delegate: Rectangle {
                    width: 14
                    height: 3
                    color: theme.statusWarning
                    opacity: 0.75
                }
            }
        }
    }

    Column {
        id: innerCol
        anchors.top: hazardRule.visible ? hazardRule.bottom : hdrBar.bottom
        anchors.topMargin: root.padding
        anchors.left: parent.left
        anchors.leftMargin: root.padding
        anchors.right: parent.right
        anchors.rightMargin: root.padding
        spacing: root.contentSpacing
    }

    // Corner fiducial alignment mark
    Item {
        visible: root.showFiducial
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 5
        width: 7
        height: 7
        opacity: 0.42
        Rectangle { anchors.centerIn: parent; width: 7; height: 1; color: theme.borderStrong }
        Rectangle { anchors.centerIn: parent; width: 1; height: 7; color: theme.borderStrong }
    }
}
