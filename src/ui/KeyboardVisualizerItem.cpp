// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "ui/KeyboardVisualizerItem.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <vector>

namespace acc::ui {

namespace {

struct KeyDef {
    double row;
    double colUnit;
    double widthUnit;
    int zone;
    const char* label;
};

// Technical representation of the Acer Aspire 7 A715-59G 15.6" keyboard + numeric keypad
const std::vector<KeyDef>& aspire7KeyLayout() {
    static const std::vector<KeyDef> kKeys = {
        // Row 0: Function row
        {0, 0.0, 1.0, 0, "ESC"}, {0, 1.1, 1.0, 0, "F1"}, {0, 2.15, 1.0, 0, "F2"}, {0, 3.2, 1.0, 0, "F3"},
        {0, 4.25, 1.0, 0, "F4"}, {0, 5.3, 1.0, 1, "F5"}, {0, 6.35, 1.0, 1, "F6"}, {0, 7.4, 1.0, 1, "F7"},
        {0, 8.45, 1.0, 1, "F8"}, {0, 9.5, 1.0, 2, "F9"}, {0, 10.55, 1.0, 2, "F10"}, {0, 11.6, 1.0, 2, "F11"},
        {0, 12.65, 1.0, 2, "F12"}, {0, 13.7, 1.25, 2, "DEL"},
        {0, 15.2, 1.0, 3, "HOME"}, {0, 16.25, 1.0, 3, "PGUP"}, {0, 17.3, 1.0, 3, "PGDN"}, {0, 18.35, 1.0, 3, "PWR"},

        // Row 1: Number row
        {1, 0.0, 1.0, 0, "`"}, {1, 1.05, 1.0, 0, "1"}, {1, 2.1, 1.0, 0, "2"}, {1, 3.15, 1.0, 0, "3"},
        {1, 4.2, 1.0, 0, "4"}, {1, 5.25, 1.0, 1, "5"}, {1, 6.3, 1.0, 1, "6"}, {1, 7.35, 1.0, 1, "7"},
        {1, 8.4, 1.0, 1, "8"}, {1, 9.45, 1.0, 2, "9"}, {1, 10.5, 1.0, 2, "0"}, {1, 11.55, 1.0, 2, "-"},
        {1, 12.6, 1.0, 2, "="}, {1, 13.65, 1.3, 2, "BKSP"},
        {1, 15.2, 1.0, 3, "NUM"}, {1, 16.25, 1.0, 3, "/"}, {1, 17.3, 1.0, 3, "*"}, {1, 18.35, 1.0, 3, "-"},

        // Row 2: QWERTY row
        {2, 0.0, 1.4, 0, "TAB"}, {2, 1.45, 1.0, 0, "Q"}, {2, 2.5, 1.0, 0, "W"}, {2, 3.55, 1.0, 0, "E"},
        {2, 4.6, 1.0, 0, "R"}, {2, 5.65, 1.0, 1, "T"}, {2, 6.7, 1.0, 1, "Y"}, {2, 7.75, 1.0, 1, "U"},
        {2, 8.8, 1.0, 1, "I"}, {2, 9.85, 1.0, 2, "O"}, {2, 10.9, 1.0, 2, "P"}, {2, 11.95, 1.0, 2, "["},
        {2, 13.0, 1.0, 2, "]"}, {2, 14.05, 0.9, 2, "\\"},
        {2, 15.2, 1.0, 3, "7"}, {2, 16.25, 1.0, 3, "8"}, {2, 17.3, 1.0, 3, "9"}, {2, 18.35, 1.0, 3, "+"},

        // Row 3: Home row
        {3, 0.0, 1.65, 0, "CAPS"}, {3, 1.7, 1.0, 0, "A"}, {3, 2.75, 1.0, 0, "S"}, {3, 3.8, 1.0, 0, "D"},
        {3, 4.85, 1.0, 1, "F"}, {3, 5.9, 1.0, 1, "G"}, {3, 6.95, 1.0, 1, "H"}, {3, 8.0, 1.0, 1, "J"},
        {3, 9.05, 1.0, 2, "K"}, {3, 10.1, 1.0, 2, "L"}, {3, 11.15, 1.0, 2, ";"}, {3, 12.2, 1.0, 2, "'"},
        {3, 13.25, 1.7, 2, "ENTER"},
        {3, 15.2, 1.0, 3, "4"}, {3, 16.25, 1.0, 3, "5"}, {3, 17.3, 1.0, 3, "6"}, {3, 18.35, 1.0, 3, "+"},

        // Row 4: Shift row
        {4, 0.0, 2.1, 0, "SHIFT"}, {4, 2.15, 1.0, 0, "Z"}, {4, 3.2, 1.0, 0, "X"}, {4, 4.25, 1.0, 0, "C"},
        {4, 5.3, 1.0, 1, "V"}, {4, 6.35, 1.0, 1, "B"}, {4, 7.4, 1.0, 1, "N"}, {4, 8.45, 1.0, 1, "M"},
        {4, 9.5, 1.0, 2, ","}, {4, 10.55, 1.0, 2, "."}, {4, 11.6, 1.0, 2, "/"}, {4, 12.65, 2.3, 2, "RSHIFT"},
        {4, 15.2, 1.0, 3, "1"}, {4, 16.25, 1.0, 3, "2"}, {4, 17.3, 1.0, 3, "3"}, {4, 18.35, 1.0, 3, "ENT"},

        // Row 5: Bottom row
        {5, 0.0, 1.2, 0, "CTRL"}, {5, 1.25, 1.1, 0, "FN"}, {5, 2.4, 1.1, 0, "SUPER"}, {5, 3.55, 1.1, 0, "ALT"},
        {5, 4.7, 4.8, 1, "SPACEBAR"},
        {5, 9.55, 1.1, 2, "RALT"}, {5, 10.7, 1.1, 2, "RCTRL"},
        {5, 11.85, 1.0, 2, "LEFT"}, {5, 12.9, 1.0, 2, "UP/DN"}, {5, 13.95, 1.0, 2, "RIGHT"},
        {5, 15.2, 2.05, 3, "0 / INS"}, {5, 17.3, 1.0, 3, "."}, {5, 18.35, 1.0, 3, "ENT"}
    };
    return kKeys;
}

} // namespace

KeyboardVisualizerItem::KeyboardVisualizerItem(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    m_zoneColors = {
        QStringLiteral("#3B9EFF"),
        QStringLiteral("#3B9EFF"),
        QStringLiteral("#3B9EFF"),
        QStringLiteral("#3B9EFF")
    };
}

void KeyboardVisualizerItem::setZoneColors(const QVariantList& z) {
    m_zoneColors = z;
    Q_EMIT stateChanged();
    update();
}

void KeyboardVisualizerItem::setSelectedZone(int idx) {
    const int clamped = std::clamp(idx, -1, 3);
    if (m_selectedZone != clamped) {
        m_selectedZone = clamped;
        Q_EMIT selectedZoneChanged(m_selectedZone);
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setBrightnessPct(int b) {
    const int clamped = std::clamp(b, 0, 100);
    if (m_brightnessPct != clamped) {
        m_brightnessPct = clamped;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setBacklightEnabled(bool e) {
    if (m_backlightEnabled != e) {
        m_backlightEnabled = e;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setHardwareConnected(bool c) {
    if (m_hwConnected != c) {
        m_hwConnected = c;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setRoutingBadge(const QString& b) {
    if (m_routingBadge != b) {
        m_routingBadge = b;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setBackgroundColor(const QColor& c) {
    if (m_bgColor != c) {
        m_bgColor = c;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setKeycapColor(const QColor& c) {
    if (m_keycapColor != c) {
        m_keycapColor = c;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setBorderColor(const QColor& c) {
    if (m_borderColor != c) {
        m_borderColor = c;
        Q_EMIT stateChanged();
        update();
    }
}

void KeyboardVisualizerItem::setTextColor(const QColor& c) {
    if (m_textColor != c) {
        m_textColor = c;
        Q_EMIT stateChanged();
        update();
    }
}

QColor KeyboardVisualizerItem::colorForZone(int zoneIdx) const {
    if (zoneIdx >= 0 && zoneIdx < m_zoneColors.size()) {
        const QColor c(m_zoneColors.at(zoneIdx).toString());
        if (c.isValid()) {
            return c;
        }
    }
    return QColor(QStringLiteral("#3B9EFF"));
}

void KeyboardVisualizerItem::mousePressEvent(QMouseEvent* event) {
    const double w = width();
    if (w > 40.0) {
        const double frac = event->position().x() / w;
        if (frac < 0.26) {
            setSelectedZone(0);
        } else if (frac < 0.52) {
            setSelectedZone(1);
        } else if (frac < 0.78) {
            setSelectedZone(2);
        } else {
            setSelectedZone(3);
        }
    }
    event->accept();
}

void KeyboardVisualizerItem::paint(QPainter* painter) {
    const double w = width();
    const double h = height();
    if (w < 120.0 || h < 80.0) {
        return;
    }

    painter->save();
    painter->fillRect(QRectF(0, 0, w, h), m_bgColor);

    const double padX = 14.0;
    const double topBarH = 26.0;
    const double bottomBarH = 24.0;
    const QRectF deckRect(padX, topBarH, w - 2.0 * padX, h - topBarH - bottomBarH);

    const bool isLightBg = m_bgColor.lightness() > 128;
    const bool isLightKeycap = m_keycapColor.lightness() > 128;
    const QColor nominalCol = isLightBg ? QColor(QStringLiteral("#168A56")) : QColor(QStringLiteral("#2EBE7E"));
    const QColor warningCol = isLightBg ? QColor(QStringLiteral("#B87503")) : QColor(QStringLiteral("#E5A024"));

    // Top status & routing bar
    QFont monoFont(QStringLiteral("monospace"));
    monoFont.setPixelSize(10);
    monoFont.setBold(true);
    painter->setFont(monoFont);
    painter->setPen(m_hwConnected ? nominalCol : warningCol);
    painter->drawText(
        QRectF(padX, 4.0, w - 2.0 * padX, 18.0),
        Qt::AlignLeft | Qt::AlignVCenter,
        QStringLiteral("CHASSIS DECK: ACER ASPIRE 7 A715-59G (102-KEY + NUMPAD)   //   %1").arg(m_routingBadge)
    );

    const double totalUnitsX = 19.45;
    const double totalRows = 6.0;
    const double unitW = deckRect.width() / totalUnitsX;
    const double rowH = deckRect.height() / totalRows;

    const double brightScale = m_backlightEnabled ? (std::clamp(m_brightnessPct, 0, 100) / 100.0) : 0.0;

    QFont keyFont(QStringLiteral("monospace"));
    keyFont.setPixelSize(std::clamp(static_cast<int>(rowH * 0.28), 8, 11));
    keyFont.setBold(false);
    painter->setFont(keyFont);

    for (const KeyDef& k : aspire7KeyLayout()) {
        const double kx = deckRect.left() + k.colUnit * unitW + 1.5;
        const double ky = deckRect.top() + k.row * rowH + 1.5;
        const double kw = std::max(6.0, k.widthUnit * unitW - 3.0);
        const double kh = std::max(6.0, rowH - 3.0);
        const QRectF keyRect(kx, ky, kw, kh);

        const QColor zoneCol = colorForZone(k.zone);
        const bool zoneSelected = (m_selectedZone == -1 || m_selectedZone == k.zone);

        // Keycap base
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_keycapColor);
        painter->drawRoundedRect(keyRect, 2.5, 2.5);

        // Keycap backlight edge & legend illumination
        if (brightScale > 0.01) {
            QColor glowTint = zoneCol;
            glowTint.setAlphaF(static_cast<float>(0.16 * brightScale));
            painter->setBrush(glowTint);
            painter->drawRoundedRect(keyRect, 2.5, 2.5);

            QColor edgeCol = zoneCol;
            edgeCol.setAlphaF(static_cast<float>(0.35 + 0.65 * brightScale));
            painter->setPen(QPen(edgeCol, zoneSelected ? 1.5 : 1.0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(keyRect, 2.5, 2.5);

            QColor legendCol = isLightKeycap ? zoneCol.darker(225) : zoneCol.lighter(120);
            legendCol.setAlphaF(static_cast<float>(isLightKeycap ? (0.75 + 0.25 * brightScale) : (0.50 + 0.50 * brightScale)));
            painter->setPen(legendCol);
        } else {
            painter->setPen(QPen(m_borderColor, 1.0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(keyRect, 2.5, 2.5);
            painter->setPen(m_textColor);
        }

        painter->drawText(keyRect.adjusted(3.0, 2.0, -2.0, -2.0), Qt::AlignLeft | Qt::AlignTop, QString::fromUtf8(k.label));
    }

    // Bottom 4-Zone Segment Ruler (clickable zone indicators)
    const QStringList zoneNames = {
        QStringLiteral("Z1 // WASD & LEFT"),
        QStringLiteral("Z2 // CENTER ALPHA"),
        QStringLiteral("Z3 // NAV & ARROWS"),
        QStringLiteral("Z4 // NUMERIC PAD")
    };
    const double segW = deckRect.width() / 4.0;
    for (int z = 0; z < 4; ++z) {
        const QRectF segRect(deckRect.left() + z * segW + 2.0, deckRect.bottom() + 4.0, segW - 4.0, 16.0);
        const bool isSel = (m_selectedZone == -1 || m_selectedZone == z);
        const QColor zCol = colorForZone(z);

        painter->setPen(QPen(isSel ? zCol : m_borderColor, isSel ? 1.5 : 1.0));
        QColor fill = zCol;
        fill.setAlphaF(isSel ? 0.18f : 0.05f);
        painter->setBrush(fill);
        painter->drawRect(segRect);

        painter->setPen(isSel ? (isLightBg ? zCol.darker(215) : zCol.lighter(130)) : m_textColor);
        painter->drawText(segRect, Qt::AlignCenter, QStringLiteral("%1 (%2)").arg(zoneNames.at(z), zCol.name().toUpper()));
    }

    painter->setPen(QPen(m_borderColor, 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0));
    painter->restore();
}

} // namespace acc::ui
