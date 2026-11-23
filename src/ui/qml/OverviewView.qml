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

        // 01 // SYSTEM TELEMETRY LOAD BARS (CMP-0002)
        IndustrialPanel {
            width: parent.width
            height: loadHdr.height + loadCol.implicitHeight + 16

            SectionHeader {
                id: loadHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "REAL-TIME SYSTEM LOAD & THERMAL TELEMETRY"
                badgeText: "LIVE TELEMETRY"
                badgeColor: theme.telemetryLime
                accentStripeColor: theme.telemetryLime
            }

            Column {
                id: loadCol
                anchors.top: loadHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 8
                spacing: 6

                TelemetryLoadRow {
                    width: parent.width
                    label: "CPU Package Thermal Envelope"
                    sourceTag: "coretemp"
                    fraction: Math.max(0.0, (ctrl.cpuPackageTempC - 25.0) / 75.0)
                    warningThreshold: 0.67
                    criticalThreshold: 0.80
                    valueText: ctrl.cpuPackageTempC.toFixed(1) + " °C  (" + ctrl.cpuUtilizationPct.toFixed(0) + "% LOAD)"
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "Discrete NVIDIA RTX 3050 Utilization"
                    sourceTag: ctrl.gpuTelemetryMeasuredByNvml ? "nvml" : "proc"
                    fraction: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuUtilizationPct / 100.0) : 0.0
                    valueText: ctrl.gpuTelemetryMeasuredByNvml
                               ? (ctrl.gpuUtilizationPct + "%  (" + ctrl.gpuTempC.toFixed(0) + "°C · " + ctrl.gpuPstate + ")")
                               : (ctrl.gpuPstate + " · UNMEASURED")
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "Physical Memory Allocation"
                    sourceTag: "proc/meminfo"
                    fraction: ctrl.memUtilizationPct / 100.0
                    valueText: ctrl.memUtilizationPct.toFixed(1) + "%  (" + ctrl.memUsedMiB + " / " + ctrl.memTotalMiB + " MiB)"
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "Battery State of Charge & Mains Power"
                    sourceTag: ctrl.acOnline ? "AC ONLINE" : "BAT0"
                    fraction: ctrl.batCapacityPct / 100.0
                    customFillColor: ctrl.acOnline ? theme.telemetryLimeBar : theme.industrialAmber
                    valueText: ctrl.batCapacityPct + "%  (" + ctrl.batStatus.toUpperCase() + " · " + ctrl.batPowerW.toFixed(1) + " W)"
                }
            }
        }

        // 02 // OPERATING PROFILE QUICK SWITCHER
        IndustrialPanel {
            width: parent.width
            height: profHdr.height + profFlow.implicitHeight + 18

            SectionHeader {
                id: profHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "ACTIVE OPERATING PROFILE  //  TUNED-PPD: " + ctrl.activeTunedProfile.toUpperCase()
                badgeText: ctrl.acOnline ? "AC MAINS ONLINE" : "BATTERY DISCHARGE"
                badgeColor: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning
            }

            Flow {
                id: profFlow
                anchors.top: profHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                spacing: 10

                Repeater {
                    model: ctrl.operatingProfilesModel
                    delegate: AtelierButton {
                        width: Math.max(180, (parent.width - 30) / 4)
                        height: 32
                        pillShape: false
                        text: modelData.name.toUpperCase() + " · " + modelData.epp.toUpperCase()
                        activeState: modelData.active
                        primaryAccent: modelData.active
                        onClicked: ctrl.selectOperatingProfile(modelData.id)
                    }
                }
            }
        }

        // 03 // 4-BAY MODULAR SUBSYSTEM TELEMETRY GRID
        Grid {
            width: parent.width
            columns: width >= 880 ? 2 : 1
            spacing: 12

            // BAY A: CPU PACKAGE & CORES
            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 158

                SectionHeader {
                    id: cpuHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.A"
                    title: "CPU // " + ctrl.cpuModelName
                    badgeText: "READ-ONLY"
                    badgeColor: theme.technicalCyan
                }

                Row {
                    anchors.top: cpuHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 16

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text { text: "PACKAGE TEMP"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.cpuPackageTempC.toFixed(1) + " °C"
                            font.family: theme.fontMono
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: ctrl.cpuPackageTempC >= 85 ? theme.statusCritical : (ctrl.cpuPackageTempC >= 75 ? theme.statusWarning : theme.textPrimary)
                        }
                        Text {
                            width: parent.width - 4
                            elide: Text.ElideRight
                            text: ctrl.cpuGovernor + " · " + ctrl.cpuEpp
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text { text: "UTILIZATION"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.cpuUtilizationPct.toFixed(1) + " %"
                            font.family: theme.fontMono
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: theme.accentReadable
                        }
                        Text {
                            text: "AVG: " + ctrl.cpuAvgFreqMhz + " MHz"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.30
                        spacing: 4
                        Text { text: "PACKAGE POWER"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.cpuPowerAvailable ? (ctrl.cpuPackagePowerW.toFixed(1) + " W") : "N/A (0400)"
                            font.family: theme.fontMono
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: ctrl.cpuPowerAvailable ? theme.textPrimary : theme.textMuted
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: ctrl.cpuPowerSource
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            // BAY B: NVIDIA DISCRETE GPU
            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 158

                SectionHeader {
                    id: gpuHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.B"
                    title: "dGPU // " + (ctrl.gpuAvailable ? ctrl.gpuName : "NVIDIA GPU NOT DETECTED")
                    badgeText: ctrl.gpuAvailable ? (ctrl.gpuTelemetryMeasuredByNvml ? ("P-STATE " + ctrl.gpuPstate) : ctrl.gpuPstate) : "UNAVAILABLE"
                }

                Row {
                    anchors.top: gpuHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 16

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text { text: "GPU CORE TEMP"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuTempC.toFixed(0) + " °C") : "--"
                            font.family: theme.fontMono
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: ctrl.gpuTelemetryMeasuredByNvml ? theme.textPrimary : theme.textMuted
                        }
                        Text {
                            text: ctrl.gpuTelemetryMeasuredByNvml ? ("CORE: " + ctrl.gpuGraphicsClockMhz + " MHz") : "UNMEASURED"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text { text: "GPU UTILIZATION"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuUtilizationPct + " %") : "--"
                            font.family: theme.fontMono
                            font.pixelSize: 26
                            font.weight: Font.Bold
                            color: ctrl.gpuTelemetryMeasuredByNvml ? theme.telemetryLime : theme.textMuted
                        }
                        Text {
                            text: ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuVramUsedMiB + " / " + ctrl.gpuVramTotalMiB + " MiB") : "VRAM: UNMEASURED"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.30
                        spacing: 4
                        Text { text: "BOARD POWER"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.gpuPowerRailGated ? "IDLE (P8)" : (ctrl.gpuTelemetryMeasuredByNvml ? (ctrl.gpuPowerDrawW.toFixed(1) + " W") : "--")
                            font.family: theme.fontMono
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: ctrl.gpuPowerRailGated ? theme.technicalCyan : (ctrl.gpuTelemetryMeasuredByNvml ? theme.textPrimary : theme.textMuted)
                        }
                        Text {
                            text: ctrl.gpuPowerLimitsMeasured ? ("TGP: " + ctrl.gpuDefaultLimitW.toFixed(0) + "/" + ctrl.gpuMaxLimitW.toFixed(0) + "W") : "LIMIT: UNMEASURED"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }

            // BAY C: COOLING, KEYBOARD & DISPLAY CAPABILITY SUMMARY
            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 158

                SectionHeader {
                    id: ctlHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.C"
                    title: "COOLING, DISPLAY & KEYBOARD ACTUATORS"
                    badgeText: ctrl.fanCapabilityBadge
                }

                Row {
                    anchors.top: ctlHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 16

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text { text: "COOLING STATE"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.manualFanSupported ? "PWM ACTIVE" : "EC AUTO"
                            font.family: theme.fontMono
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: ctrl.manualFanSupported ? theme.telemetryLime : theme.technicalCyan
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: ctrl.fanControlModeLabel
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.30
                        spacing: 4
                        Text { text: "eDP DISPLAY"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.displayBrightnessPct + " %"
                            font.family: theme.fontMono
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: ctrl.displayBrightnessSupported ? "intel_backlight RW" : "NOT DETECTED"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text { text: "KEYBOARD (CLV0001)"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Row {
                            spacing: 6
                            Rectangle {
                                width: 16; height: 16; radius: 2
                                anchors.verticalCenter: parent.verticalCenter
                                color: ctrl.kbdEnabled ? ctrl.kbdPickerHex : theme.surfaceHeader
                                border.width: 1; border.color: theme.borderStrong
                            }
                            Text {
                                text: ctrl.kbdEnabled ? ("LVL " + ctrl.kbdBrightnessLevel + "/4") : "OFF"
                                font.family: theme.fontMono
                                font.pixelSize: 20
                                font.weight: Font.Bold
                                color: theme.textPrimary
                            }
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: "GRB [" + ctrl.kbdFirmwarePacketHex + "]"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.accentReadable
                        }
                    }
                }
            }

            // BAY D: BATTERY, STORAGE & BOARD THERMAL
            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 158

                SectionHeader {
                    id: thHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.D"
                    title: "POWER SUPPLY (BAT0) & STORAGE THERMALS"
                    badgeText: ctrl.thermalStateLabel.toUpperCase()
                }

                Row {
                    anchors.top: thHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 14
                    spacing: 16

                    Column {
                        width: parent.width * 0.34
                        spacing: 4
                        Text { text: "BATTERY & LIMIT"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.batCapacityPct + " %"
                            font.family: theme.fontMono
                            font.pixelSize: 24
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: ctrl.batChargeLimitSupported ? ("LIMIT: " + ctrl.batChargeThresholdPct + "% (RW)") : "LIMIT: NOT EXPOSED"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: ctrl.batChargeLimitSupported ? theme.telemetryLime : theme.statusWarning
                        }
                    }

                    Column {
                        width: parent.width * 0.32
                        spacing: 4
                        Text { text: "STORAGE PEAK"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.maxNvmeTempC.toFixed(1) + " °C"
                            font.family: theme.fontMono
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: theme.textPrimary
                        }
                        Text {
                            text: "Board Peak: " + ctrl.maxBoardTempC.toFixed(1) + "°C"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }

                    Column {
                        width: parent.width * 0.30
                        spacing: 4
                        Text { text: "MEMORY & PSI"; font.family: theme.fontMono; font.pixelSize: 10; color: theme.textSecondary }
                        Text {
                            text: ctrl.memAvailableMiB + " MiB"
                            font.family: theme.fontMono
                            font.pixelSize: 18
                            font.weight: Font.Bold
                            color: theme.telemetryLime
                        }
                        Text {
                            width: parent.width
                            elide: Text.ElideRight
                            text: "MEM PSI: " + ctrl.psiMemSome10.toFixed(2) + "%"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: theme.textSecondary
                        }
                    }
                }
            }
        }

        // 04 // SYNCHRONIZED 60-SECOND TELEMETRY STRIP CHARTS
        Grid {
            width: parent.width
            columns: width >= 880 ? 2 : 1
            spacing: 12

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 220

                SectionHeader {
                    id: chart1Hdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "04.A"
                    title: "THERMAL TRACE (60S ROLLING WINDOW)"
                    badgeText: "°C"
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
                    seriesBColor: theme.telemetryLime
                    backgroundColor: theme.surfacePrimary
                    gridColor: theme.borderSubtle
                    textColor: theme.textSecondary
                }
            }

            IndustrialPanel {
                width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                height: 220

                SectionHeader {
                    id: chart2Hdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "04.B"
                    title: "COMPUTE UTILIZATION TRACE (60S ROLLING WINDOW)"
                    badgeText: "%"
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
                    seriesAColor: theme.telemetryLime
                    seriesBColor: theme.technicalCyan
                    backgroundColor: theme.surfacePrimary
                    gridColor: theme.borderSubtle
                    textColor: theme.textSecondary
                }
            }
        }
    }
}
