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
        Rectangle {
            width: parent.width
            height: 152
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: modeHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "PLATFORM IDENTITY & HARDWARE BACKEND VERIFICATION MODE"
                badgeText: ctrl.mockModeActive ? "MOCK FIXTURE ACTIVE" : "PHYSICAL HARDWARE"
                badgeColor: ctrl.mockModeActive ? theme.statusWarning : theme.statusNominal
            }

            Column {
                anchors.top: modeHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 10

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: "MODEL: " + ctrl.productTitle + "   |   BOARD: " + ctrl.boardTitle + "   |   BIOS: " + ctrl.biosTitle
                    font.family: "monospace"
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    color: theme.textPrimary
                }

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: "ODM ARCHITECTURE: " + ctrl.odmPlatformNote + "   |   WMI: " + ctrl.wmiGuidsJoined
                    font.family: "monospace"
                    font.pixelSize: 11
                    color: theme.textSecondary
                }

                Row {
                    width: parent.width
                    spacing: 8

                    AtelierButton {
                        text: "PHYSICAL HARDWARE (READ-VERIFIED)"
                        activeState: ctrl.hardwareModeIndex === 0
                        onClicked: ctrl.switchHardwareMode(0)
                    }

                    AtelierButton {
                        text: "MOCK: A715-59G MIRROR"
                        activeState: ctrl.hardwareModeIndex === 1
                        onClicked: ctrl.switchHardwareMode(1)
                    }

                    AtelierButton {
                        text: "MOCK: FULL CAPABILITIES (RGB + PWM + BAT LIMIT)"
                        activeState: ctrl.hardwareModeIndex === 2
                        onClicked: ctrl.switchHardwareMode(2)
                    }

                    AtelierButton {
                        text: "EXPORT REDACTED REPORT (MD + JSON)"
                        primaryAccent: true
                        onClicked: ctrl.exportDiagnosticReport("")
                    }
                }
            }
        }

        // 02 // COMPLETE 13-CAPABILITY EVIDENCE MATRIX
        Rectangle {
            width: parent.width
            height: 48 + ctrl.capabilitiesModel.length * 42
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: capMatHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "EVIDENCE-BASED HARDWARE CAPABILITY MATRIX"
                badgeText: ctrl.capabilitiesModel.length + " INTERFACES AUDITED"
                badgeColor: theme.statusInfo
            }

            Column {
                anchors.top: capMatHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 8
                spacing: 4

                Repeater {
                    model: ctrl.capabilitiesModel
                    delegate: Rectangle {
                        width: parent.width
                        height: 38
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.borderSubtle

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 10

                            Rectangle {
                                width: 106
                                height: 22
                                anchors.verticalCenter: parent.verticalCenter
                                color: "transparent"
                                border.width: 1
                                border.color: {
                                    if (modelData.stateId === "fully_supported") return theme.statusNominal;
                                    if (modelData.stateId === "read_only") return theme.statusInfo;
                                    return theme.statusWarning;
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.stateBadge
                                    font.family: "monospace"
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: {
                                        if (modelData.stateId === "fully_supported") return theme.statusNominal;
                                        if (modelData.stateId === "read_only") return theme.statusInfo;
                                        return theme.statusWarning;
                                    }
                                }
                            }

                            Column {
                                width: parent.width - 126
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 2

                                Row {
                                    width: parent.width
                                    Text {
                                        width: parent.width * 0.46
                                        elide: Text.ElideRight
                                        text: modelData.title
                                        font.family: "sans-serif"
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: theme.textPrimary
                                    }
                                    Text {
                                        width: parent.width * 0.54
                                        horizontalAlignment: Text.AlignRight
                                        elide: Text.ElideMiddle
                                        text: modelData.interfacePath
                                        font.family: "monospace"
                                        font.pixelSize: 10
                                        color: theme.textMuted
                                    }
                                }

                                Text {
                                    width: parent.width
                                    elide: Text.ElideRight
                                    text: modelData.summary
                                    font.family: "sans-serif"
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
        Rectangle {
            width: parent.width
            height: 210
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: logHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "REDACTED APPLICATION & HARDWARE EVENT LOG (UUID / SERIAL / MAC STRIPPED)"
                badgeText: ctrl.diagnosticLogsModel.length + " EVENTS"
                badgeColor: theme.accentReadable
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
                    height: 22
                    color: theme.surfaceElevated

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: "[" + modelData.timestamp + "] [" + modelData.level + "] [" + modelData.subsystem + "] " + modelData.message
                        font.family: "monospace"
                        font.pixelSize: 11
                        color: modelData.level === "WARN" || modelData.level === "ERROR" ? theme.statusWarning : theme.textSecondary
                    }
                }
            }
        }
    }
}
