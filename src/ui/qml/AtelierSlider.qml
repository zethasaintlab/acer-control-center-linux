// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Item {
    id: root
    property string label: "Parameter"
    property string unit: "%"
    property int minValue: 0
    property int maxValue: 100
    property int value: 50
    property int stepSize: 1
    property bool enabledState: true
    property color fillColor: theme.accentPrimary
    signal valueCommitted(int newValue)

    implicitWidth: 240
    implicitHeight: 44
    activeFocusOnTab: root.enabledState

    function clampAndEmit(v) {
        var c = Math.max(root.minValue, Math.min(root.maxValue, Math.round(v)));
        root.value = c;
        root.valueCommitted(c);
    }

    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 18

        Text {
            text: root.label
            width: parent.width * 0.68
            elide: Text.ElideRight
            font.family: "sans-serif"
            font.pixelSize: 12
            font.weight: Font.Medium
            color: root.enabledState ? theme.textPrimary : theme.textMuted
        }

        Text {
            text: root.value + (root.unit.length > 0 ? (" " + root.unit) : "")
            width: parent.width * 0.32
            horizontalAlignment: Text.AlignRight
            font.family: "monospace"
            font.pixelSize: 12
            font.weight: Font.Bold
            color: root.enabledState ? theme.accentReadable : theme.textMuted
        }
    }

    Rectangle {
        id: track
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        height: 8
        radius: 2
        color: theme.surfaceHeader
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? theme.accentBorder : theme.borderSubtle

        Rectangle {
            width: {
                var span = Math.max(1, root.maxValue - root.minValue);
                var frac = Math.max(0.0, Math.min(1.0, (root.value - root.minValue) / span));
                return Math.max(4, parent.width * frac);
            }
            height: parent.height
            radius: 2
            color: root.enabledState ? root.fillColor : theme.borderStrong
        }

        Rectangle {
            id: thumb
            width: 10
            height: 16
            radius: 2
            anchors.verticalCenter: parent.verticalCenter
            x: {
                var span = Math.max(1, root.maxValue - root.minValue);
                var frac = Math.max(0.0, Math.min(1.0, (root.value - root.minValue) / span));
                return Math.max(0, Math.min(track.width - width, track.width * frac - width * 0.5));
            }
            color: root.enabledState ? theme.textPrimary : theme.textMuted
            border.width: 1
            border.color: theme.canvasBg
        }

        MouseArea {
            anchors.fill: parent
            anchors.topMargin: -6
            anchors.bottomMargin: -6
            enabled: root.enabledState
            cursorShape: root.enabledState ? Qt.PointingHandCursor : Qt.ForbiddenCursor

            function updateFromMouse(mx) {
                var frac = Math.max(0.0, Math.min(1.0, mx / Math.max(1.0, track.width)));
                var raw = root.minValue + frac * (root.maxValue - root.minValue);
                root.clampAndEmit(raw);
            }

            onPressed: function(mouse) {
                root.forceActiveFocus();
                updateFromMouse(mouse.x);
            }
            onPositionChanged: function(mouse) {
                if (pressed) updateFromMouse(mouse.x);
            }
        }
    }

    Keys.onLeftPressed: {
        if (root.enabledState) root.clampAndEmit(root.value - root.stepSize);
    }
    Keys.onRightPressed: {
        if (root.enabledState) root.clampAndEmit(root.value + root.stepSize);
    }
}
