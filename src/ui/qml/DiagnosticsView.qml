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
        anchors.margins: 16
        spacing: 14

        // 01 // PLATFORM IDENTITY & SAFE HARDWARE MODE SWITCHER
        IndustrialPanel {
            width: parent.width
            height: modeHdr.height + modeBody.implicitHeight + 24

            SectionHeader {
                id: modeHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "PLATFORM IDENTITY & HARDWARE BACKEND MODE"
                badgeText: ctrl.mockModeActive ? "MOCK HARDWARE" : "PHYSICAL HARDWARE"
            }

            Column {
                id: modeBody
                anchors.top: modeHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 8

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: "MODEL: " + ctrl.productTitle + "   |   BOARD: " + ctrl.boardTitle + "   |   BIOS: " + ctrl.biosTitle
                    font.family: theme.fontMono
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: theme.textPrimary
                }

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: "ODM: " + ctrl.odmPlatformNote + "   |   WMI: " + ctrl.wmiGuidsJoined
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    color: theme.textSecondary
                }

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: "KERNEL: " + (ctrl.kernelRelease.length > 0 ? ctrl.kernelRelease : "--")
                          + "   |   DAEMON: " + (ctrl.daemonReachable ? "ACTIVE" : "UNAVAILABLE")
                          + "   |   ACPI: " + (ctrl.acpiDevicesJoined.length > 0 ? ctrl.acpiDevicesJoined : "none")
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    color: theme.textSecondary
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    AtelierButton {
                        text: "PHYSICAL HARDWARE"
                        activeState: ctrl.hardwareModeIndex === 0
                        onClicked: ctrl.switchHardwareMode(0)
                    }
                    AtelierButton {
                        text: "MOCK: A715-59G MIRROR"
                        activeState: ctrl.hardwareModeIndex === 1
                        onClicked: ctrl.switchHardwareMode(1)
                    }
                    AtelierButton {
                        text: "MOCK: CLV0001 BOUND"
                        activeState: ctrl.hardwareModeIndex === 3
                        onClicked: ctrl.switchHardwareMode(3)
                    }
                    AtelierButton {
                        text: "MOCK: FULL CAPABILITIES"
                        activeState: ctrl.hardwareModeIndex === 2
                        onClicked: ctrl.switchHardwareMode(2)
                    }
                    AtelierButton {
                        text: "EXPORT REDACTED REPORT"
                        primaryAccent: true
                        onClicked: ctrl.exportDiagnosticReport("")
                    }
                }
            }
        }

        // 02 // COMPLETE 13-CAPABILITY EVIDENCE MATRIX (BIMODAL CAD INSPECTION SURFACE)
        IndustrialPanel {
            width: parent.width
            height: capMatHdr.height + capListCol.implicitHeight + 20

            SectionHeader {
                id: capMatHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "EVIDENCE-BASED HARDWARE CAPABILITY MATRIX"
                badgeText: ctrl.capabilitiesModel.length + " INTERFACES AUDITED"
                accentStripeColor: theme.telemetryLime
            }

            Column {
                id: capListCol
                anchors.top: capMatHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                spacing: 5

                Repeater {
                    model: ctrl.capabilitiesModel
                    delegate: Rectangle {
                        width: parent.width
                        height: 44
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.borderSubtle

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 12

                            CapabilityBadge {
                                width: 158
                                anchors.verticalCenter: parent.verticalCenter
                                badgeText: modelData.stateBadge
                            }

                            Column {
                                width: Math.max(120, parent.width - 170)
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2

                                Row {
                                    width: parent.width
                                    Text {
                                        width: parent.width * 0.48
                                        elide: Text.ElideRight
                                        text: modelData.title
                                        font.family: theme.fontSans
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: theme.textPrimary
                                    }
                                    Text {
                                        width: parent.width * 0.52
                                        horizontalAlignment: Text.AlignRight
                                        elide: Text.ElideMiddle
                                        text: modelData.interfacePath
                                        font.family: theme.fontMono
                                        font.pixelSize: 10
                                        color: theme.textMuted
                                    }
                                }

                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: modelData.summary
                                    font.family: theme.fontSans
                                    font.pixelSize: 11
                                    color: theme.textSecondary
                                }
                            }
                        }
                    }
                }
            }
        }

        // 03 // REDACTED DIAGNOSTIC LOG STREAM
        IndustrialPanel {
            width: parent.width
            height: 210

            SectionHeader {
                id: logHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "REDACTED HARDWARE & APPLICATION EVENT LOG (SERIAL / UUID / MAC STRIPPED)"
                badgeText: ctrl.diagnosticLogsModel.length + " EVENTS"
            }

            ListView {
                anchors.top: logHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                clip: true
                model: ctrl.diagnosticLogsModel
                spacing: 3
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 24
                    radius: theme.radiusStructural
                    color: theme.surfaceElevated

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: "[" + modelData.timestamp + "] [" + modelData.level + "] [" + modelData.subsystem + "] " + modelData.message
                        font.family: theme.fontMono
                        font.pixelSize: 11
                        color: (modelData.level === "WARN" || modelData.level === "ERROR") ? theme.statusWarning : theme.textSecondary
                    }
                }
            }
        }
    }
}
