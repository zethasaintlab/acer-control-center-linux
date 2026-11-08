// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

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
        Rectangle {
            width: parent.width
            height: 150
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: gpuIdHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "DISCRETE GPU ARCHITECTURE // " + (ctrl.gpuAvailable ? ctrl.gpuName : "NVIDIA GPU NOT DETECTED")
                badgeText: ctrl.gpuSourceLabel
                badgeColor: ctrl.gpuAvailable ? theme.statusNominal : theme.statusWarning
            }

            Row {
                anchors.top: gpuIdHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 14
                spacing: 20

                Column {
                    width: parent.width * 0.25
                    spacing: 4
                    Text { text: "PERFORMANCE STATE"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.gpuAvailable ? ctrl.gpuPstate : "--"; font.family: "monospace"; font.pixelSize: 28; font.weight: Font.Bold; color: theme.accentReadable }
                    Text { text: "DRIVER: " + ctrl.gpuDriverVersion; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }

                Column {
                    width: parent.width * 0.25
                    spacing: 4
                    Text { text: "DIE TEMPERATURE"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.gpuAvailable ? (ctrl.gpuTempC.toFixed(0) + " °C") : "--"; font.family: "monospace"; font.pixelSize: 28; font.weight: Font.Bold; color: theme.textPrimary }
                    Text { text: "PCI BUS: " + ctrl.gpuPciBusId; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }

                Column {
                    width: parent.width * 0.25
                    spacing: 4
                    Text { text: "CORE / MEMORY LOAD"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text { text: ctrl.gpuAvailable ? (ctrl.gpuUtilizationPct + "% / " + ctrl.gpuMemUtilizationPct + "%") : "--"; font.family: "monospace"; font.pixelSize: 26; font.weight: Font.Bold; color: theme.textPrimary }
                    Text { text: "SUBSYS: " + ctrl.gpuSubsystemId; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                }

                Column {
                    width: parent.width * 0.20
                    spacing: 4
                    Text { text: "TGP BOARD POWER"; font.family: "monospace"; font.pixelSize: 10; color: theme.textSecondary }
                    Text {
                        text: ctrl.gpuPowerRailGated ? "RAIL GATED (P8)" : (ctrl.gpuPowerDrawW.toFixed(1) + " W")
                        font.family: "monospace"
                        font.pixelSize: 22
                        font.weight: Font.Bold
                        color: ctrl.gpuPowerRailGated ? theme.statusInfo : theme.statusNominal
                    }
                    Text {
                        text: "DEFAULT " + ctrl.gpuDefaultLimitW.toFixed(0) + "W / MAX " + ctrl.gpuMaxLimitW.toFixed(0) + "W"
                        font.family: "monospace"
                        font.pixelSize: 10
                        color: theme.textSecondary
                    }
                }
            }
        }

        // 02 // CLOCK DOMAINS, VRAM ALLOCATION & THROTTLE REASON DECODER
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 220
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: clkHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "CLOCK DOMAINS & GDDR6 VRAM ALLOCATION"
                    badgeText: ctrl.gpuVramTotalMiB + " MiB GDDR6"
                    badgeColor: theme.accentReadable
                }

                Column {
                    anchors.top: clkHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 14
                    spacing: 14

                    // Graphics / SM Clock Bar
                    Column {
                        width: parent.width
                        spacing: 4
                        Row {
                            width: parent.width
                            Text { text: "GRAPHICS / SM CLOCK"; width: parent.width * 0.5; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                            Text { text: ctrl.gpuGraphicsClockMhz + " / " + ctrl.gpuMaxGraphicsClockMhz + " MHz"; width: parent.width * 0.5; horizontalAlignment: Text.AlignRight; font.family: "monospace"; font.pixelSize: 11; font.weight: Font.Bold; color: theme.textPrimary }
                        }
                        Rectangle {
                            width: parent.width; height: 8; color: theme.surfaceHeader
                            Rectangle {
                                width: parent.width * Math.min(1.0, ctrl.gpuGraphicsClockMhz / Math.max(1, ctrl.gpuMaxGraphicsClockMhz))
                                height: parent.height; color: theme.accentPrimary
                            }
                        }
                    }

                    // Memory Clock Bar
                    Column {
                        width: parent.width
                        spacing: 4
                        Row {
                            width: parent.width
                            Text { text: "GDDR6 MEMORY CLOCK"; width: parent.width * 0.5; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                            Text { text: ctrl.gpuMemClockMhz + " / " + ctrl.gpuMaxMemClockMhz + " MHz"; width: parent.width * 0.5; horizontalAlignment: Text.AlignRight; font.family: "monospace"; font.pixelSize: 11; font.weight: Font.Bold; color: theme.textPrimary }
                        }
                        Rectangle {
                            width: parent.width; height: 8; color: theme.surfaceHeader
                            Rectangle {
                                width: parent.width * Math.min(1.0, ctrl.gpuMemClockMhz / Math.max(1, ctrl.gpuMaxMemClockMhz))
                                height: parent.height; color: theme.statusNominal
                            }
                        }
                    }

                    // VRAM Bar
                    Column {
                        width: parent.width
                        spacing: 4
                        Row {
                            width: parent.width
                            Text { text: "VRAM ALLOCATED"; width: parent.width * 0.5; font.family: "monospace"; font.pixelSize: 11; color: theme.textSecondary }
                            Text { text: ctrl.gpuVramUsedMiB + " / " + ctrl.gpuVramTotalMiB + " MiB"; width: parent.width * 0.5; horizontalAlignment: Text.AlignRight; font.family: "monospace"; font.pixelSize: 11; font.weight: Font.Bold; color: theme.textPrimary }
                        }
                        Rectangle {
                            width: parent.width; height: 8; color: theme.surfaceHeader
                            Rectangle {
                                width: parent.width * Math.min(1.0, ctrl.gpuVramUsedMiB / Math.max(1, ctrl.gpuVramTotalMiB))
                                height: parent.height; color: theme.statusWarning
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 220
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: throtHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "NVML CLOCKS EVENT & THROTTLE REASON DECODER"
                    badgeText: ctrl.gpuPowerRailGated ? "P8 RAIL GATED FILTER ACTIVE" : "LIVE NVML"
                    badgeColor: theme.statusInfo
                }

                Column {
                    anchors.top: throtHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 14
                    spacing: 10

                    Repeater {
                        model: ctrl.gpuThrottleReasons
                        delegate: Rectangle {
                            width: parent.width
                            height: 28
                            color: theme.surfaceElevated
                            border.width: 1
                            border.color: theme.borderSubtle

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: "ACTIVE FLAG: " + modelData
                                font.family: "monospace"
                                font.pixelSize: 11
                                color: theme.accentReadable
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: ctrl.gpuPowerRailGated
                              ? "Note: In P8 idle state on GA107BM (driver " + ctrl.gpuDriverVersion + "), NVML raw power sensor reports " + ctrl.gpuRawPowerW.toFixed(2) + " W because the dGPU power rail ADC is gated. Our adapter filters this hardware quirk automatically."
                              : "Hybrid Graphics Note: Dynamic Boost (60W base -> 75W max) is managed by `nvidia-powerd.service` alongside Intel Raptor Lake i915."
                        font.family: "sans-serif"
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }
                }
            }
        }
    }
}
