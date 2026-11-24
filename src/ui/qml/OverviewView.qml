// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: mainCol.implicitHeight + 28
    clip: true

    readonly property bool splitTwoCol: width >= 960
    readonly property bool telemetryStale: ctrl.telemetryStale
    readonly property bool telemetryUnavailable: !ctrl.telemetryAvailable

    Column {
        id: mainCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 12

        // 01 // ASYMMETRIC COMMAND DECK: TELEMETRY MATRIX + OPERATING POLICY
        Grid {
            width: parent.width
            columns: root.splitTwoCol ? 2 : 1
            spacing: 12

            // LEFT DECK (60% wide): TELEMETRY HEROES + ENVELOPE LOAD BARS
            CommandModule {
                id: telDeck
                width: root.splitTwoCol ? (parent.width - 12) * 0.60 : parent.width
                indexCode: "01"
                title: "SUBSYSTEM TELEMETRY DECK"
                provenanceText: ctrl.telemetryFreshness.toUpperCase()
                badgeText: root.telemetryUnavailable
                           ? "UNAVAILABLE"
                           : (root.telemetryStale ? "STALE TELEMETRY" : "LIVE TELEMETRY")
                stripeColor: root.telemetryStale ? theme.statusWarning : theme.telemetryLime
                contentSpacing: 10

                Grid {
                    width: parent.width
                    columns: width >= 540 ? 2 : 1
                    spacing: 8

                    TelemetryMetricTile {
                        width: parent.columns === 2 ? (parent.width - 8) * 0.5 : parent.width
                        label: "CPU PACKAGE"
                        sourceTag: "coretemp"
                        valueText: ctrl.cpuPackageTempC.toFixed(1)
                        unitText: "°C"
                        valueColor: ctrl.cpuPackageTempC >= 85
                                    ? theme.statusCritical
                                    : (ctrl.cpuPackageTempC >= 75 ? theme.statusWarning : theme.textPrimary)
                        accentBarColor: theme.telemetryLimeBar
                        subText: ctrl.cpuUtilizationPct.toFixed(0) + "% LOAD · "
                                 + (ctrl.cpuPowerAvailable ? (ctrl.cpuPackagePowerW.toFixed(1) + "W") : "RAPL N/A")
                                 + " · " + ctrl.cpuAvgFreqMhz + " MHz"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }

                    TelemetryMetricTile {
                        width: parent.columns === 2 ? (parent.width - 8) * 0.5 : parent.width
                        label: "DISCRETE RTX 3050"
                        sourceTag: ctrl.gpuTelemetryMeasuredByNvml ? "nvml" : "proc"
                        valueText: !ctrl.gpuAvailable
                                   ? "N/A"
                                   : (ctrl.gpuTelemetryMeasuredByNvml ? ctrl.gpuTempC.toFixed(0) : ctrl.gpuPstate)
                        unitText: (ctrl.gpuAvailable && ctrl.gpuTelemetryMeasuredByNvml) ? "°C" : ""
                        valueColor: ctrl.gpuPowerRailGated ? theme.technicalCyan : theme.textPrimary
                        accentBarColor: theme.technicalCyan
                        subText: !ctrl.gpuAvailable
                                 ? "dGPU NOT DETECTED"
                                 : (ctrl.gpuPowerRailGated
                                    ? ("P8 RAIL GATED · " + ctrl.gpuVramUsedMiB + "/" + ctrl.gpuVramTotalMiB + " MiB")
                                    : (ctrl.gpuUtilizationPct + "% · " + ctrl.gpuPowerDrawW.toFixed(1) + "W · " + ctrl.gpuPstate))
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }

                    TelemetryMetricTile {
                        width: parent.columns === 2 ? (parent.width - 8) * 0.5 : parent.width
                        label: "PHYSICAL MEMORY"
                        sourceTag: "proc/meminfo"
                        valueText: ctrl.memUtilizationPct.toFixed(1)
                        unitText: "%"
                        valueColor: theme.accentReadable
                        accentBarColor: theme.accentPrimary
                        subText: ctrl.memUsedMiB + " / " + ctrl.memTotalMiB + " MiB · PSI " + ctrl.psiMemSome10.toFixed(2) + "%"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }

                    TelemetryMetricTile {
                        width: parent.columns === 2 ? (parent.width - 8) * 0.5 : parent.width
                        label: "POWER & BATTERY"
                        sourceTag: ctrl.acOnline ? "AC MAINS" : "BAT0"
                        valueText: ctrl.batPresent ? ("" + ctrl.batCapacityPct) : "--"
                        unitText: ctrl.batPresent ? "%" : ""
                        valueColor: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning
                        accentBarColor: ctrl.acOnline ? theme.telemetryLimeBar : theme.industrialAmber
                        subText: ctrl.batStatus.toUpperCase() + " · " + ctrl.batPowerW.toFixed(1) + "W · NVMe " + ctrl.maxNvmeTempC.toFixed(0) + "°C"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                }

                Column {
                    width: parent.width
                    spacing: 6

                    TelemetryLoadRow {
                        width: parent.width
                        label: "CPU Thermal Headroom"
                        sourceTag: "coretemp"
                        fraction: Math.max(0.0, (ctrl.cpuPackageTempC - 25.0) / 75.0)
                        warningThreshold: 0.67
                        criticalThreshold: 0.80
                        valueText: ctrl.cpuPackageTempC.toFixed(1) + " °C (" + ctrl.cpuUtilizationPct.toFixed(0) + "%)"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }

                    TelemetryLoadRow {
                        width: parent.width
                        label: "GPU Framebuffer & Compute"
                        sourceTag: ctrl.gpuTelemetryMeasuredByNvml ? "nvml" : "proc"
                        fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuUtilizationPct / 100.0) : 0.0
                        valueText: ctrl.gpuTelemetryMeasuredByNvml
                                   ? (ctrl.gpuUtilizationPct + "% · " + ctrl.gpuVramUsedMiB + " MiB")
                                   : (ctrl.gpuPstate + " · UNMEASURED")
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }

                    TelemetryLoadRow {
                        width: parent.width
                        label: "RAM Allocation Ratio"
                        sourceTag: "meminfo"
                        fraction: ctrl.memUtilizationPct / 100.0
                        valueText: ctrl.memUsedMiB + " / " + ctrl.memTotalMiB + " MiB"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                }
            }

            // RIGHT DECK (40% wide): OPERATING POLICY & READBACK LEDGER
            CommandModule {
                id: policyDeck
                width: root.splitTwoCol ? (parent.width - 12) * 0.40 : parent.width
                height: root.splitTwoCol ? Math.max(telDeck.implicitHeight, policyDeck.implicitHeight) : policyDeck.implicitHeight
                indexCode: "02"
                title: "POLICY & SUBSYSTEM STATE"
                badgeText: ctrl.controlOperationPending
                           ? "AUTH PENDING"
                           : ("MODE: " + ctrl.activeProfileId.toUpperCase())
                stripeColor: theme.brandYellow
                contentSpacing: 10

                // Quick operating profile matrix
                Grid {
                    width: parent.width
                    columns: width >= 380 ? 2 : 1
                    spacing: 6

                    Repeater {
                        model: ctrl.operatingProfilesModel
                        delegate: AtelierButton {
                            width: parent.columns === 2 ? (parent.width - 6) * 0.5 : parent.width
                            height: 32
                            pillShape: false
                            text: modelData.name.toUpperCase()
                            activeState: modelData.active
                            primaryAccent: modelData.active
                            enabledState: !ctrl.controlOperationPending
                            onClicked: ctrl.selectOperatingProfile(modelData.id)
                        }
                    }
                }

                // Hairline subsystem inspection ledger
                Column {
                    width: parent.width
                    spacing: 0

                    CadKeyValueRow {
                        width: parent.width
                        keyText: "TUNED-PPD PROFILE"
                        valueText: ctrl.activeTunedProfile.toUpperCase()
                        valueColor: theme.accentReadable
                    }
                    CadKeyValueRow {
                        width: parent.width
                        keyText: "CPU POLICY"
                        valueText: ctrl.cpuGovernor.toUpperCase() + " · " + ctrl.cpuEpp
                    }
                    CadKeyValueRow {
                        width: parent.width
                        keyText: "COOLING"
                        valueText: ctrl.manualFanSupported ? "PWM MANUAL" : "FIRMWARE AUTOMATIC"
                        valueColor: ctrl.manualFanSupported ? theme.telemetryLime : theme.technicalCyan
                    }
                    CadKeyValueRow {
                        width: parent.width
                        keyText: "BATTERY CARE"
                        valueText: ctrl.batChargeLimitSupported ? ("LIMIT " + ctrl.batChargeThresholdPct + "% RW") : "LIMIT NOT EXPOSED"
                        valueColor: ctrl.batChargeLimitSupported ? theme.telemetryLime : theme.statusWarning
                    }
                    CadKeyValueRow {
                        width: parent.width
                        keyText: "KEYBOARD (CLV0001)"
                        valueText: ctrl.kbdEnabled
                                   ? ("LVL " + ctrl.kbdBrightnessLevel + "/4 · [" + ctrl.kbdFirmwarePacketHex + "]")
                                   : "BACKLIGHT OFF"
                    }
                    CadKeyValueRow {
                        width: parent.width
                        keyText: "eDP LUMINANCE"
                        valueText: ctrl.displayBrightnessSupported ? (ctrl.displayBrightnessPct + "% RW") : "NOT DETECTED"
                        showDivider: false
                    }
                }

                // Direct workspace links
                Flow {
                    width: parent.width
                    spacing: 6

                    AtelierButton {
                        height: 28
                        pillShape: false
                        text: "CPU TUNING"
                        onClicked: ctrl.currentTabIndex = 1
                    }
                    AtelierButton {
                        height: 28
                        pillShape: false
                        text: "KEYBOARD STUDIO"
                        onClicked: ctrl.currentTabIndex = 3
                    }
                    AtelierButton {
                        height: 28
                        pillShape: false
                        text: "DIAGNOSTICS"
                        onClicked: ctrl.currentTabIndex = 6
                    }
                }
            }
        }

        // 02 // DUAL 60-SECOND TELEMETRY STRIP CHARTS
        Grid {
            width: parent.width
            columns: width >= 860 ? 2 : 1
            spacing: 12

            CommandModule {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                indexCode: "03.A"
                title: "THERMAL TRACE (60S WINDOW)"
                provenanceText: "coretemp + nvml"
                badgeText: root.telemetryStale ? "STALE TRACE" : "°C"
                badgeColor: root.telemetryStale ? theme.statusWarning : theme.technicalCyan
                explicitContentHeight: 190
                padding: 8

                TelemetryStripChart {
                    width: parent.width
                    height: 190
                    opacity: root.telemetryStale ? 0.72 : 1.0
                    minValue: 25
                    maxValue: 100
                    unitLabel: "°C"
                    seriesALabel: "CPU PKG"
                    seriesBLabel: "RTX 3050"
                    seriesA: ctrl.historyCpuTemp
                    seriesB: ctrl.historyGpuTemp
                    seriesAColor: theme.accentPrimary
                    seriesBColor: theme.telemetryLime
                    backgroundColor: theme.surfaceSunken
                    gridColor: theme.hairlineRule
                    textColor: theme.textSecondary
                }
            }

            CommandModule {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                indexCode: "03.B"
                title: "COMPUTE UTILIZATION TRACE (60S WINDOW)"
                provenanceText: "proc/stat + nvml"
                badgeText: root.telemetryStale ? "STALE TRACE" : "%"
                badgeColor: root.telemetryStale ? theme.statusWarning : theme.telemetryLime
                explicitContentHeight: 190
                padding: 8

                TelemetryStripChart {
                    width: parent.width
                    height: 190
                    opacity: root.telemetryStale ? 0.72 : 1.0
                    minValue: 0
                    maxValue: 100
                    unitLabel: "%"
                    seriesALabel: "CPU LOAD"
                    seriesBLabel: "GPU LOAD"
                    seriesA: ctrl.historyCpuLoad
                    seriesB: ctrl.historyGpuLoad
                    seriesAColor: theme.telemetryLime
                    seriesBColor: theme.technicalCyan
                    backgroundColor: theme.surfaceSunken
                    gridColor: theme.hairlineRule
                    textColor: theme.textSecondary
                }
            }
        }

        // 03 // HONEST HARDWARE CAPABILITY READINESS BAR
        CommandModule {
            width: parent.width
            indexCode: "04"
            title: "HARDWARE CONTROL & READBACK READINESS"
            provenanceText: ctrl.hardwareModeLabel.toUpperCase()
            badgeText: ctrl.daemonReachable ? "POLKIT DAEMON READY" : "DAEMON OFFLINE"
            badgeColor: ctrl.daemonReachable ? theme.statusNominal : theme.statusWarning
            padding: 10

            Grid {
                width: parent.width
                columns: width >= 980 ? 4 : (width >= 560 ? 2 : 1)
                spacing: 8

                Rectangle {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceOverlay
                    border.width: 1; border.color: theme.borderSubtle
                    Column {
                        anchors.fill: parent; anchors.margins: 8; spacing: 3
                        Text { text: "CPU POLICY (P-STATE)"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                        CapabilityBadge { maxWidth: parent.width; badgeText: "FULL RW" }
                    }
                }

                Rectangle {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceOverlay
                    border.width: 1; border.color: theme.borderSubtle
                    Column {
                        anchors.fill: parent; anchors.margins: 8; spacing: 3
                        Text { text: "COOLING ACTUATOR"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                        CapabilityBadge { maxWidth: parent.width; badgeText: ctrl.fanCapabilityBadge }
                    }
                }

                Rectangle {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceOverlay
                    border.width: 1; border.color: theme.borderSubtle
                    Column {
                        anchors.fill: parent; anchors.margins: 8; spacing: 3
                        Text { text: "KEYBOARD (CLV0001)"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                        CapabilityBadge {
                            maxWidth: parent.width
                            badgeText: ctrl.kbdHardwareConnected
                                       ? (ctrl.kbdPhysicalReadbackSupported ? "FULL RW (SYSFS)" : "STATIC RGB · NO READBACK")
                                       : (ctrl.kbdBackendKindId === "clv0001_unbound" ? "UNBOUND (PREVIEW)" : "NOT DETECTED")
                        }
                    }
                }

                Rectangle {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceOverlay
                    border.width: 1; border.color: theme.borderSubtle
                    Column {
                        anchors.fill: parent; anchors.margins: 8; spacing: 3
                        Text { text: "BATTERY CHARGE LIMIT"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                        CapabilityBadge { maxWidth: parent.width; badgeText: ctrl.batChargeLimitSupported ? "FULL RW" : "NOT EXPOSED" }
                    }
                }
            }
        }
    }
}
