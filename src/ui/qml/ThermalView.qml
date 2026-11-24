// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: thCol.implicitHeight + 28
    clip: true

    property int manualDutyPct: 65
    readonly property bool splitLower: width >= 900
    readonly property bool telemetryStale: ctrl.telemetryStale
    readonly property bool telemetryUnavailable: !ctrl.telemetryAvailable

    Column {
        id: thCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 12

        // 01 // COOLING CAPABILITY POSTURE & VERIFIED ACTUATORS
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "COOLING SUBSYSTEM & EMBEDDED CONTROLLER (PNP0C09:00)"
            provenanceText: ctrl.manualFanSupported ? "hwmon PWM RW" : "FIRMWARE THERMAL LOOP"
            badgeText: ctrl.fanCapabilityBadge
            hazardMode: !ctrl.manualFanSupported

            Rectangle {
                width: parent.width
                height: expTxt.implicitHeight + 16
                radius: theme.radiusStructural
                color: theme.surfaceOverlay
                border.width: 1
                border.color: ctrl.manualFanSupported ? theme.telemetryLime : theme.borderStrong

                Row {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    Rectangle {
                        width: 4
                        height: parent.height
                        color: ctrl.manualFanSupported ? theme.telemetryLime : theme.statusWarning
                    }

                    Text {
                        id: expTxt
                        width: parent.width - 18
                        wrapMode: Text.WordWrap
                        text: ctrl.fanStatusExplanation
                        font.family: theme.fontSans
                        font.pixelSize: 12
                        color: theme.textPrimary
                    }
                }
            }

            Text {
                visible: ctrl.fansModel.length === 0
                width: parent.width
                wrapMode: Text.WordWrap
                text: root.telemetryUnavailable
                      ? "Collecting initial thermal and tachometer snapshot..."
                      : "No active hwmon fan tachometer channels reported on this platform."
                font.family: theme.fontSans
                font.pixelSize: 11
                color: theme.textMuted
            }

            // Live Tachometers (ONLY shown when genuine hwmon fan*_input sensors exist)
            Grid {
                visible: ctrl.fansModel.length > 0
                width: parent.width
                columns: width >= 540 ? 2 : 1
                spacing: 8

                Repeater {
                    model: ctrl.fansModel
                    delegate: TelemetryMetricTile {
                        width: parent.columns === 2 ? (parent.width - 8) * 0.5 : parent.width
                        label: modelData.label.toUpperCase()
                        sourceTag: "hwmon"
                        valueText: "" + modelData.rpm
                        unitText: "RPM"
                        valueColor: theme.telemetryLime
                        accentBarColor: theme.telemetryLimeBar
                        subText: "PWM DUTY: " + modelData.pwmPct + "% · " + (modelData.writable ? "WRITABLE" : "READ-ONLY")
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                }
            }

            // Control strip (fails closed unless verified PWM interface detected)
            Flow {
                width: parent.width
                spacing: 8

                AtelierButton {
                    height: 32
                    text: "FIRMWARE AUTO"
                    activeState: !ctrl.manualFanSupported || ctrl.fanControlModeLabel.indexOf("Auto") >= 0
                    enabledState: ctrl.manualFanSupported && !ctrl.controlOperationPending
                    pendingState: ctrl.controlOperationPending && ctrl.controlOperationName.indexOf("fan") >= 0
                    onClicked: ctrl.setFanMode("auto", 50)
                }

                AtelierButton {
                    height: 32
                    text: "APPLY MANUAL DUTY (" + root.manualDutyPct + "%)"
                    enabledState: ctrl.manualFanSupported && !ctrl.controlOperationPending
                    pendingState: ctrl.controlOperationPending && ctrl.controlOperationName.indexOf("fan") >= 0
                    onClicked: ctrl.setFanMode("manual", root.manualDutyPct)
                }

                AtelierButton {
                    height: 32
                    text: "MAX COOLING (100%)"
                    enabledState: ctrl.manualFanSupported && !ctrl.controlOperationPending
                    pendingState: ctrl.controlOperationPending && ctrl.controlOperationName.indexOf("fan") >= 0
                    onClicked: ctrl.setFanMode("manual", 100)
                }

                AtelierSlider {
                    width: Math.min(280, parent.width)
                    label: ctrl.manualFanSupported ? "PWM Duty Override" : "PWM Override (Locked)"
                    unit: "%"
                    minValue: 25
                    maxValue: 100
                    value: root.manualDutyPct
                    enabledState: ctrl.manualFanSupported && !ctrl.controlOperationPending
                    onValueCommitted: function(v) { root.manualDutyPct = v; }
                }
            }
        }

        // 02 // THERMAL RESPONSE SCHEMATIC (LEFT) + VERIFIED HARDWARE DIODES (RIGHT)
        Grid {
            width: parent.width
            columns: root.splitLower ? 2 : 1
            spacing: 12

            CommandModule {
                id: curveMod
                width: root.splitLower ? (parent.width - 12) * 0.54 : parent.width
                indexCode: "02.A"
                title: "THERMAL RESPONSE CURVE SCHEMATIC"
                provenanceText: "°C vs PWM %"
                badgeText: ctrl.manualFanSupported ? ctrl.fanCapabilityBadge : "PREVIEW ONLY"
                hazardMode: !ctrl.manualFanSupported
                contentSpacing: 10

                FanCurveEditor {
                    id: curveEditor
                    width: parent.width
                    height: root.splitLower ? Math.max(270, sensorMod.implicitHeight - curveMod.headerHeight - 66) : 270
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Chart
                    Accessible.name: "Thermal Response Curve Schematic"
                    Accessible.description: "Left/Right selects control point, Up/Down adjusts fan duty percentage, Shift+Left/Right adjusts temperature threshold."
                    points: ctrl.fanCurvePoints
                    currentTempC: ctrl.cpuPackageTempC
                    hardwarePwmSupported: ctrl.manualFanSupported
                    accentColor: theme.accentPrimary
                    backgroundColor: theme.surfaceSunken
                    gridColor: theme.hairlineRule
                    textColor: theme.textSecondary
                    onCurveEdited: function(updatedPts) { ctrl.updateFanCurvePoints(updatedPts); }
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    AtelierButton {
                        height: 32
                        text: "RESET DEFAULT SCHEMATIC"
                        onClicked: curveEditor.resetToDefaultCurve()
                    }

                    AtelierButton {
                        height: 32
                        text: ctrl.manualFanSupported
                              ? ("APPLY CURVE TARGET (" + curveEditor.interpolatedDutyPct + "%)")
                              : ("PREVIEW TARGET ONLY (" + curveEditor.interpolatedDutyPct + "%)")
                        enabledState: ctrl.manualFanSupported && !ctrl.controlOperationPending
                        pendingState: ctrl.controlOperationPending && ctrl.controlOperationName.indexOf("fan") >= 0
                        primaryAccent: ctrl.manualFanSupported
                        onClicked: ctrl.setFanMode("curve", curveEditor.interpolatedDutyPct)
                    }
                }
            }

            CommandModule {
                id: sensorMod
                width: root.splitLower ? (parent.width - 12) * 0.46 : parent.width
                height: root.splitLower ? Math.max(curveMod.implicitHeight, sensorMod.implicitHeight) : sensorMod.implicitHeight
                indexCode: "02.B"
                title: "VERIFIED HARDWARE THERMAL ZONES"
                provenanceText: "hwmon + thermal_zone"
                badgeText: ctrl.thermalSensorsModel.length + " SENSORS (RO)"
                badgeColor: theme.statusInfo
                contentSpacing: 6

                Text {
                    visible: ctrl.thermalSensorsModel.length === 0
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: root.telemetryUnavailable
                          ? "Awaiting thermal zone and hwmon sensor readback..."
                          : "No thermal diodes reported by kernel."
                    font.family: theme.fontSans
                    font.pixelSize: 11
                    color: theme.textMuted
                }

                Repeater {
                    model: ctrl.thermalSensorsModel
                    delegate: TelemetryLoadRow {
                        width: parent.width
                        label: modelData.label
                        sourceTag: modelData.sourceDriver
                        fraction: Math.max(0.0, (modelData.tempC - 25.0) / 75.0)
                        warningThreshold: 0.67
                        criticalThreshold: 0.80
                        valueText: modelData.tempC.toFixed(1) + " °C"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                }
            }
        }
    }
}
