// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: mainCol.implicitHeight + 28
    clip: true

    Column {
        id: mainCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // OPERATING PROFILE QUICK SWITCHER
        Rectangle {
            width: parent.width
            height: 76
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: profHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "ACTIVE OPERATING PROFILE  //  TUNED-PPD: " + ctrl.activeTunedProfile.toUpperCase()
                badgeText: ctrl.acOnline ? "AC MAINS ONLINE" : "BATTERY DISCHARGE"
                badgeColor: ctrl.acOnline ? theme.statusNominal : theme.statusWarning
            }

            Row {
                anchors.top: profHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                spacing: 10

                Repeater {
                    model: ctrl.operatingProfilesModel
                    delegate: AtelierButton {
                        width: (parent.width - 30) / 4
                        height: 28
                        text: modelData.name.toUpperCase() + " (" + modelData.epp + ")"
                        activeState: modelData.active
                        primaryAccent: modelData.active
                        onClicked: ctrl.selectOperatingProfile(modelData.id)
                    }
                }
            }
        }

        // 02 // 4-BAY HARDWARE TELEMETRY INSTRUMENT GRID
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            // BAY A: CPU PACKAGE & CORES
            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 156
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: cpuHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.A"
                    title: "CPU // " + ctrl.cpuModelName
                    badgeText: "CORETEMP"
                    badgeColor: theme.statusInfo
                }

                Row {
                    anchors.top: cpuHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 18

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text {
                            text: "PACKAGE TEMP"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.cpuPackageTempC.toFixed(1) + " °C"
                            font.family: "monospace"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: ctrl.cpuPackageTempC >= 85 ? theme.statusCritical : (ctrl.cpuPackageTempC >= 75 ? theme.statusWarning : theme.textPrimary)
                        }
                        Text {
                            width: parent.width - 4
                            elide: Text.ElideRight
                            text: ctrl.cpuGovernor + " · " + ctrl.cpuEpp
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text {
                            text: "TOTAL UTILIZATION"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.cpuUtilizationPct.toFixed(1) + " %"
                            font.family: "monospace"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: theme.accentReadable
                        }
                        Text {
                            text: "AVG CLOCK: " + ctrl.cpuAvgFreqMhz + " MHz"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.30
                        spacing: 4
                        Text {
                            text: "PACKAGE POWER"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.cpuPowerAvailable ? (ctrl.cpuPackagePowerW.toFixed(1) + " W") : "N/A (0400)"
                            font.family: "monospace"
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: ctrl.cpuPowerAvailable ? theme.textPrimary : theme.textMuted
                        }
                        Text {
                            text: ctrl.cpuPowerSource
                            width: parent.width
                            elide: Text.ElideRight
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            // BAY B: NVIDIA DISCRETE GPU
            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 156
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: gpuHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.B"
                    title: "dGPU // " + (ctrl.gpuAvailable ? ctrl.gpuName : "NVIDIA GPU NOT DETECTED")
                    badgeText: ctrl.gpuAvailable ? ("P-STATE " + ctrl.gpuPstate) : "UNAVAILABLE"
                    badgeColor: ctrl.gpuAvailable ? theme.statusNominal : theme.textMuted
                }

                Row {
                    anchors.top: gpuHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 18

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text {
                            text: "GPU CORE TEMP"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.gpuAvailable ? (ctrl.gpuTempC.toFixed(0) + " °C") : "--"
                            font.family: "monospace"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: "CORE: " + ctrl.gpuGraphicsClockMhz + " MHz"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text {
                            text: "GPU LOAD & VRAM"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.gpuAvailable ? (ctrl.gpuUtilizationPct + " %") : "--"
                            font.family: "monospace"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: theme.accentReadable
                        }
                        Text {
                            text: "VRAM: " + ctrl.gpuVramUsedMiB + " / " + ctrl.gpuVramTotalMiB + " MiB"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.30
                        spacing: 4
                        Text {
                            text: "BOARD POWER"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.gpuPowerRailGated ? "IDLE (P8)" : (ctrl.gpuPowerDrawW.toFixed(1) + " W")
                            font.family: "monospace"
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: ctrl.gpuPowerRailGated ? theme.statusInfo : theme.textPrimary
                        }
                        Text {
                            text: "TGP CAP: " + ctrl.gpuDefaultLimitW.toFixed(0) + "W / " + ctrl.gpuMaxLimitW.toFixed(0) + "W"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            // BAY C: MEMORY & LINUX PSI PRESSURE
            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 156
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: memHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.C"
                    title: "SYSTEM MEMORY & LINUX PSI STALL METRICS"
                    badgeText: "READ-ONLY /PROC"
                    badgeColor: theme.statusInfo
                }

                Row {
                    anchors.top: memHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 18

                    Column {
                        width: parent.width * 0.36
                        spacing: 4
                        Text {
                            text: "RAM ALLOCATION"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.memUtilizationPct.toFixed(1) + " %"
                            font.family: "monospace"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: ctrl.memUsedMiB + " / " + ctrl.memTotalMiB + " MiB"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text {
                            text: "AVAILABLE / CACHED"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.memAvailableMiB + " MiB"
                            font.family: "monospace"
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: theme.statusNominal
                        }
                        Text {
                            text: "CACHED: " + ctrl.memCachedMiB + " MiB"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.28
                        spacing: 4
                        Text {
                            text: "PSI STALL (AVG10)"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: "MEM " + ctrl.psiMemSome10.toFixed(2) + "%"
                            font.family: "monospace"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: "CPU " + ctrl.psiCpuSome10.toFixed(2) + "% | IO " + ctrl.psiIoSome10.toFixed(2) + "%"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            // BAY D: THERMAL & BATTERY SUMMARY
            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 156
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: thHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "02.D"
                    title: "THERMAL ENVELOPE & POWER SUPPLY (BAT0)"
                    badgeText: ctrl.thermalStateLabel.toUpperCase()
                    badgeColor: ctrl.thermalStateLabel === "Nominal" ? theme.statusNominal : theme.statusWarning
                }

                Row {
                    anchors.top: thHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 18

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text {
                            text: "BATTERY STATE"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.batCapacityPct + " %"
                            font.family: "monospace"
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: ctrl.batStatus + " (" + ctrl.batPowerW.toFixed(1) + " W)"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.33
                        spacing: 4
                        Text {
                            text: "STORAGE & BOARD"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.maxNvmeTempC.toFixed(1) + " °C"
                            font.family: "monospace"
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: "NVMe Peak | Board " + ctrl.maxBoardTempC.toFixed(1) + "°C"
                            font.family: "monospace"
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.29
                        spacing: 4
                        Text {
                            text: "COOLING CONTROL"
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                        Text {
                            text: ctrl.manualFanSupported ? "PWM ACTIVE" : "EC AUTO"
                            font.family: "monospace"
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: ctrl.manualFanSupported ? theme.statusNominal : theme.statusInfo
                        }
                        Text {
                            text: ctrl.fanControlModeLabel
                            width: parent.width
                            elide: Text.ElideRight
                            font.family: "monospace"
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }
        }

        // 03 // SYNCHRONIZED 60-SECOND TELEMETRY STRIP CHARTS
        Grid {
            width: parent.width
            columns: width >= 920 ? 2 : 1
            spacing: 12

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 228
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: chart1Hdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.A"
                    title: "THERMAL TRACE (60S ROLLING WINDOW)"
                    badgeText: "°C"
                    badgeColor: theme.accentReadable
                }

                TelemetryStripChart {
                    anchors.top: chart1Hdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    minValue: 25
                    maxValue: 100
                    unitLabel: "°C"
                    seriesALabel: "CPU PKG"
                    seriesBLabel: "RTX 3050"
                    seriesA: ctrl.historyCpuTemp
                    seriesB: ctrl.historyGpuTemp
                    seriesAColor: theme.accentPrimary
                    seriesBColor: theme.statusNominal
                    backgroundColor: theme.surfacePrimary
                    gridColor: theme.borderSubtle
                    textColor: theme.textSecondary
                }
            }

            Rectangle {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 228
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: chart2Hdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.B"
                    title: "COMPUTE UTILIZATION TRACE (60S ROLLING WINDOW)"
                    badgeText: "%"
                    badgeColor: theme.accentReadable
                }

                TelemetryStripChart {
                    anchors.top: chart2Hdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    minValue: 0
                    maxValue: 100
                    unitLabel: "%"
                    seriesALabel: "CPU LOAD"
                    seriesBLabel: "GPU LOAD"
                    seriesA: ctrl.historyCpuLoad
                    seriesB: ctrl.historyGpuLoad
                    seriesAColor: theme.accentPrimary
                    seriesBColor: theme.statusWarning
                    backgroundColor: theme.surfacePrimary
                    gridColor: theme.borderSubtle
                    textColor: theme.textSecondary
                }
            }
        }
    }
}
