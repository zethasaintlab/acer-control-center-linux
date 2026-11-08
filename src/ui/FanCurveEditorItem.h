// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QVariantList>

namespace acc::ui {

class FanCurveEditorItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
    Q_PROPERTY(double currentTempC READ currentTempC WRITE setCurrentTempC NOTIFY telemetryChanged)
    Q_PROPERTY(int interpolatedDutyPct READ interpolatedDutyPct NOTIFY pointsChanged)
    Q_PROPERTY(bool hardwarePwmSupported READ hardwarePwmSupported WRITE setHardwarePwmSupported NOTIFY appearanceChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY appearanceChanged)

public:
    explicit FanCurveEditorItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QVariantList points() const { return m_points; }
    void setPoints(const QVariantList& pts);

    double currentTempC() const { return m_currentTempC; }
    void setCurrentTempC(double t);

    int interpolatedDutyPct() const;

    bool hardwarePwmSupported() const { return m_hwPwmSupported; }
    void setHardwarePwmSupported(bool b);

    QColor accentColor() const { return m_accentColor; }
    void setAccentColor(const QColor& c);

    QColor backgroundColor() const { return m_bgColor; }
    void setBackgroundColor(const QColor& c);

    QColor gridColor() const { return m_gridColor; }
    void setGridColor(const QColor& c);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& c);

    Q_INVOKABLE void resetToDefaultCurve();

Q_SIGNALS:
    void pointsChanged();
    void telemetryChanged();
    void appearanceChanged();
    void curveEdited(const QVariantList& updatedPoints);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QRectF plotArea() const;
    QPointF nodeToScreen(int tempC, int dutyPct, const QRectF& plot) const;
    void enforceMonotonicCurve();

    QVariantList m_points;
    int m_selectedIndex = 0;
    bool m_dragging = false;
    double m_currentTempC = 49.0;
    bool m_hwPwmSupported = false;

    QColor m_accentColor = QColor(QStringLiteral("#3B9EFF"));
    QColor m_bgColor = QColor(QStringLiteral("#13161B"));
    QColor m_gridColor = QColor(QStringLiteral("#232832"));
    QColor m_textColor = QColor(QStringLiteral("#9AA4B5"));
};

} // namespace acc::ui
