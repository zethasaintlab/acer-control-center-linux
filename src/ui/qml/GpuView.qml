// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Flickable {
    id: root
    contentWidth: width
    contentHeight: gpuCol.implicitHeight + 28
    clip: true

    Column {
        id: gpuCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // NVIDIA GPU IDENTIFICATION & HYBRID GRAPHICS BANNER
        IndustrialPanel {
            width: parent.width
            height: gpuIdHdr.height + gpuGrid.implicitHeight + 24

            SectionHeader {
                id: gpuIdHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "DISCRETE GPU ARCHITECTURE // " + (ctrl.gpuAvailable ? ctrl.gpuName : "NVIDIA GPU NOT DETECTED")
                badgeText: ctrl.gpuAvailable ? "READ-ONLY" : "UNAVAILABLE"
            }

            Grid {
                id: gpuGrid
                anchors.top: gpuIdHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
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
                        Text { text: "PERFORMANCE STATE"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textSecondary }
                        Text { text: ctrl.gpuAvailable ? ctrl.gpuPstate : "--"; font.family: theme.fontMono; font.pixelSize: 26; font.weight: Font.Bold; color: theme.accentReadable }
                        Text { width: parent.width; elide: Text.ElideRight; text: "DRIVER: " + ctrl.gpuDriverVersion; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
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
                        Text { text: "DIE TEMPERATURE"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textSecondary }
                        Text {
                            text: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuTempC.toFixed(0) + " °C") : "--"
                            font.family: theme.fontMono; font.pixelSize: 26; font.weight: Font.Bold
                            color: ctrl.gpuTelemetryMeasuredByNvml ? theme.textPrimary : theme.textMuted
                        }
                        Text { width: parent.width; elide: Text.ElideRight; text: "PCI: " + ctrl.gpuPciBusId; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
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
                        Text { text: "CORE / MEM LOAD"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textSecondary }
                        Text {
                            text: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuUtilizationPct + "% / " + ctrl.gpuMemUtilizationPct + "%") : "--"
                            font.family: theme.fontMono; font.pixelSize: 24; font.weight: Font.Bold
                            color: ctrl.gpuTelemetryMeasuredByNvml ? theme.telemetryLime : theme.textMuted
                        }
                        Text { text: "SUBSYS: " + (ctrl.gpuSubsystemId.length > 0 ? ctrl.gpuSubsystemId : "--"); font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
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
                        Text { text: "TGP BOARD POWER"; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textSecondary }
                        Text {
                            text: ctrl.gpuPowerRailGated ? "RAIL GATED (P8)" : (ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuPowerDrawW.toFixed(1) + " W") : "UNMEASURED")
                            font.family: theme.fontMono; font.pixelSize: 18; font.weight: Font.Bold
                            color: ctrl.gpuPowerRailGated ? theme.technicalCyan : (ctrl.gpuTelemetryMeasuredByNvml ? theme.telemetryLime : theme.textMuted)
                        }
                        Text {
                            text: ctrl.gpuPowerLimitsMeasured ? ("TGP " + ctrl.gpuDefaultLimitW.toFixed(0) + "W / MAX " + ctrl.gpuMaxLimitW.toFixed(0) + "W") : "LIMITS: UNMEASURED"
                            font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary
                        }
                    }
                }
            }
        }

        // 02 // SEGMENTED CLOCKS, VRAM & THROTTLE REASON DECODER
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.52 : parent.width
                height: 224

                SectionHeader {
                    id: clkHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "CLOCK DOMAINS & GDDR6 VRAM GAUGES"
                    badgeText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramTotalMiB + " MiB GDDR6") : "UNMEASURED"
                }

                Column {
                    anchors.top: clkHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 10

                    TelemetryLoadRow {
                        width: parent.width
                        label: "Graphics / SM Clock"
                        sourceTag: "nvml"
                        fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuGraphicsClockMhz / Math.max(1, ctrl.gpuMaxGraphicsClockMhz)) : 0.0
                        valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuGraphicsClockMhz + " / " + ctrl.gpuMaxGraphicsClockMhz + " MHz") : "UNMEASURED"
                    }

                    TelemetryLoadRow {
                        width: parent.width
                        label: "GDDR6 Memory Clock"
                        sourceTag: "nvml"
                        fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuMemClockMhz / Math.max(1, ctrl.gpuMaxMemClockMhz)) : 0.0
                        valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuMemClockMhz + " / " + ctrl.gpuMaxMemClockMhz + " MHz") : "UNMEASURED"
                    }

                    TelemetryLoadRow {
                        width: parent.width
                        label: "VRAM Framebuffer Allocated"
                        sourceTag: "nvml"
                        fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramUsedMiB / Math.max(1, ctrl.gpuVramTotalMiB)) : 0.0
                        valueText: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramUsedMiB + " / " + ctrl.gpuVramTotalMiB + " MiB") : "UNMEASURED"
                    }
                }
            }

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.48 : parent.width
                height: 224

                SectionHeader {
                    id: throtHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "NVML CLOCKS EVENT & THROTTLE DECODER"
                    badgeText: ctrl.gpuPowerRailGated ? "P8 FILTER ACTIVE" : "LIVE NVML"
                }

                Column {
                    anchors.top: throtHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 12
                    spacing: 8

                    Repeater {
                        model: ctrl.gpuThrottleReasons
                        delegate: Rectangle {
                            width: parent.width
                            height: 28
                            radius: theme.radiusStructural
                            color: theme.surfaceElevated
                            border.width: 1
                            border.color: theme.borderSubtle

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "ACTIVE FLAG: " + modelData
                                font.family: theme.fontMono
                                font.pixelSize: 11
                                color: theme.accentReadable
                            }
                        }
                    }

                    Text {
                        width: parent.width
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
