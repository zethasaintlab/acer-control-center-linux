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
        Rectangle {
            width: parent.width
            height: 126
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: ecHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "COOLING SUBSYSTEM & EMBEDDED CONTROLLER (PNP0C09:00) STATUS"
                badgeText: ctrl.manualFanSupported ? "FULL RW (HWMON PWM)" : "FIRMWARE EC AUTO (NO KERNEL PWM)"
                badgeColor: ctrl.manualFanSupported ? theme.statusNominal : theme.statusWarning
            }

            Column {
                anchors.top: ecHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 10

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: ctrl.fanStatusExplanation
                    font.family: "sans-serif"
                    font.pixelSize: 12
                    color: theme.textSecondary
                }

                Row {
                    spacing: 10

                    AtelierButton {
                        text: "RESTORE FIRMWARE AUTO"
                        activeState: !ctrl.manualFanSupported
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
                        width: 240
                        height: 36
                        label: "Manual Duty Target"
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

        // 02 // INTERACTIVE FAN CURVE EDITOR & LIVE SENSOR TABLE
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.54 : parent.width
                height: 320
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: curveHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "PIECEWISE THERMAL RESPONSE CURVE (°C VS PWM %)"
                    badgeText: ctrl.manualFanSupported ? "LIVE PWM" : "PREVIEW / SIM"
                    badgeColor: ctrl.manualFanSupported ? theme.statusNominal : theme.statusWarning
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
                    onCurveEdited: function(updatedPts) {
                        ctrl.updateFanCurvePoints(updatedPts);
                    }
                }

                Row {
                    id: curveFooter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    height: 32
                    spacing: 10

                    AtelierButton {
                        text: "RESET DEFAULT CURVE"
                        onClicked: curveEditor.resetToDefaultCurve()
                    }

                    AtelierButton {
                        text: "APPLY CURVE TARGET (" + curveEditor.interpolatedDutyPct + "%)"
                        enabledState: ctrl.manualFanSupported
                        primaryAccent: ctrl.manualFanSupported
                        onClicked: ctrl.setFanMode("curve", curveEditor.interpolatedDutyPct)
                    }
                }
            }

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.46 : parent.width
                height: 320
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: sensHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "VERIFIED HARDWARE THERMAL DIODES & ZONES"
                    badgeText: ctrl.thermalSensorsModel.length + " SENSORS"
                    badgeColor: theme.statusInfo
                }

                Column {
                    anchors.top: sensHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    spacing: 6

                    Repeater {
                        model: ctrl.thermalSensorsModel
                        delegate: Rectangle {
                            width: parent.width
                            height: 30
                            color: theme.surfaceElevated
                            border.width: 1
                            border.color: theme.borderSubtle

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10

                                Text {
                                    width: parent.width * 0.52
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight
                                    text: modelData.label
                                    font.family: "sans-serif"
                                    font.pixelSize: 12
                                    color: theme.textPrimary
                                }

                                Text {
                                    width: parent.width * 0.24
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "[" + modelData.sourceDriver + "]"
                                    font.family: "monospace"
                                    font.pixelSize: 10
                                    color: theme.textMuted
                                }

                                Text {
                                    width: parent.width * 0.24
                                    anchors.verticalCenter: parent.verticalCenter
                                    horizontalAlignment: Text.AlignRight
                                    text: modelData.tempC.toFixed(1) + " °C"
                                    font.family: "monospace"
                                    font.pixelSize: 12
                                    font.weight: Font.Bold
                                    color: modelData.tempC >= 80 ? theme.statusWarning : theme.accentReadable
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
