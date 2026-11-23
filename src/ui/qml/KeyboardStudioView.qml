// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Controls 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: kbdCol.implicitHeight + 28
    clip: true

    readonly property bool twoColStudio: width >= 920

    Column {
        id: kbdCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 12

        // 01 // CHASSIS ORTHOGRAPHIC DECK + PROTOCOL TELEMETRY CONSOLE
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "ORTHOGRAPHIC CHASSIS DECK // " + ctrl.kbdControlModeBadge
            provenanceText: "PACKET GRB [" + ctrl.kbdFirmwarePacketHex + "]"
            badgeText: ctrl.kbdCapabilityBadge
            hazardMode: !ctrl.kbdPhysicalReadbackSupported
            contentSpacing: 10

            // Protocol & physical readback notice
            Rectangle {
                width: parent.width
                height: capNoteTxt.implicitHeight + 14
                radius: theme.radiusStructural
                color: theme.surfaceOverlay
                border.width: 1
                border.color: ctrl.kbdPhysicalReadbackSupported ? theme.telemetryLime : theme.borderStrong

                Row {
                    anchors.fill: parent
                    anchors.margins: 7
                    spacing: 10

                    Rectangle {
                        width: 4
                        height: parent.height
                        color: ctrl.kbdPhysicalReadbackSupported ? theme.telemetryLime : theme.statusWarning
                    }

                    Text {
                        id: capNoteTxt
                        width: parent.width - 18
                        wrapMode: Text.WordWrap
                        text: ctrl.kbdHardwareExplanation
                        font.family: theme.fontSans
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }
                }
            }

            // Orthographic Keyboard Deck Visualizer inside sunken well
            KeyboardVisualizer {
                id: deckVis
                width: parent.width
                height: 186
                zoneColors: ctrl.kbdZoneHexList
                selectedZone: ctrl.kbdSelectedZone
                brightnessPct: ctrl.kbdBrightnessPct
                backlightEnabled: ctrl.kbdEnabled
                hardwareConnected: ctrl.kbdHardwareConnected
                multiZoneSupported: ctrl.kbdRgbSupported
                routingBadge: ctrl.kbdRoutingBadge
                backgroundColor: theme.surfaceSunken
                keycapColor: theme.surfaceOverlay
                borderColor: theme.hairlineRule
                textColor: theme.textSecondary
                onSelectedZoneChanged: function(zIdx) { ctrl.kbdSelectedZone = zIdx; }
            }

            // Zone topology & discrete 5-level brightness strip
            Grid {
                width: parent.width
                columns: width >= 880 ? 2 : 1
                spacing: 8

                Flow {
                    width: parent.columns === 2 ? (parent.width - 8) * 0.48 : parent.width
                    spacing: 6

                    AtelierButton {
                        visible: ctrl.kbdRgbSupported
                        pillShape: false; height: 28
                        text: "ALL ZONES"
                        activeState: ctrl.kbdSelectedZone === -1
                        onClicked: ctrl.kbdSelectedZone = -1
                    }
                    AtelierButton {
                        visible: ctrl.kbdRgbSupported
                        pillShape: false; height: 28
                        text: "Z1 WASD"
                        activeState: ctrl.kbdSelectedZone === 0
                        onClicked: ctrl.kbdSelectedZone = 0
                    }
                    AtelierButton {
                        visible: ctrl.kbdRgbSupported
                        pillShape: false; height: 28
                        text: "Z2 ALPHA"
                        activeState: ctrl.kbdSelectedZone === 1
                        onClicked: ctrl.kbdSelectedZone = 1
                    }
                    AtelierButton {
                        visible: ctrl.kbdRgbSupported
                        pillShape: false; height: 28
                        text: "Z3 NAV"
                        activeState: ctrl.kbdSelectedZone === 2
                        onClicked: ctrl.kbdSelectedZone = 2
                    }
                    AtelierButton {
                        visible: ctrl.kbdRgbSupported
                        pillShape: false; height: 28
                        text: "Z4 NUM"
                        activeState: ctrl.kbdSelectedZone === 3
                        onClicked: ctrl.kbdSelectedZone = 3
                    }

                    AtelierButton {
                        visible: !ctrl.kbdRgbSupported
                        pillShape: false; height: 28
                        text: "SINGLE-ZONE 4-BYTE GRB LOCK"
                        activeState: true
                        onClicked: ctrl.kbdSelectedZone = -1
                    }
                }

                Flow {
                    width: parent.columns === 2 ? (parent.width - 8) * 0.52 : parent.width
                    spacing: 5

                    Repeater {
                        model: ["0: OFF", "1: 25%", "2: 50%", "3: 75%", "4: 100%"]
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
                }
            }
        }

        // 02 // TWO-COLUMN STUDIO: LEFT (PRESETS + WHEEL), RIGHT (sRGB CHANNELS + PROFILES + APPLY)
        Grid {
            width: parent.width
            columns: root.twoColStudio ? 2 : 1
            spacing: 12

            // LEFT COLUMN: VERIFIED 17 PRESETS + HSV/HSL WHEEL
            CommandModule {
                id: kbdLeftMod
                width: root.twoColStudio ? (parent.width - 12) * 0.50 : parent.width
                indexCode: "02.A"
                title: ctrl.kbdSupportsArbitraryRgb
                       ? "17 CLV0001 PRESETS & HSV WHEEL"
                       : "17 VERIFIED CLV0001 PRESETS"
                badgeText: !ctrl.kbdCustomPreviewDirty
                           ? ("PRESET: " + ctrl.kbdPresetName.toUpperCase())
                           : (ctrl.kbdSelectionSource === "random_rgb" ? ("RANDOM: " + ctrl.kbdPickerHex) : ("CUSTOM: " + ctrl.kbdPickerHex))
                badgeColor: !ctrl.kbdCustomPreviewDirty ? theme.telemetryLime : theme.technicalCyan
                contentSpacing: 12

                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: ctrl.kbdPresetsModel
                        delegate: Rectangle {
                            id: presetChip
                            width: chipRow.implicitWidth + 14
                            height: 28
                            radius: theme.radiusStructural
                            activeFocusOnTab: true
                            color: modelData.selected ? theme.surfaceSelectedDark : theme.surfaceOverlay
                            border.width: (presetChip.activeFocus || modelData.selected) ? 2 : 1
                            border.color: presetChip.activeFocus
                                              ? theme.accentBorder
                                              : (modelData.selected ? theme.brandYellow : theme.borderSubtle)

                            Row {
                                id: chipRow
                                anchors.centerIn: parent
                                spacing: 5

                                Rectangle {
                                    width: 10; height: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: modelData.previewHex
                                    border.width: 1; border.color: theme.darkInk
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

                Rectangle { width: parent.width; height: 1; color: theme.hairlineRule }

                // HSV Wheel + Value/Lightness sliders side-by-side when wide enough
                Flow {
                    width: parent.width
                    spacing: 14

                    HsvColorWheel {
                        width: 158
                        height: 158
                        hue: ctrl.kbdPickerHue
                        saturation: ctrl.kbdPickerSat
                        value: ctrl.kbdPickerVal
                        focusRingColor: theme.accentBorder
                        onColorEdited: function(h, s, v) { ctrl.setKbdColorFromHsv(h, s, v); }
                    }

                    Column {
                        width: Math.max(180, parent.width - 174)
                        spacing: 10

                        Text {
                            text: "STATIC COLOR SYNTHESIS  //  H: " + ctrl.kbdPickerHue.toFixed(0) + "°"
                            font.family: theme.fontMono
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: theme.textSecondary
                        }

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
            }

            // RIGHT COLUMN: sRGB CHANNELS, HEX, PROFILES & HARDWARE APPLY
            CommandModule {
                id: kbdRightMod
                width: root.twoColStudio ? (parent.width - 12) * 0.50 : parent.width
                height: root.twoColStudio ? Math.max(kbdLeftMod.implicitHeight, kbdRightMod.implicitHeight) : kbdRightMod.implicitHeight
                indexCode: "02.B"
                title: "sRGB CHANNELS, PROFILES & HARDWARE APPLY"
                badgeText: ctrl.kbdPickerHex
                contentSpacing: 9

                AtelierSlider {
                    width: parent.width
                    label: "Red Channel (R)"
                    unit: ""
                    minValue: 0; maxValue: 255
                    value: ctrl.kbdPickerR
                    fillColor: theme.statusCritical
                    onValueCommitted: function(v) { ctrl.setKbdColorFromRgb(v, ctrl.kbdPickerG, ctrl.kbdPickerB); }
                }

                AtelierSlider {
                    width: parent.width
                    label: "Green Channel (G)"
                    unit: ""
                    minValue: 0; maxValue: 255
                    value: ctrl.kbdPickerG
                    fillColor: theme.telemetryLimeBar
                    onValueCommitted: function(v) { ctrl.setKbdColorFromRgb(ctrl.kbdPickerR, v, ctrl.kbdPickerB); }
                }

                AtelierSlider {
                    width: parent.width
                    label: "Blue Channel (B)"
                    unit: ""
                    minValue: 0; maxValue: 255
                    value: ctrl.kbdPickerB
                    fillColor: theme.technicalCyan
                    onValueCommitted: function(v) { ctrl.setKbdColorFromRgb(ctrl.kbdPickerR, ctrl.kbdPickerG, v); }
                }

                Flow {
                    width: parent.width
                    spacing: 6

                    Rectangle {
                        width: 30; height: 30; radius: theme.radiusStructural
                        color: ctrl.kbdPickerHex; border.width: 1; border.color: theme.borderStrong
                    }

                    TextField {
                        id: kbdHexField
                        property bool hexInvalid: false
                        width: 86; height: 30
                        text: ctrl.kbdPickerHex
                        font.family: theme.fontMono; font.pixelSize: 12
                        color: hexInvalid ? theme.statusCritical : theme.textPrimary
                        selectByMouse: true
                        background: Rectangle {
                            radius: theme.radiusStructural
                            color: theme.surfaceSunken
                            border.width: (kbdHexField.activeFocus || kbdHexField.hexInvalid) ? 2 : 1
                            border.color: kbdHexField.hexInvalid
                                          ? theme.statusCritical
                                          : (kbdHexField.activeFocus ? theme.accentBorder : theme.borderSubtle)
                        }
                        onTextEdited: hexInvalid = false
                        onEditingFinished: hexInvalid = !ctrl.setKbdColorFromHex(text)
                    }

                    Connections {
                        target: ctrl
                        function onKeyboardStudioChanged() {
                            kbdHexField.hexInvalid = false;
                            kbdHexField.text = ctrl.kbdPickerHex;
                        }
                    }

                    AtelierButton { height: 30; pillShape: false; text: "SAVE SWATCH"; onClicked: ctrl.saveCurrentKbdColorToSwatches() }
                    AtelierButton { height: 30; pillShape: false; text: "RANDOM RGB"; onClicked: ctrl.selectRandomKbdColor() }
                }

                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: ctrl.kbdCustomSwatches
                        delegate: Rectangle {
                            id: kbdSwatch
                            width: 22; height: 22; radius: theme.radiusStructural
                            activeFocusOnTab: true
                            color: modelData
                            border.width: (kbdSwatch.activeFocus || ctrl.kbdPickerHex === modelData) ? 2 : 1
                            border.color: kbdSwatch.activeFocus ? theme.brandYellow : (ctrl.kbdPickerHex === modelData ? theme.textPrimary : theme.borderSubtle)
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: { kbdSwatch.forceActiveFocus(); ctrl.setKbdColorFromHex(modelData); }
                            }
                            Keys.onReturnPressed: ctrl.setKbdColorFromHex(modelData)
                            Keys.onSpacePressed: ctrl.setKbdColorFromHex(modelData)
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: theme.hairlineRule }

                Flow {
                    width: parent.width
                    spacing: 8

                    AtelierButton {
                        height: 30
                        pillShape: false
                        text: ctrl.kbdEnabled ? "BACKLIGHT: ON" : "BACKLIGHT: OFF"
                        activeState: ctrl.kbdEnabled
                        onClicked: ctrl.kbdEnabled = !ctrl.kbdEnabled
                    }

                    AtelierButton {
                        height: 30
                        pillShape: false
                        text: ctrl.kbdRgbSupported ? ("MODE: " + ctrl.kbdMode.toUpperCase()) : "EFFECTS PAUSED (STATIC)"
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
                    minValue: 0; maxValue: 100
                    value: ctrl.kbdBrightnessPct
                    onValueCommitted: function(v) { ctrl.kbdBrightnessPct = v; }
                }

                Flow {
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: ctrl.kbdProfilesModel
                        delegate: AtelierButton {
                            pillShape: false; height: 28
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
                        width: Math.max(100, parent.width - 118)
                        height: 30
                        placeholderText: "New profile name..."
                        placeholderTextColor: theme.textMuted
                        font.family: theme.fontSans; font.pixelSize: 12; color: theme.textPrimary
                        background: Rectangle {
                            radius: theme.radiusStructural
                            color: theme.surfaceSunken
                            border.width: newProfNameField.activeFocus ? 2 : 1
                            border.color: newProfNameField.activeFocus ? theme.accentBorder : theme.borderSubtle
                        }
                    }

                    AtelierButton {
                        width: 110; height: 30
                        pillShape: false
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
                    pendingState: ctrl.controlOperationPending
                    enabledState: ctrl.kbdHardwareConnected && !ctrl.controlOperationPending
                    primaryAccent: ctrl.kbdHardwareConnected
                    text: {
                        if (ctrl.kbdBackendKindId === "clv0001_acpi_dsm") {
                            if (!ctrl.kbdEnabled)
                                return "APPLY CLV0001 BACKLIGHT OFF (NO HW READBACK)";
                            if (ctrl.kbdCustomPreviewDirty) {
                                if (!ctrl.kbdSupportsArbitraryRgb)
                                    return "PRESET-ONLY MODULE (SNAP TO PRESET)";
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
                    onClicked: ctrl.applyKeyboardLightingToHardware()
                }
            }
        }
    }
}
