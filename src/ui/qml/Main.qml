// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: mainWindow
    width: 1440
    height: 900
    minimumWidth: 680
    minimumHeight: 540
    visible: true
    title: "Acer Control Center — Linux Edition (" + ctrl.productTitle + ")"
    color: theme.canvasBg

    readonly property bool isNarrowLayout: mainWindow.width < 820
    readonly property bool isWideRail: !isNarrowLayout && !theme.compactNavigation && mainWindow.width >= 1160
    readonly property int railWidth: isNarrowLayout ? 0 : (isWideRail ? 222 : 64)

    property var navItems: [
        { code: "01", group: "OPERATIONS", shortTitle: "Deck", title: "Command Deck", icon: "overview", header: "01 // COMMAND DECK & SUBSYSTEM POSTURE", source: "OverviewView.qml" },
        { code: "02", group: "OPERATIONS", shortTitle: "CPU/RAM", title: "Performance", icon: "performance", header: "02 // CPU POLICY & MEMORY ARCHITECTURE", source: "PerformanceView.qml" },
        { code: "03", group: "OPERATIONS", shortTitle: "Thermal", title: "Thermal & Fans", icon: "thermal", header: "03 // THERMAL ENVELOPE & COOLING", source: "ThermalView.qml" },
        { code: "04", group: "OPERATIONS", shortTitle: "RGB", title: "Keyboard Studio", icon: "keyboard", header: "04 // CLV0001 LIGHTING WORKBENCH", source: "KeyboardStudioView.qml" },
        { code: "05", group: "OPERATIONS", shortTitle: "Power", title: "Power & Display", icon: "battery", header: "05 // POWER SUPPLY & eDP LUMINANCE", source: "BatteryView.qml" },
        { code: "06", group: "OPERATIONS", shortTitle: "dGPU", title: "NVIDIA dGPU", icon: "gpu", header: "06 // DISCRETE RTX 3050 TELEMETRY", source: "GpuView.qml" },
        { code: "07", group: "SYSTEM", shortTitle: "Diag", title: "Diagnostics", icon: "diagnostics", header: "07 // PLATFORM IDENTITY & CAPABILITY AUDIT", source: "DiagnosticsView.qml" },
        { code: "08", group: "SYSTEM", shortTitle: "Config", title: "Configuration", icon: "settings", header: "08 // SYSTEM & THEME CONFIGURATION", source: "SettingsView.qml" }
    ]

    function navLiveReadout(idx) {
        if (!ctrl.telemetryAvailable) return "--";
        if (idx === 0) return ctrl.activeProfileId.toUpperCase();
        if (idx === 1) return ctrl.cpuPackageTempC.toFixed(0) + "°C · " + ctrl.cpuUtilizationPct.toFixed(0) + "%";
        if (idx === 2) return ctrl.manualFanSupported ? "PWM" : "EC AUTO";
        if (idx === 3) return ctrl.kbdEnabled ? ("LVL " + ctrl.kbdBrightnessLevel) : "OFF";
        if (idx === 4) return ctrl.batCapacityPct + "% · " + (ctrl.acOnline ? "AC" : "DC");
        if (idx === 5) return ctrl.gpuAvailable ? ctrl.gpuPstate : "N/A";
        if (idx === 6) return ctrl.capabilitiesModel.length + " CAPS";
        if (idx === 7) return theme.darkTheme ? "HUD" : "CAD";
        return "";
    }

    Shortcut { sequence: "Alt+1"; onActivated: ctrl.currentTabIndex = 0 }
    Shortcut { sequence: "Alt+2"; onActivated: ctrl.currentTabIndex = 1 }
    Shortcut { sequence: "Alt+3"; onActivated: ctrl.currentTabIndex = 2 }
    Shortcut { sequence: "Alt+4"; onActivated: ctrl.currentTabIndex = 3 }
    Shortcut { sequence: "Alt+5"; onActivated: ctrl.currentTabIndex = 4 }
    Shortcut { sequence: "Alt+6"; onActivated: ctrl.currentTabIndex = 5 }
    Shortcut { sequence: "Alt+7"; onActivated: ctrl.currentTabIndex = 6 }
    Shortcut { sequence: "Alt+8"; onActivated: ctrl.currentTabIndex = 7 }

    // 52PX GLOBAL COMMAND HEADER
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        Row {
            id: brandRow
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: topRightRow.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // Industrial mark
            Rectangle {
                width: 32
                height: 32
                radius: theme.radiusStructural
                color: theme.brandYellow
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    anchors.centerIn: parent
                    text: "ACC"
                    font.family: theme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.Black
                    color: theme.darkInk
                }
            }

            Column {
                width: Math.max(80, parent.width - 42)
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                Row {
                    width: parent.width
                    spacing: 8
                    Text {
                        text: mainWindow.width < 900 ? "ACER CTRL" : "ACER CONTROL CENTER"
                        font.family: theme.fontDisplay
                        font.pixelSize: 14
                        font.weight: Font.Black
                        font.letterSpacing: 1.0
                        color: theme.textPrimary
                    }
                    Rectangle {
                        visible: mainWindow.width >= 960
                        width: editionLbl.implicitWidth + 10
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter
                        radius: theme.radiusStructural
                        color: theme.surfaceOverlay
                        border.width: 1
                        border.color: theme.borderSubtle
                        Text {
                            id: editionLbl
                            anchors.centerIn: parent
                            text: "FEDORA 45 // ENDFIELD-CMD"
                            font.family: theme.fontMono
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: theme.accentReadable
                        }
                    }
                }

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: ctrl.productTitle + "  //  " + ctrl.boardTitle + "  //  " + ctrl.biosTitle
                    font.family: theme.fontMono
                    font.pixelSize: 9
                    color: theme.textSecondary
                }
            }
        }

        // Right live telemetry pills, active mode & async sync trigger
        Row {
            id: topRightRow
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            // CPU pill
            Rectangle {
                visible: mainWindow.width >= 920
                height: 26
                width: cpuTopRow.implicitWidth + 14
                radius: theme.radiusStructural
                color: theme.surfacePrimary
                border.width: 1
                border.color: ctrl.telemetryStale ? theme.statusWarning : theme.borderSubtle
                opacity: ctrl.telemetryAvailable ? (ctrl.telemetryStale ? 0.72 : 1.0) : 0.55
                Row {
                    id: cpuTopRow
                    anchors.centerIn: parent
                    spacing: 5
                    Rectangle {
                        width: 6; height: 6; radius: 1; anchors.verticalCenter: parent.verticalCenter
                        color: !ctrl.telemetryAvailable
                               ? theme.textMuted
                               : (ctrl.cpuPackageTempC >= 85 ? theme.statusCritical : theme.telemetryLime)
                    }
                    Text {
                        text: ctrl.telemetryAvailable
                              ? ("CPU " + ctrl.cpuPackageTempC.toFixed(0) + "°C · " + ctrl.cpuUtilizationPct.toFixed(0) + "%")
                              : "CPU --"
                        font.family: theme.fontMono
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        color: theme.textPrimary
                    }
                }
            }

            // dGPU pill
            Rectangle {
                visible: mainWindow.width >= 1080
                height: 26
                width: gpuTopText.implicitWidth + 14
                radius: theme.radiusStructural
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                opacity: ctrl.telemetryAvailable ? (ctrl.telemetryStale ? 0.72 : 1.0) : 0.55
                Text {
                    id: gpuTopText
                    anchors.centerIn: parent
                    text: !ctrl.telemetryAvailable
                          ? "GPU --"
                          : (ctrl.gpuAvailable
                             ? (ctrl.gpuTelemetryMeasuredByNvml
                                ? ("RTX 3050 " + ctrl.gpuTempC.toFixed(0) + "°C · " + ctrl.gpuPstate)
                                : ("dGPU " + ctrl.gpuPstate))
                             : "GPU N/A")
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: theme.textPrimary
                }
            }

            // Battery pill
            Rectangle {
                visible: mainWindow.width >= 1240
                height: 26
                width: batTopText.implicitWidth + 14
                radius: theme.radiusStructural
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                opacity: ctrl.telemetryAvailable ? (ctrl.telemetryStale ? 0.72 : 1.0) : 0.55
                Text {
                    id: batTopText
                    anchors.centerIn: parent
                    text: ctrl.telemetryAvailable
                          ? ("BAT " + ctrl.batCapacityPct + "% · " + (ctrl.acOnline ? "AC" : "DC"))
                          : "BAT --"
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning
                }
            }

            // Active Operating Profile stadium pill
            Rectangle {
                height: 26
                width: profTopText.implicitWidth + 16
                radius: height / 2
                color: theme.accentPrimary
                Text {
                    id: profTopText
                    anchors.centerIn: parent
                    text: "MODE: " + ctrl.activeProfileId.toUpperCase()
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: theme.accentOnText
                }
            }

            // Hardware posture badge
            CapabilityBadge {
                anchors.verticalCenter: parent.verticalCenter
                height: 26
                badgeText: ctrl.mockModeActive ? "MOCK HW" : "PHYSICAL HW"
                customColor: ctrl.mockModeActive ? theme.statusWarning : theme.statusNominal
            }

            // Manual async refresh action button
            AtelierButton {
                height: 26
                pillShape: false
                text: ctrl.telemetryRefreshing ? "SYNCING" : "SYNC"
                pendingState: ctrl.telemetryRefreshing
                onClicked: ctrl.refreshNow()
            }
        }
    }

    // LEFT ARCHITECTURAL COMMAND RAIL (Visible on wide/medium viewports >= 820px)
    Rectangle {
        id: navRail
        visible: !mainWindow.isNarrowLayout
        anchors.top: topBar.bottom
        anchors.bottom: bottomStatusBar.top
        anchors.left: parent.left
        width: mainWindow.railWidth
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        Behavior on width {
            NumberAnimation { duration: theme.transitionDurationMs }
        }

        Column {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 8
            spacing: 4

            Repeater {
                model: mainWindow.navItems
                delegate: Column {
                    width: parent.width
                    spacing: 4

                    // Group divider header before Index 00 & Index 06 (Wide rail only)
                    Item {
                        visible: mainWindow.isWideRail && (index === 0 || index === 6)
                        width: parent.width
                        height: index === 0 ? 18 : 26

                        Rectangle {
                            visible: index === 6
                            anchors.top: parent.top
                            anchors.topMargin: 4
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 1
                            color: theme.hairlineRule
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            text: "// " + modelData.group
                            font.family: theme.fontMono
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            font.letterSpacing: 1.0
                            color: theme.textMuted
                        }
                    }

                    Rectangle {
                        id: navDelegate
                        width: parent.width
                        height: mainWindow.isWideRail ? 44 : 46
                        radius: theme.radiusStructural
                        activeFocusOnTab: true
                        property bool isCurrent: ctrl.currentTabIndex === index
                        property bool isCompact: !mainWindow.isWideRail
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData.code + " " + modelData.title
                        Accessible.checkable: true
                        Accessible.checked: navDelegate.isCurrent

                        color: isCurrent
                               ? theme.surfaceSelectedDark
                               : (navMouse.containsMouse ? theme.surfaceElevated : "transparent")
                        border.width: navDelegate.activeFocus ? 2 : (isCurrent ? 1 : 0)
                        border.color: navDelegate.activeFocus
                                      ? theme.accentBorder
                                      : (isCurrent ? theme.brandYellow : "transparent")

                        Rectangle {
                            visible: navDelegate.isCurrent
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 4
                            color: theme.brandYellow
                        }

                        Row {
                            visible: !navDelegate.isCompact
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            spacing: 10

                            Rectangle {
                                width: 28
                                height: 28
                                radius: 0
                                anchors.verticalCenter: parent.verticalCenter
                                color: navDelegate.isCurrent ? theme.brandYellow : theme.surfaceSunken
                                border.width: navDelegate.isCurrent ? 0 : 1
                                border.color: theme.hairlineRule

                                AtelierIcon {
                                    anchors.centerIn: parent
                                    iconName: modelData.icon
                                    strokeColor: navDelegate.isCurrent ? theme.darkInk : theme.textSecondary
                                }
                            }

                            Column {
                                width: parent.width - 38
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 1

                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: modelData.code + " · " + modelData.title.toUpperCase()
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    font.weight: navDelegate.isCurrent ? Font.Bold : Font.Medium
                                    color: navDelegate.isCurrent ? theme.textOnSelectedDark : theme.textPrimary
                                }
                                Row {
                                    width: parent.width
                                    Text {
                                        width: parent.width * 0.42
                                        text: "ALT+" + (index + 1)
                                        font.family: theme.fontMono
                                        font.pixelSize: 9
                                        color: navDelegate.isCurrent ? theme.brandYellow : theme.textMuted
                                    }
                                    Text {
                                        width: parent.width * 0.58
                                        horizontalAlignment: Text.AlignRight
                                        elide: Text.ElideRight
                                        text: mainWindow.navLiveReadout(index)
                                        font.family: theme.fontMono
                                        font.pixelSize: 9
                                        font.weight: Font.DemiBold
                                        color: navDelegate.isCurrent ? theme.telemetryLimeBar : theme.textSecondary
                                    }
                                }
                            }
                        }

                        // Compact 64px rail item (icon + code)
                        Column {
                            visible: navDelegate.isCompact
                            anchors.centerIn: parent
                            spacing: 2

                            Rectangle {
                                width: 24
                                height: 24
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: navDelegate.isCurrent ? theme.brandYellow : "transparent"
                                AtelierIcon {
                                    anchors.centerIn: parent
                                    iconName: modelData.icon
                                    strokeColor: navDelegate.isCurrent ? theme.darkInk : theme.textSecondary
                                }
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: modelData.code
                                font.family: theme.fontMono
                                font.pixelSize: 9
                                font.weight: Font.Bold
                                color: navDelegate.isCurrent ? theme.brandYellow : theme.textMuted
                            }
                        }

                        MouseArea {
                            id: navMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                navDelegate.forceActiveFocus();
                                ctrl.currentTabIndex = index;
                            }
                        }

                        Keys.onReturnPressed: ctrl.currentTabIndex = index
                        Keys.onSpacePressed: ctrl.currentTabIndex = index
                    }
                }
            }
        }

        // Bottom rail telemetry summary
        Column {
            visible: mainWindow.isWideRail
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            spacing: 3

            Rectangle { width: parent.width; height: 1; color: theme.hairlineRule }

            Text {
                text: "ASYNC WORKER TELEMETRY"
                font.family: theme.fontMono
                font.pixelSize: 9
                font.weight: Font.Bold
                color: theme.textSecondary
            }
            Text {
                text: "POLL: " + ctrl.pollingIntervalMs + "ms · FRESHNESS: " + ctrl.telemetryFreshness.toUpperCase()
                font.family: theme.fontMono
                font.pixelSize: 9
                color: ctrl.telemetryStale ? theme.statusWarning : theme.textMuted
            }
            Text {
                text: "DAEMON: " + (ctrl.daemonReachable ? "ONLINE (POLKIT)" : "OFFLINE")
                font.family: theme.fontMono
                font.pixelSize: 9
                color: ctrl.daemonReachable ? theme.telemetryLime : theme.statusWarning
            }
        }
    }

    // HORIZONTAL TILED NAVIGATION STRIP (< 820px width)
    Rectangle {
        id: narrowTabStrip
        visible: mainWindow.isNarrowLayout
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: visible ? 40 : 0
        color: theme.surfaceSunken
        border.width: 1
        border.color: theme.borderSubtle

        Flickable {
            anchors.fill: parent
            anchors.margins: 4
            contentWidth: narrowTabRow.implicitWidth
            clip: true

            Row {
                id: narrowTabRow
                height: parent.height
                spacing: 5

                Repeater {
                    model: mainWindow.navItems
                    delegate: AtelierButton {
                        height: 30
                        pillShape: false
                        text: modelData.code + " " + modelData.shortTitle.toUpperCase()
                        activeState: ctrl.currentTabIndex === index
                        onClicked: ctrl.currentTabIndex = index
                    }
                }
            }
        }
    }

    // CONTEXTUAL WORKSPACE HEADER BAR (32px)
    Rectangle {
        id: workspaceBanner
        anchors.top: narrowTabStrip.visible ? narrowTabStrip.bottom : topBar.bottom
        anchors.left: navRail.visible ? navRail.right : parent.left
        anchors.right: parent.right
        height: 32
        color: theme.surfacePrimary
        border.width: 1
        border.color: theme.borderSubtle

        Row {
            id: wsLeftRow
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: wsRightMeta.visible ? wsRightMeta.left : parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "+"
                font.family: theme.fontMono
                font.pixelSize: 12
                font.weight: Font.Bold
                color: theme.brandYellow
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(80, parent.width - 22)
                elide: Text.ElideRight
                text: "// " + mainWindow.navItems[ctrl.currentTabIndex].header
                font.family: theme.fontMono
                font.pixelSize: 11
                font.weight: Font.Bold
                font.letterSpacing: 0.8
                color: theme.textPrimary
            }
        }

        Text {
            id: wsRightMeta
            visible: mainWindow.width >= 960
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: "FAN: " + ctrl.fanCapabilityBadge + "  ·  BAT CARE: " + (ctrl.batChargeLimitSupported ? "RW" : "NOT EXPOSED") + "  ·  KBD: " + ctrl.kbdControlModeBadge
            font.family: theme.fontMono
            font.pixelSize: 9
            color: theme.textSecondary
        }
    }

    // NON-BLOCKING STAGE 24 ASYNC TELEMETRY & POLKIT AUTH STATE STRIP
    TelemetryStateBanner {
        id: globalStateBanner
        anchors.top: workspaceBanner.bottom
        anchors.topMargin: visible ? 8 : 0
        anchors.left: navRail.visible ? navRail.right : parent.left
        anchors.leftMargin: 14
        anchors.right: parent.right
        anchors.rightMargin: 14
    }

    // CENTRAL WORKSPACE LOADER (Lazy single-view instantiation; Stage 24)
    Item {
        id: workspaceArea
        anchors.top: globalStateBanner.visible ? globalStateBanner.bottom : workspaceBanner.bottom
        anchors.bottom: bottomStatusBar.top
        anchors.left: navRail.visible ? navRail.right : parent.left
        anchors.right: parent.right

        Loader {
            id: workspaceLoader
            objectName: "workspaceLoader"
            anchors.fill: parent
            asynchronous: false
            source: mainWindow.navItems[ctrl.currentTabIndex].source
        }
    }

    // 30PX BOTTOM HARDWARE READBACK & TELEMETRY CONDUIT BAR
    Rectangle {
        id: bottomStatusBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 30
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: runtimeMetaRow.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Rectangle {
                width: 8
                height: 8
                radius: 1
                anchors.verticalCenter: parent.verticalCenter
                color: ctrl.controlOperationPending
                       ? theme.industrialAmber
                       : (ctrl.statusBannerIsError ? theme.statusCritical : theme.telemetryLime)
            }

            Text {
                width: Math.max(80, parent.width - 18)
                elide: Text.ElideRight
                text: ctrl.statusBannerText
                font.family: theme.fontMono
                font.pixelSize: 10
                color: ctrl.statusBannerIsError ? theme.statusCritical : theme.textSecondary
            }
        }

        Row {
            id: runtimeMetaRow
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Text {
                id: telemetryFreshnessLbl
                objectName: "telemetryFreshnessLbl"
                anchors.verticalCenter: parent.verticalCenter
                text: "TELEMETRY: " + ctrl.telemetryFreshness.toUpperCase()
                      + (ctrl.telemetryRefreshing ? " (POLLING)" : "")
                      + (ctrl.controlOperationPending ? "  |  AUTH: PENDING" : (ctrl.controlAuthState === "denied" ? "  |  AUTH: DENIED" : ""))
                font.family: theme.fontMono
                font.pixelSize: 10
                color: ctrl.telemetryFreshness === "fresh"
                       ? theme.telemetryLime
                       : (ctrl.telemetryFreshness === "stale" ? theme.statusWarning : theme.statusCritical)
            }

            Text {
                visible: mainWindow.width >= 960
                anchors.verticalCenter: parent.verticalCenter
                text: ctrl.hardwareModeLabel.toUpperCase() + "  |  KERNEL " + ctrl.kernelRelease
                font.family: theme.fontMono
                font.pixelSize: 10
                color: theme.textMuted
            }
        }
    }
}
