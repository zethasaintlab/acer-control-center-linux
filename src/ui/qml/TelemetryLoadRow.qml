// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string label: "Metric"
    property string sourceTag: ""
    property string valueText: "0%"
    property real fraction: 0.0
    property real warningThreshold: 0.78
    property real criticalThreshold: 0.90
    property int segmentCount: 16
    property color customFillColor: "transparent"
    property bool stale: false
    property bool unavailable: false

    readonly property bool compactRows: root.width < 520
    readonly property real clampedFrac: unavailable ? 0.0 : Math.max(0.0, Math.min(1.0, fraction))
    readonly property color barColor: {
        if (unavailable) return theme.textMuted;
        if (stale) return theme.statusWarning;
        if (customFillColor.a > 0.01) return customFillColor;
        if (clampedFrac >= criticalThreshold) return theme.statusCritical;
        if (clampedFrac >= warningThreshold) return theme.statusWarning;
        return theme.telemetryLimeBar;
    }

    implicitHeight: compactRows ? 54 : 34
    height: compactRows ? 54 : 34
    radius: theme.radiusStructural
    Accessible.role: Accessible.StaticText
    Accessible.name: root.label + ": " + (root.unavailable ? "Unavailable" : root.valueText)
    color: theme.surfaceOverlay
    border.width: 1
    border.color: stale ? theme.statusWarning : theme.borderSubtle

    // Wide desktop row layout
    Row {
        visible: !root.compactRows
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        Row {
            width: parent.width * 0.44
            height: parent.height
            spacing: 6

            Rectangle {
                width: 3
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: root.barColor
            }

            Text {
                width: root.sourceTag.length > 0 ? Math.max(40, parent.width - srcWide.implicitWidth - 18) : (parent.width - 12)
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: root.label
                font.family: theme.fontSans
                font.pixelSize: 12
                font.weight: Font.Medium
                color: root.unavailable ? theme.textMuted : theme.textPrimary
            }

            Text {
                id: srcWide
                visible: root.sourceTag.length > 0 || root.stale
                anchors.verticalCenter: parent.verticalCenter
                text: root.stale ? ("[STALE · " + root.sourceTag + "]") : ("[" + root.sourceTag + "]")
                font.family: theme.fontMono
                font.pixelSize: 9
                color: root.stale ? theme.statusWarning : theme.textMuted
            }
        }

        Item {
            width: Math.max(60, parent.width * 0.30)
            height: parent.height
            opacity: root.stale ? 0.65 : 1.0

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 9
                spacing: 2

                Repeater {
                    model: root.segmentCount
                    delegate: Rectangle {
                        width: Math.max(2, (parent.width - (root.segmentCount - 1) * 2) / root.segmentCount)
                        height: 9
                        radius: 1
                        property bool filled: ((index + 0.5) / root.segmentCount) <= root.clampedFrac && root.clampedFrac > 0.001
                        color: filled ? root.barColor : theme.surfaceSunken
                        border.width: filled ? 0 : 1
                        border.color: theme.hairlineRule
                    }
                }
            }
        }

        Text {
            width: Math.max(60, parent.width * 0.26 - 20)
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            text: root.unavailable ? "-- (UNAVAILABLE)" : root.valueText
            font.family: theme.fontMono
            font.pixelSize: 11
            font.weight: Font.Bold
            color: root.unavailable
                   ? theme.textMuted
                   : (root.stale
                      ? theme.statusWarning
                      : (root.clampedFrac >= root.criticalThreshold
                         ? theme.statusCritical
                         : (root.clampedFrac >= root.warningThreshold ? theme.statusWarning : theme.textPrimary)))
        }
    }

    // Compact tiled layout (< 520px width)
    Column {
        visible: root.compactRows
        anchors.fill: parent
        anchors.margins: 8
        spacing: 5

        Row {
            width: parent.width
            spacing: 6

            Text {
                width: parent.width * 0.56
                elide: Text.ElideRight
                text: root.label
                font.family: theme.fontSans
                font.pixelSize: 11
                font.weight: Font.Medium
                color: root.unavailable ? theme.textMuted : theme.textPrimary
            }

            Text {
                width: parent.width * 0.44 - 6
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                text: root.unavailable ? "--" : root.valueText
                font.family: theme.fontMono
                font.pixelSize: 11
                font.weight: Font.Bold
                color: root.stale ? theme.statusWarning : theme.textPrimary
            }
        }

        Row {
            width: parent.width
            height: 8
            spacing: 2
            Repeater {
                model: root.segmentCount
                delegate: Rectangle {
                    width: Math.max(2, (parent.width - (root.segmentCount - 1) * 2) / root.segmentCount)
                    height: 8
                    radius: 1
                    property bool filled: ((index + 0.5) / root.segmentCount) <= root.clampedFrac && root.clampedFrac > 0.001
                    color: filled ? root.barColor : theme.surfaceSunken
                    border.width: filled ? 0 : 1
                    border.color: theme.hairlineRule
                }
            }
        }
    }
}
