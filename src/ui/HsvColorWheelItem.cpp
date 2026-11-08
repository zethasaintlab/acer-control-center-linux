// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "ui/HsvColorWheelItem.h"
#include "core/ColorScience.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace acc::ui {

HsvColorWheelItem::HsvColorWheelItem(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(ItemAcceptsInputMethod, true);
    setFocusPolicy(Qt::StrongFocus);
}

void HsvColorWheelItem::setHue(double h) {
    double wrapped = std::fmod(h, 360.0);
    if (wrapped < 0.0) {
        wrapped += 360.0;
    }
    if (std::abs(m_hue - wrapped) > 0.05) {
        m_hue = wrapped;
        Q_EMIT appearanceChanged();
        update();
    }
}

void HsvColorWheelItem::setSaturation(double s) {
    const double clamped = std::clamp(s, 0.0, 1.0);
    if (std::abs(m_saturation - clamped) > 0.002) {
        m_saturation = clamped;
        Q_EMIT appearanceChanged();
        update();
    }
}

void HsvColorWheelItem::setValue(double v) {
    const double clamped = std::clamp(v, 0.0, 1.0);
    if (std::abs(m_value - clamped) > 0.002) {
        m_value = clamped;
        rebuildWheelImage();
        Q_EMIT appearanceChanged();
        update();
    }
}

void HsvColorWheelItem::setFocusRingColor(const QColor& c) {
    if (m_focusRingColor != c) {
        m_focusRingColor = c;
        Q_EMIT appearanceChanged();
        update();
    }
}

void HsvColorWheelItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        rebuildWheelImage();
    }
}

void HsvColorWheelItem::rebuildWheelImage() {
    const int side = static_cast<int>(std::min(width(), height()));
    if (side < 24) {
        return;
    }

    QImage img(side, side, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    const double radius = (side * 0.5) - 6.0;
    const double cx = side * 0.5;
    const double cy = side * 0.5;

    for (int y = 0; y < side; ++y) {
        QRgb* scan = reinterpret_cast<QRgb*>(img.scanLine(y));
        const double dy = y - cy;
        for (int x = 0; x < side; ++x) {
            const double dx = x - cx;
            const double dist = std::hypot(dx, dy);
            if (dist <= radius + 1.0) {
                const double sat = std::clamp(dist / radius, 0.0, 1.0);
                double angleDeg = std::atan2(-dy, dx) * (180.0 / std::numbers::pi);
                if (angleDeg < 0.0) {
                    angleDeg += 360.0;
                }
                const core::RgbColor rgb = core::ColorScience::hsvToRgb(core::HsvColor{angleDeg, sat, m_value});
                int alpha = 255;
                if (dist > radius) {
                    alpha = static_cast<int>(std::clamp((radius + 1.0 - dist) * 255.0, 0.0, 255.0));
                }
                const int pr = (rgb.r * alpha) / 255;
                const int pg = (rgb.g * alpha) / 255;
                const int pb = (rgb.b * alpha) / 255;
                scan[x] = qRgba(pr, pg, pb, alpha);
            }
        }
    }
    m_wheelCache = img;
}

void HsvColorWheelItem::updateFromPoint(const QPointF& pt) {
    const double side = std::min(width(), height());
    const double radius = (side * 0.5) - 6.0;
    if (radius <= 4.0) {
        return;
    }
    const double cx = width() * 0.5;
    const double dy = pt.y() - (height() * 0.5);
    const double dx = pt.x() - cx;
    const double dist = std::hypot(dx, dy);

    double angleDeg = std::atan2(-dy, dx) * (180.0 / std::numbers::pi);
    if (angleDeg < 0.0) {
        angleDeg += 360.0;
    }
    const double sat = std::clamp(dist / radius, 0.0, 1.0);

    m_hue = angleDeg;
    m_saturation = sat;
    Q_EMIT colorEdited(m_hue, m_saturation, m_value);
    update();
}

void HsvColorWheelItem::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus(Qt::MouseFocusReason);
    updateFromPoint(event->position());
    event->accept();
}

void HsvColorWheelItem::mouseMoveEvent(QMouseEvent* event) {
    updateFromPoint(event->position());
    event->accept();
}

void HsvColorWheelItem::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Left:
            setHue(m_hue - 3.0);
            Q_EMIT colorEdited(m_hue, m_saturation, m_value);
            event->accept();
            return;
        case Qt::Key_Right:
            setHue(m_hue + 3.0);
            Q_EMIT colorEdited(m_hue, m_saturation, m_value);
            event->accept();
            return;
        case Qt::Key_Up:
            setSaturation(m_saturation + 0.03);
            Q_EMIT colorEdited(m_hue, m_saturation, m_value);
            event->accept();
            return;
        case Qt::Key_Down:
            setSaturation(m_saturation - 0.03);
            Q_EMIT colorEdited(m_hue, m_saturation, m_value);
            event->accept();
            return;
        default:
            QQuickPaintedItem::keyPressEvent(event);
            return;
    }
}

void HsvColorWheelItem::paint(QPainter* painter) {
    const double side = std::min(width(), height());
    if (side < 24.0) {
        return;
    }
    if (m_wheelCache.isNull() || m_wheelCache.width() != static_cast<int>(side)) {
        rebuildWheelImage();
    }

    painter->save();
    const double ox = (width() - side) * 0.5;
    const double oy = (height() - side) * 0.5;
    painter->drawImage(QPointF(ox, oy), m_wheelCache);

    const double radius = (side * 0.5) - 6.0;
    const double cx = width() * 0.5;
    const double cy = height() * 0.5;

    // Outer technical ring
    painter->setPen(QPen(hasActiveFocus() ? m_focusRingColor : QColor(QStringLiteral("#343C4A")), hasActiveFocus() ? 2.0 : 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(cx, cy), radius + 2.0, radius + 2.0);

    // Selected color reticle
    const double rad = m_hue * (std::numbers::pi / 180.0);
    const double rDist = m_saturation * radius;
    const double rx = cx + std::cos(rad) * rDist;
    const double ry = cy - std::sin(rad) * rDist;

    const core::RgbColor curRgb = core::ColorScience::hsvToRgb(core::HsvColor{m_hue, m_saturation, m_value});
    painter->setPen(QPen(QColor(QStringLiteral("#0D0F12")), 3.0));
    painter->setBrush(QColor(curRgb.r, curRgb.g, curRgb.b));
    painter->drawEllipse(QPointF(rx, ry), 7.0, 7.0);
    painter->setPen(QPen(QColor(QStringLiteral("#FFFFFF")), 1.5));
    painter->drawEllipse(QPointF(rx, ry), 7.0, 7.0);

    painter->restore();
}

} // namespace acc::ui
