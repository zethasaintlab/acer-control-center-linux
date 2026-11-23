// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Flickable {
    id: root
    contentWidth: width
    contentHeight: gpuCol.implicitHeight + 28
    clip: true

    readonly property bool splitLower: width >= 900
    readonly property bool telemetryStale: ctrl.telemetryStale
    readonly property bool telemetryUnavailable: !ctrl.telemetryAvailable

    Column {
        id: gpuCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 12

        // 01 // DISCRETE GPU ARCHITECTURE & TELEMETRY MATRIX
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "dGPU ARCHITECTURE // " + (ctrl.gpuAvailable ? ctrl.gpuName : "NVIDIA GPU NOT DETECTED")
            provenanceText: ctrl.gpuSourceLabel
            badgeText: ctrl.gpuAvailable ? (ctrl.gpuPowerRailGated ? "P8 RAIL GATED" : "READ-ONLY") : "UNAVAILABLE"

            Grid {
                width: parent.width
                columns: width >= 880 ? 4 : (width >= 520 ? 2 : 1)
                spacing: 8

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "PERFORMANCE STATE"
                    sourceTag: "nvml"
                    valueText: ctrl.gpuAvailable ? ctrl.gpuPstate : "--"
                    valueColor: theme.accentReadable
                    accentBarColor: theme.brandYellow
                    subText: "DRIVER: " + ctrl.gpuDriverVersion
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "DIE TEMPERATURE"
                    sourceTag: "nvml"
                    valueText: ctrl.gpuTelemetryMeasuredByNvml ? ctrl.gpuTempC.toFixed(0) : "--"
                    unitText: ctrl.gpuTelemetryMeasuredByNvml ? "°C" : ""
                    accentBarColor: theme.telemetryLimeBar
                    subText: "PCI: " + ctrl.gpuPciBusId
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "CORE / MEM UTILIZATION"
                    sourceTag: "nvml"
                    valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuUtilizationPct + "% / " + ctrl.gpuMemUtilizationPct + "%") : "--"
                    valueColor: ctrl.gpuTelemetryMeasuredByNvml ? theme.telemetryLime : theme.textMuted
                    accentBarColor: theme.technicalCyan
                    subText: "SUBSYS: " + (ctrl.gpuSubsystemId.length > 0 ? ctrl.gpuSubsystemId : "--")
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                    heroPixelSize: 21
                }

                TelemetryMetricTile {
                    width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                    label: "TGP BOARD POWER"
                    sourceTag: ctrl.gpuPowerRailGated ? "gated" : "nvml"
                    valueText: ctrl.gpuPowerRailGated
                               ? "GATED (P8)"
                               : (ctrl.gpuTelemetryMeasuredByNvml ? ctrl.gpuPowerDrawW.toFixed(1) : "--")
                    unitText: (!ctrl.gpuPowerRailGated && ctrl.gpuTelemetryMeasuredByNvml) ? "W" : ""
                    valueColor: ctrl.gpuPowerRailGated ? theme.technicalCyan : theme.textPrimary
                    accentBarColor: ctrl.gpuPowerRailGated ? theme.technicalCyan : theme.telemetryLimeBar
                    subText: ctrl.gpuPowerLimitsMeasured
                             ? ("TGP " + ctrl.gpuDefaultLimitW.toFixed(0) + "W / MAX " + ctrl.gpuMaxLimitW.toFixed(0) + "W")
                             : "LIMITS: UNMEASURED"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                    heroPixelSize: ctrl.gpuPowerRailGated ? 19 : 24
                }
            }
        }

        // 02 // CLOCK DOMAINS (LEFT) & THROTTLE / RAIL-GATE DECODER (RIGHT)
        Grid {
            width: parent.width
            columns: root.splitLower ? 2 : 1
            spacing: 12

            CommandModule {
                id: clkMod
                width: root.splitLower ? (parent.width - 12) * 0.52 : parent.width
                indexCode: "02.A"
                title: "CLOCK DOMAINS & GDDR6 VRAM"
                provenanceText: "nvmlDeviceGetClockInfo"
                badgeText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramTotalMiB + " MiB GDDR6") : "UNMEASURED"
                contentSpacing: 8

                TelemetryLoadRow {
                    width: parent.width
                    label: "Graphics / SM Clock"
                    sourceTag: "nvml"
                    fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuGraphicsClockMhz / Math.max(1, ctrl.gpuMaxGraphicsClockMhz)) : 0.0
                    valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuGraphicsClockMhz + " / " + ctrl.gpuMaxGraphicsClockMhz + " MHz") : "UNMEASURED"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "GDDR6 Memory Clock"
                    sourceTag: "nvml"
                    fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuMemClockMhz / Math.max(1, ctrl.gpuMaxMemClockMhz)) : 0.0
                    valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuMemClockMhz + " / " + ctrl.gpuMaxMemClockMhz + " MHz") : "UNMEASURED"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "VRAM Framebuffer Allocated"
                    sourceTag: "nvml"
                    fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramUsedMiB / Math.max(1, ctrl.gpuVramTotalMiB)) : 0.0
                    valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramUsedMiB + " / " + ctrl.gpuVramTotalMiB + " MiB") : "UNMEASURED"
                    stale: root.telemetryStale
                    unavailable: root.telemetryUnavailable
                }
            }

            CommandModule {
                id: throtMod
                width: root.splitLower ? (parent.width - 12) * 0.48 : parent.width
                height: root.splitLower ? Math.max(clkMod.implicitHeight, throtMod.implicitHeight) : throtMod.implicitHeight
                indexCode: "02.B"
                title: "CLOCKS EVENT & THROTTLE DECODER"
                badgeText: ctrl.gpuPowerRailGated ? "P8 FILTER ACTIVE" : "LIVE NVML"
                contentSpacing: 8

                Repeater {
                    model: ctrl.gpuThrottleReasons
                    delegate: Rectangle {
                        width: parent.width
                        height: 28
                        radius: theme.radiusStructural
                        color: theme.surfaceOverlay
                        border.width: 1
                        border.color: theme.borderSubtle
                        Text {
                            anchors.left: parent.left; anchors.leftMargin: 10
                            anchors.right: parent.right; anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                            text: "ACTIVE FLAG: " + modelData
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.accentReadable
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: gpuNoteTxt.implicitHeight + 14
                    radius: theme.radiusStructural
                    color: theme.surfaceSunken
                    border.width: 1; border.color: theme.hairlineRule
                    Text {
                        id: gpuNoteTxt
                        anchors.fill: parent; anchors.margins: 7
                        wrapMode: Text.WordWrap
                        text: ctrl.gpuPowerRailGated
                              ? "GA107BM P8 Idle Note: NVML raw power reports " + ctrl.gpuRawPowerW.toFixed(2) + " W because dGPU rail ADC is gated. Filtered automatically."
                              : "Hybrid Graphics Note: Dynamic Boost (60W -> 75W) managed by nvidia-powerd.service alongside Intel Raptor Lake i915."
                        font.family: theme.fontSans
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }
                }
            }
        }
    }
}
