// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QColor>
#include <QQuickPaintedItem>
#include <QString>
#include <QVariantList>

namespace acc::ui {

class KeyboardVisualizerItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList zoneColors READ zoneColors WRITE setZoneColors NOTIFY stateChanged)
    Q_PROPERTY(int selectedZone READ selectedZone WRITE setSelectedZone NOTIFY selectedZoneChanged)
    Q_PROPERTY(int brightnessPct READ brightnessPct WRITE setBrightnessPct NOTIFY stateChanged)
    Q_PROPERTY(bool backlightEnabled READ backlightEnabled WRITE setBacklightEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool hardwareConnected READ hardwareConnected WRITE setHardwareConnected NOTIFY stateChanged)
    Q_PROPERTY(QString routingBadge READ routingBadge WRITE setRoutingBadge NOTIFY stateChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY stateChanged)
    Q_PROPERTY(QColor keycapColor READ keycapColor WRITE setKeycapColor NOTIFY stateChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY stateChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY stateChanged)

public:
    explicit KeyboardVisualizerItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QVariantList zoneColors() const { return m_zoneColors; }
    void setZoneColors(const QVariantList& z);

    int selectedZone() const { return m_selectedZone; }
    void setSelectedZone(int idx);

    int brightnessPct() const { return m_brightnessPct; }
    void setBrightnessPct(int b);

    bool backlightEnabled() const { return m_backlightEnabled; }
    void setBacklightEnabled(bool e);

    bool hardwareConnected() const { return m_hwConnected; }
    void setHardwareConnected(bool c);

    QString routingBadge() const { return m_routingBadge; }
    void setRoutingBadge(const QString& b);

    QColor backgroundColor() const { return m_bgColor; }
    void setBackgroundColor(const QColor& c);

    QColor keycapColor() const { return m_keycapColor; }
    void setKeycapColor(const QColor& c);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& c);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& c);

Q_SIGNALS:
    void stateChanged();
    void selectedZoneChanged(int zoneIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QColor colorForZone(int zoneIdx) const;

    QVariantList m_zoneColors;
    int m_selectedZone = 0; // -1 = All Zones, 0..3 = individual zone
    int m_brightnessPct = 80;
    bool m_backlightEnabled = true;
    bool m_hwConnected = false;
    QString m_routingBadge = QStringLiteral("PREVIEW MODE — NO KERNEL LED NODE");
    QColor m_bgColor = QColor(QStringLiteral("#101318"));
    QColor m_keycapColor = QColor(QStringLiteral("#181C23"));
    QColor m_borderColor = QColor(QStringLiteral("#262C38"));
    QColor m_textColor = QColor(QStringLiteral("#9AA4B5"));
};

} // namespace acc::ui
