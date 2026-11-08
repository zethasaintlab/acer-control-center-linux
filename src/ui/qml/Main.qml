// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: mainWindow
    width: 1360
    height: 860
    minimumWidth: 1120
    minimumHeight: 720
    visible: true
    title: "Acer Control Center — Linux Edition (" + ctrl.productTitle + ")"
    color: theme.canvasBg

    property var navItems: [
        { code: "01", title: "Overview", icon: "overview" },
        { code: "02", title: "Performance", icon: "performance" },
        { code: "03", title: "Thermal & Fans", icon: "thermal" },
        { code: "04", title: "Keyboard Studio", icon: "keyboard" },
        { code: "05", title: "Battery & Power", icon: "battery" },
        { code: "06", title: "NVIDIA GPU", icon: "gpu" },
        { code: "07", title: "Diagnostics", icon: "diagnostics" },
        { code: "08", title: "Settings", icon: "settings" }
    ]

    Shortcut { sequence: "Alt+1"; onActivated: ctrl.currentTabIndex = 0 }
    Shortcut { sequence: "Alt+2"; onActivated: ctrl.currentTabIndex = 1 }
    Shortcut { sequence: "Alt+3"; onActivated: ctrl.currentTabIndex = 2 }
    Shortcut { sequence: "Alt+4"; onActivated: ctrl.currentTabIndex = 3 }
    Shortcut { sequence: "Alt+5"; onActivated: ctrl.currentTabIndex = 4 }
    Shortcut { sequence: "Alt+6"; onActivated: ctrl.currentTabIndex = 5 }
    Shortcut { sequence: "Alt+7"; onActivated: ctrl.currentTabIndex = 6 }
    Shortcut { sequence: "Alt+8"; onActivated: ctrl.currentTabIndex = 7 }

    // TOP ARCHITECTURAL INSTRUMENT STATUS BAR
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 46
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            Rectangle {
                width: 8
                height: 22
                color: theme.accentPrimary
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1
                Text {
                    text: "ACER CONTROL CENTER // LINUX EDITION"
                    font.family: "monospace"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.letterSpacing: 1.0
                    color: theme.textPrimary
                }
                Text {
                    text: ctrl.productTitle + "  //  BOARD: " + ctrl.boardTitle + "  //  BIOS: " + ctrl.biosTitle
                    font.family: "monospace"
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            // Live CPU Pill
            Rectangle {
                height: 26
                width: cpuTopText.implicitWidth + 16
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                Text {
                    id: cpuTopText
                    anchors.centerIn: parent
                    text: "CPU " + ctrl.cpuPackageTempC.toFixed(0) + "°C / " + ctrl.cpuUtilizationPct.toFixed(0) + "%"
                    font.family: "monospace"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: theme.textPrimary
                }
            }

            // Live GPU Pill
            Rectangle {
                height: 26
                width: gpuTopText.implicitWidth + 16
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle
                Text {
                    id: gpuTopText
                    anchors.centerIn: parent
                    text: ctrl.gpuAvailable ? ("RTX 3050 " + ctrl.gpuTempC.toFixed(0) + "°C / " + ctrl.gpuPstate) : "GPU N/A"
                    font.family: "monospace"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: theme.textPrimary
                }
            }

            // Active Profile Badge
            Rectangle {
                height: 26
                width: profTopText.implicitWidth + 16
                color: theme.accentSurfaceTint
                border.width: 1
                border.color: theme.accentBorder
                Text {
                    id: profTopText
                    anchors.centerIn: parent
                    text: "MODE: " + ctrl.activeProfileId.toUpperCase()
                    font.family: "monospace"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    color: theme.accentReadable
                }
            }

            // Hardware / Mock Mode Badge
            Rectangle {
                height: 26
                width: hwModeTopText.implicitWidth + 16
                color: "transparent"
                border.width: 1
                border.color: ctrl.mockModeActive ? theme.statusWarning : theme.statusNominal
                Text {
                    id: hwModeTopText
                    anchors.centerIn: parent
                    text: ctrl.mockModeActive ? "MOCK HARDWARE" : "PHYSICAL HW"
                    font.family: "monospace"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    color: ctrl.mockModeActive ? theme.statusWarning : theme.statusNominal
                }
            }
        }
    }

    // LEFT ARCHITECTURAL NAVIGATION RAIL
    Rectangle {
        id: navRail
        anchors.top: topBar.bottom
        anchors.bottom: bottomStatusBar.top
        anchors.left: parent.left
        width: theme.compactNavigation ? 64 : 216
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
                delegate: Rectangle {
                    id: navDelegate
                    width: parent.width
                    height: 42
                    radius: 2
                    activeFocusOnTab: true
                    property bool isCurrent: ctrl.currentTabIndex === index

                    color: isCurrent ? theme.accentSurfaceTint : (navMouse.containsMouse ? theme.surfaceElevated : "transparent")
                    border.width: navDelegate.activeFocus ? 2 : (isCurrent ? 1 : 0)
                    border.color: navDelegate.activeFocus || isCurrent ? theme.accentBorder : "transparent"

                    Rectangle {
                        visible: navDelegate.isCurrent
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 3
                        color: theme.accentPrimary
                    }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        spacing: 12

                        AtelierIcon {
                            anchors.verticalCenter: parent.verticalCenter
                            iconName: modelData.icon
                            strokeColor: navDelegate.isCurrent ? theme.accentReadable : theme.textSecondary
                        }

                        Column {
                            visible: !theme.compactNavigation
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 1

                            Text {
                                text: modelData.code + " // " + modelData.title.toUpperCase()
                                font.family: "monospace"
                                font.pixelSize: 11
                                font.weight: navDelegate.isCurrent ? Font.Bold : Font.Medium
                                color: navDelegate.isCurrent ? theme.textPrimary : theme.textSecondary
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

        // Bottom rail metadata
        Column {
            visible: !theme.compactNavigation
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            spacing: 4

            Text {
                text: "PRECISION HARDWARE ATELIER"
                font.family: "monospace"
                font.pixelSize: 9
                font.weight: Font.Bold
                color: theme.textMuted
            }
            Text {
                text: "POLLING: " + ctrl.pollingIntervalMs + "ms | WAYLAND"
                font.family: "monospace"
                font.pixelSize: 9
                color: theme.textMuted
            }
        }
    }

    // CENTRAL WORKSPACE AREA
    Item {
        id: workspaceArea
        anchors.top: topBar.bottom
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

    // BOTTOM HARDWARE READBACK & STATUS BAR
    Rectangle {
        id: bottomStatusBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 28
        color: theme.surfaceHeader
        border.width: 1
        border.color: theme.borderSubtle

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Rectangle {
                width: 8
                height: 8
                radius: 4
                anchors.verticalCenter: parent.verticalCenter
                color: ctrl.statusBannerIsError ? theme.statusWarning : theme.statusNominal
            }

            Text {
                text: ctrl.statusBannerText
                font.family: "monospace"
                font.pixelSize: 11
                color: ctrl.statusBannerIsError ? theme.statusWarning : theme.textSecondary
            }
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: ctrl.hardwareModeLabel
            font.family: "monospace"
            font.pixelSize: 10
            color: theme.textMuted
        }
    }
}
