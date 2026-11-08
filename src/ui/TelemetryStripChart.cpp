// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "ui/TelemetryStripChart.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace acc::ui {

TelemetryStripChart::TelemetryStripChart(QQuickItem* parent)
    : QQuickPaintedItem(parent) {
    setAntialiasing(true);
}

void TelemetryStripChart::setSeriesA(const QVariantList& v) {
    m_seriesA = v;
    Q_EMIT seriesChanged();
    update();
}

void TelemetryStripChart::setSeriesB(const QVariantList& v) {
    m_seriesB = v;
    Q_EMIT seriesChanged();
    update();
}

void TelemetryStripChart::setMinValue(double v) {
    if (std::abs(m_minValue - v) > 1e-4) {
        m_minValue = v;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setMaxValue(double v) {
    if (std::abs(m_maxValue - v) > 1e-4) {
        m_maxValue = v;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setUnitLabel(const QString& v) {
    if (m_unitLabel != v) {
        m_unitLabel = v;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setSeriesALabel(const QString& v) {
    if (m_seriesALabel != v) {
        m_seriesALabel = v;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setSeriesBLabel(const QString& v) {
    if (m_seriesBLabel != v) {
        m_seriesBLabel = v;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setSeriesAColor(const QColor& c) {
    if (m_seriesAColor != c) {
        m_seriesAColor = c;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setSeriesBColor(const QColor& c) {
    if (m_seriesBColor != c) {
        m_seriesBColor = c;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setBackgroundColor(const QColor& c) {
    if (m_bgColor != c) {
        m_bgColor = c;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setGridColor(const QColor& c) {
    if (m_gridColor != c) {
        m_gridColor = c;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::setTextColor(const QColor& c) {
    if (m_textColor != c) {
        m_textColor = c;
        Q_EMIT configChanged();
        update();
    }
}

void TelemetryStripChart::paint(QPainter* painter) {
    const double w = width();
    const double h = height();
    if (w < 40.0 || h < 30.0) {
        return;
    }

    painter->save();
    painter->fillRect(QRectF(0, 0, w, h), m_bgColor);

    const double leftPad = 42.0;
    const double rightPad = 10.0;
    const double topPad = 24.0;
    const double bottomPad = 18.0;
    const QRectF plotRect(leftPad, topPad, std::max(10.0, w - leftPad - rightPad), std::max(10.0, h - topPad - bottomPad));

    // 1. Draw architectural 1px gridlines & y-axis labels
    QFont axisFont(QStringLiteral("monospace"));
    axisFont.setPixelSize(10);
    painter->setFont(axisFont);

    const double span = std::max(1.0, m_maxValue - m_minValue);
    for (int i = 0; i <= 4; ++i) {
        const double frac = i / 4.0;
        const double y = plotRect.bottom() - frac * plotRect.height();
        painter->setPen(QPen(m_gridColor, 1.0));
        painter->drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));

        const double val = m_minValue + frac * span;
        painter->setPen(m_textColor);
        painter->drawText(
            QRectF(2.0, y - 7.0, leftPad - 6.0, 14.0),
            Qt::AlignRight | Qt::AlignVCenter,
            QStringLiteral("%1").arg(static_cast<int>(std::lround(val)))
        );
    }

    // Vertical time subdivisions (every 20%)
    for (int j = 0; j <= 5; ++j) {
        const double x = plotRect.left() + (j / 5.0) * plotRect.width();
        painter->setPen(QPen(m_gridColor, 1.0, Qt::DotLine));
        painter->drawLine(QPointF(x, plotRect.top()), QPointF(x, plotRect.bottom()));
    }

    auto drawSeries = [&](const QVariantList& values, const QColor& color, bool fillUnder) {
        if (values.isEmpty()) {
            return;
        }
        const int n = values.size();
        QPainterPath linePath;
        QPainterPath areaPath;

        for (int i = 0; i < n; ++i) {
            const double val = std::clamp(values.at(i).toDouble(), m_minValue, m_maxValue);
            const double x = (n == 1)
                                 ? plotRect.right()
                                 : (plotRect.left() + (static_cast<double>(i) / (n - 1)) * plotRect.width());
            const double norm = (val - m_minValue) / span;
            const double y = plotRect.bottom() - norm * plotRect.height();

            if (i == 0) {
                linePath.moveTo(x, y);
                areaPath.moveTo(x, plotRect.bottom());
                areaPath.lineTo(x, y);
            } else {
                linePath.lineTo(x, y);
                areaPath.lineTo(x, y);
            }
            if (i == n - 1) {
                areaPath.lineTo(x, plotRect.bottom());
                areaPath.closeSubpath();
            }
        }

        if (fillUnder && n >= 2) {
            QColor fillC = color;
            fillC.setAlphaF(0.12f);
            painter->fillPath(areaPath, fillC);
        }
        painter->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawPath(linePath);
    };

    drawSeries(m_seriesA, m_seriesAColor, true);
    if (!m_seriesB.isEmpty()) {
        drawSeries(m_seriesB, m_seriesBColor, false);
    }

    // Top legend & latest value readout
    painter->setPen(m_seriesAColor);
    const double lastA = m_seriesA.isEmpty() ? 0.0 : m_seriesA.last().toDouble();
    const QString headerA = QStringLiteral("%1: %2 %3")
                                .arg(m_seriesALabel)
                                .arg(lastA, 0, 'f', 1)
                                .arg(m_unitLabel);
    painter->drawText(QRectF(plotRect.left(), 3.0, 200.0, 16.0), Qt::AlignLeft | Qt::AlignVCenter, headerA);

    if (!m_seriesBLabel.isEmpty() && !m_seriesB.isEmpty()) {
        painter->setPen(m_seriesBColor);
        const double lastB = m_seriesB.last().toDouble();
        const QString headerB = QStringLiteral("%1: %2 %3")
                                    .arg(m_seriesBLabel)
                                    .arg(lastB, 0, 'f', 1)
                                    .arg(m_unitLabel);
        painter->drawText(QRectF(plotRect.left() + 165.0, 3.0, 200.0, 16.0), Qt::AlignLeft | Qt::AlignVCenter, headerB);
    }

    // Bottom time axis caption
    painter->setPen(m_textColor);
    painter->drawText(QRectF(plotRect.left(), plotRect.bottom() + 2.0, 120.0, 14.0), Qt::AlignLeft, QStringLiteral("-60s"));
    painter->drawText(QRectF(plotRect.right() - 80.0, plotRect.bottom() + 2.0, 80.0, 14.0), Qt::AlignRight, QStringLiteral("LIVE (0s)"));

    // Outer 1px border
    painter->setPen(QPen(m_gridColor, 1.0));
    painter->drawRect(QRectF(0.5, 0.5, w - 1.0, h - 1.0));
    painter->restore();
}

} // namespace acc::ui
