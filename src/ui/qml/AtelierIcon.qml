// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
import QtQuick 2.15

Canvas {
    id: root
    property string iconName: "overview"
    property color strokeColor: "#ECEFF4"
    width: 18
    height: 18

    onIconNameChanged: requestPaint()
    onStrokeColorChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();
        ctx.strokeStyle = root.strokeColor;
        ctx.fillStyle = root.strokeColor;
        ctx.lineWidth = 1.5;
        ctx.lineCap = "round";
        ctx.lineJoin = "round";

        if (iconName === "overview") {
            ctx.strokeRect(2, 2, 5.5, 5.5);
            ctx.strokeRect(10.5, 2, 5.5, 5.5);
            ctx.strokeRect(2, 10.5, 5.5, 5.5);
            ctx.strokeRect(10.5, 10.5, 5.5, 5.5);
        } else if (iconName === "performance") {
            ctx.beginPath();
            ctx.arc(9, 11, 6.5, Math.PI, 0, false);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(9, 11);
            ctx.lineTo(13, 6.5);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(9, 11, 1.5, 0, Math.PI * 2, false);
            ctx.fill();
        } else if (iconName === "thermal") {
            ctx.beginPath();
            ctx.moveTo(9, 2.5);
            ctx.lineTo(9, 11.5);
            ctx.stroke();
            ctx.beginPath();
            ctx.arc(9, 13.5, 2.5, 0, Math.PI * 2, false);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(12, 4.5); ctx.lineTo(14.5, 4.5);
            ctx.moveTo(12, 7.5); ctx.lineTo(14, 7.5);
            ctx.moveTo(12, 10.5); ctx.lineTo(14.5, 10.5);
            ctx.stroke();
        } else if (iconName === "keyboard") {
            ctx.strokeRect(1.5, 4, 15, 10);
            ctx.beginPath();
            ctx.moveTo(4, 7); ctx.lineTo(5.5, 7);
            ctx.moveTo(7.5, 7); ctx.lineTo(9, 7);
            ctx.moveTo(11, 7); ctx.lineTo(14, 7);
            ctx.moveTo(4, 9.5); ctx.lineTo(6, 9.5);
            ctx.moveTo(8, 9.5); ctx.lineTo(10, 9.5);
            ctx.moveTo(12, 9.5); ctx.lineTo(14, 9.5);
            ctx.moveTo(5.5, 12); ctx.lineTo(12.5, 12);
            ctx.stroke();
        } else if (iconName === "battery") {
            ctx.strokeRect(2, 5, 12.5, 8);
            ctx.fillRect(14.5, 7.2, 1.8, 3.6);
            ctx.beginPath();
            ctx.moveTo(7.5, 6.8);
            ctx.lineTo(6, 9);
            ctx.lineTo(8.5, 9);
            ctx.lineTo(7, 11.2);
            ctx.stroke();
        } else if (iconName === "gpu") {
            ctx.strokeRect(3, 4.5, 12, 9);
            ctx.beginPath();
            ctx.arc(9, 9, 2.8, 0, Math.PI * 2, false);
            ctx.stroke();
            ctx.beginPath();
            ctx.moveTo(1, 6.5); ctx.lineTo(3, 6.5);
            ctx.moveTo(1, 11.5); ctx.lineTo(3, 11.5);
            ctx.moveTo(6, 13.5); ctx.lineTo(6, 15.5);
            ctx.moveTo(12, 13.5); ctx.lineTo(12, 15.5);
            ctx.stroke();
        } else if (iconName === "diagnostics") {
            ctx.strokeRect(2.5, 2.5, 13, 13);
            ctx.beginPath();
            ctx.moveTo(4.5, 10);
            ctx.lineTo(7, 10);
            ctx.lineTo(8.3, 6);
            ctx.lineTo(10, 12.5);
            ctx.lineTo(11.3, 9);
            ctx.lineTo(13.5, 9);
            ctx.stroke();
        } else if (iconName === "settings") {
            ctx.beginPath();
            ctx.moveTo(3, 5); ctx.lineTo(15, 5);
            ctx.moveTo(3, 9); ctx.lineTo(15, 9);
            ctx.moveTo(3, 13); ctx.lineTo(15, 13);
            ctx.stroke();
            ctx.fillRect(6, 3.3, 2.4, 3.4);
            ctx.fillRect(11, 7.3, 2.4, 3.4);
            ctx.fillRect(5, 11.3, 2.4, 3.4);
        }
    }
}
