// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: batCol.implicitHeight + 28
    clip: true

    property int stagedThreshold: ctrl.batChargeThresholdPct

    Column {
        id: batCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // POWER SUPPLY & BATTERY TELEMETRY
        Rectangle {
            width: parent.width
            height: 156
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: batHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "BATTERY & POWER SUPPLY SUBSYSTEM (/SYS/CLASS/POWER_SUPPLY/BAT0 & AC)"
                badgeText: ctrl.acOnline ? "AC ADAPTER ONLINE" : "ON BATTERY POWER"
                badgeColor: ctrl.acOnline ? theme.statusNominal : theme.statusWarning
            }

            Row {
                anchors.top: batHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 14
                spacing: 20

                Column {
                    width: parent.width * 0.23
                    spacing: 4
                    Text { text: "STATE OF CHARGE"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.batCapacityPct + " %"; font.family: "monospace"; font.pixelSize: 28; font.weight: Font.Bold; color: theme.accentReadable }
                    Text { text: "STATUS: " + ctrl.batStatus.toUpperCase(); font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }

                Column {
                    width: parent.width * 0.24
                    spacing: 4
                    Text { text: "ELECTRICAL TELEMETRY"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.batPowerW.toFixed(1) + " W"; font.family: "monospace"; font.pixelSize: 28; font.weight: Font.Bold; color: theme.textPrimary }
                    Text { text: ctrl.batVoltageV.toFixed(2) + " V  |  " + ctrl.batCurrentA.toFixed(2) + " A"; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }

                Column {
                    width: parent.width * 0.25
                    spacing: 4
                    Text { text: "EST. CAPACITY RATIO (FULL / DESIGN)"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.batWearHealthPct.toFixed(1) + " %"; font.family: "monospace"; font.pixelSize: 28; font.weight: Font.Bold; color: ctrl.batWearHealthPct >= 75 ? theme.statusNominal : theme.statusWarning }
                    Text { text: "Full: " + ctrl.batChargeFullAh.toFixed(2) + " Ah | Design: " + ctrl.batChargeDesignAh.toFixed(2) + " Ah"; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }

                Column {
                    width: parent.width * 0.20
                    spacing: 4
                    Text { text: "CHEMISTRY & CYCLES"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.batTechnology; font.family: "monospace"; font.pixelSize: 24; font.weight: Font.Bold; color: theme.textPrimary }
                    Text { text: "CYCLE COUNT: " + ctrl.batCycleCount; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }
            }
        }

        // 02 // CHARGE THRESHOLD & DISPLAY BACKLIGHT CONTROLS
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 224
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: limHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "BATTERY CHARGE LIMIT THRESHOLD (80% CARE MODE)"
                    badgeText: ctrl.batChargeLimitSupported ? "FULL RW" : "NOT DETECTED"
                    badgeColor: ctrl.batChargeLimitSupported ? theme.statusNominal : theme.statusWarning
                }

                Column {
                    anchors.top: limHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 14
                    spacing: 12

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: ctrl.batStatusExplanation
                        font.family: "sans-serif"
                        font.pixelSize: 12
                        color: theme.textSecondary
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Charge Control End Threshold"
                        unit: "%"
                        minValue: 50
                        maxValue: 100
                        value: root.stagedThreshold
                        enabledState: ctrl.batChargeLimitSupported
                        onValueCommitted: function(v) { root.stagedThreshold = v; }
                    }

                    Row {
                        spacing: 10
                        AtelierButton {
                            text: "80% BATTERY CARE"
                            enabledState: ctrl.batChargeLimitSupported
                            onClicked: {
                                root.stagedThreshold = 80;
                                ctrl.applyBatteryChargeLimit(80);
                            }
                        }
                        AtelierButton {
                            text: "100% FULL TRAVEL CHARGE"
                            enabledState: ctrl.batChargeLimitSupported
                            onClicked: {
                                root.stagedThreshold = 100;
                                ctrl.applyBatteryChargeLimit(100);
                            }
                        }
                        AtelierButton {
                            text: "APPLY THRESHOLD (" + root.stagedThreshold + "%)"
                            enabledState: ctrl.batChargeLimitSupported
                            primaryAccent: ctrl.batChargeLimitSupported
                            onClicked: ctrl.applyBatteryChargeLimit(root.stagedThreshold)
                        }
                    }
                }
            }

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 224
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: blHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "INTERNAL eDP BACKLIGHT (INTEL_BACKLIGHT)"
                    badgeText: ctrl.displayBrightnessSupported ? "FULL RW (48,000 STEPS)" : "NOT DETECTED"
                    badgeColor: ctrl.displayBrightnessSupported ? theme.statusNominal : theme.textMuted
                }

                Column {
                    anchors.top: blHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 14
                    spacing: 14

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "Controls `/sys/class/backlight/intel_backlight` via `systemd-logind` (`org.freedesktop.login1.Session.SetBrightness`) with zero custom root escalation."
                        font.family: "sans-serif"
                        font.pixelSize: 12
                        color: theme.textSecondary
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "eDP Panel Luminance"
                        unit: "%"
                        minValue: 5
                        maxValue: 100
                        value: ctrl.displayBrightnessPct
                        enabledState: ctrl.displayBrightnessSupported
                        onValueCommitted: function(v) { ctrl.applyDisplayBrightness(v); }
                    }

                    Row {
                        spacing: 10
                        AtelierButton {
                            text: "25% LOW LIGHT"
                            enabledState: ctrl.displayBrightnessSupported
                            onClicked: ctrl.applyDisplayBrightness(25)
                        }
                        AtelierButton {
                            text: "50% STUDIO"
                            enabledState: ctrl.displayBrightnessSupported
                            onClicked: ctrl.applyDisplayBrightness(50)
                        }
                        AtelierButton {
                            text: "80% DAYLIGHT"
                            enabledState: ctrl.displayBrightnessSupported
                            onClicked: ctrl.applyDisplayBrightness(80)
                        }
                        AtelierButton {
                            text: "100% PEAK"
                            enabledState: ctrl.displayBrightnessSupported
                            onClicked: ctrl.applyDisplayBrightness(100)
                        }
                    }
                }
            }
        }

        // 03 // BATTERY DISCHARGE / CHARGE POWER HISTORY
        Rectangle {
            width: parent.width
            height: 224
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: pwrChartHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "BATTERY ELECTRICAL POWER TRACE (V × I WATTS, 60S ROLLING)"
                badgeText: "WATTS (W)"
                badgeColor: theme.accentReadable
            }

            TelemetryStripChart {
                anchors.top: pwrChartHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                minValue: 0
                maxValue: 80
                unitLabel: "W"
                seriesALabel: "BAT0 POWER"
                seriesBLabel: ""
                seriesA: ctrl.historyBatteryWatts
                seriesAColor: theme.accentPrimary
                backgroundColor: theme.surfacePrimary
                gridColor: theme.borderSubtle
                textColor: theme.textSecondary
            }
        }
    }
}
