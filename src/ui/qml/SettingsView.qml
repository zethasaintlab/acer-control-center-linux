// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15
import QtQuick.Controls 2.15
import AcerControl.Ui 1.0

Flickable {
    id: root
    contentWidth: width
    contentHeight: setCol.implicitHeight + 28
    clip: true

    Column {
        id: setCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 14

        // 01 // SURFACE THEME, MOTION & TELEMETRY CADENCE
        Rectangle {
            width: parent.width
            height: 152
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: appThemeHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "INTERFACE THEME, ACCESSIBILITY MOTION & TELEMETRY CADENCE"
                badgeText: theme.darkTheme ? "GRAPHITE INSTRUMENT (DARK)" : "ANODIZED ALUMINUM (LIGHT)"
                badgeColor: theme.accentReadable
            }

            Column {
                anchors.top: appThemeHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 12

                Row {
                    spacing: 10

                    AtelierButton {
                        text: "DARK THEME (GRAPHITE)"
                        activeState: theme.darkTheme
                        onClicked: theme.darkTheme = true
                    }

                    AtelierButton {
                        text: "LIGHT THEME (ALUMINUM)"
                        activeState: !theme.darkTheme
                        onClicked: theme.darkTheme = false
                    }

                    AtelierButton {
                        text: theme.reducedMotion ? "REDUCED MOTION: ON (0ms)" : "REDUCED MOTION: OFF (140ms)"
                        activeState: theme.reducedMotion
                        onClicked: theme.reducedMotion = !theme.reducedMotion
                    }

                    AtelierButton {
                        text: theme.compactNavigation ? "NAV RAIL: COMPACT" : "NAV RAIL: EXPANDED"
                        activeState: theme.compactNavigation
                        onClicked: theme.compactNavigation = !theme.compactNavigation
                    }
                }

                Row {
                    spacing: 10

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "TELEMETRY POLLING CADENCE:"
                        font.family: "monospace"
                        font.pixelSize: 11
                        color: theme.textSecondary
                    }

                    Repeater {
                        model: [500, 1000, 2000, 5000]
                        delegate: AtelierButton {
                            text: modelData + " MS"
                            activeState: ctrl.pollingIntervalMs === modelData
                            onClicked: ctrl.pollingIntervalMs = modelData
                        }
                    }
                }
            }
        }

        // 02 // UNRESTRICTED APPLICATION THEME ACCENT COLOR STUDIO (WCAG GUARDRAILED)
        Rectangle {
            width: parent.width
            height: 320
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: accHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "APPLICATION THEME ACCENT COLOR STUDIO (INDEPENDENT OF PHYSICAL KEYBOARD LEDS)"
                badgeText: "WCAG CONTRAST " + theme.contrastOnSurface.toFixed(2) + ":1 (" + (theme.wcagAaaPass ? "AAA" : (theme.wcagAaPass ? "AA PASS" : "ADJUSTED")) + ")"
                badgeColor: theme.wcagAaPass ? theme.statusNominal : theme.statusWarning
            }

            Row {
                anchors.top: accHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 12
                spacing: 18

                Column {
                    width: 184
                    spacing: 8

                    HsvColorWheel {
                        width: 170
                        height: 170
                        hue: theme.accentHue
                        saturation: theme.accentSat
                        value: theme.accentVal
                        focusRingColor: theme.accentBorder
                        onColorEdited: function(h, s, v) {
                            theme.setAccentFromHsv(h, s, v);
                        }
                    }

                    AtelierSlider {
                        width: 170
                        label: "HSV Value"
                        unit: "%"
                        minValue: 10
                        maxValue: 100
                        value: Math.round(theme.accentVal * 100)
                        onValueCommitted: function(v) {
                            theme.setAccentFromHsv(theme.accentHue, theme.accentSat, v / 100.0);
                        }
                    }
                }

                Column {
                    width: (parent.width - 220) * 0.52
                    spacing: 8

                    AtelierSlider {
                        width: parent.width
                        label: "Accent Red (R)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: theme.accentR
                        fillColor: "#E04F5F"
                        onValueCommitted: function(v) { theme.setAccentFromRgb(v, theme.accentG, theme.accentB); }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Accent Green (G)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: theme.accentG
                        fillColor: "#2EBE7E"
                        onValueCommitted: function(v) { theme.setAccentFromRgb(theme.accentR, v, theme.accentB); }
                    }

                    AtelierSlider {
                        width: parent.width
                        label: "Accent Blue (B)"
                        unit: ""
                        minValue: 0
                        maxValue: 255
                        value: theme.accentB
                        fillColor: "#3B9EFF"
                        onValueCommitted: function(v) { theme.setAccentFromRgb(theme.accentR, theme.accentG, v); }
                    }

                    Row {
                        spacing: 8

                        Rectangle {
                            width: 34
                            height: 30
                            radius: 2
                            color: theme.accentPrimary
                            border.width: 1
                            border.color: theme.borderStrong
                        }

                        TextField {
                            id: appHexInput
                            width: 108
                            height: 30
                            text: theme.accentHex
                            font.family: "monospace"
                            font.pixelSize: 12
                            color: theme.textPrimary
                            selectByMouse: true
                            background: Rectangle {
                                color: theme.surfaceElevated
                                border.width: appHexInput.activeFocus ? 2 : 1
                                border.color: appHexInput.activeFocus ? theme.accentBorder : theme.borderSubtle
                            }
                            onEditingFinished: theme.setAccentFromHex(text)
                        }

                        AtelierButton {
                            height: 30
                            text: "SAVE ACCENT SWATCH"
                            onClicked: theme.saveCurrentAccentToSwatches()
                        }
                    }

                    Row {
                        spacing: 6
                        Repeater {
                            model: theme.accentSwatches
                            delegate: Rectangle {
                                id: accSwatch
                                width: 24
                                height: 24
                                radius: 2
                                activeFocusOnTab: true
                                color: modelData
                                border.width: (accSwatch.activeFocus || theme.accentHex === modelData) ? 2 : 1
                                border.color: accSwatch.activeFocus ? theme.accentBorder : (theme.accentHex === modelData ? theme.textPrimary : theme.borderSubtle)

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        accSwatch.forceActiveFocus();
                                        theme.setAccentFromHex(modelData);
                                    }
                                }

                                Keys.onReturnPressed: theme.setAccentFromHex(modelData)
                                Keys.onSpacePressed: theme.setAccentFromHex(modelData)
                            }
                        }
                    }
                }

                Column {
                    width: (parent.width - 220) * 0.44
                    spacing: 10

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "WCAG 2.2 Contrast Guardrail Engine: When you pick any custom sRGB accent color, ThemeEngine computes relative luminance and automatically derives contrast-safe text and border tokens (>= 4.5:1 AA) so custom colors never make text unreadable."
                        font.family: "sans-serif"
                        font.pixelSize: 12
                        color: theme.textSecondary
                    }

                    Rectangle {
                        width: parent.width
                        height: 68
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.accentBorder

                        Column {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4
                            Text {
                                text: "DERIVED ACCESSIBLE TEXT TOKEN (" + theme.contrastOnSurface.toFixed(2) + ":1)"
                                font.family: "monospace"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: theme.accentReadable
                            }
                            Text {
                                text: "ON-ACCENT BUTTON CONTRAST: " + theme.contrastOnAccent.toFixed(2) + ":1"
                                font.family: "monospace"
                                font.pixelSize: 11
                                color: theme.textSecondary
                            }
                        }
                    }
                }
            }
        }

        // 03 // CONFIGURATION & PROFILE BUNDLE IMPORT / EXPORT
        Rectangle {
            width: parent.width
            height: 98
            color: theme.surfacePrimary
            border.width: 1
            border.color: theme.borderSubtle

            SectionHeader {
                id: expHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "VERSIONED PROFILE BUNDLE PERSISTENCE (SCHEMA V1)"
                badgeText: "~/.config/acer-control-center"
                badgeColor: theme.statusInfo
            }

            Row {
                anchors.top: expHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 12

                AtelierButton {
                    text: "EXPORT PROFILE BUNDLE JSON"
                    primaryAccent: true
                    onClicked: ctrl.exportProfileBundle("")
                }

                AtelierButton {
                    text: "IMPORT & VALIDATE PROFILE BUNDLE"
                    onClicked: ctrl.importProfileBundle("")
                }
            }
        }
    }
}
