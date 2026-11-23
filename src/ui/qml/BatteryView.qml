// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: batCol.implicitHeight + 28
    clip: true

    property int stagedThreshold: ctrl.batChargeThresholdPct

    Connections {
        target: ctrl
        function onTelemetryUpdated() {
            if (ctrl.batChargeLimitSupported) {
                root.stagedThreshold = ctrl.batChargeThresholdPct;
            }
        }
    }

    Column {
        id: batCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // POWER SUPPLY & BATTERY TELEMETRY WITH PROVENANCE TAGS (Section 3.2)
        IndustrialPanel {
            width: parent.width
            height: batHdr.height + batTelCol.implicitHeight + 24

            SectionHeader {
                id: batHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "BATTERY TELEMETRY & PROVENANCE (/SYS/CLASS/POWER_SUPPLY/BAT0 & AC)"
                badgeText: "READ-ONLY"
            }

            Column {
                id: batTelCol
                anchors.top: batHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 10

                Grid {
                    width: parent.width
                    columns: width >= 880 ? 4 : 2
                    spacing: 10

                    Rectangle {
                        width: (parent.width - (parent.columns - 1) * 10) / parent.columns
                        height: 82
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1; border.color: theme.borderSubtle
                        Column {
                            anchors.fill: parent; anchors.margins: 10; spacing: 3
                            Text { text: "DIRECT FUEL-GAUGE SOC"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.technicalCyan }
                            Text { text: ctrl.batCapacityPct + " %"; font.family: theme.fontMono; font.pixelSize: 26; font.weight: Font.Bold; color: theme.accentReadable }
                            Text { text: ctrl.batStatus.toUpperCase() + " (" + ctrl.batChargeNowAh.toFixed(2) + " Ah)"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        }
                    }

                    Rectangle {
                        width: (parent.width - (parent.columns - 1) * 10) / parent.columns
                        height: 82
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1; border.color: theme.borderSubtle
                        Column {
                            anchors.fill: parent; anchors.margins: 10; spacing: 3
                            Text { text: "DERIVED POWER (V × I)"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                            Text { text: ctrl.batPowerW.toFixed(1) + " W"; font.family: theme.fontMono; font.pixelSize: 26; font.weight: Font.Bold; color: theme.textPrimary }
                            Text { text: ctrl.batVoltageV.toFixed(2) + " V  |  " + ctrl.batCurrentA.toFixed(2) + " A"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        }
                    }

                    Rectangle {
                        width: (parent.width - (parent.columns - 1) * 10) / parent.columns
                        height: 82
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1; border.color: theme.borderSubtle
                        Column {
                            anchors.fill: parent; anchors.margins: 10; spacing: 3
                            Text { text: "EST. CAPACITY RATIO"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.statusWarning }
                            Text {
                                text: ctrl.batWearHealthPct.toFixed(1) + " %"
                                font.family: theme.fontMono; font.pixelSize: 26; font.weight: Font.Bold
                                color: ctrl.batWearHealthPct >= 75 ? theme.telemetryLime : theme.statusWarning
                            }
                            Text { text: ctrl.batChargeFullAh.toFixed(2) + " / " + ctrl.batChargeDesignAh.toFixed(2) + " Ah (Est.)"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        }
                    }

                    Rectangle {
                        width: (parent.width - (parent.columns - 1) * 10) / parent.columns
                        height: 82
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1; border.color: theme.borderSubtle
                        Column {
                            anchors.fill: parent; anchors.margins: 10; spacing: 3
                            Text { text: "AC MAINS & CHEMISTRY"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                            Text {
                                text: ctrl.acOnline ? "AC ONLINE" : "ON BATTERY"
                                font.family: theme.fontMono; font.pixelSize: 20; font.weight: Font.Bold
                                color: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning
                            }
                            Text { text: ctrl.batTechnology + "  ·  CYCLES: " + ctrl.batCycleCountText; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        }
                    }
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "Current State of Charge vs Configured Policy Limit"
                    sourceTag: ctrl.batChargeLimitSupported ? ("LIMIT " + ctrl.batChargeThresholdPct + "%") : "UNCONFIGURED (100%)"
                    fraction: ctrl.batCapacityPct / 100.0
                    customFillColor: theme.telemetryLimeBar
                    valueText: ctrl.batCapacityPct + "%  (SOC)"
                }
            }
        }

        // 02 // CHARGE THRESHOLD CONTROL & INTERNAL DISPLAY LUMINANCE
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: parent.columns === 2
                        ? Math.max(limHdr.height + limBody.implicitHeight + 24, blHdr.height + blBody.implicitHeight + 24)
                        : (limHdr.height + limBody.implicitHeight + 24)

                SectionHeader {
                    id: limHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "BATTERY CARE CHARGE THRESHOLD"
                    badgeText: ctrl.batCapabilityBadge
                    hazardStripe: !ctrl.batChargeLimitSupported
                }

                Column {
                    id: limBody
                    anchors.top: limHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 10

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: ctrl.batStatusExplanation
                        font.family: theme.fontSans
                        font.pixelSize: 12
                        color: theme.textSecondary
                    }

                    Row {
                        width: parent.width
                        spacing: 12

                        Text {
                            height: 34
                            verticalAlignment: Text.AlignVCenter
                            text: "80% BATTERY CARE MODE:"
                            font.family: theme.fontMono
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }

                        SegmentedPillToggle {
                            width: 190
                            leftLabel: "100% FULL"
                            rightLabel: "80% CARE"
                            rightActive: ctrl.batChargeLimitSupported && ctrl.batChargeThresholdPct <= 80
                            enabledState: ctrl.batChargeLimitSupported
                            onToggled: function(careOn) {
                                var target = careOn ? 80 : 100;
                                root.stagedThreshold = target;
                                ctrl.applyBatteryChargeLimit(target);
                            }
                        }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: ctrl.batChargeLimitSupported ? "Charge Control End Threshold" : "Charge End Threshold (Not Exposed)"
                        unit: "%"
                        minValue: 50
                        maxValue: 100
                        value: root.stagedThreshold
                        enabledState: ctrl.batChargeLimitSupported
                        onValueCommitted: function(v) { root.stagedThreshold = v; }
                    }

                    Flow {
                        width: parent.width
                        spacing: 8

                        AtelierButton {
                            text: "APPLY THRESHOLD (" + root.stagedThreshold + "%)"
                            enabledState: ctrl.batChargeLimitSupported
                            primaryAccent: ctrl.batChargeLimitSupported
                            onClicked: ctrl.applyBatteryChargeLimit(root.stagedThreshold)
                        }

                        AtelierButton {
                            text: "RESTORE PREVIOUS (" + ctrl.batPreviousThresholdPct + "%)"
                            enabledState: ctrl.batChargeLimitSupported
                            onClicked: ctrl.restorePreviousBatteryChargeLimit()
                        }
                    }
                }
            }

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: parent.columns === 2
                        ? Math.max(limHdr.height + limBody.implicitHeight + 24, blHdr.height + blBody.implicitHeight + 24)
                        : (blHdr.height + blBody.implicitHeight + 24)

                SectionHeader {
                    id: blHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "INTERNAL eDP DISPLAY LUMINANCE (INTEL_BACKLIGHT)"
                    badgeText: ctrl.displayBrightnessSupported ? "FULL RW" : "NOT DETECTED"
                }

                Column {
                    id: blBody
                    anchors.top: blHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 12

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "Controls `/sys/class/backlight/intel_backlight` via unprivileged `systemd-logind` (`org.freedesktop.login1.Session.SetBrightness`, 48,000 hardware steps)."
                        font.family: theme.fontSans
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

                    Flow {
                        width: parent.width
                        spacing: 8
                        AtelierButton { text: "25% LOW"; enabledState: ctrl.displayBrightnessSupported; onClicked: ctrl.applyDisplayBrightness(25) }
                        AtelierButton { text: "50% CAD"; enabledState: ctrl.displayBrightnessSupported; onClicked: ctrl.applyDisplayBrightness(50) }
                        AtelierButton { text: "80% DAY"; enabledState: ctrl.displayBrightnessSupported; onClicked: ctrl.applyDisplayBrightness(80) }
                        AtelierButton { text: "100% MAX"; enabledState: ctrl.displayBrightnessSupported; onClicked: ctrl.applyDisplayBrightness(100) }
                    }
                }
            }
        }

        // 03 // BATTERY DISCHARGE / CHARGE POWER HISTORY
        IndustrialPanel {
            width: parent.width
            height: 224

            SectionHeader {
                id: pwrChartHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "BATTERY ELECTRICAL POWER TRACE (V × I WATTS, 60S ROLLING)"
                badgeText: "WATTS (W)"
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
