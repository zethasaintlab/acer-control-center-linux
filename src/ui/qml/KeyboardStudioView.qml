// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Controls 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: kbdCol.implicitHeight + 28
    clip: true

    Column {
        id: kbdCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // HARDWARE CAPABILITY & SEPARATION NOTICE
        Rectangle {
            width: parent.width
            height: 98
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: kbdCapHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "KEYBOARD LIGHTING ROUTING // " + ctrl.kbdControlModeBadge
                badgeText: ctrl.kbdCapabilityBadge
                badgeColor: ctrl.kbdHardwareConnected ? theme.statusNominal : theme.statusWarning
            }

            Text {
                anchors.top: kbdCapHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                wrapMode: Text.WordWrap
                text: ctrl.kbdHardwareExplanation
                font.family: "sans-serif"
                font.pixelSize: 12
                color: theme.textSecondary
            }
        }

        // 02 // INTERACTIVE ACER ASPIRE 7 CHASSIS DECK VISUALIZER
        Rectangle {
            width: parent.width
            height: 238
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: deckHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "INTERACTIVE CHASSIS KEYBOARD DECK & 4-ZONE SELECTOR"
                badgeText: ctrl.kbdEnabled ? ("BRIGHTNESS " + ctrl.kbdBrightnessPct + "%") : "BACKLIGHT OFF"
                badgeColor: ctrl.kbdEnabled ? theme.accentReadable : theme.textMuted
            }

            KeyboardVisualizer {
                id: deckVis
                anchors.top: deckHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: zoneBar.top
                anchors.margins: 8
                zoneColors: ctrl.kbdZoneHexList
                selectedZone: ctrl.kbdSelectedZone
                brightnessPct: ctrl.kbdBrightnessPct
                backlightEnabled: ctrl.kbdEnabled
                hardwareConnected: ctrl.kbdHardwareConnected
                routingBadge: ctrl.kbdRoutingBadge
                backgroundColor: theme.canvasBg
                keycapColor: theme.surfaceElevated
                borderColor: theme.borderSubtle
                textColor: theme.textSecondary
                onSelectedZoneChanged: function(zIdx) {
                    ctrl.kbdSelectedZone = zIdx;
                }
            }

            Row {
                id: zoneBar
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                height: 30
                spacing: 8

                AtelierButton {
                    text: "ALL ZONES (SYNC)"
                    activeState: ctrl.kbdSelectedZone === -1
                    onClicked: ctrl.kbdSelectedZone = -1
                }
                AtelierButton {
                    text: "ZONE 1 (WASD)"
                    activeState: ctrl.kbdSelectedZone === 0
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 0
                }
                AtelierButton {
                    text: "ZONE 2 (ALPHA)"
                    activeState: ctrl.kbdSelectedZone === 1
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 1
                }
                AtelierButton {
                    text: "ZONE 3 (NAV)"
                    activeState: ctrl.kbdSelectedZone === 2
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 2
                }
                AtelierButton {
                    text: "ZONE 4 (NUMPAD)"
                    activeState: ctrl.kbdSelectedZone === 3
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 3
                }
            }
        }

        // 02.B // VERIFIED CLV0001 HARDWARE PRESETS & 5-LEVEL BRIGHTNESS MATRIX
        Rectangle {
            width: parent.width
            height: clvHdr.height + clvBodyCol.implicitHeight + 22
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: clvHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02.B"
                title: "VERIFIED CLV0001 PRESETS (17) & DISCRETE LEVELS (ACPI _DSM 0x67)"
                badgeText: ctrl.kbdCustomPreviewDirty
                           ? ("CUSTOM RGB PREVIEW (NEAREST: " + ctrl.kbdNearestPresetName.toUpperCase() + ")")
                           : ("GRB [" + ctrl.kbdFirmwarePacketHex + "]")
                badgeColor: ctrl.kbdCustomPreviewDirty
                            ? theme.statusWarning
                            : (ctrl.kbdSupportsPresetColors ? theme.statusNominal : theme.statusWarning)
            }

            Column {
                id: clvBodyCol
                anchors.top: clvHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                spacing: 10

                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: ctrl.kbdPresetsModel
                        delegate: Rectangle {
                            id: presetChip
                            width: chipRow.implicitWidth + 16
                            height: 26
                            radius: 2
                            activeFocusOnTab: true
                            color: modelData.selected ? theme.surfaceElevated : theme.surfaceHeader
                            border.width: (presetChip.activeFocus || modelData.selected) ? 2 : 1
                            border.color: presetChip.activeFocus
                                              ? theme.accentBorder
                                              : (modelData.selected ? theme.accentReadable : theme.borderSubtle)

                            Row {
                                id: chipRow
                                anchors.centerIn: parent
                                spacing: 6

                                Rectangle {
                                    width: 10
                                    height: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: modelData.previewHex
                                    border.width: 1
                                    border.color: theme.borderSubtle
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.name.toUpperCase()
                                    font.family: "monospace"
                                    font.pixelSize: 10
                                    font.weight: modelData.selected ? Font.Bold : Font.Normal
                                    color: modelData.selected ? theme.textPrimary : theme.textSecondary
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    presetChip.forceActiveFocus();
                                    ctrl.selectKbdPreset(modelData.name);
                                }
                            }

                            Keys.onReturnPressed: ctrl.selectKbdPreset(modelData.name)
                            Keys.onSpacePressed: ctrl.selectKbdPreset(modelData.name)
                        }
                    }
                }

                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: [
                            "0: OFF (0x00)",
                            "1: 25% (0x2F)",
                            "2: 50% (0x5F)",
                            "3: 75% (0x8F)",
                            "4: 100% (0xBF)"
                        ]
                        delegate: AtelierButton {
                            height: 28
                            text: modelData
                            activeState: ctrl.kbdBrightnessLevel === index
                            onClicked: ctrl.setKbdBrightnessLevel(index)
                        }
                    }

                    AtelierButton {
                        visible: ctrl.kbdCustomPreviewDirty
                        height: 28
                        text: "SNAP TO " + ctrl.kbdNearestPresetName.toUpperCase()
                        primaryAccent: true
                        onClicked: ctrl.snapToNearestKbdPreset()
                    }

                    Text {
                        height: 28
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                        text: ctrl.kbdBackendKindId === "clv0001_acpi_dsm"
                              ? "CLV0001 BOUND // WRITE-ONLY (_DSM 0x67 — NO HW READBACK)"
                              : (ctrl.kbdPhysicalReadbackSupported
                                     ? "SYSFS LED ACTIVE (VERIFIED READBACK)"
                                     : "CLV0001 PROTOCOL PREVIEW (_DSM 0x67 — WRITE-ONLY)")
                        font.family: "monospace"
                        font.pixelSize: 10
                        color: ctrl.kbdPhysicalReadbackSupported
                               ? theme.statusNominal
                               : (ctrl.kbdBackendKindId === "clv0001_acpi_dsm" ? theme.statusInfo : theme.textSecondary)
                    }
                }
            }
        }

        // 03 // STUDIO PREVIEW & SYSFS LED COLOR PICKER
        Grid {
            width: parent.width
            columns: width >= 960 ? 3 : 1
            spacing: 12

            // COLUMN A: HSV/HSL COLOR WHEEL & VALUE/LIGHTNESS SLIDERS
            Rectangle {
                width: parent.columns === 3 ? (parent.width - 24) / 3 : parent.width
                height: 310
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: wheelHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.A"
                    title: "UNRESTRICTED HSV / HSL WHEEL"
                    badgeText: "H:" + ctrl.kbdPickerHue.toFixed(0) + "°"
                    badgeColor: theme.accentReadable
                }

                HsvColorWheel {
                    anchors.top: wheelHdr.bottom
                    anchors.topMargin: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 164
                    height: 164
                    hue: ctrl.kbdPickerHue
                    saturation: ctrl.kbdPickerSat
                    value: ctrl.kbdPickerVal
                    focusRingColor: theme.accentBorder
                    onColorEdited: function(h, s, v) {
                        ctrl.setKbdColorFromHsv(h, s, v);
                    }
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    spacing: 6

                    AtelierSlider {
                        width: parent.width
                        label: "HSV Value (Brightness)"
                        unit: "%"
                        minValue: 0
                        maxValue: 100
                        value: Math.round(ctrl.kbdPickerVal * 100)
                        onValueCommitted: function(v) {
                            ctrl.setKbdColorFromHsv(ctrl.kbdPickerHue, ctrl.kbdPickerSat, v / 100.0);
                        }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "HSL Lightness"
                        unit: "%"
                        minValue: 5
                        maxValue: 95
                        value: Math.round(ctrl.kbdPickerHslLight * 100)
                        onValueCommitted: function(v) {
                            ctrl.setKbdColorFromHsl(ctrl.kbdPickerHue, ctrl.kbdPickerSat, v / 100.0);
                        }
                    }
                }
            }

            // COLUMN B: RGB SLIDERS, NUMERICAL STEPPERS, HEX INPUT & SWATCHES
            Rectangle {
                width: parent.columns === 3 ? (parent.width - 24) / 3 : parent.width
                height: 310
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: rgbHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.B"
                    title: "sRGB CHANNELS & SWATCHES"
                    badgeText: ctrl.kbdPickerHex
                    badgeColor: theme.accentReadable
                }

                Column {
                    anchors.top: rgbHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    spacing: 8

                    AtelierSlider {
                        width: parent.width
                        label: "Red Channel (R)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: ctrl.kbdPickerR
                        fillColor: "#E04F5F"
                        onValueCommitted: function(v) {
                            ctrl.setKbdColorFromRgb(v, ctrl.kbdPickerG, ctrl.kbdPickerB);
                        }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Green Channel (G)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: ctrl.kbdPickerG
                        fillColor: "#2EBE7E"
                        onValueCommitted: function(v) {
                            ctrl.setKbdColorFromRgb(ctrl.kbdPickerR, v, ctrl.kbdPickerB);
                        }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Blue Channel (B)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: ctrl.kbdPickerB
                        fillColor: "#3B9EFF"
                        onValueCommitted: function(v) {
                            ctrl.setKbdColorFromRgb(ctrl.kbdPickerR, ctrl.kbdPickerG, v);
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            width: 34
                            height: 30
                            radius: 2
                            color: ctrl.kbdPickerHex
                            border.width: 1
                            border.color: theme.borderStrong
                        }

                        TextField {
                            id: kbdHexField
                            width: 104
                            height: 30
                            text: ctrl.kbdPickerHex
                            font.family: "monospace"
                            font.pixelSize: 12
                            color: theme.textPrimary
                            selectByMouse: true
                            background: Rectangle {
                                color: theme.surfaceElevated
                                border.width: kbdHexField.activeFocus ? 2 : 1
                                border.color: kbdHexField.activeFocus ? theme.accentBorder : theme.borderSubtle
                            }
                            onEditingFinished: ctrl.setKbdColorFromHex(text)
                        }

                        AtelierButton {
                            height: 30
                            text: "SAVE SWATCH"
                            onClicked: ctrl.saveCurrentKbdColorToSwatches()
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: ctrl.kbdCustomSwatches
                            delegate: Rectangle {
                                id: kbdSwatch
                                width: 22
                                height: 22
                                radius: 2
                                activeFocusOnTab: true
                                color: modelData
                                border.width: (kbdSwatch.activeFocus || ctrl.kbdPickerHex === modelData) ? 2 : 1
                                border.color: kbdSwatch.activeFocus ? theme.accentBorder : (ctrl.kbdPickerHex === modelData ? theme.textPrimary : theme.borderSubtle)

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        kbdSwatch.forceActiveFocus();
                                        ctrl.setKbdColorFromHex(modelData);
                                    }
                                }

                                Keys.onReturnPressed: ctrl.setKbdColorFromHex(modelData)
                                Keys.onSpacePressed: ctrl.setKbdColorFromHex(modelData)
                            }
                        }
                    }
                }
            }

            // COLUMN C: HARDWARE ILLUMINATION & PROFILE PERSISTENCE
            Rectangle {
                width: parent.columns === 3 ? (parent.width - 24) / 3 : parent.width
                height: 310
                color: theme.surfacePrimary
                border.width: 1
                border.color: theme.borderSubtle

                SectionHeader {
                    id: profColHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.C"
                    title: "PROFILES & HARDWARE APPLY"
                    badgeText: ctrl.kbdMode.toUpperCase()
                    badgeColor: theme.statusInfo
                }

                Column {
                    anchors.top: profColHdr.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    spacing: 8

                    Row {
                        width: parent.width
                        spacing: 8

                        AtelierButton {
                            width: (parent.width - 8) * 0.5
                            text: ctrl.kbdEnabled ? "BACKLIGHT: ON" : "BACKLIGHT: OFF"
                            activeState: ctrl.kbdEnabled
                            onClicked: ctrl.kbdEnabled = !ctrl.kbdEnabled
                        }

                        AtelierButton {
                            width: (parent.width - 8) * 0.5
                            text: ctrl.kbdRgbSupported ? ("MODE: " + ctrl.kbdMode.toUpperCase()) : "MODE: STATIC ONLY"
                            enabledState: ctrl.kbdRgbSupported
                            onClicked: {
                                if (ctrl.kbdMode === "static") ctrl.kbdMode = "breathing";
                                else if (ctrl.kbdMode === "breathing") ctrl.kbdMode = "wave";
                                else ctrl.kbdMode = "static";
                            }
                        }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Master Illumination Brightness"
                        unit: "%"
                        minValue: 0
                        maxValue: 100
                        value: ctrl.kbdBrightnessPct
                        onValueCommitted: function(v) { ctrl.kbdBrightnessPct = v; }
                    }

                    Row {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: ctrl.kbdProfilesModel
                            delegate: AtelierButton {
                                width: (parent.width - 12) / 3
                                height: 28
                                text: modelData.name
                                activeState: modelData.active
                                onClicked: ctrl.selectKbdProfile(modelData.id)
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 8

                        TextField {
                            id: newProfNameField
                            width: parent.width - 118
                            height: 30
                            placeholderText: "Custom profile name..."
                            placeholderTextColor: theme.textMuted
                            font.family: "sans-serif"
                            font.pixelSize: 12
                            color: theme.textPrimary
                            background: Rectangle {
                                color: theme.surfaceElevated
                                border.width: newProfNameField.activeFocus ? 2 : 1
                                border.color: newProfNameField.activeFocus ? theme.accentBorder : theme.borderSubtle
                            }
                        }

                        AtelierButton {
                            width: 110
                            height: 30
                            text: "SAVE PROFILE"
                            onClicked: {
                                ctrl.saveNewKbdProfile(newProfNameField.text);
                                newProfNameField.text = "";
                            }
                        }
                    }

                    AtelierButton {
                        width: parent.width
                        height: 36
                        text: {
                            if (ctrl.kbdBackendKindId === "clv0001_acpi_dsm")
                                return "SEND CLV0001 _DSM 0x67 PRESET (NO HW READBACK)";
                            if (ctrl.kbdHardwareConnected)
                                return "APPLY TO HARDWARE & VERIFY READBACK";
                            if (ctrl.kbdBackendKindId === "clv0001_unbound")
                                return "CLV0001 KERNEL DRIVER UNBOUND (PREVIEW ONLY)";
                            return "PHYSICAL KEYBOARD LED NOT DETECTED (PREVIEW ONLY)";
                        }
                        enabledState: ctrl.kbdHardwareConnected
                        primaryAccent: ctrl.kbdHardwareConnected
                        onClicked: ctrl.applyKeyboardLightingToHardware()
                    }
                }
            }
        }
    }
}
