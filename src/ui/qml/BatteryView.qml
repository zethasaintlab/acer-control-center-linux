// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: batCol.implicitHeight + 28
    clip: true

    property int stagedThreshold: ctrl.batChargeThresholdPct
    readonly property bool splitMidDeck: width >= 920
    readonly property bool telemetryStale: ctrl.telemetryStale
    readonly property bool telemetryUnavailable: !ctrl.telemetryAvailable

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
        anchors.margins: 14
        spacing: 12

        // 01 // ELECTRICAL PROVENANCE & BATTERY TELEMETRY MATRIX
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "POWER SUPPLY & BATTERY PROVENANCE"
            provenanceText: "/sys/class/power_supply/BAT0 + AC"
            badgeText: ctrl.acOnline ? "AC MAINS ONLINE" : "ON BATTERY"
            badgeColor: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning

            Grid {
                width: parent.width
                columns: width >= 880 ? 4 : (width >= 520 ? 2 : 1)
                spacing: 8

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "FUEL-GAUGE SOC"
                    sourceTag: "BAT0"
                    valueText: ctrl.batPresent ? ("" + ctrl.batCapacityPct) : "--"
                    unitText: ctrl.batPresent ? "%" : ""
                    valueColor: theme.accentReadable
                    accentBarColor: theme.brandYellow
                    subText: ctrl.batStatus.toUpperCase() + " (" + ctrl.batChargeNowAh.toFixed(2) + " Ah)"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "DERIVED POWER (V × I)"
                    sourceTag: "calc"
                    valueText: ctrl.batPowerW.toFixed(1)
                    unitText: "W"
                    accentBarColor: theme.technicalCyan
                    subText: ctrl.batVoltageV.toFixed(2) + " V · " + ctrl.batCurrentA.toFixed(2) + " A"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "EST. CAPACITY RATIO"
                    sourceTag: "estimate"
                    valueText: ctrl.batWearHealthPct.toFixed(1)
                    unitText: "%"
                    valueColor: ctrl.batWearHealthPct >= 75 ? theme.telemetryLime : theme.statusWarning
                    accentBarColor: ctrl.batWearHealthPct >= 75 ? theme.telemetryLimeBar : theme.statusWarning
                    subText: ctrl.batChargeFullAh.toFixed(2) + " / " + ctrl.batChargeDesignAh.toFixed(2) + " Ah"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "MAINS & CYCLES"
                    sourceTag: ctrl.batTechnology
                    valueText: ctrl.acOnline ? "AC ON" : "DC BAT"
                    valueColor: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning
                    accentBarColor: ctrl.acOnline ? theme.telemetryLimeBar : theme.industrialAmber
                    subText: "CYCLES: " + ctrl.batCycleCountText
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }
            }

            TelemetryLoadRow {
                width: parent.width
                label: "Current State of Charge vs Policy Limit"
                sourceTag: ctrl.batChargeLimitSupported ? ("LIMIT " + ctrl.batChargeThresholdPct + "%") : "UNCONFIGURED (100%)"
                fraction: ctrl.batCapacityPct / 100.0
                customFillColor: theme.telemetryLimeBar
                valueText: ctrl.batCapacityPct + "% SOC"
                stale: root.telemetryStale
                unavailable: root.telemetryUnavailable
            }
        }

        // 02 // SPLIT CONTROL BAY: BATTERY CHARGE LIMIT (LEFT) & eDP DISPLAY + POWER TRACE (RIGHT)
        Grid {
            width: parent.width
            columns: root.splitMidDeck ? 2 : 1
            spacing: 12

            // LEFT: BATTERY CARE POLICY (FAILS CLOSED UNLESS VERIFIED)
            CommandModule {
                id: limMod
                width: root.splitMidDeck ? (parent.width - 12) * 0.50 : parent.width
                indexCode: "02.A"
                title: "BATTERY CARE CHARGE THRESHOLD"
                provenanceText: "charge_control_end_threshold"
                badgeText: ctrl.batCapabilityBadge
                hazardMode: !ctrl.batChargeLimitSupported
                contentSpacing: 10

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: ctrl.batStatusExplanation
                    font.family: theme.fontSans
                    font.pixelSize: 12
                    color: theme.textSecondary
                }

                Flow {
                    width: parent.width
                    spacing: 10

                    Text {
                        height: 34
                        verticalAlignment: Text.AlignVCenter
                        text: "80% BATTERY CARE:"
                        font.family: theme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: theme.textPrimary
                    }

                    SegmentedPillToggle {
                        width: Math.min(210, parent.width)
                        leftLabel: "100% FULL"
                        rightLabel: "80% CARE"
                        rightActive: ctrl.batChargeLimitSupported && ctrl.batChargeThresholdPct <= 80
                        enabledState: ctrl.batChargeLimitSupported && !ctrl.controlOperationPending
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
                    minValue: 50; maxValue: 100
                    value: root.stagedThreshold
                    enabledState: ctrl.batChargeLimitSupported && !ctrl.controlOperationPending
                    onValueCommitted: function(v) { root.stagedThreshold = v; }
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    AtelierButton {
                        height: 32
                        text: "APPLY THRESHOLD (" + root.stagedThreshold + "%)"
                        enabledState: ctrl.batChargeLimitSupported && !ctrl.controlOperationPending
                        pendingState: ctrl.controlOperationPending && ctrl.controlOperationName.indexOf("battery") >= 0
                        primaryAccent: ctrl.batChargeLimitSupported
                        onClicked: ctrl.applyBatteryChargeLimit(root.stagedThreshold)
                    }

                    AtelierButton {
                        height: 32
                        pillShape: false
                        text: "RESTORE PREVIOUS (" + ctrl.batPreviousThresholdPct + "%)"
                        enabledState: ctrl.batChargeLimitSupported && !ctrl.controlOperationPending
                        onClicked: ctrl.restorePreviousBatteryChargeLimit()
                    }
                }
            }

            // RIGHT: eDP LUMINANCE + POWER TRACE
            CommandModule {
                id: blMod
                width: root.splitMidDeck ? (parent.width - 12) * 0.50 : parent.width
                height: root.splitMidDeck ? Math.max(limMod.implicitHeight, blMod.implicitHeight) : blMod.implicitHeight
                indexCode: "02.B"
                title: "eDP DISPLAY LUMINANCE"
                provenanceText: "org.freedesktop.login1.Session"
                badgeText: ctrl.displayBrightnessSupported ? "FULL RW" : "NOT DETECTED"
                contentSpacing: 10

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: "Adjusts `/sys/class/backlight/intel_backlight` asynchronously via `systemd-logind` (`SetBrightness`)."
                    font.family: theme.fontSans
                    font.pixelSize: 12
                    color: theme.textSecondary
                }

                AtelierSlider {
                    width: parent.width
                    label: "eDP Panel Luminance"
                    unit: "%"
                    minValue: 5; maxValue: 100
                    value: ctrl.displayBrightnessPct
                    enabledState: ctrl.displayBrightnessSupported && !ctrl.controlOperationPending
                    onValueCommitted: function(v) { ctrl.applyDisplayBrightness(v); }
                }

                Flow {
                    width: parent.width
                    spacing: 6

                    AtelierButton { height: 28; pillShape: false; text: "25% LOW"; enabledState: ctrl.displayBrightnessSupported && !ctrl.controlOperationPending; onClicked: ctrl.applyDisplayBrightness(25) }
                    AtelierButton { height: 28; pillShape: false; text: "50% CAD"; enabledState: ctrl.displayBrightnessSupported && !ctrl.controlOperationPending; onClicked: ctrl.applyDisplayBrightness(50) }
                    AtelierButton { height: 28; pillShape: false; text: "80% DAY"; enabledState: ctrl.displayBrightnessSupported && !ctrl.controlOperationPending; onClicked: ctrl.applyDisplayBrightness(80) }
                    AtelierButton { height: 28; pillShape: false; text: "100% MAX"; enabledState: ctrl.displayBrightnessSupported && !ctrl.controlOperationPending; onClicked: ctrl.applyDisplayBrightness(100) }
                }
            }
        }

        // 03 // BATTERY ELECTRICAL POWER HISTORY
        CommandModule {
            width: parent.width
            indexCode: "03"
            title: "ELECTRICAL POWER TRACE (V × I WATTS, 60S ROLLING)"
            provenanceText: "BAT0 power_now / voltage × current"
            badgeText: root.telemetryStale ? "STALE TRACE" : "WATTS (W)"
            explicitContentHeight: 196
            padding: 8

            TelemetryStripChart {
                width: parent.width
                height: 196
                opacity: root.telemetryStale ? 0.72 : 1.0
                minValue: 0; maxValue: 80
                unitLabel: "W"
                seriesALabel: "BAT0 POWER"
                seriesBLabel: ""
                seriesA: ctrl.historyBatteryWatts
                seriesAColor: theme.accentPrimary
                backgroundColor: theme.surfaceSunken
                gridColor: theme.hairlineRule
                textColor: theme.textSecondary
            }
        }
    }
}
