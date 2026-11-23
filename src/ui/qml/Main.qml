// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: mainWindow
    width: 1440
    height: 900
    minimumWidth: 1024
    minimumHeight: 680
    visible: true
    title: "Acer Control Center — Linux Edition (" + ctrl.productTitle + ")"
    color: theme.canvasBg

    property var navItems: [
        { code: "01", title: "Overview", icon: "overview", header: "// 01 · COMMAND CENTER OVERVIEW" },
        { code: "02", title: "CPU & Memory", icon: "performance", header: "// 02 · CPU PERFORMANCE & MEMORY ARCHITECTURE" },
        { code: "03", title: "Thermal & Fans", icon: "thermal", header: "// 03 · THERMAL ENVELOPE & COOLING SYSTEM" },
        { code: "04", title: "Keyboard Studio", icon: "keyboard", header: "// 04 · CLV0001 KEYBOARD LIGHTING STUDIO" },
        { code: "05", title: "Battery & Display", icon: "battery", header: "// 05 · BATTERY MANAGEMENT & DISPLAY LUMINANCE" },
        { code: "06", title: "NVIDIA dGPU", icon: "gpu", header: "// 06 · NVIDIA RTX 3050 DISCRETE GPU" },
        { code: "07", title: "Diagnostics", icon: "diagnostics", header: "// 07 · HARDWARE DIAGNOSTICS & CAPABILITY MATRIX" },
        { code: "08", title: "Settings", icon: "settings", header: "// 08 · DESIGN SYSTEM & CONFIGURATION" }
    ]

    Shortcut { sequence: "Alt+1"; onActivated: ctrl.currentTabIndex = 0 }
    Shortcut { sequence: "Alt+2"; onActivated: ctrl.currentTabIndex = 1 }
    Shortcut { sequence: "Alt+3"; onActivated: ctrl.currentTabIndex = 2 }
    Shortcut { sequence: "Alt+4"; onActivated: ctrl.currentTabIndex = 3 }
    Shortcut { sequence: "Alt+5"; onActivated: ctrl.currentTabIndex = 4 }
    Shortcut { sequence: "Alt+6"; onActivated: ctrl.currentTabIndex = 5 }
    Shortcut { sequence: "Alt+7"; onActivated: ctrl.currentTabIndex = 6 }
    Shortcut { sequence: "Alt+8"; onActivated: ctrl.currentTabIndex = 7 }

    // 56PX TOP GLOBAL COMMAND & TELEMETRY HEADER
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        // Left active registration block + Acer brand identity
        Row {
            id: brandRow
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            Rectangle {
                width: 34
                height: 34
                radius: theme.radiusStructural
                color: theme.brandYellow
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    anchors.centerIn: parent
                    text: "ACC"
                    font.family: theme.fontMono
                    font.pixelSize: 12
                    font.weight: Font.Black
                    color: theme.darkInk
                }
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Row {
                    spacing: 8
                    Text {
                        text: "ACER CONTROL CENTER"
                        font.family: theme.fontDisplay
                        font.pixelSize: 14
                        font.weight: Font.Black
                        font.letterSpacing: 1.2
                        color: theme.textPrimary
                    }
                    Rectangle {
                        width: editionLbl.implicitWidth + 10
                        height: 16
                        anchors.verticalCenter: parent.verticalCenter
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.borderStrong
                        Text {
                            id: editionLbl
                            anchors.centerIn: parent
                            text: "FEDORA 45 // INDUSTRIAL"
                            font.family: theme.fontMono
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: theme.accentReadable
                        }
                    }
                }
                Text {
                    text: ctrl.productTitle + "  //  BOARD: " + ctrl.boardTitle + "  //  " + ctrl.biosTitle
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
            }
        }

        // Right live telemetry pills & active hardware posture
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            // Live CPU pill
            Rectangle {
                visible: mainWindow.width >= 1120
                height: 28
                width: cpuTopRow.implicitWidth + 16
                radius: theme.radiusStructural
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                Row {
                    id: cpuTopRow
                    anchors.centerIn: parent
                    spacing: 6
                    Rectangle {
                        width: 6; height: 6; radius: 1; anchors.verticalCenter: parent.verticalCenter
                        color: ctrl.cpuPackageTempC >= 85 ? theme.statusCritical : theme.telemetryLime
                    }
                    Text {
                        text: "CPU " + ctrl.cpuPackageTempC.toFixed(0) + "°C · " + ctrl.cpuUtilizationPct.toFixed(0) + "%"
                        font.family: theme.fontMono
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: theme.textPrimary
                    }
                }
            }

            // Live GPU pill
            Rectangle {
                visible: mainWindow.width >= 1220
                height: 28
                width: gpuTopText.implicitWidth + 16
                radius: theme.radiusStructural
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                Text {
                    id: gpuTopText
                    anchors.centerIn: parent
                    text: ctrl.gpuAvailable
                          ? (ctrl.gpuTelemetryMeasuredByNvml
                                 ? ("RTX 3050 " + ctrl.gpuTempC.toFixed(0) + "°C · " + ctrl.gpuPstate)
                                 : ("dGPU " + ctrl.gpuPstate))
                          : "GPU N/A"
                    font.family: theme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: theme.textPrimary
                }
            }

            // Live BAT0 pill
            Rectangle {
                visible: mainWindow.width >= 1340
                height: 28
                width: batTopText.implicitWidth + 16
                radius: theme.radiusStructural
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                Text {
                    id: batTopText
                    anchors.centerIn: parent
                    text: "BAT " + ctrl.batCapacityPct + "% · " + (ctrl.acOnline ? "AC" : "DC")
                    font.family: theme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: ctrl.acOnline ? theme.telemetryLime : theme.statusWarning
                }
            }

            // Active Operating Mode badge
            Rectangle {
                height: 28
                width: profTopText.implicitWidth + 16
                radius: height / 2
                color: theme.accentPrimary
                Text {
                    id: profTopText
                    anchors.centerIn: parent
                    text: "MODE: " + ctrl.activeProfileId.toUpperCase()
                    font.family: theme.fontMono
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: theme.accentOnText
                }
            }

            // Physical / Mock Hardware Badge
            CapabilityBadge {
                anchors.verticalCenter: parent.verticalCenter
                height: 28
                badgeText: ctrl.mockModeActive ? "MOCK HARDWARE" : "PHYSICAL HW"
                customColor: ctrl.mockModeActive ? theme.statusWarning : theme.statusNominal
            }
        }
    }

    // LEFT ARCHITECTURAL NAVIGATION RAIL (CMP-0001)
    Rectangle {
        id: navRail
        anchors.top: topBar.bottom
        anchors.bottom: bottomStatusBar.top
        anchors.left: parent.left
        width: (theme.compactNavigation || mainWindow.width < 1180) ? 64 : 224
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
            spacing: 5

            Repeater {
                model: mainWindow.navItems
                delegate: Rectangle {
                    id: navDelegate
                    width: parent.width
                    height: 44
                    radius: theme.radiusStructural
                    activeFocusOnTab: true
                    property bool isCurrent: ctrl.currentTabIndex === index
                    property bool isCompact: navRail.width < 120

                    color: isCurrent
                           ? theme.surfaceSelectedDark
                           : (navMouse.containsMouse ? theme.surfaceElevated : "transparent")
                    border.width: navDelegate.activeFocus ? 2 : (isCurrent ? 1 : 0)
                    border.color: navDelegate.activeFocus
                                  ? theme.accentBorder
                                  : (isCurrent ? theme.brandYellow : "transparent")

                    // Active vertical accent bar
                    Rectangle {
                        visible: navDelegate.isCurrent
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 4
                        color: theme.brandYellow
                    }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: isCompact ? 0 : 12
                        anchors.rightMargin: isCompact ? 0 : 8
                        spacing: 12

                        // Square icon housing (Brand Yellow square when active)
                        Rectangle {
                            width: 30
                            height: 30
                            radius: 0
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: isCompact ? parent.horizontalCenter : undefined
                            color: navDelegate.isCurrent ? theme.brandYellow : "transparent"

                            AtelierIcon {
                                anchors.centerIn: parent
                                iconName: modelData.icon
                                strokeColor: navDelegate.isCurrent ? theme.darkInk : theme.textSecondary
                            }
                        }

                        Column {
                            visible: !isCompact
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 1

                            Text {
                                text: modelData.code + " // " + modelData.title.toUpperCase()
                                font.family: theme.fontMono
                                font.pixelSize: 11
                                font.weight: navDelegate.isCurrent ? Font.Bold : Font.Medium
                                color: navDelegate.isCurrent ? theme.textOnSelectedDark : theme.textSecondary
                            }
                            Text {
                                text: "ALT+" + (index + 1)
                                font.family: theme.fontMono
                                font.pixelSize: 9
                                color: navDelegate.isCurrent ? theme.brandYellow : theme.textMuted
                            }
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

        // Bottom engineering rail metadata
        Column {
            visible: navRail.width >= 180
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            spacing: 4

            Rectangle {
                width: parent.width
                height: 1
                color: theme.borderSubtle
            }

            Text {
                text: "INDUSTRIAL TELEMETRY"
                font.family: theme.fontMono
                font.pixelSize: 9
                font.weight: Font.Bold
                color: theme.textSecondary
            }
            Text {
                text: "CADENCE: " + ctrl.pollingIntervalMs + " MS"
                font.family: theme.fontMono
                font.pixelSize: 9
                color: theme.textMuted
            }
            Text {
                text: "DAEMON: " + (ctrl.daemonReachable ? "ONLINE (POLKIT)" : "OFFLINE")
                font.family: theme.fontMono
                font.pixelSize: 9
                color: ctrl.daemonReachable ? theme.telemetryLime : theme.statusWarning
            }
        }
    }

    // CONTEXTUAL WORKSPACE HEADER BAR (34px)
    Rectangle {
        id: workspaceBanner
        anchors.top: topBar.bottom
        anchors.left: navRail.right
        anchors.right: parent.right
        height: 34
        color: theme.surfacePrimary
        border.width: 1
        border.color: theme.borderSubtle

        Row {
            id: wsLeftRow
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "+"
                font.family: theme.fontMono
                font.pixelSize: 13
                font.weight: Font.Bold
                color: theme.brandYellow
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: mainWindow.navItems[ctrl.currentTabIndex].header
                font.family: theme.fontMono
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 1.0
                color: theme.textPrimary
            }
        }

        Text {
            anchors.left: wsLeftRow.right
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            text: "FANS: " + ctrl.fanCapabilityBadge + "  ·  BAT CARE: " + (ctrl.batChargeLimitSupported ? ctrl.batCapabilityBadge : "CHARGE LIMIT NOT DETECTED") + "  ·  KBD: " + ctrl.kbdCapabilityBadge
            font.family: theme.fontMono
            font.pixelSize: 10
            color: theme.textSecondary
        }
    }

    // CENTRAL WORKSPACE STACK
    Item {
        id: workspaceArea
        anchors.top: workspaceBanner.bottom
        anchors.bottom: bottomStatusBar.top
        anchors.left: navRail.right
        anchors.right: parent.right

        OverviewView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 0
        }
        PerformanceView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 1
        }
        ThermalView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 2
        }
        KeyboardStudioView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 3
        }
        BatteryView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 4
        }
        GpuView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 5
        }
        DiagnosticsView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 6
        }
        SettingsView {
            anchors.fill: parent
            visible: ctrl.currentTabIndex === 7
        }
    }

    // 30PX BOTTOM HARDWARE READBACK & TELEMETRY FOOTER (TYP-008)
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
            anchors.leftMargin: 14
            anchors.right: hwModeFooterLbl.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Rectangle {
                width: 8
                height: 8
                radius: 1
                anchors.verticalCenter: parent.verticalCenter
                color: ctrl.statusBannerIsError ? theme.statusCritical : theme.telemetryLime
            }

            Text {
                width: Math.max(120, parent.width - 20)
                elide: Text.ElideRight
                text: ctrl.statusBannerText
                font.family: theme.fontMono
                font.pixelSize: 11
                color: ctrl.statusBannerIsError ? theme.statusCritical : theme.textSecondary
            }
        }

        Text {
            id: hwModeFooterLbl
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: ctrl.hardwareModeLabel.toUpperCase() + "  |  KERNEL " + ctrl.kernelRelease
            font.family: theme.fontMono
            font.pixelSize: 10
            color: theme.textMuted
        }
    }
}
