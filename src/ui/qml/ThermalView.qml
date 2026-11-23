// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: thCol.implicitHeight + 28
    clip: true

    property int manualDutyPct: 65

    Column {
        id: thCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // HARDWARE COOLING & EC CAPABILITY BANNER
        IndustrialPanel {
            width: parent.width
            height: ecHdr.height + ecBodyCol.implicitHeight + 24

            SectionHeader {
                id: ecHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "COOLING SUBSYSTEM & EMBEDDED CONTROLLER (PNP0C09:00)"
                badgeText: ctrl.fanCapabilityBadge
                hazardStripe: !ctrl.manualFanSupported
            }

            Column {
                id: ecBodyCol
                anchors.top: ecHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 10

                // Honest safety / actuator capability box
                Rectangle {
                    width: parent.width
                    height: expTxt.implicitHeight + 16
                    radius: theme.radiusStructural
                    color: theme.surfaceElevated
                    border.width: 1
                    border.color: ctrl.manualFanSupported ? theme.telemetryLime : theme.borderStrong

                    Row {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        Rectangle {
                            width: 4
                            height: parent.height
                            color: ctrl.manualFanSupported ? theme.telemetryLime : theme.technicalCyan
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

                // Active Fan Tachometers (ONLY rendered when genuine hwmon fan*_input sensors exist)
                Flow {
                    visible: ctrl.fansModel.length > 0
                    width: parent.width
                    spacing: 10

                    Repeater {
                        model: ctrl.fansModel
                        delegate: Rectangle {
                            width: 240
                            height: 46
                            radius: theme.radiusStructural
                            color: theme.surfaceElevated
                            border.width: 1
                            border.color: theme.telemetryLime
                            Column {
                                anchors.centerIn: parent
                                width: parent.width - 16
                                spacing: 2
                                Text {
                                    text: modelData.label.toUpperCase()
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    color: theme.textSecondary
                                }
                                Text {
                                    text: modelData.rpm + " RPM  (" + modelData.pwmPct + "% PWM)"
                                    font.family: theme.fontMono
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    color: theme.telemetryLime
                                }
                            }
                        }
                    }
                }

                // Controls (strictly disabled on Kelilu_RTH when no verified hwmon PWM actuator exists)
                Flow {
                    width: parent.width
                    spacing: 10

                    AtelierButton {
                        text: "FIRMWARE AUTO"
                        activeState: !ctrl.manualFanSupported || ctrl.fanControlModeLabel.indexOf("Auto") >= 0
                        enabledState: ctrl.manualFanSupported
                        onClicked: ctrl.setFanMode("auto", 50)
                    }

                    AtelierButton {
                        text: "APPLY MANUAL DUTY (" + root.manualDutyPct + "%)"
                        enabledState: ctrl.manualFanSupported
                        onClicked: ctrl.setFanMode("manual", root.manualDutyPct)
                    }

                    AtelierButton {
                        text: "MAX COOLING (100%)"
                        enabledState: ctrl.manualFanSupported
                        onClicked: ctrl.setFanMode("manual", 100)
                    }

                    AtelierSlider {
                        width: 260
                        height: 40
                        label: ctrl.manualFanSupported ? "PWM Duty Override" : "PWM Override (Unavailable)"
                        unit: "%"
                        minValue: 25
                        maxValue: 100
                        value: root.manualDutyPct
                        enabledState: ctrl.manualFanSupported
                        onValueCommitted: function(v) { root.manualDutyPct = v; }
                    }
                }
            }
        }

        // 02 // INTERACTIVE THERMAL RESPONSE SCHEMATIC & VERIFIED SENSOR GAUGES
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.52 : parent.width
                height: 360

                SectionHeader {
                    id: curveHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "PIECEWISE THERMAL RESPONSE SCHEMATIC (°C VS PWM %)"
                    badgeText: ctrl.manualFanSupported ? ctrl.fanCapabilityBadge : "PREVIEW ONLY"
                    hazardStripe: !ctrl.manualFanSupported
                }

                FanCurveEditor {
                    id: curveEditor
                    anchors.top: curveHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: curveFooter.top
                    anchors.margins: 10
                    points: ctrl.fanCurvePoints
                    currentTempC: ctrl.cpuPackageTempC
                    hardwarePwmSupported: ctrl.manualFanSupported
                    accentColor: theme.accentPrimary
                    backgroundColor: theme.surfacePrimary
                    gridColor: theme.borderSubtle
                    textColor: theme.textSecondary
                    onCurveEdited: function(updatedPts) { ctrl.updateFanCurvePoints(updatedPts); }
                }

                Row {
                    id: curveFooter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    height: 34
                    spacing: 10

                    AtelierButton {
                        text: "RESET DEFAULT SCHEMATIC"
                        onClicked: curveEditor.resetToDefaultCurve()
                    }

                    AtelierButton {
                        text: ctrl.manualFanSupported
                              ? ("APPLY CURVE TARGET (" + curveEditor.interpolatedDutyPct + "%)")
                              : ("PREVIEW TARGET ONLY (" + curveEditor.interpolatedDutyPct + "%)")
                        enabledState: ctrl.manualFanSupported
                        primaryAccent: ctrl.manualFanSupported
                        onClicked: ctrl.setFanMode("curve", curveEditor.interpolatedDutyPct)
                    }
                }
            }

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.48 : parent.width
                height: 360

                SectionHeader {
                    id: sensHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "VERIFIED HARDWARE THERMAL DIODES & ZONES"
                    badgeText: "READ-ONLY"
                }

                Flickable {
                    anchors.top: sensHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    contentWidth: width
                    contentHeight: sensCol.implicitHeight
                    clip: true

                    Column {
                        id: sensCol
                        width: parent.width
                        spacing: 6

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
                            }
                        }
                    }
                }
            }
        }
    }
}
