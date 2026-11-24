// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root

    property string label: "METRIC"
    property string valueText: "--"
    property string unitText: ""
    property string subText: ""
    property string sourceTag: ""
    property color valueColor: theme.textPrimary
    property color accentBarColor: theme.borderStrong
    property bool stale: false
    property bool unavailable: false
    property int heroPixelSize: 24

    implicitHeight: 92
    height: 92
    radius: theme.radiusStructural
    color: theme.surfaceOverlay
    border.width: 1
    border.color: stale ? theme.statusWarning : theme.borderSubtle
    Accessible.role: Accessible.StaticText
    Accessible.name: root.label + ": " + (root.unavailable ? "Unavailable" : (root.valueText + (root.unitText.length > 0 ? (" " + root.unitText) : "")))

    // Left semantic telemetry register bar
    Rectangle {
        id: leftTick
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        color: root.unavailable
               ? theme.textMuted
               : (root.stale ? theme.statusWarning : root.accentBarColor)
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 10
        anchors.topMargin: 9
        anchors.bottomMargin: 8
        spacing: 5

        Row {
            width: parent.width
            height: 14
            spacing: 6

            Text {
                width: srcChip.visible ? Math.max(40, parent.width - srcChip.implicitWidth - 6) : parent.width
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: root.label
                font.family: theme.fontMono
                font.pixelSize: 9
                font.weight: Font.Bold
                font.letterSpacing: 0.6
                color: theme.textSecondary
            }

            Text {
                id: srcChip
                visible: root.sourceTag.length > 0 || root.stale
                anchors.verticalCenter: parent.verticalCenter
                text: root.stale ? ("STALE // " + root.sourceTag) : ("[" + root.sourceTag + "]")
                font.family: theme.fontMono
                font.pixelSize: 9
                font.weight: root.stale ? Font.Bold : Font.Normal
                color: root.stale ? theme.statusWarning : theme.textMuted
            }
        }

        Row {
            width: parent.width
            height: 30
            spacing: 5
            opacity: root.unavailable ? 0.50 : (root.stale ? 0.68 : 1.0)

            Text {
                height: parent.height
                verticalAlignment: Text.AlignBottom
                text: root.unavailable ? "--" : root.valueText
                font.family: theme.fontDisplay
                font.pixelSize: root.heroPixelSize
                font.weight: Font.Black
                font.letterSpacing: -0.5
                color: root.unavailable ? theme.textMuted : root.valueColor
            }

            Text {
                id: unitLbl
                visible: !root.unavailable && root.unitText.length > 0
                height: parent.height
                verticalAlignment: Text.AlignBottom
                bottomPadding: 3
                text: root.unitText
                font.family: theme.fontMono
                font.pixelSize: 12
                font.weight: Font.Bold
                color: theme.textSecondary
            }
        }

        Text {
            width: parent.width
            height: 14
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: root.unavailable ? "AWAITING ASYNC WORKER TELEMETRY" : root.subText
            font.family: theme.fontMono
            font.pixelSize: 10
            color: root.unavailable ? theme.textMuted : theme.textSecondary
        }
    }
}
