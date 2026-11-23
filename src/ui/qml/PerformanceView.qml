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
    property bool stagedEdited: false

    function syncStagedFromHardware() {
        if (ctrl.cpuGovernor.length > 0) root.stagedGov = ctrl.cpuGovernor;
        if (ctrl.cpuEpp.length > 0) root.stagedEpp = ctrl.cpuEpp;
        root.stagedTurbo = ctrl.cpuTurboEnabled;
        root.stagedMinPerf = ctrl.cpuMinPerfPct;
        root.stagedMaxPerf = ctrl.cpuMaxPerfPct;
    }

    Connections {
        target: ctrl
        function onTelemetryUpdated() { if (!root.stagedEdited) root.syncStagedFromHardware(); }
        function onSettingsUpdated() { if (!root.stagedEdited) root.syncStagedFromHardware(); }
    }

    Column {
        id: perfCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // OPERATING PROFILES & POWER TRANSITION RULES
        IndustrialPanel {
            width: parent.width
            height: opHdr.height + profileGrid.implicitHeight + autoRow.implicitHeight + 34

            SectionHeader {
                id: opHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "OPERATING PROFILE ENGINE  //  TUNED-PPD: " + ctrl.activeTunedProfile.toUpperCase()
                badgeText: "FULL RW"
            }

            Grid {
                id: profileGrid
                anchors.top: opHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                columns: width >= 920 ? 4 : 2
                spacing: 10

                Repeater {
                    model: ctrl.operatingProfilesModel
                    delegate: Rectangle {
                        id: profCard
                        width: (parent.width - (parent.columns - 1) * 10) / parent.columns
                        height: 88
                        radius: theme.radiusStructural
                        activeFocusOnTab: true
                        color: modelData.active ? theme.surfaceSelectedDark : theme.surfaceElevated
                        border.width: (profCard.activeFocus || modelData.active) ? 2 : 1
                        border.color: (profCard.activeFocus || modelData.active) ? theme.brandYellow : theme.borderSubtle

                        Rectangle {
                            visible: modelData.active
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 4
                            color: theme.brandYellow
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            anchors.leftMargin: modelData.active ? 12 : 10
                            spacing: 4

                            Text {
                                text: (modelData.active ? "● " : "") + modelData.name.toUpperCase()
                                font.family: theme.fontMono
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: modelData.active ? theme.brandYellow : theme.textPrimary
                            }

                            Text {
                                text: modelData.subtitle
                                width: parent.width
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                font.family: theme.fontSans
                                font.pixelSize: 11
                                color: modelData.active ? theme.surfaceCadLight : theme.textSecondary
                            }

                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: modelData.governor + " · " + modelData.epp + " · " + modelData.minPerfPct + "-" + modelData.maxPerfPct + "%"
                                font.family: theme.fontMono
                                font.pixelSize: 10
                                color: modelData.active ? theme.telemetryLimeBar : theme.textMuted
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                profCard.forceActiveFocus();
                                root.stagedGov = modelData.governor;
                                root.stagedEpp = modelData.epp;
                                root.stagedTurbo = modelData.turboEnabled;
                                root.stagedMinPerf = modelData.minPerfPct;
                                root.stagedMaxPerf = modelData.maxPerfPct;
                                root.stagedEdited = false;
                                ctrl.selectOperatingProfile(modelData.id);
                            }
                        }

                        Keys.onReturnPressed: ctrl.selectOperatingProfile(modelData.id)
                        Keys.onSpacePressed: ctrl.selectOperatingProfile(modelData.id)
                    }
                }
            }

            Flow {
                id: autoRow
                anchors.top: profileGrid.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 14

                AtelierButton {
                    text: ctrl.autoSwitchOnPower ? "AUTO AC/BATTERY SWITCH: ON" : "AUTO AC/BATTERY SWITCH: OFF"
                    activeState: ctrl.autoSwitchOnPower
                    onClicked: ctrl.autoSwitchOnPower = !ctrl.autoSwitchOnPower
                }

                Text {
                    height: 32
                    verticalAlignment: Text.AlignVCenter
                    text: "AC: " + ctrl.acProfileId.toUpperCase() + "  |  BATTERY: " + ctrl.batteryProfileId.toUpperCase()
                    font.family: theme.fontMono
                    font.pixelSize: 11
                    color: theme.textSecondary
                }
            }
        }

        // 02 // INTEL P-STATE & HWP HARDWARE TUNING
        IndustrialPanel {
            width: parent.width
            height: pstateHdr.height + pstateCol.implicitHeight + 28

            SectionHeader {
                id: pstateHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "INTEL P-STATE & HWP HARDWARE TUNING (POLKIT GATED)"
                badgeText: "FULL RW"
            }

            Column {
                id: pstateCol
                anchors.top: pstateHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 14
                spacing: 12

                Flow {
                    width: parent.width
                    spacing: 10

                    Text {
                        width: 128
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "CPU GOVERNOR:"
                        font.family: theme.fontMono
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }

                    AtelierButton {
                        pillShape: false
                        text: "POWERSAVE (HWP)"
                        activeState: root.stagedGov === "powersave"
                        onClicked: { root.stagedEdited = true; root.stagedGov = "powersave"; }
                    }

                    AtelierButton {
                        pillShape: false
                        text: "PERFORMANCE"
                        activeState: root.stagedGov === "performance"
                        onClicked: { root.stagedEdited = true; root.stagedGov = "performance"; }
                    }

                    AtelierButton {
                        pillShape: false
                        text: root.stagedTurbo ? "TURBO: ENABLED (4.9 GHz)" : "TURBO: DISABLED (2.4 GHz)"
                        activeState: root.stagedTurbo
                        onClicked: { root.stagedEdited = true; root.stagedTurbo = !root.stagedTurbo; }
                    }
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    Text {
                        width: 128
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                        text: "HWP EPP HINT:"
                        font.family: theme.fontMono
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }

                    Repeater {
                        model: ["power", "balance_power", "balance_performance", "performance"]
                        delegate: AtelierButton {
                            pillShape: false
                            text: modelData.toUpperCase()
                            activeState: root.stagedEpp === modelData
                            onClicked: { root.stagedEdited = true; root.stagedEpp = modelData; }
                        }
                    }
                }

                Row {
                    width: parent.width
                    spacing: 18

                    AtelierSlider {
                        width: Math.max(180, (parent.width - 250) * 0.5)
                        label: "Minimum P-State Perf"
                        unit: "%"
                        minValue: 10
                        maxValue: 100
                        value: root.stagedMinPerf
                        onValueCommitted: function(v) {
                            root.stagedEdited = true;
                            root.stagedMinPerf = Math.min(v, root.stagedMaxPerf);
                        }
                    }

                    AtelierSlider {
                        width: Math.max(180, (parent.width - 250) * 0.5)
                        label: "Maximum P-State Perf"
                        unit: "%"
                        minValue: 10
                        maxValue: 100
                        value: root.stagedMaxPerf
                        onValueCommitted: function(v) {
                            root.stagedEdited = true;
                            root.stagedMaxPerf = Math.max(v, root.stagedMinPerf);
                        }
                    }

                    AtelierButton {
                        width: 210
                        height: 38
                        anchors.bottom: parent.bottom
                        text: "APPLY & VERIFY P-STATE"
                        primaryAccent: true
                        onClicked: {
                            root.stagedEdited = false;
                            ctrl.applyCustomCpuTuning(
                                root.stagedGov,
                                root.stagedEpp,
                                root.stagedTurbo,
                                root.stagedMinPerf,
                                root.stagedMaxPerf
                            );
                        }
                    }
                }
            }
        }

        // 03 // SYSTEM MEMORY & LINUX PSI PRESSURE INSTRUMENTATION (Section 5.7.D)
        IndustrialPanel {
            width: parent.width
            height: memHdr.height + memBodyCol.implicitHeight + 24

            SectionHeader {
                id: memHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "PHYSICAL MEMORY ALLOCATION & LINUX PSI PRESSURE STALLS"
                badgeText: "READ-ONLY"
            }

            Column {
                id: memBodyCol
                anchors.top: memHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 8

                Grid {
                    width: parent.width
                    columns: width >= 880 ? 5 : 2
                    spacing: 8

                    Repeater {
                        model: [
                            { title: "TOTAL PHYSICAL", val: ctrl.memTotalMiB + " MiB", col: theme.textPrimary },
                            { title: "ACTIVE USED", val: ctrl.memUsedMiB + " MiB", col: theme.accentReadable },
                            { title: "AVAILABLE", val: ctrl.memAvailableMiB + " MiB", col: theme.telemetryLime },
                            { title: "PAGE CACHE", val: ctrl.memCachedMiB + " MiB", col: theme.textPrimary },
                            { title: "KERNEL BUFFERS", val: ctrl.memBuffersMiB + " MiB", col: theme.textSecondary }
                        ]
                        delegate: Rectangle {
                            width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                            height: 52
                            radius: theme.radiusStructural
                            color: theme.surfaceElevated
                            border.width: 1
                            border.color: theme.borderSubtle
                            Column {
                                anchors.centerIn: parent
                                width: parent.width - 14
                                spacing: 2
                                Text { text: modelData.title; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                                Text { text: modelData.val; font.family: theme.fontMono; font.pixelSize: 15; font.weight: Font.Bold; color: modelData.col }
                            }
                        }
                    }
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "System RAM Allocation Ratio"
                    sourceTag: "proc/meminfo"
                    fraction: ctrl.memUtilizationPct / 100.0
                    valueText: ctrl.memUtilizationPct.toFixed(1) + " %  (" + ctrl.memUsedMiB + " MiB)"
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "CPU PSI Contention (some avg10)"
                    sourceTag: ctrl.psiAvailable ? "proc/pressure/cpu" : "UNAVAILABLE"
                    fraction: Math.min(1.0, ctrl.psiCpuSome10 / 25.0)
                    valueText: ctrl.psiCpuSome10.toFixed(2) + " %"
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "Memory PSI Stall (some / full avg10)"
                    sourceTag: ctrl.psiAvailable ? "proc/pressure/memory" : "UNAVAILABLE"
                    fraction: Math.min(1.0, ctrl.psiMemSome10 / 25.0)
                    valueText: "some " + ctrl.psiMemSome10.toFixed(2) + "% · full " + ctrl.psiMemFull10.toFixed(2) + "%"
                }

                TelemetryLoadRow {
                    width: parent.width
                    label: "Block I/O PSI Stall (some avg10)"
                    sourceTag: ctrl.psiAvailable ? "proc/pressure/io" : "UNAVAILABLE"
                    fraction: Math.min(1.0, ctrl.psiIoSome10 / 25.0)
                    valueText: ctrl.psiIoSome10.toFixed(2) + " %"
                }
            }
        }

        // 04 // PER-THREAD FREQUENCY, LOAD & CORE THERMAL RACK
        IndustrialPanel {
            width: parent.width
            height: coreRackHdr.height + coreGrid.implicitHeight + 24

            SectionHeader {
                id: coreRackHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "04"
                title: "PER-THREAD FREQUENCY, UTILIZATION & CORE THERMAL RACK"
                badgeText: "MAX " + ctrl.cpuMaxFreqMhz + " MHz"
            }

            Grid {
                id: coreGrid
                anchors.top: coreRackHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                columns: width >= 960 ? 4 : 2
                spacing: 8

                Repeater {
                    model: ctrl.cpuCoresModel
                    delegate: Rectangle {
                        width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                        height: 42
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.borderSubtle

                        Column {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            Row {
                                width: parent.width
                                Text {
                                    text: modelData.label + " · " + modelData.tempC.toFixed(0) + "°C"
                                    width: parent.width * 0.45
                                    elide: Text.ElideRight
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: modelData.tempC >= 85 ? theme.statusWarning : theme.textPrimary
                                }
                                Text {
                                    text: modelData.freqMhz + " MHz (" + modelData.utilizationPct.toFixed(0) + "%)"
                                    width: parent.width * 0.55
                                    horizontalAlignment: Text.AlignRight
                                    elide: Text.ElideRight
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    color: theme.textSecondary
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 6
                                color: theme.surfaceHeader
                                Rectangle {
                                    width: parent.width * Math.min(1.0, modelData.freqMhz / Math.max(1000, ctrl.cpuMaxFreqMhz))
                                    height: parent.height
                                    color: theme.telemetryLimeBar
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
