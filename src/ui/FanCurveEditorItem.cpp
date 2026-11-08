// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "ui/FanCurveEditorItem.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

namespace acc::ui {

namespace {
constexpr int kMinTemp = 30;
constexpr int kMaxTemp = 95;
constexpr int kMinDuty = 20;
constexpr int kMaxDuty = 100;
} // namespace

FanCurveEditorItem::FanCurveEditorItem(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(ItemAcceptsInputMethod, true);
    setFocusPolicy(Qt::StrongFocus);
    resetToDefaultCurve();
}

void FanCurveEditorItem::resetToDefaultCurve() {
    QVariantList def;
    auto addPt = [&](int t, int d) {
        QVariantMap m;
        m.insert(QStringLiteral("tempC"), t);
        m.insert(QStringLiteral("dutyPct"), d);
        def.append(m);
    };
    addPt(40, 25);
    addPt(55, 40);
    addPt(68, 60);
    addPt(78, 82);
    addPt(88, 100);
    setPoints(def);
    Q_EMIT curveEdited(m_points);
}

void FanCurveEditorItem::setPoints(const QVariantList& pts) {
    if (pts.size() >= 2) {
        m_points = pts;
        enforceMonotonicCurve();
        Q_EMIT pointsChanged();
        update();
    }
}

void FanCurveEditorItem::setCurrentTempC(double t) {
    if (std::abs(m_currentTempC - t) > 0.1) {
        m_currentTempC = t;
        Q_EMIT telemetryChanged();
        Q_EMIT pointsChanged();
        update();
    }
}

int FanCurveEditorItem::interpolatedDutyPct() const {
    if (m_points.isEmpty()) {
        return 50;
    }
    const double t = m_currentTempC;
    const QVariantMap first = m_points.first().toMap();
    if (t <= first.value(QStringLiteral("tempC")).toDouble()) {
        return first.value(QStringLiteral("dutyPct")).toInt();
    }
    for (int i = 0; i < m_points.size() - 1; ++i) {
        const QVariantMap a = m_points.at(i).toMap();
        const QVariantMap b = m_points.at(i + 1).toMap();
        const double tA = a.value(QStringLiteral("tempC")).toDouble();
        const double tB = b.value(QStringLiteral("tempC")).toDouble();
        const double dA = a.value(QStringLiteral("dutyPct")).toDouble();
        const double dB = b.value(QStringLiteral("dutyPct")).toDouble();
        if (t >= tA && t <= tB) {
            const double span = std::max(1.0, tB - tA);
            const double frac = (t - tA) / span;
            return std::clamp(static_cast<int>(std::lround(dA + frac * (dB - dA))), kMinDuty, kMaxDuty);
        }
    }
    return m_points.last().toMap().value(QStringLiteral("dutyPct")).toInt();
}

void FanCurveEditorItem::setHardwarePwmSupported(bool b) {
    if (m_hwPwmSupported != b) {
        m_hwPwmSupported = b;
        Q_EMIT appearanceChanged();
        update();
    }
}

void FanCurveEditorItem::setAccentColor(const QColor& c) {
    if (m_accentColor != c) {
        m_accentColor = c;
        Q_EMIT appearanceChanged();
        update();
    }
}

void FanCurveEditorItem::setBackgroundColor(const QColor& c) {
    if (m_bgColor != c) {
        m_bgColor = c;
        Q_EMIT appearanceChanged();
        update();
    }
}

void FanCurveEditorItem::setGridColor(const QColor& c) {
    if (m_gridColor != c) {
        m_gridColor = c;
        Q_EMIT appearanceChanged();
        update();
    }
}

void FanCurveEditorItem::setTextColor(const QColor& c) {
    if (m_textColor != c) {
        m_textColor = c;
        Q_EMIT appearanceChanged();
        update();
    }
}

QRectF FanCurveEditorItem::plotArea() const {
    const double leftPad = 46.0;
    const double rightPad = 18.0;
    const double topPad = 28.0;
    const double bottomPad = 26.0;
    return QRectF(
        leftPad,
        topPad,
        std::max(40.0, width() - leftPad - rightPad),
        std::max(40.0, height() - topPad - bottomPad)
    );
}

QPointF FanCurveEditorItem::nodeToScreen(int tempC, int dutyPct, const QRectF& plot) const {
    const double xNorm = static_cast<double>(tempC - kMinTemp) / static_cast<double>(kMaxTemp - kMinTemp);
    const double yNorm = static_cast<double>(dutyPct - kMinDuty) / static_cast<double>(kMaxDuty - kMinDuty);
    return QPointF(
        plot.left() + std::clamp(xNorm, 0.0, 1.0) * plot.width(),
        plot.bottom() - std::clamp(yNorm, 0.0, 1.0) * plot.height()
    );
}

void FanCurveEditorItem::enforceMonotonicCurve() {
    for (int i = 0; i < m_points.size(); ++i) {
        QVariantMap m = m_points.at(i).toMap();
        int t = std::clamp(m.value(QStringLiteral("tempC")).toInt(), kMinTemp, kMaxTemp);
        int d = std::clamp(m.value(QStringLiteral("dutyPct")).toInt(), kMinDuty, kMaxDuty);
        if (i > 0) {
            const QVariantMap prev = m_points.at(i - 1).toMap();
            t = std::max(t, prev.value(QStringLiteral("tempC")).toInt() + 3);
            d = std::max(d, prev.value(QStringLiteral("dutyPct")).toInt());
        }
        m.insert(QStringLiteral("tempC"), std::min(t, kMaxTemp));
        m.insert(QStringLiteral("dutyPct"), std::min(d, kMaxDuty));
        m_points[i] = m;
    }
}

void FanCurveEditorItem::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus(Qt::MouseFocusReason);
    const QRectF plot = plotArea();
    double bestDist = 22.0;
    int hitIdx = -1;
    for (int i = 0; i < m_points.size(); ++i) {
        const QVariantMap m = m_points.at(i).toMap();
        const QPointF pt = nodeToScreen(m.value(QStringLiteral("tempC")).toInt(), m.value(QStringLiteral("dutyPct")).toInt(), plot);
        const double dist = std::hypot(event->position().x() - pt.x(), event->position().y() - pt.y());
        if (dist < bestDist) {
            bestDist = dist;
            hitIdx = i;
        }
    }
    if (hitIdx >= 0) {
        m_selectedIndex = hitIdx;
        m_dragging = true;
        update();
    }
    event->accept();
}

void FanCurveEditorItem::mouseMoveEvent(QMouseEvent* event) {
    if (!m_dragging || m_selectedIndex < 0 || m_selectedIndex >= m_points.size()) {
        return;
    }
    const QRectF plot = plotArea();
    const double xFrac = std::clamp((event->position().x() - plot.left()) / plot.width(), 0.0, 1.0);
    const double yFrac = std::clamp((plot.bottom() - event->position().y()) / plot.height(), 0.0, 1.0);

    int newTemp = static_cast<int>(std::lround(kMinTemp + xFrac * (kMaxTemp - kMinTemp)));
    int newDuty = static_cast<int>(std::lround(kMinDuty + yFrac * (kMaxDuty - kMinDuty)));

    if (m_selectedIndex > 0) {
        const QVariantMap prev = m_points.at(m_selectedIndex - 1).toMap();
        newTemp = std::max(newTemp, prev.value(QStringLiteral("tempC")).toInt() + 3);
        newDuty = std::max(newDuty, prev.value(QStringLiteral("dutyPct")).toInt());
    }
    if (m_selectedIndex + 1 < m_points.size()) {
        const QVariantMap next = m_points.at(m_selectedIndex + 1).toMap();
        newTemp = std::min(newTemp, next.value(QStringLiteral("tempC")).toInt() - 3);
        newDuty = std::min(newDuty, next.value(QStringLiteral("dutyPct")).toInt());
    }

    QVariantMap cur = m_points.at(m_selectedIndex).toMap();
    cur.insert(QStringLiteral("tempC"), newTemp);
    cur.insert(QStringLiteral("dutyPct"), newDuty);
    m_points[m_selectedIndex] = cur;

    Q_EMIT pointsChanged();
    Q_EMIT curveEdited(m_points);
    update();
    event->accept();
}

void FanCurveEditorItem::mouseReleaseEvent(QMouseEvent* event) {
    m_dragging = false;
    event->accept();
}

void FanCurveEditorItem::keyPressEvent(QKeyEvent* event) {
    if (m_points.isEmpty()) {
        return;
    }
    m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_points.size()) - 1);
    QVariantMap cur = m_points.at(m_selectedIndex).toMap();
    int duty = cur.value(QStringLiteral("dutyPct")).toInt();

    if (event->key() == Qt::Key_Left) {
        m_selectedIndex = std::max(0, m_selectedIndex - 1);
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right) {
        m_selectedIndex = std::min(static_cast<int>(m_points.size()) - 1, m_selectedIndex + 1);
        update();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        duty += (event->key() == Qt::Key_Up) ? 2 : -2;
        cur.insert(QStringLiteral("dutyPct"), std::clamp(duty, kMinDuty, kMaxDuty));
        m_points[m_selectedIndex] = cur;
        enforceMonotonicCurve();
        Q_EMIT pointsChanged();
        Q_EMIT curveEdited(m_points);
        update();
        event->accept();
        return;
    }
    QQuickPaintedItem::keyPressEvent(event);
}

void FanCurveEditorItem::paint(QPainter* painter) {
    const double w = width();
    const double h = height();
    if (w < 80.0 || h < 60.0) {
        return;
    }

    painter->save();
    painter->fillRect(QRectF(0, 0, w, h), m_bgColor);
    const QRectF plot = plotArea();

    QFont monoFont(QStringLiteral("monospace"));
    monoFont.setPixelSize(10);
    painter->setFont(monoFont);

    // 1. Horizontal PWM gridlines
    for (int d = 20; d <= 100; d += 20) {
        const double y = nodeToScreen(kMinTemp, d, plot).y();
        painter->setPen(QPen(m_gridColor, 1.0));
        painter->drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter->setPen(m_textColor);
        const double yOffset = (d == 20) ? -10.0 : -7.0;
        painter->drawText(QRectF(2.0, y + yOffset, plot.left() - 6.0, 14.0), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("%1%").arg(d));
    }

    // 2. Vertical temperature gridlines
    for (int t = 30; t <= 90; t += 15) {
        const double x = nodeToScreen(t, kMinDuty, plot).x();
        painter->setPen(QPen(m_gridColor, 1.0, Qt::DotLine));
        painter->drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        painter->setPen(m_textColor);
        painter->drawText(QRectF(x - 20.0, plot.bottom() + 6.0, 40.0, 14.0), Qt::AlignCenter, QStringLiteral("%1°C").arg(t));
    }

    // 3. Live CPU temperature vertical crosshair
    const double curTempX = nodeToScreen(static_cast<int>(std::lround(m_currentTempC)), kMinDuty, plot).x();
    painter->setPen(QPen(QColor(QStringLiteral("#E5A024")), 1.2, Qt::DashLine));
    painter->drawLine(QPointF(curTempX, plot.top()), QPointF(curTempX, plot.bottom()));

    // 4. Piecewise curve & area fill
    if (m_points.size() >= 2) {
        QPainterPath linePath;
        QPainterPath fillPath;
        for (int i = 0; i < m_points.size(); ++i) {
            const QVariantMap m = m_points.at(i).toMap();
            const QPointF pt = nodeToScreen(m.value(QStringLiteral("tempC")).toInt(), m.value(QStringLiteral("dutyPct")).toInt(), plot);
            if (i == 0) {
                linePath.moveTo(pt);
                fillPath.moveTo(pt.x(), plot.bottom());
                fillPath.lineTo(pt);
            } else {
                linePath.lineTo(pt);
                fillPath.lineTo(pt);
            }
            if (i == m_points.size() - 1) {
                fillPath.lineTo(pt.x(), plot.bottom());
                fillPath.closeSubpath();
            }
        }
        QColor fillC = m_accentColor;
        fillC.setAlphaF(0.14f);
        painter->fillPath(fillPath, fillC);
        painter->setPen(QPen(m_accentColor, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(linePath);

        // 5. Draggable control nodes
        for (int i = 0; i < m_points.size(); ++i) {
            const QVariantMap m = m_points.at(i).toMap();
            const int t = m.value(QStringLiteral("tempC")).toInt();
            const int d = m.value(QStringLiteral("dutyPct")).toInt();
            const QPointF pt = nodeToScreen(t, d, plot);
            const bool selected = (i == m_selectedIndex);

            painter->setPen(QPen(selected ? QColor(QStringLiteral("#FFFFFF")) : m_bgColor, 2.0));
            painter->setBrush(m_accentColor);
            painter->drawEllipse(pt, selected ? 6.5 : 5.0, selected ? 6.5 : 5.0);

            painter->setPen(m_textColor);
            painter->drawText(
                QRectF(pt.x() - 30.0, pt.y() - 20.0, 60.0, 14.0),
                Qt::AlignCenter,
                QStringLiteral("%1°/%2%").arg(t).arg(d)
            );
        }
    }

    // 6. Top status badge
    const QString modeBadge = m_hwPwmSupported
                                  ? QStringLiteral("HARDWARE PWM CURVE ACTIVE  |  LIVE TARGET: %1% @ %2°C")
                                        .arg(interpolatedDutyPct())
                                        .arg(m_currentTempC, 0, 'f', 1)
                                  : QStringLiteral("PREVIEW / SIMULATION ONLY (FIRMWARE EC AUTO)  |  SIMULATED TARGET: %1% @ %2°C")
                                        .arg(interpolatedDutyPct())
                                        .arg(m_currentTempC, 0, 'f', 1);
    const bool isLightBg = m_bgColor.lightness() > 128;
    const QColor nominalCol = isLightBg ? QColor(QStringLiteral("#168A56")) : QColor(QStringLiteral("#2EBE7E"));
    const QColor warningCol = isLightBg ? QColor(QStringLiteral("#B87503")) : QColor(QStringLiteral("#E5A024"));
    painter->setPen(m_hwPwmSupported ? nominalCol : warningCol);
    painter->drawText(QRectF(plot.left(), 4.0, plot.width(), 18.0), Qt::AlignLeft | Qt::AlignVCenter, modeBadge);

    painter->setPen(QPen(hasActiveFocus() ? m_accentColor : m_gridColor, hasActiveFocus() ? 2.0 : 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0));
    painter->restore();
}

} // namespace acc::ui
