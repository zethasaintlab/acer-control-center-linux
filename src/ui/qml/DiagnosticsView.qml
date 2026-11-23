// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Flickable {
    id: root
    contentWidth: width
    contentHeight: diagCol.implicitHeight + 28
    clip: true

    Column {
        id: diagCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 12

        // 01 // PLATFORM IDENTITY & HARDWARE BACKEND SWITCHER
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "PLATFORM IDENTITY & SIMULATION BACKEND"
            provenanceText: ctrl.dmiAvailable ? "DMI + sysfs" : "fixture"
            badgeText: ctrl.mockModeActive ? "MOCK HARDWARE" : "PHYSICAL HARDWARE"
            badgeColor: ctrl.mockModeActive ? theme.statusWarning : theme.statusNominal
            contentSpacing: 10

            Grid {
                width: parent.width
                columns: width >= 860 ? 2 : 1
                spacing: 12

                Column {
                    width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                    spacing: 0
                    CadKeyValueRow { width: parent.width; keyText: "PRODUCT"; valueText: ctrl.productTitle }
                    CadKeyValueRow { width: parent.width; keyText: "BOARD / BIOS"; valueText: ctrl.boardTitle + " · " + ctrl.biosTitle }
                    CadKeyValueRow { width: parent.width; keyText: "ODM PLATFORM"; valueText: ctrl.odmPlatformNote; showDivider: false }
                }

                Column {
                    width: parent.columns === 2 ? (parent.width - 12) * 0.5 : parent.width
                    spacing: 0
                    CadKeyValueRow { width: parent.width; keyText: "KERNEL"; valueText: ctrl.kernelRelease.length > 0 ? ctrl.kernelRelease : "--" }
                    CadKeyValueRow {
                        width: parent.width; keyText: "DAEMON";
                        valueText: ctrl.daemonReachable ? "ONLINE (POLKIT READY)" : "UNAVAILABLE"
                        valueColor: ctrl.daemonReachable ? theme.telemetryLime : theme.statusWarning
                    }
                    CadKeyValueRow {
                        width: parent.width; keyText: "ACPI / WMI";
                        valueText: (ctrl.acpiDevicesJoined.length > 0 ? ctrl.acpiDevicesJoined : "none") + " | " + ctrl.wmiGuidsJoined
                        showDivider: false
                    }
                }
            }

            Flow {
                width: parent.width
                spacing: 6

                AtelierButton { pillShape: false; height: 30; text: "PHYSICAL HW"; activeState: ctrl.hardwareModeIndex === 0; onClicked: ctrl.switchHardwareMode(0) }
                AtelierButton { pillShape: false; height: 30; text: "MOCK: A715-59G MIRROR"; activeState: ctrl.hardwareModeIndex === 1; onClicked: ctrl.switchHardwareMode(1) }
                AtelierButton { pillShape: false; height: 30; text: "MOCK: CLV0001 BOUND"; activeState: ctrl.hardwareModeIndex === 3; onClicked: ctrl.switchHardwareMode(3) }
                AtelierButton { pillShape: false; height: 30; text: "MOCK: FULL CAPABILITIES"; activeState: ctrl.hardwareModeIndex === 2; onClicked: ctrl.switchHardwareMode(2) }
                AtelierButton { height: 30; text: "EXPORT REDACTED REPORT"; primaryAccent: true; onClicked: ctrl.exportDiagnosticReport("") }
            }
        }

        // 02 // COMPLETE EVIDENCE-BASED HARDWARE CAPABILITY MATRIX
        CommandModule {
            width: parent.width
            indexCode: "02"
            title: "EVIDENCE-BASED HARDWARE CAPABILITY MATRIX"
            provenanceText: "HardwareDiscovery"
            badgeText: ctrl.capabilitiesModel.length + " INTERFACES"
            stripeColor: theme.telemetryLime
            contentSpacing: 5

            Repeater {
                model: ctrl.capabilitiesModel
                delegate: Rectangle {
                    width: parent.width
                    height: capRowCol.implicitHeight + 14
                    radius: theme.radiusStructural
                    color: theme.surfaceOverlay
                    border.width: 1
                    border.color: theme.borderSubtle

                    Column {
                        id: capRowCol
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 7
                        spacing: 4

                        Flow {
                            width: parent.width
                            spacing: 8

                            CapabilityBadge {
                                badgeText: modelData.stateBadge
                            }

                            Text {
                                height: 20
                                verticalAlignment: Text.AlignVCenter
                                text: modelData.title
                                font.family: theme.fontSans
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                color: theme.textPrimary
                            }

                            Text {
                                height: 20
                                verticalAlignment: Text.AlignVCenter
                                text: "[" + modelData.interfacePath + "]"
                                font.family: theme.fontMono
                                font.pixelSize: 10
                                color: theme.textMuted
                            }
                        }

                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: modelData.summary
                            font.family: theme.fontSans
                            font.pixelSize: 11
                            color: theme.textSecondary
                        }
                    }
                }
            }
        }

        // 03 // REDACTED HARDWARE & APPLICATION EVENT STREAM
        CommandModule {
            width: parent.width
            indexCode: "03"
            title: "REDACTED EVENT & AUDIT STREAM"
            provenanceText: "SERIAL / UUID / MAC STRIPPED"
            badgeText: ctrl.diagnosticLogsModel.length + " EVENTS"
            contentSpacing: 4

            Column {
                width: parent.width
                spacing: 4

                Repeater {
                    model: ctrl.diagnosticLogsModel
                    delegate: Rectangle {
                        width: parent.width
                        height: logLineTxt.implicitHeight + 10
                        radius: theme.radiusStructural
                        color: theme.surfaceOverlay
                        border.width: 1; border.color: theme.hairlineRule

                        Rectangle {
                            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: 3
                            color: (modelData.level === "WARN" || modelData.level === "ERROR")
                                   ? theme.statusWarning
                                   : theme.technicalCyan
                        }

                        Text {
                            id: logLineTxt
                            anchors.left: parent.left; anchors.leftMargin: 10
                            anchors.right: parent.right; anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            wrapMode: Text.WordWrap
                            text: "[" + modelData.timestamp + "] [" + modelData.level + "] [" + modelData.subsystem + "] " + modelData.message
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            color: (modelData.level === "WARN" || modelData.level === "ERROR")
                                   ? theme.statusWarning
                                   : theme.textSecondary
                        }
                    }
                }
            }
        }
    }
}
