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

    readonly property bool splitMidDeck: width >= 940
    readonly property bool telemetryStale: ctrl.telemetryStale
    readonly property bool telemetryUnavailable: !ctrl.telemetryAvailable

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
        anchors.margins: 14
        spacing: 12

        // 01 // OPERATING PROFILES & AUTOMATIC POWER SOURCE POLICY
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "OPERATING PROFILE ENGINE"
            provenanceText: "TUNED-PPD: " + ctrl.activeTunedProfile.toUpperCase()
            badgeText: ctrl.controlOperationPending ? "AUTH PENDING" : "FULL RW"

            Grid {
                width: parent.width
                columns: width >= 900 ? 4 : (width >= 540 ? 2 : 1)
                spacing: 8

                Repeater {
                    model: ctrl.operatingProfilesModel
                    delegate: Rectangle {
                        id: profCard
                        width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                        height: 86
                        radius: theme.radiusStructural
                        activeFocusOnTab: !ctrl.controlOperationPending
                        opacity: ctrl.controlOperationPending && !modelData.active ? 0.65 : 1.0
                        color: modelData.active ? theme.surfaceSelectedDark : theme.surfaceOverlay
                        border.width: (profCard.activeFocus || modelData.active) ? 2 : 1
                        border.color: profCard.activeFocus
                                      ? theme.accentBorder
                                      : (modelData.active ? theme.brandYellow : theme.borderSubtle)

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
                                width: parent.width
                                elide: Text.ElideRight
                                text: (modelData.active ? "● " : "") + modelData.name.toUpperCase()
                                font.family: theme.fontMono
                                font.pixelSize: 11
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
                                color: modelData.active ? "#CFD5D9" : theme.textSecondary
                            }

                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: modelData.governor + " · " + modelData.epp + " (" + modelData.minPerfPct + "-" + modelData.maxPerfPct + "%)"
                                font.family: theme.fontMono
                                font.pixelSize: 9
                                color: modelData.active ? theme.telemetryLimeBar : theme.textMuted
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !ctrl.controlOperationPending
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
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

                        Keys.onReturnPressed: if (!ctrl.controlOperationPending) ctrl.selectOperatingProfile(modelData.id)
                        Keys.onSpacePressed: if (!ctrl.controlOperationPending) ctrl.selectOperatingProfile(modelData.id)
                    }
                }
            }

            Flow {
                width: parent.width
                spacing: 12

                AtelierButton {
                    height: 30
                    pillShape: false
                    text: ctrl.autoSwitchOnPower ? "AUTO AC/BAT SWITCH: ON" : "AUTO AC/BAT SWITCH: OFF"
                    activeState: ctrl.autoSwitchOnPower
                    onClicked: ctrl.autoSwitchOnPower = !ctrl.autoSwitchOnPower
                }

                Text {
                    height: 30
                    verticalAlignment: Text.AlignVCenter
                    text: "AC RULE: " + ctrl.acProfileId.toUpperCase() + "  //  DC BATTERY RULE: " + ctrl.batteryProfileId.toUpperCase()
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
            }
        }

        // 02 // SPLIT WORKBENCH: INTEL P-STATE CONTROL (LEFT) & MEMORY / PSI LEDGER (RIGHT)
        Grid {
            width: parent.width
            columns: root.splitMidDeck ? 2 : 1
            spacing: 12

            // LEFT: INTEL P-STATE & HWP HARDWARE TUNING
            CommandModule {
                id: pstateMod
                width: root.splitMidDeck ? (parent.width - 12) * 0.54 : parent.width
                indexCode: "02"
                title: "INTEL P-STATE & HWP TUNING"
                provenanceText: "/sys/devices/system/cpu/intel_pstate"
                badgeText: root.stagedEdited ? "STAGED UNAPPLIED" : (ctrl.controlOperationPending ? "AUTH PENDING" : "FULL RW")
                badgeColor: root.stagedEdited ? theme.statusWarning : "transparent"

                Column {
                    width: parent.width
                    spacing: 10

                    Text {
                        text: "SCALING GOVERNOR & TURBO BOOST"
                        font.family: theme.fontMono
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: theme.textMuted
                    }

                    Flow {
                        width: parent.width
                        spacing: 8

                        AtelierButton {
                            pillShape: false
                            height: 30
                            text: "POWERSAVE (HWP)"
                            activeState: root.stagedGov === "powersave"
                            onClicked: { root.stagedEdited = true; root.stagedGov = "powersave"; }
                        }

                        AtelierButton {
                            pillShape: false
                            height: 30
                            text: "PERFORMANCE"
                            activeState: root.stagedGov === "performance"
                            onClicked: { root.stagedEdited = true; root.stagedGov = "performance"; }
                        }

                        AtelierButton {
                            pillShape: false
                            height: 30
                            text: root.stagedTurbo ? "TURBO: ON (4.9GHz)" : "TURBO: OFF (2.4GHz)"
                            activeState: root.stagedTurbo
                            onClicked: { root.stagedEdited = true; root.stagedTurbo = !root.stagedTurbo; }
                        }
                    }

                    Text {
                        text: "HWP ENERGY PERFORMANCE PREFERENCE (EPP)"
                        font.family: theme.fontMono
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        color: theme.textMuted
                    }

                    Flow {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: ["power", "balance_power", "balance_performance", "performance"]
                            delegate: AtelierButton {
                                pillShape: false
                                height: 28
                                text: modelData.toUpperCase()
                                activeState: root.stagedEpp === modelData
                                onClicked: { root.stagedEdited = true; root.stagedEpp = modelData; }
                            }
                        }
                    }

                    Grid {
                        width: parent.width
                        columns: width >= 460 ? 2 : 1
                        spacing: 12

                        AtelierSlider {
                            width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                            label: "Min P-State Perf"
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
                            width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                            label: "Max P-State Perf"
                            unit: "%"
                            minValue: 10
                            maxValue: 100
                            value: root.stagedMaxPerf
                            onValueCommitted: function(v) {
                                root.stagedEdited = true;
                                root.stagedMaxPerf = Math.max(v, root.stagedMinPerf);
                            }
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: 8

                        AtelierButton {
                            height: 34
                            text: "APPLY & VERIFY P-STATE"
                            primaryAccent: true
                            pendingState: ctrl.controlOperationPending
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

                        AtelierButton {
                            visible: root.stagedEdited
                            height: 34
                            pillShape: false
                            text: "REVERT TO HARDWARE"
                            onClicked: {
                                root.stagedEdited = false;
                                root.syncStagedFromHardware();
                            }
                        }
                    }
                }
            }

            // RIGHT: SYSTEM MEMORY & LINUX PSI STALLS
            CommandModule {
                id: memMod
                width: root.splitMidDeck ? (parent.width - 12) * 0.46 : parent.width
                height: root.splitMidDeck ? Math.max(pstateMod.implicitHeight, memMod.implicitHeight) : memMod.implicitHeight
                indexCode: "03"
                title: "MEMORY & PSI CONTENTION"
                provenanceText: "/proc/meminfo + /proc/pressure"
                badgeText: root.telemetryStale ? "STALE" : "READ-ONLY"

                Grid {
                    width: parent.width
                    columns: width >= 440 ? 3 : 2
                    spacing: 6

                    Repeater {
                        model: [
                            { title: "TOTAL", val: ctrl.memTotalMiB + " MiB", col: theme.textPrimary },
                            { title: "ACTIVE USED", val: ctrl.memUsedMiB + " MiB", col: theme.accentReadable },
                            { title: "AVAILABLE", val: ctrl.memAvailableMiB + " MiB", col: theme.telemetryLime },
                            { title: "CACHE", val: ctrl.memCachedMiB + " MiB", col: theme.textPrimary },
                            { title: "BUFFERS", val: ctrl.memBuffersMiB + " MiB", col: theme.textSecondary }
                        ]
                        delegate: Rectangle {
                            width: (parent.width - (parent.columns - 1) * 6) / parent.columns
                            height: 46
                            radius: theme.radiusStructural
                            color: theme.surfaceOverlay
                            border.width: 1
                            border.color: theme.borderSubtle
                            Column {
                                anchors.centerIn: parent
                                width: parent.width - 12
                                spacing: 2
                                Text { text: modelData.title; font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted }
                                Text {
                                    width: parent.width; elide: Text.ElideRight
                                    text: root.telemetryUnavailable ? "--" : modelData.val
                                    font.family: theme.fontDisplay; font.pixelSize: 13; font.weight: Font.Bold
                                    color: root.telemetryUnavailable ? theme.textMuted : modelData.col
                                }
                            }
                        }
                    }
                }

                Column {
                    width: parent.width
                    spacing: 6

                    TelemetryLoadRow {
                        width: parent.width
                        label: "RAM Allocation Ratio"
                        sourceTag: "meminfo"
                        fraction: ctrl.memUtilizationPct / 100.0
                        valueText: ctrl.memUtilizationPct.toFixed(1) + "%"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                    TelemetryLoadRow {
                        width: parent.width
                        label: "CPU PSI Contention"
                        sourceTag: ctrl.psiAvailable ? "psi/cpu" : "UNAVAILABLE"
                        fraction: Math.min(1.0, ctrl.psiCpuSome10 / 25.0)
                        valueText: ctrl.psiCpuSome10.toFixed(2) + "%"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                    TelemetryLoadRow {
                        width: parent.width
                        label: "Memory PSI Stall"
                        sourceTag: ctrl.psiAvailable ? "psi/mem" : "UNAVAILABLE"
                        fraction: Math.min(1.0, ctrl.psiMemSome10 / 25.0)
                        valueText: ctrl.psiMemSome10.toFixed(2) + "% / " + ctrl.psiMemFull10.toFixed(2) + "%"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                    TelemetryLoadRow {
                        width: parent.width
                        label: "Block I/O PSI Stall"
                        sourceTag: ctrl.psiAvailable ? "psi/io" : "UNAVAILABLE"
                        fraction: Math.min(1.0, ctrl.psiIoSome10 / 25.0)
                        valueText: ctrl.psiIoSome10.toFixed(2) + "%"
                        stale: root.telemetryStale
                        unavailable: root.telemetryUnavailable
                    }
                }
            }
        }

        // 04 // PER-THREAD FREQUENCY, UTILIZATION & CORE THERMAL RACK
        CommandModule {
            width: parent.width
            indexCode: "04"
            title: "PER-THREAD FREQUENCY & CORE THERMAL RACK"
            provenanceText: "scaling_cur_freq + coretemp"
            badgeText: "MAX " + ctrl.cpuMaxFreqMhz + " MHz"

            Grid {
                width: parent.width
                columns: width >= 960 ? 4 : (width >= 560 ? 2 : 1)
                spacing: 8
                opacity: root.telemetryUnavailable ? 0.50 : (root.telemetryStale ? 0.68 : 1.0)

                Repeater {
                    model: ctrl.cpuCoresModel
                    delegate: Rectangle {
                        width: (parent.width - (parent.columns - 1) * 8) / parent.columns
                        height: 42
                        radius: theme.radiusStructural
                        color: theme.surfaceOverlay
                        border.width: 1
                        border.color: theme.borderSubtle

                        Column {
                            anchors.fill: parent
                            anchors.margins: 7
                            spacing: 4

                            Row {
                                width: parent.width
                                Text {
                                    text: modelData.label + " · " + modelData.tempC.toFixed(0) + "°C"
                                    width: parent.width * 0.46
                                    elide: Text.ElideRight
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: modelData.tempC >= 85 ? theme.statusWarning : theme.textPrimary
                                }
                                Text {
                                    text: modelData.freqMhz + " MHz (" + modelData.utilizationPct.toFixed(0) + "%)"
                                    width: parent.width * 0.54
                                    horizontalAlignment: Text.AlignRight
                                    elide: Text.ElideRight
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    color: theme.textSecondary
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 5
                                color: theme.surfaceSunken
                                Rectangle {
                                    width: parent.width * Math.min(1.0, modelData.freqMhz / Math.max(1000, ctrl.cpuMaxFreqMhz))
                                    height: parent.height
                                    color: modelData.tempC >= 85 ? theme.statusWarning : theme.telemetryLimeBar
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
