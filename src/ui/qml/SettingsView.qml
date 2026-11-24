// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Controls 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: setCol.implicitHeight + 28
    clip: true

    readonly property bool splitStudio: width >= 900

    Column {
        id: setCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 12

        // 01 // BIMODAL SURFACE THEME, MOTION, NAVIGATION & TELEMETRY CADENCE
        CommandModule {
            width: parent.width
            indexCode: "01"
            title: "BIMODAL SURFACE THEME, MOTION & CADENCE"
            badgeText: theme.darkTheme ? "DARK TACTICAL HUD" : "LIGHT CAD SHEET"
            contentSpacing: 6

            // Adaptive setting rows (wrap cleanly below 780px!)
            Repeater {
                model: 4
                delegate: Rectangle {
                    width: parent.width
                    height: rowContent.implicitHeight + 16
                    radius: theme.radiusStructural
                    color: theme.surfaceOverlay
                    border.width: 1
                    border.color: theme.borderSubtle

                    Flow {
                        id: rowContent
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 8
                        spacing: 12

                        Column {
                            width: Math.max(220, parent.width >= 660 ? (parent.width - 332) : parent.width)
                            spacing: 2

                            Text {
                                width: parent.width; elide: Text.ElideRight
                                font.family: theme.fontSans; font.pixelSize: 13; font.weight: Font.Medium; color: theme.textPrimary
                                text: index === 0 ? "Bimodal Surface Polarity"
                                    : (index === 1 ? "Reduced Motion Transitions"
                                    : (index === 2 ? "Command Rail Geometry" : "Async Telemetry Cadence"))
                            }
                            Text {
                                width: parent.width; elide: Text.ElideRight
                                font.family: theme.fontMono; font.pixelSize: 10; color: theme.textMuted
                                text: index === 0 ? "Switch Dark Tactical HUD (#222524) vs Light CAD Engineering Sheet (#D8DCDF)"
                                    : (index === 1 ? "Zero-latency digital switches (0 ms) vs smooth transitions (140 ms)"
                                    : (index === 2 ? "Expand full telemetry nav rail (222 px) or lock compact square rail (64 px)"
                                                   : "Worker thread /proc, hwmon, NVML, and async D-Bus interval"))
                            }
                        }

                        SegmentedPillToggle {
                            visible: index < 3
                            width: Math.min(316, parent.width)
                            leftLabel: index === 0 ? "LIGHT CAD" : (index === 1 ? "140ms MOTION" : "EXPANDED")
                            rightLabel: index === 0 ? "DARK HUD" : (index === 1 ? "REDUCED (0ms)" : "COMPACT (64px)")
                            rightActive: index === 0 ? theme.darkTheme : (index === 1 ? theme.reducedMotion : theme.compactNavigation)
                            onToggled: function(val) {
                                if (index === 0) theme.darkTheme = val;
                                else if (index === 1) theme.reducedMotion = val;
                                else theme.compactNavigation = val;
                            }
                        }

                        Flow {
                            visible: index === 3
                            width: Math.min(316, parent.width)
                            spacing: 6
                            Repeater {
                                model: [500, 1000, 2000, 5000]
                                delegate: AtelierButton {
                                    pillShape: false
                                    width: 74; height: 32
                                    text: modelData + "ms"
                                    activeState: ctrl.pollingIntervalMs === modelData
                                    onClicked: ctrl.pollingIntervalMs = modelData
                                }
                            }
                        }
                    }
                }
            }
        }

        // 02 // APPLICATION THEME ACCENT STUDIO (INDEPENDENT OF PHYSICAL KEYBOARD RGB)
        CommandModule {
            width: parent.width
            indexCode: "02"
            title: "APPLICATION ACCENT STUDIO (INDEPENDENT OF KEYBOARD LEDS)"
            provenanceText: "ColorScience WCAG 2.2"
            badgeText: "WCAG " + theme.contrastOnSurface.toFixed(2) + ":1 (" + (theme.wcagAaaPass ? "AAA" : (theme.wcagAaPass ? "AA PASS" : "GUARDRAIL")) + ")"
            badgeColor: theme.wcagAaPass ? theme.telemetryLime : theme.statusWarning

            Grid {
                width: parent.width
                columns: root.splitStudio ? 2 : 1
                spacing: 14

                // Left: HSV Wheel + Swatches + WCAG Verification Card
                Column {
                    width: root.splitStudio ? (parent.width - 14) * 0.44 : parent.width
                    spacing: 10

                    Flow {
                        width: parent.width
                        spacing: 14

                        HsvColorWheel {
                            width: 162; height: 162
                            hue: theme.accentHue
                            saturation: theme.accentSat
                            value: theme.accentVal
                            focusRingColor: theme.accentBorder
                            onColorEdited: function(h, s, v) { theme.setAccentFromHsv(h, s, v); }
                        }

                        Column {
                            width: Math.max(170, parent.width - 180)
                            spacing: 8

                            AtelierSlider {
                                width: parent.width
                                label: "HSV Value"
                                unit: "%"
                                minValue: 10; maxValue: 100
                                value: Math.round(theme.accentVal * 100)
                                onValueCommitted: function(v) { theme.setAccentFromHsv(theme.accentHue, theme.accentSat, v / 100.0); }
                            }

                            Rectangle {
                                width: parent.width
                                height: 66
                                radius: theme.radiusStructural
                                color: theme.surfaceOverlay
                                border.width: 1; border.color: theme.accentBorder
                                Column {
                                    anchors.fill: parent; anchors.margins: 8; spacing: 3
                                    Text {
                                        text: "SURFACE: " + theme.contrastOnSurface.toFixed(2) + ":1"
                                        font.family: theme.fontMono; font.pixelSize: 11; font.weight: Font.Bold; color: theme.accentReadable
                                    }
                                    Text {
                                        text: "ON-ACCENT CTA: " + theme.contrastOnAccent.toFixed(2) + ":1"
                                        font.family: theme.fontMono; font.pixelSize: 11; color: theme.textSecondary
                                    }
                                    Text {
                                        text: "KEYBOARD RGB ISOLATED"
                                        font.family: theme.fontMono; font.pixelSize: 9; color: theme.textMuted
                                    }
                                }
                            }
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: 6
                        Repeater {
                            model: theme.accentSwatches
                            delegate: Rectangle {
                                id: accSwatch
                                width: 24; height: 24; radius: theme.radiusStructural
                                activeFocusOnTab: true
                                Accessible.role: Accessible.Button
                                Accessible.name: "Select accent swatch " + modelData
                                Accessible.checkable: true
                                Accessible.checked: theme.accentHex === modelData
                                color: modelData
                                border.width: (accSwatch.activeFocus || theme.accentHex === modelData) ? 2 : 1
                                border.color: accSwatch.activeFocus ? theme.brandYellow : (theme.accentHex === modelData ? theme.textPrimary : theme.borderSubtle)
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: { accSwatch.forceActiveFocus(); theme.setAccentFromHex(modelData); }
                                }
                                Keys.onReturnPressed: theme.setAccentFromHex(modelData)
                                Keys.onSpacePressed: theme.setAccentFromHex(modelData)
                            }
                        }
                    }
                }

                // Right: RGB Sliders & Hex Entry
                Column {
                    width: root.splitStudio ? (parent.width - 14) * 0.56 : parent.width
                    spacing: 9

                    AtelierSlider {
                        width: parent.width; label: "Accent Red (R)"; unit: ""; minValue: 0; maxValue: 255
                        value: theme.accentR; fillColor: theme.statusCritical
                        onValueCommitted: function(v) { theme.setAccentFromRgb(v, theme.accentG, theme.accentB); }
                    }
                    AtelierSlider {
                        width: parent.width; label: "Accent Green (G)"; unit: ""; minValue: 0; maxValue: 255
                        value: theme.accentG; fillColor: theme.telemetryLimeBar
                        onValueCommitted: function(v) { theme.setAccentFromRgb(theme.accentR, v, theme.accentB); }
                    }
                    AtelierSlider {
                        width: parent.width; label: "Accent Blue (B)"; unit: ""; minValue: 0; maxValue: 255
                        value: theme.accentB; fillColor: theme.technicalCyan
                        onValueCommitted: function(v) { theme.setAccentFromRgb(theme.accentR, theme.accentG, v); }
                    }

                    Flow {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            width: 30; height: 30; radius: theme.radiusStructural
                            color: theme.accentPrimary; border.width: 1; border.color: theme.borderStrong
                        }

                        TextField {
                            id: appHexInput
                            property bool hexInvalid: false
                            width: 92; height: 30
                            text: theme.accentHex
                            font.family: theme.fontMono; font.pixelSize: 12
                            color: hexInvalid ? theme.statusCritical : theme.textPrimary
                            selectByMouse: true
                            Accessible.role: Accessible.EditableText
                            Accessible.name: "Application accent hex color"
                            background: Rectangle {
                                radius: theme.radiusStructural
                                color: theme.surfaceSunken
                                border.width: (appHexInput.activeFocus || appHexInput.hexInvalid) ? 2 : 1
                                border.color: appHexInput.hexInvalid
                                              ? theme.statusCritical
                                              : (appHexInput.activeFocus ? theme.accentBorder : theme.borderSubtle)
                            }
                            onTextEdited: hexInvalid = false
                            onEditingFinished: hexInvalid = !theme.setAccentFromHex(text)
                        }

                        Connections {
                            target: theme
                            function onAccentChanged() {
                                if (!appHexInput.activeFocus) {
                                    appHexInput.hexInvalid = false;
                                    appHexInput.text = theme.accentHex;
                                }
                            }
                        }

                        AtelierButton { height: 30; pillShape: false; text: "SAVE SWATCH"; onClicked: theme.saveCurrentAccentToSwatches() }
                        AtelierButton { height: 30; pillShape: false; text: "ENDFIELD #FFEA00"; onClicked: theme.setAccentFromHex("#FFEA00") }
                    }

                    Text {
                        visible: appHexInput.hexInvalid
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "Invalid hex color. Enter a 6-digit sRGB value such as #FFEA00."
                        font.family: theme.fontMono
                        font.pixelSize: 10
                        color: theme.statusCritical
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "WCAG 2.2 Guardrail: Custom accent colors automatically derive accessible readable text tokens (>= 4.5:1 AA against dark HUD or light CAD surfaces) without modifying physical keyboard lighting."
                        font.family: theme.fontSans
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }
                }
            }
        }

        // 03 // VERSIONED PROFILE BUNDLE PERSISTENCE
        CommandModule {
            width: parent.width
            indexCode: "03"
            title: "PROFILE BUNDLE PERSISTENCE (SCHEMA V1)"
            provenanceText: "~/.config/acer-control-center"
            badgeText: "JSON SCHEMA V1"

            Flow {
                width: parent.width
                spacing: 10

                AtelierButton {
                    height: 32
                    text: "EXPORT PROFILE BUNDLE JSON"
                    primaryAccent: true
                    onClicked: ctrl.exportProfileBundle("")
                }

                AtelierButton {
                    height: 32
                    pillShape: false
                    text: "IMPORT & VALIDATE PROFILE BUNDLE"
                    onClicked: ctrl.importProfileBundle("")
                }
            }
        }
    }
}
