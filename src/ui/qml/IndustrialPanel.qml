// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property bool lightCadSurface: false
    property bool showCrosshairs: true
    property bool highlighted: false

    radius: theme.radiusStructural
    color: lightCadSurface ? theme.surfaceCadLight : theme.surfacePrimary
    border.width: 1
    border.color: highlighted
                  ? theme.accentBorder
                  : (lightCadSurface ? theme.borderCadSubtle : theme.borderSubtle)

    // Subtle registration crosshairs (+) in bottom-right technical corner
    Item {
        visible: root.showCrosshairs
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 6
        width: 7
        height: 7
        opacity: 0.45
        Rectangle {
            anchors.centerIn: parent
            width: 7
            height: 1
            color: root.lightCadSurface ? theme.textMuted : theme.borderStrong
        }
        Rectangle {
            anchors.centerIn: parent
            width: 1
            height: 7
            color: root.lightCadSurface ? theme.textMuted : theme.borderStrong
        }
    }
}
