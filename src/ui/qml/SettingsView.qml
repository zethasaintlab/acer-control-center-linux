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

        // 01 // BIMODAL SURFACE THEME, MOTION & TELEMETRY CADENCE (SCR-0016 54px rows)
        IndustrialPanel {
            width: parent.width
            height: appThemeHdr.height + setRowsCol.implicitHeight + 20

            SectionHeader {
                id: appThemeHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "01"
                title: "INTERFACE THEME (BIMODAL), MOTION & TELEMETRY CADENCE"
                badgeText: theme.darkTheme ? "DARK TACTICAL HUD" : "LIGHT CAD SCHEMATIC"
            }

            Column {
                id: setRowsCol
                anchors.top: appThemeHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 10
                spacing: 6

                // Row 1: Bimodal Dark HUD vs Light CAD Schematic
                Rectangle {
                    width: parent.width
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceElevated
                    border.width: 1
                    border.color: theme.borderSubtle

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14

                        Column {
                            width: Math.max(180, parent.width - 348)
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Text {
                                text: "Bimodal Engineering Surface Polarity"
                                font.family: theme.fontSans; font.pixelSize: 13; font.weight: Font.Medium; color: theme.textPrimary
                            }
                            Text {
                                text: "Switches between Dark Tactical HUD (#222524) and Light CAD Instrument Surface (#D8DCDF)"
                                elide: Text.ElideRight; width: parent.width
                                font.family: theme.fontMono; font.pixelSize: 10; color: theme.textMuted
                            }
                        }

                        SegmentedPillToggle {
                            width: 336
                            anchors.verticalCenter: parent.verticalCenter
                            leftLabel: "LIGHT CAD (#D8DCDF)"
                            rightLabel: "DARK HUD (#222524)"
                            rightActive: theme.darkTheme
                            onToggled: function(isDark) { theme.darkTheme = isDark; }
                        }
                    }
                }

                // Row 2: Accessibility Motion
                Rectangle {
                    width: parent.width
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceElevated
                    border.width: 1
                    border.color: theme.borderSubtle

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14

                        Column {
                            width: Math.max(180, parent.width - 348)
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Text {
                                text: "Reduced Motion & Instant State Switching"
                                font.family: theme.fontSans; font.pixelSize: 13; font.weight: Font.Medium; color: theme.textPrimary
                            }
                            Text {
                                text: "Zero-latency digital shutter transitions (0 ms vs 140 ms)"
                                elide: Text.ElideRight; width: parent.width
                                font.family: theme.fontMono; font.pixelSize: 10; color: theme.textMuted
                            }
                        }

                        SegmentedPillToggle {
                            width: 336
                            anchors.verticalCenter: parent.verticalCenter
                            leftLabel: "STANDARD (140ms)"
                            rightLabel: "REDUCED (0ms)"
                            rightActive: theme.reducedMotion
                            onToggled: function(red) { theme.reducedMotion = red; }
                        }
                    }
                }

                // Row 3: Navigation Density
                Rectangle {
                    width: parent.width
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceElevated
                    border.width: 1
                    border.color: theme.borderSubtle

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14

                        Column {
                            width: Math.max(180, parent.width - 348)
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Text {
                                text: "Command Navigation Rail Geometry"
                                font.family: theme.fontSans; font.pixelSize: 13; font.weight: Font.Medium; color: theme.textPrimary
                            }
                            Text {
                                text: "Expand workspace labels or lock compact 64px square icon rail"
                                elide: Text.ElideRight; width: parent.width
                                font.family: theme.fontMono; font.pixelSize: 10; color: theme.textMuted
                            }
                        }

                        SegmentedPillToggle {
                            width: 336
                            anchors.verticalCenter: parent.verticalCenter
                            leftLabel: "EXPANDED (224px)"
                            rightLabel: "COMPACT (64px)"
                            rightActive: theme.compactNavigation
                            onToggled: function(cmp) { theme.compactNavigation = cmp; }
                        }
                    }
                }

                // Row 4: Telemetry Polling Cadence
                Rectangle {
                    width: parent.width
                    height: 54
                    radius: theme.radiusStructural
                    color: theme.surfaceElevated
                    border.width: 1
                    border.color: theme.borderSubtle

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14

                        Column {
                            width: Math.max(180, parent.width - 376)
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Text {
                                text: "Telemetry Sampling Cadence"
                                font.family: theme.fontSans; font.pixelSize: 13; font.weight: Font.Medium; color: theme.textPrimary
                            }
                            Text {
                                text: "Controls /proc, hwmon, NVML, and RAPL polling frequency"
                                elide: Text.ElideRight; width: parent.width
                                font.family: theme.fontMono; font.pixelSize: 10; color: theme.textMuted
                            }
                        }

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6
                            Repeater {
                                model: [500, 1000, 2000, 5000]
                                delegate: AtelierButton {
                                    pillShape: false
                                    width: 86
                                    height: 30
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

        // 02 // UNRESTRICTED APPLICATION THEME ACCENT COLOR STUDIO (WCAG 2.2 AA GUARDRAILED)
        IndustrialPanel {
            width: parent.width
            height: accHdr.height + accRow.implicitHeight + 28

            SectionHeader {
                id: accHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "02"
                title: "APPLICATION THEME ACCENT STUDIO (INDEPENDENT OF PHYSICAL KEYBOARD LEDS)"
                badgeText: "WCAG " + theme.contrastOnSurface.toFixed(2) + ":1 (" + (theme.wcagAaaPass ? "AAA" : (theme.wcagAaPass ? "AA PASS" : "GUARDRAIL")) + ")"
                badgeColor: theme.wcagAaPass ? theme.telemetryLime : theme.statusWarning
            }

            Row {
                id: accRow
                anchors.top: accHdr.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 12
                spacing: 18

                Column {
                    width: 176
                    spacing: 8

                    HsvColorWheel {
                        width: 170
                        height: 170
                        hue: theme.accentHue
                        saturation: theme.accentSat
                        value: theme.accentVal
                        focusRingColor: theme.accentBorder
                        onColorEdited: function(h, s, v) { theme.setAccentFromHsv(h, s, v); }
                    }

                    AtelierSlider {
                        width: 170
                        label: "HSV Value"
                        unit: "%"
                        minValue: 10
                        maxValue: 100
                        value: Math.round(theme.accentVal * 100)
                        onValueCommitted: function(v) { theme.setAccentFromHsv(theme.accentHue, theme.accentSat, v / 100.0); }
                    }
                }

                Column {
                    width: Math.max(240, (parent.width - 216) * 0.54)
                    spacing: 8

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
                            width: 96; height: 30
                            text: theme.accentHex
                            font.family: theme.fontMono; font.pixelSize: 12; color: theme.textPrimary
                            selectByMouse: true
                            background: Rectangle {
                                radius: theme.radiusStructural
                                color: theme.surfaceElevated
                                border.width: appHexInput.activeFocus ? 2 : 1
                                border.color: appHexInput.activeFocus ? theme.accentBorder : theme.borderSubtle
                            }
                            onEditingFinished: theme.setAccentFromHex(text)
                        }

                        AtelierButton { height: 30; text: "SAVE SWATCH"; onClicked: theme.saveCurrentAccentToSwatches() }
                        AtelierButton { height: 30; text: "ENDFIELD #FFEA00"; onClicked: theme.setAccentFromHex("#FFEA00") }
                    }

                    Row {
                        spacing: 6
                        Repeater {
                            model: theme.accentSwatches
                            delegate: Rectangle {
                                id: accSwatch
                                width: 24; height: 24; radius: theme.radiusStructural
                                activeFocusOnTab: true
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

                Column {
                    width: Math.max(200, (parent.width - 216) * 0.42)
                    spacing: 10

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "WCAG 2.2 Contrast Guardrail Engine: Selecting any custom sRGB application accent automatically computes readable surface text (>= 4.5:1 AA against #222524 Dark HUD or #D8DCDF Light CAD) without affecting physical keyboard LEDs."
                        font.family: theme.fontSans
                        font.pixelSize: 12
                        color: theme.textSecondary
                    }

                    Rectangle {
                        width: parent.width
                        height: 68
                        radius: theme.radiusStructural
                        color: theme.surfaceElevated
                        border.width: 1
                        border.color: theme.accentBorder

                        Column {
                            anchors.fill: parent; anchors.margins: 10; spacing: 4
                            Text {
                                text: "SURFACE TEXT CONTRAST: " + theme.contrastOnSurface.toFixed(2) + ":1"
                                font.family: theme.fontMono; font.pixelSize: 11; font.weight: Font.Bold; color: theme.accentReadable
                            }
                            Text {
                                text: "ON-ACCENT CTA CONTRAST: " + theme.contrastOnAccent.toFixed(2) + ":1"
                                font.family: theme.fontMono; font.pixelSize: 11; color: theme.textSecondary
                            }
                        }
                    }
                }
            }
        }

        // 03 // CONFIGURATION & PROFILE BUNDLE IMPORT / EXPORT
        IndustrialPanel {
            width: parent.width
            height: 98

            SectionHeader {
                id: expHdr
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                indexCode: "03"
                title: "VERSIONED PROFILE BUNDLE PERSISTENCE (SCHEMA V1)"
                badgeText: "~/.config/acer-control-center"
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
