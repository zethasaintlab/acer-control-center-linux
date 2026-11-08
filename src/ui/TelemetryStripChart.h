// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>
#include <QVariantList>

namespace acc::ui {

class TelemetryStripChart : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList seriesA READ seriesA WRITE setSeriesA NOTIFY seriesChanged)
    Q_PROPERTY(QVariantList seriesB READ seriesB WRITE setSeriesB NOTIFY seriesChanged)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY configChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY configChanged)
    Q_PROPERTY(QString unitLabel READ unitLabel WRITE setUnitLabel NOTIFY configChanged)
    Q_PROPERTY(QString seriesALabel READ seriesALabel WRITE setSeriesALabel NOTIFY configChanged)
    Q_PROPERTY(QString seriesBLabel READ seriesBLabel WRITE setSeriesBLabel NOTIFY configChanged)
    Q_PROPERTY(QColor seriesAColor READ seriesAColor WRITE setSeriesAColor NOTIFY configChanged)
    Q_PROPERTY(QColor seriesBColor READ seriesBColor WRITE setSeriesBColor NOTIFY configChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY configChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY configChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY configChanged)

public:
    explicit TelemetryStripChart(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QVariantList seriesA() const { return m_seriesA; }
    void setSeriesA(const QVariantList& v);

    QVariantList seriesB() const { return m_seriesB; }
    void setSeriesB(const QVariantList& v);

    double minValue() const { return m_minValue; }
    void setMinValue(double v);

    double maxValue() const { return m_maxValue; }
    void setMaxValue(double v);

    QString unitLabel() const { return m_unitLabel; }
    void setUnitLabel(const QString& v);

    QString seriesALabel() const { return m_seriesALabel; }
    void setSeriesALabel(const QString& v);

    QString seriesBLabel() const { return m_seriesBLabel; }
    void setSeriesBLabel(const QString& v);

    QColor seriesAColor() const { return m_seriesAColor; }
    void setSeriesAColor(const QColor& c);

    QColor seriesBColor() const { return m_seriesBColor; }
    void setSeriesBColor(const QColor& c);

    QColor backgroundColor() const { return m_bgColor; }
    void setBackgroundColor(const QColor& c);

    QColor gridColor() const { return m_gridColor; }
    void setGridColor(const QColor& c);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& c);

Q_SIGNALS:
    void seriesChanged();
    void configChanged();

private:
    QVariantList m_seriesA;
    QVariantList m_seriesB;
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    QString m_unitLabel = QStringLiteral("%");
    QString m_seriesALabel = QStringLiteral("CPU");
    QString m_seriesBLabel;
    QColor m_seriesAColor = QColor(QStringLiteral("#3B9EFF"));
    QColor m_seriesBColor = QColor(QStringLiteral("#2EBE7E"));
    QColor m_bgColor = QColor(QStringLiteral("#13161B"));
    QColor m_gridColor = QColor(QStringLiteral("#232832"));
    QColor m_textColor = QColor(QStringLiteral("#9AA4B5"));
};

} // namespace acc::ui
