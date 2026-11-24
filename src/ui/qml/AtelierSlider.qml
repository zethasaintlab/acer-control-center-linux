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
    property color fillColor: theme.telemetryLimeBar
    property bool dragging: false
    property int dragValue: value
    readonly property int displayValue: dragging ? dragValue : clampValue(root.value)
    signal valueMoved(int newValue)
    signal valueCommitted(int newValue)

    implicitWidth: 220
    implicitHeight: 46
    activeFocusOnTab: root.enabledState
    opacity: root.enabledState ? 1.0 : 0.55

    Accessible.role: Accessible.Slider
    Accessible.name: root.label
    Accessible.description: root.displayValue + (root.unit.length > 0 ? (" " + root.unit) : "")

    function clampValue(v) {
        return Math.max(root.minValue, Math.min(root.maxValue, Math.round(v)));
    }

    function commitValue(v) {
        var c = clampValue(v);
        root.valueCommitted(c);
    }

    onEnabledStateChanged: {
        if (!root.enabledState && root.dragging) {
            root.dragging = false;
        }
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
            font.family: theme.fontSans
            font.pixelSize: 12
            font.weight: Font.Medium
            color: root.enabledState ? theme.textPrimary : theme.textMuted
        }

        Text {
            text: root.displayValue + (root.unit.length > 0 ? (" " + root.unit) : "")
            width: parent.width * 0.32
            horizontalAlignment: Text.AlignRight
            font.family: theme.fontMono
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
        height: 10
        radius: 1
        color: theme.surfaceSunken
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? theme.accentBorder : theme.borderStrong

        Repeater {
            model: [0.25, 0.50, 0.75]
            delegate: Rectangle {
                x: Math.round(track.width * modelData)
                y: 0
                width: 1
                height: track.height
                color: theme.hairlineRule
                z: 1
            }
        }

        Rectangle {
            width: {
                var span = Math.max(1, root.maxValue - root.minValue);
                var frac = Math.max(0.0, Math.min(1.0, (root.displayValue - root.minValue) / span));
                return Math.max(4, parent.width * frac);
            }
            height: parent.height
            radius: 1
            color: root.enabledState ? root.fillColor : theme.borderStrong
            z: 2
        }

        Rectangle {
            id: thumb
            width: 8
            height: 18
            radius: 1
            z: 3
            anchors.verticalCenter: parent.verticalCenter
            x: {
                var span = Math.max(1, root.maxValue - root.minValue);
                var frac = Math.max(0.0, Math.min(1.0, (root.displayValue - root.minValue) / span));
                return Math.max(0, Math.min(track.width - width, track.width * frac - width * 0.5));
            }
            color: root.enabledState ? theme.brandYellow : theme.textMuted
            border.width: 1
            border.color: theme.darkInk
        }

        MouseArea {
            anchors.fill: parent
            anchors.topMargin: -6
            anchors.bottomMargin: -6
            enabled: root.enabledState
            cursorShape: root.enabledState ? Qt.PointingHandCursor : Qt.ForbiddenCursor

            function valueFromMouse(mx) {
                var frac = Math.max(0.0, Math.min(1.0, mx / Math.max(1.0, track.width)));
                var raw = root.minValue + frac * (root.maxValue - root.minValue);
                return root.clampValue(raw);
            }

            onPressed: function(mouse) {
                root.forceActiveFocus();
                root.dragValue = valueFromMouse(mouse.x);
                root.dragging = true;
                root.valueMoved(root.dragValue);
            }
            onPositionChanged: function(mouse) {
                if (pressed && root.dragging) {
                    var nextVal = valueFromMouse(mouse.x);
                    if (nextVal !== root.dragValue) {
                        root.dragValue = nextVal;
                        root.valueMoved(nextVal);
                    }
                }
            }
            onReleased: function(mouse) {
                if (root.dragging) {
                    var finalVal = valueFromMouse(mouse.x);
                    root.dragValue = finalVal;
                    root.dragging = false;
                    root.commitValue(finalVal);
                }
            }
            onCanceled: {
                root.dragging = false;
            }
        }
    }

    Keys.onPressed: function(event) {
        if (!root.enabledState) {
            return;
        }
        var pageStep = Math.max(root.stepSize, 10);
        if (event.key === Qt.Key_Left) {
            root.commitValue(root.displayValue - root.stepSize);
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            root.commitValue(root.displayValue + root.stepSize);
            event.accepted = true;
        } else if (event.key === Qt.Key_PageUp) {
            root.commitValue(root.displayValue + pageStep);
            event.accepted = true;
        } else if (event.key === Qt.Key_PageDown) {
            root.commitValue(root.displayValue - pageStep);
            event.accepted = true;
        } else if (event.key === Qt.Key_Home) {
            root.commitValue(root.minValue);
            event.accepted = true;
        } else if (event.key === Qt.Key_End) {
            root.commitValue(root.maxValue);
            event.accepted = true;
        }
    }
}
