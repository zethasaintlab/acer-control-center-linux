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
        IndustrialPanel {
            width: parent.width
            height: kbdCapHdr.height + kbdCapTxt.implicitHeight + 22

            SectionHeader {
                id: kbdCapHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "KEYBOARD LIGHTING ROUTING // " + ctrl.kbdControlModeBadge
                badgeText: ctrl.kbdCapabilityBadge
                hazardStripe: !ctrl.kbdPhysicalReadbackSupported
            }

            Text {
                id: kbdCapTxt
                anchors.top: kbdCapHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                wrapMode: Text.WordWrap
                text: ctrl.kbdHardwareExplanation
                font.family: theme.fontSans
                font.pixelSize: 12
                color: theme.textSecondary
            }
        }

        // 02 // ORTHOGRAPHIC CHASSIS KEYBOARD DECK SCHEMATIC (CMP-0010)
        IndustrialPanel {
            width: parent.width
            height: 246

            SectionHeader {
                id: deckHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "ORTHOGRAPHIC CHASSIS KEYBOARD DECK SCHEMATIC"
                badgeText: ctrl.kbdEnabled ? ("BRIGHTNESS " + ctrl.kbdBrightnessPct + "%") : "BACKLIGHT OFF"
                badgeColor: ctrl.kbdEnabled ? theme.telemetryLime : theme.textMuted
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
                multiZoneSupported: ctrl.kbdRgbSupported
                routingBadge: ctrl.kbdRoutingBadge
                backgroundColor: theme.canvasBg
                keycapColor: theme.surfaceElevated
                borderColor: theme.borderSubtle
                textColor: theme.textSecondary
                onSelectedZoneChanged: function(zIdx) { ctrl.kbdSelectedZone = zIdx; }
            }

            Flow {
                id: zoneBar
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 8
                spacing: 8

                AtelierButton {
                    visible: ctrl.kbdRgbSupported
                    pillShape: false
                    height: 28
                    text: "ALL ZONES (SYNC)"
                    activeState: ctrl.kbdSelectedZone === -1
                    onClicked: ctrl.kbdSelectedZone = -1
                }
                AtelierButton {
                    visible: ctrl.kbdRgbSupported
                    pillShape: false
                    height: 28
                    text: "Z1 (WASD)"
                    activeState: ctrl.kbdSelectedZone === 0
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 0
                }
                AtelierButton {
                    visible: ctrl.kbdRgbSupported
                    pillShape: false
                    height: 28
                    text: "Z2 (ALPHA)"
                    activeState: ctrl.kbdSelectedZone === 1
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 1
                }
                AtelierButton {
                    visible: ctrl.kbdRgbSupported
                    pillShape: false
                    height: 28
                    text: "Z3 (NAV)"
                    activeState: ctrl.kbdSelectedZone === 2
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 2
                }
                AtelierButton {
                    visible: ctrl.kbdRgbSupported
                    pillShape: false
                    height: 28
                    text: "Z4 (NUMPAD)"
                    activeState: ctrl.kbdSelectedZone === 3
                    enabledState: ctrl.kbdRgbSupported
                    onClicked: ctrl.kbdSelectedZone = 3
                }

                AtelierButton {
                    visible: !ctrl.kbdRgbSupported
                    pillShape: false
                    height: 28
                    text: "UNIFORM SINGLE-ZONE"
                    activeState: true
                    onClicked: ctrl.kbdSelectedZone = -1
                }

                Text {
                    visible: !ctrl.kbdRgbSupported
                    height: 28
                    verticalAlignment: Text.AlignVCenter
                    text: "PER-ZONE SPLIT UNSUPPORTED ON CLV0001 (_DSM 0x67 SINGLE 4-BYTE GRB PAYLOAD)"
                    font.family: theme.fontMono
                    font.pixelSize: 10
                    color: theme.textSecondary
                }
            }
        }

        // 02.B // VERIFIED CLV0001 HARDWARE PRESETS & 5-LEVEL BRIGHTNESS MATRIX (CMP-0005)
        IndustrialPanel {
            width: parent.width
            height: clvHdr.height + clvBodyCol.implicitHeight + 22

            SectionHeader {
                id: clvHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02.B"
                title: ctrl.kbdSupportsArbitraryRgb
                       ? "CLV0001 PRESETS (17) + STATIC 24-BIT RGB (ACPI _DSM 0x67)"
                       : "VERIFIED CLV0001 PRESETS (17) & DISCRETE LEVELS"
                badgeText: {
                    if (!ctrl.kbdCustomPreviewDirty)
                        return "PRESET: " + ctrl.kbdPresetName.toUpperCase() + " // GRB [" + ctrl.kbdFirmwarePacketHex + "]";
                    if (ctrl.kbdSelectionSource === "random_rgb")
                        return "RANDOM RGB: " + ctrl.kbdPickerHex + " // GRB [" + ctrl.kbdFirmwarePacketHex + "]";
                    return "CUSTOM RGB: " + ctrl.kbdPickerHex + " // GRB [" + ctrl.kbdFirmwarePacketHex + "]";
                }
                badgeColor: !ctrl.kbdCustomPreviewDirty
                            ? (ctrl.kbdSupportsPresetColors ? theme.telemetryLime : theme.statusWarning)
                            : (ctrl.kbdSupportsArbitraryRgb ? theme.technicalCyan : theme.statusWarning)
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
                            height: 28
                            radius: theme.radiusStructural
                            activeFocusOnTab: true
                            color: modelData.selected ? theme.surfaceSelectedDark : theme.surfaceElevated
                            border.width: (presetChip.activeFocus || modelData.selected) ? 2 : 1
                            border.color: presetChip.activeFocus
                                              ? theme.accentBorder
                                              : (modelData.selected ? theme.brandYellow : theme.borderSubtle)

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
                                    border.color: theme.darkInk
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.name.toUpperCase()
                                    font.family: theme.fontMono
                                    font.pixelSize: 10
                                    font.weight: modelData.selected ? Font.Bold : Font.Medium
                                    color: modelData.selected ? theme.brandYellow : theme.textPrimary
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
                            pillShape: false
                            height: 28
                            text: modelData
                            activeState: ctrl.kbdBrightnessLevel === index
                            onClicked: ctrl.setKbdBrightnessLevel(index)
                        }
                    }

                    AtelierButton {
                        visible: ctrl.kbdCustomPreviewDirty && !ctrl.kbdSupportsArbitraryRgb
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
                              ? (ctrl.kbdSupportsArbitraryRgb
                                 ? "STATIC RGB + 17 PRESETS // WRITE-ONLY _DSM // EFFECTS PAUSED"
                                 : "PRESET-ONLY MODULE // NO HW READBACK")
                              : (ctrl.kbdPhysicalReadbackSupported
                                     ? "SYSFS LED ACTIVE (VERIFIED READBACK)"
                                     : "CLV0001 PROTOCOL PREVIEW (WRITE-ONLY)")
                        font.family: theme.fontMono
                        font.pixelSize: 10
                        color: ctrl.kbdPhysicalReadbackSupported
                               ? theme.telemetryLime
                               : (ctrl.kbdBackendKindId === "clv0001_acpi_dsm" ? theme.technicalCyan : theme.textSecondary)
                    }
                }
            }
        }

        // 03 // STATIC 24-BIT CUSTOM RGB STUDIO
        Grid {
            width: parent.width
            columns: width >= 980 ? 3 : 1
            spacing: 12

            // COLUMN A: HSV/HSL COLOR WHEEL
            IndustrialPanel {
                width: parent.columns === 3 ? (parent.width - 24) / 3 : parent.width
                height: 318

                SectionHeader {
                    id: wheelHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.A"
                    title: "STATIC HSV / HSL WHEEL"
                    badgeText: "H:" + ctrl.kbdPickerHue.toFixed(0) + "°"
                }

                HsvColorWheel {
                    anchors.top: wheelHdr.bottom
                    anchors.topMargin: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 160
                    height: 160
                    hue: ctrl.kbdPickerHue
                    saturation: ctrl.kbdPickerSat
                    value: ctrl.kbdPickerVal
                    focusRingColor: theme.accentBorder
                    onColorEdited: function(h, s, v) { ctrl.setKbdColorFromHsv(h, s, v); }
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    spacing: 6

                    AtelierSlider {
                        width: parent.width
                        label: "HSV Value (Luminance)"
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

            // COLUMN B: sRGB CHANNELS, HEX INPUT & SWATCHES
            IndustrialPanel {
                width: parent.columns === 3 ? (parent.width - 24) / 3 : parent.width
                height: 318

                SectionHeader {
                    id: rgbHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.B"
                    title: "sRGB CHANNELS & HEX"
                    badgeText: ctrl.kbdPickerHex
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
                        fillColor: theme.statusCritical
                        onValueCommitted: function(v) { ctrl.setKbdColorFromRgb(v, ctrl.kbdPickerG, ctrl.kbdPickerB); }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Green Channel (G)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: ctrl.kbdPickerG
                        fillColor: theme.telemetryLimeBar
                        onValueCommitted: function(v) { ctrl.setKbdColorFromRgb(ctrl.kbdPickerR, v, ctrl.kbdPickerB); }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Blue Channel (B)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: ctrl.kbdPickerB
                        fillColor: theme.technicalCyan
                        onValueCommitted: function(v) { ctrl.setKbdColorFromRgb(ctrl.kbdPickerR, ctrl.kbdPickerG, v); }
                    }

                    Row {
                        width: parent.width
                        spacing: 6

                        Rectangle {
                            width: 30
                            height: 30
                            radius: theme.radiusStructural
                            color: ctrl.kbdPickerHex
                            border.width: 1
                            border.color: theme.borderStrong
                        }

                        TextField {
                            id: kbdHexField
                            property bool hexInvalid: false
                            width: 84
                            height: 30
                            text: ctrl.kbdPickerHex
                            font.family: theme.fontMono
                            font.pixelSize: 12
                            color: hexInvalid ? theme.statusCritical : theme.textPrimary
                            selectByMouse: true
                            background: Rectangle {
                                radius: theme.radiusStructural
                                color: theme.surfaceElevated
                                border.width: (kbdHexField.activeFocus || kbdHexField.hexInvalid) ? 2 : 1
                                border.color: kbdHexField.hexInvalid
                                              ? theme.statusCritical
                                              : (kbdHexField.activeFocus ? theme.accentBorder : theme.borderSubtle)
                            }
                            onTextEdited: hexInvalid = false
                            onEditingFinished: { hexInvalid = !ctrl.setKbdColorFromHex(text); }
                        }

                        Connections {
                            target: ctrl
                            function onKeyboardStudioChanged() {
                                kbdHexField.hexInvalid = false;
                                kbdHexField.text = ctrl.kbdPickerHex;
                            }
                        }

                        AtelierButton {
                            height: 30
                            text: "SAVE"
                            onClicked: ctrl.saveCurrentKbdColorToSwatches()
                        }

                        AtelierButton {
                            height: 30
                            text: "RANDOM"
                            onClicked: ctrl.selectRandomKbdColor()
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
                                radius: theme.radiusStructural
                                activeFocusOnTab: true
                                color: modelData
                                border.width: (kbdSwatch.activeFocus || ctrl.kbdPickerHex === modelData) ? 2 : 1
                                border.color: kbdSwatch.activeFocus ? theme.brandYellow : (ctrl.kbdPickerHex === modelData ? theme.textPrimary : theme.borderSubtle)

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

            // COLUMN C: HARDWARE ILLUMINATION & EXPLICIT APPLY CTA (CMP-0007)
            IndustrialPanel {
                width: parent.columns === 3 ? (parent.width - 24) / 3 : parent.width
                height: 318

                SectionHeader {
                    id: profColHdr
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    indexCode: "03.C"
                    title: "PROFILES & HARDWARE APPLY"
                    badgeText: ctrl.kbdRgbSupported ? ctrl.kbdMode.toUpperCase() : "STATIC ONLY"
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
                            width: (parent.width - 8) * 0.48
                            text: ctrl.kbdEnabled ? "BACKLIGHT: ON" : "BACKLIGHT: OFF"
                            activeState: ctrl.kbdEnabled
                            onClicked: ctrl.kbdEnabled = !ctrl.kbdEnabled
                        }

                        AtelierButton {
                            width: (parent.width - 8) * 0.52
                            text: ctrl.kbdRgbSupported
                                  ? ("MODE: " + ctrl.kbdMode.toUpperCase())
                                  : "EFFECTS PAUSED (STATIC)"
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
                        label: "Master Brightness Level"
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
                                pillShape: false
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
                            placeholderText: "Profile name..."
                            placeholderTextColor: theme.textMuted
                            font.family: theme.fontSans
                            font.pixelSize: 12
                            color: theme.textPrimary
                            background: Rectangle {
                                radius: theme.radiusStructural
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
                        height: 38
                        text: {
                            if (ctrl.kbdBackendKindId === "clv0001_acpi_dsm") {
                                if (!ctrl.kbdEnabled)
                                    return "APPLY CLV0001 BACKLIGHT OFF (NO HW READBACK)";
                                if (ctrl.kbdCustomPreviewDirty) {
                                    if (!ctrl.kbdSupportsArbitraryRgb)
                                        return "PRESET-ONLY DRIVER MODULE (SNAP TO PRESET)";
                                    if (ctrl.kbdSelectionSource === "random_rgb")
                                        return "APPLY RANDOM RGB " + ctrl.kbdPickerHex + " (NO READBACK)";
                                    return "APPLY CUSTOM RGB " + ctrl.kbdPickerHex + " (NO READBACK)";
                                }
                                return "APPLY PRESET '" + ctrl.kbdPresetName.toUpperCase() + "' (NO READBACK)";
                            }
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
