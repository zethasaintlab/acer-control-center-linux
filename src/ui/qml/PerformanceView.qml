// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Flickable {
    id: root
    contentWidth: width
    contentHeight: perfCol.implicitHeight + 28
    clip: true

    property string stagedGov: ctrl.cpuGovernor.length > 0 ? ctrl.cpuGovernor : "powersave"
    property string stagedEpp: ctrl.cpuEpp.length > 0 ? ctrl.cpuEpp : "balance_performance"
    property bool stagedTurbo: ctrl.cpuTurboEnabled
    property int stagedMinPerf: ctrl.cpuMinPerfPct
    property int stagedMaxPerf: ctrl.cpuMaxPerfPct

    Column {
        id: perfCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // OPERATING PROFILES & POWER TRANSITION RULES
        Rectangle {
            width: parent.width
            height: 196
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: opHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "OPERATING PROFILE ENGINE  //  SYSTEM DAEMON: " + ctrl.activeTunedProfile
                badgeText: "FULL RW (TUNED-PPD)"
                badgeColor: theme.statusNominal
            }

            Row {
                id: profileRow
                anchors.top: opHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                height: 92
                spacing: 10

                Repeater {
                    model: ctrl.operatingProfilesModel
                    delegate: Rectangle {
                        id: profCard
                        width: (profileRow.width - 30) / 4
                        height: 92
                        activeFocusOnTab: true
                        color: modelData.active ? theme.accentSurfaceTint : theme.surfaceElevated
                        border.width: (profCard.activeFocus || modelData.active) ? 2 : 1
                        border.color: (profCard.activeFocus || modelData.active) ? theme.accentBorder : theme.borderSubtle

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Text {
                                text: modelData.name.toUpperCase()
                                font.family: "monospace"
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: modelData.active ? theme.accentReadable : theme.textPrimary
                            }

                            Text {
                                text: modelData.subtitle
                                width: parent.width
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                font.family: "sans-serif"
                                font.pixelSize: 11
                                color: theme.textSecondary
                            }

                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: modelData.governor + " · " + modelData.epp + " · " + modelData.minPerfPct + "-" + modelData.maxPerfPct + "%"
                                font.family: "monospace"
                                font.pixelSize: 10
                                font.letterSpacing: -0.3
                                color: theme.textMuted
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                profCard.forceActiveFocus();
                                ctrl.selectOperatingProfile(modelData.id);
                            }
                        }

                        Keys.onReturnPressed: ctrl.selectOperatingProfile(modelData.id)
                        Keys.onSpacePressed: ctrl.selectOperatingProfile(modelData.id)
                    }
                }
            }

            Row {
                anchors.top: profileRow.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 14

                AtelierButton {
                    text: ctrl.autoSwitchOnPower ? "AUTO AC/BATTERY SWITCH: ENABLED" : "AUTO AC/BATTERY SWITCH: DISABLED"
                    activeState: ctrl.autoSwitchOnPower
                    onClicked: ctrl.autoSwitchOnPower = !ctrl.autoSwitchOnPower
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "AC Profile: " + ctrl.acProfileId.toUpperCase() + "   |   Battery Profile: " + ctrl.batteryProfileId.toUpperCase()
                    font.family: "monospace"
                    font.pixelSize: 11
                    color: theme.textSecondary
                }
            }
        }

        // 02 // INTEL P-STATE & HWP HARDWARE TUNING
        Rectangle {
            width: parent.width
            height: 224
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: pstateHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "INTEL P-STATE & HWP HARDWARE TUNING (DAEMON READBACK VERIFIED)"
                badgeText: "POLKIT GATED"
                badgeColor: theme.statusInfo
            }

            Column {
                anchors.top: pstateHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 14
                spacing: 12

                Row {
                    width: parent.width
                    spacing: 12

                    Text {
                        width: 130
                        anchors.verticalCenter: parent.verticalCenter
                        text: "CPU GOVERNOR:"
                        font.family: "monospace"
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }

                    AtelierButton {
                        text: "POWERSAVE (HWP)"
                        activeState: root.stagedGov === "powersave"
                        onClicked: root.stagedGov = "powersave"
                    }

                    AtelierButton {
                        text: "PERFORMANCE"
                        activeState: root.stagedGov === "performance"
                        onClicked: root.stagedGov = "performance"
                    }

                    Item { width: 16; height: 1 }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "TURBO BOOST:"
                        font.family: "monospace"
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }

                    AtelierButton {
                        text: root.stagedTurbo ? "TURBO ENABLED (4.9 GHz)" : "TURBO DISABLED (2.4 GHz)"
                        activeState: root.stagedTurbo
                        onClicked: root.stagedTurbo = !root.stagedTurbo
                    }
                }

                Row {
                    width: parent.width
                    spacing: 8

                    Text {
                        width: 130
                        anchors.verticalCenter: parent.verticalCenter
                        text: "HWP EPP HINT:"
                        font.family: "monospace"
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }

                    Repeater {
                        model: ["power", "balance_power", "balance_performance", "performance"]
                        delegate: AtelierButton {
                            text: modelData.toUpperCase()
                            activeState: root.stagedEpp === modelData
                            onClicked: root.stagedEpp = modelData
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 20

                    AtelierSlider {
                        width: (parent.width - 240) * 0.5
                        label: "Minimum P-State Performance"
                        unit: "%"
                        minValue: 10
                        maxValue: 100
                        value: root.stagedMinPerf
                        onValueCommitted: function(v) {
                            root.stagedMinPerf = Math.min(v, root.stagedMaxPerf);
                        }
                    }

                    AtelierSlider {
                        width: (parent.width - 240) * 0.5
                        label: "Maximum P-State Performance"
                        unit: "%"
                        minValue: 10
                        maxValue: 100
                        value: root.stagedMaxPerf
                        onValueCommitted: function(v) {
                            root.stagedMaxPerf = Math.max(v, root.stagedMinPerf);
                        }
                    }

                    AtelierButton {
                        width: 196
                        height: 36
                        anchors.bottom: parent.bottom
                        text: "APPLY & VERIFY P-STATE"
                        primaryAccent: true
                        onClicked: ctrl.applyCustomCpuTuning(
                            root.stagedGov,
                            root.stagedEpp,
                            root.stagedTurbo,
                            root.stagedMinPerf,
                            root.stagedMaxPerf
                        )
                    }
                }
            }
        }

        // 03 // PER-CORE CLOCK & UTILIZATION RACK
        Rectangle {
            width: parent.width
            height: 230
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: coreRackHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "PER-THREAD FREQUENCY & LOAD TELEMETRY RACK"
                badgeText: "MAX " + ctrl.cpuMaxFreqMhz + " MHz"
                badgeColor: theme.accentReadable
            }

            Grid {
                anchors.top: coreRackHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 12
                columns: 4
                spacing: 8

                Repeater {
                    model: ctrl.cpuCoresModel
                    delegate: Rectangle {
                        width: (parent.width - 24) / 4
                        height: 38
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.borderSubtle

                        Column {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 3

                            Row {
                                width: parent.width
                                Text {
                                    text: modelData.label
                                    width: parent.width * 0.35
                                    font.family: "monospace"
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: theme.textPrimary
                                }
                                Text {
                                    text: modelData.freqMhz + " MHz (" + modelData.utilizationPct.toFixed(0) + "%)"
                                    width: parent.width * 0.65
                                    horizontalAlignment: Text.AlignRight
                                    font.family: "monospace"
                                    font.pixelSize: 10
                                    color: theme.textSecondary
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 5
                                color: theme.surfaceHeader
                                Rectangle {
                                    width: parent.width * Math.min(1.0, modelData.freqMhz / Math.max(1000, ctrl.cpuMaxFreqMhz))
                                    height: parent.height
                                    color: theme.accentPrimary
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
