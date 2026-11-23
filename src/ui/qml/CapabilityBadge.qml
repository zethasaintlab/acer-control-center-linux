// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Rectangle {
    id: root
    property string badgeText: "READ-ONLY"
    property color customColor: "transparent"

    readonly property string normText: badgeText.toUpperCase().trim()
    readonly property color resolvedColor: {
        if (customColor.a > 0.01) return customColor;
        if (normText.indexOf("FULL RW") >= 0 || normText.indexOf("ACTIVE") >= 0 || normText.indexOf("AC MAINS") >= 0)
            return theme.statusNominal;
        if (normText.indexOf("FIRMWARE AUTOMATIC") >= 0 || normText.indexOf("READ-ONLY") >= 0 || normText.indexOf("P-STATE") >= 0)
            return theme.statusInfo;
        if (normText.indexOf("ERROR") >= 0 || normText.indexOf("UNAVAILABLE") >= 0 || normText.indexOf("UNSUPPORTED") >= 0 || normText.indexOf("CRITICAL") >= 0)
            return theme.statusCritical;
        if (normText.indexOf("SIMULATED") >= 0 || normText.indexOf("NOT DETECTED") >= 0 || normText.indexOf("PREVIEW ONLY") >= 0 || normText.indexOf("PARTIAL") >= 0 || normText.indexOf("MOCK") >= 0 || normText.indexOf("BATTERY") >= 0)
            return theme.statusWarning;
        return theme.accentReadable;
    }

    readonly property string glyph: {
        if (normText.indexOf("FULL RW") >= 0) return "◆";
        if (normText.indexOf("SIMULATED") >= 0) return "◎";
        if (normText.indexOf("FIRMWARE AUTOMATIC") >= 0) return "⚙";
        if (normText.indexOf("READ-ONLY") >= 0) return "◈";
        if (normText.indexOf("PARTIAL") >= 0) return "◐";
        if (normText.indexOf("PREVIEW ONLY") >= 0) return "◎";
        if (normText.indexOf("NOT DETECTED") >= 0) return "◇";
        if (normText.indexOf("UNAVAILABLE") >= 0 || normText.indexOf("ERROR") >= 0) return "▲";
        return "■";
    }

    visible: badgeText.length > 0
    implicitHeight: 20
    implicitWidth: badgeRow.implicitWidth + 14
    height: 20
    width: Math.min(430, badgeRow.implicitWidth + 14)
    radius: theme.radiusStructural
    color: Qt.rgba(resolvedColor.r, resolvedColor.g, resolvedColor.b, theme.darkTheme ? 0.14 : 0.10)
    border.width: 1
    border.color: resolvedColor

    Row {
        id: badgeRow
        anchors.centerIn: parent
        spacing: 5

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.glyph
            font.family: theme.fontMono
            font.pixelSize: 10
            color: root.resolvedColor
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, 400)
            elide: Text.ElideRight
            text: root.badgeText
            font.family: theme.fontMono
            font.pixelSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 0.4
            color: root.resolvedColor
        }
    }
}
