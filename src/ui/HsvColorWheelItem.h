// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QColor>
#include <QImage>
#include <QQuickPaintedItem>

namespace acc::ui {

class HsvColorWheelItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(double hue READ hue WRITE setHue NOTIFY appearanceChanged)
    Q_PROPERTY(double saturation READ saturation WRITE setSaturation NOTIFY appearanceChanged)
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY appearanceChanged)
    Q_PROPERTY(QColor focusRingColor READ focusRingColor WRITE setFocusRingColor NOTIFY appearanceChanged)

public:
    explicit HsvColorWheelItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    double hue() const { return m_hue; }
    void setHue(double h);

    double saturation() const { return m_saturation; }
    void setSaturation(double s);

    double value() const { return m_value; }
    void setValue(double v);

    QColor focusRingColor() const { return m_focusRingColor; }
    void setFocusRingColor(const QColor& c);

Q_SIGNALS:
    void colorEdited(double hue, double saturation, double value);
    void appearanceChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void rebuildWheelImage();
    void updateFromPoint(const QPointF& pt);

    double m_hue = 210.0;       // [0..360)
    double m_saturation = 0.77; // [0..1]
    double m_value = 1.0;       // [0..1]
    QColor m_focusRingColor = QColor(QStringLiteral("#3B9EFF"));
    QImage m_wheelCache;
};

} // namespace acc::ui
