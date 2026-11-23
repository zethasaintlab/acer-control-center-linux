// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string label: "Metric"
    property string sourceTag: ""
    property string valueText: "0%"
    property real fraction: 0.0        // [0.0, 1.0]
    property real warningThreshold: 0.78
    property real criticalThreshold: 0.90
    property int segmentCount: 16
    property color customFillColor: "transparent"

    readonly property real clampedFrac: Math.max(0.0, Math.min(1.0, fraction))
    readonly property color barColor: {
        if (customFillColor.a > 0.01) return customFillColor;
        if (clampedFrac >= criticalThreshold) return theme.statusCritical;
        if (clampedFrac >= warningThreshold) return theme.statusWarning;
        return theme.telemetryLimeBar;
    }

    height: 34
    radius: theme.radiusStructural
    color: theme.surfaceElevated
    border.width: 1
    border.color: theme.borderSubtle

    Row {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        // Left technical label + optional source badge
        Row {
            width: parent.width * 0.45
            height: parent.height
            spacing: 6

            Rectangle {
                width: 3
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: root.barColor
            }

            Text {
                width: root.sourceTag.length > 0 ? Math.max(40, parent.width - srcLbl.implicitWidth - 18) : (parent.width - 12)
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: root.label
                font.family: theme.fontSans
                font.pixelSize: 12
                font.weight: Font.Medium
                color: theme.textPrimary
            }

            Text {
                id: srcLbl
                visible: root.sourceTag.length > 0
                anchors.verticalCenter: parent.verticalCenter
                text: "[" + root.sourceTag + "]"
                font.family: theme.fontMono
                font.pixelSize: 10
                color: theme.textMuted
            }
        }

        // Middle segmented telemetry gauge bar
        Item {
            width: Math.max(60, parent.width * 0.33)
            height: parent.height

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 10
                spacing: 2

                Repeater {
                    model: root.segmentCount
                    delegate: Rectangle {
                        width: Math.max(3, (parent.width - (root.segmentCount - 1) * 2) / root.segmentCount)
                        height: 10
                        radius: 1
                        property bool filled: ((index + 0.5) / root.segmentCount) <= root.clampedFrac && root.clampedFrac > 0.001
                        color: filled ? root.barColor : theme.surfaceHeader
                        border.width: filled ? 0 : 1
                        border.color: theme.borderSubtle
                    }
                }
            }
        }

        // Right tabular numeric readout
        Text {
            width: Math.max(54, parent.width * 0.22 - 20)
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            text: root.valueText
            font.family: theme.fontMono
            font.pixelSize: 12
            font.weight: Font.Bold
            color: root.clampedFrac >= root.criticalThreshold
                   ? theme.statusCritical
                   : (root.clampedFrac >= root.warningThreshold ? theme.statusWarning : theme.textPrimary)
        }
    }
}
