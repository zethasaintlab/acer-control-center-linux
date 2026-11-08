// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/ColorScience.h"
#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>

namespace acc::app {

class ThemeEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY themeChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY motionChanged)
    Q_PROPERTY(bool compactNavigation READ compactNavigation WRITE setCompactNavigation NOTIFY layoutChanged)

    // Surface & Text Tokens
    Q_PROPERTY(QColor canvasBg READ canvasBg NOTIFY themeChanged)
    Q_PROPERTY(QColor surfacePrimary READ surfacePrimary NOTIFY themeChanged)
    Q_PROPERTY(QColor surfaceElevated READ surfaceElevated NOTIFY themeChanged)
    Q_PROPERTY(QColor surfaceHeader READ surfaceHeader NOTIFY themeChanged)
    Q_PROPERTY(QColor borderSubtle READ borderSubtle NOTIFY themeChanged)
    Q_PROPERTY(QColor borderStrong READ borderStrong NOTIFY themeChanged)
    Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY themeChanged)
    Q_PROPERTY(QColor textMuted READ textMuted NOTIFY themeChanged)

    // Semantic Status Tokens
    Q_PROPERTY(QColor statusNominal READ statusNominal NOTIFY themeChanged)
    Q_PROPERTY(QColor statusWarning READ statusWarning NOTIFY themeChanged)
    Q_PROPERTY(QColor statusCritical READ statusCritical NOTIFY themeChanged)
    Q_PROPERTY(QColor statusInfo READ statusInfo NOTIFY themeChanged)

    // Unrestricted Application Accent Tokens (WCAG Verified)
    Q_PROPERTY(QColor accentPrimary READ accentPrimary NOTIFY accentChanged)
    Q_PROPERTY(QColor accentOnText READ accentOnText NOTIFY accentChanged)
    Q_PROPERTY(QColor accentReadable READ accentReadable NOTIFY accentChanged)
    Q_PROPERTY(QColor accentBorder READ accentBorder NOTIFY accentChanged)
    Q_PROPERTY(QColor accentSurfaceTint READ accentSurfaceTint NOTIFY accentChanged)
    Q_PROPERTY(QString accentHex READ accentHex NOTIFY accentChanged)
    Q_PROPERTY(int accentR READ accentR NOTIFY accentChanged)
    Q_PROPERTY(int accentG READ accentG NOTIFY accentChanged)
    Q_PROPERTY(int accentB READ accentB NOTIFY accentChanged)
    Q_PROPERTY(double accentHue READ accentHue NOTIFY accentChanged)
    Q_PROPERTY(double accentSat READ accentSat NOTIFY accentChanged)
    Q_PROPERTY(double accentVal READ accentVal NOTIFY accentChanged)
    Q_PROPERTY(double accentHslLight READ accentHslLight NOTIFY accentChanged)
    Q_PROPERTY(double contrastOnSurface READ contrastOnSurface NOTIFY accentChanged)
    Q_PROPERTY(double contrastOnAccent READ contrastOnAccent NOTIFY accentChanged)
    Q_PROPERTY(bool wcagAaPass READ wcagAaPass NOTIFY accentChanged)
    Q_PROPERTY(bool wcagAaaPass READ wcagAaaPass NOTIFY accentChanged)
    Q_PROPERTY(QStringList accentSwatches READ accentSwatches NOTIFY swatchesChanged)
    Q_PROPERTY(int transitionDurationMs READ transitionDurationMs NOTIFY motionChanged)

public:
    explicit ThemeEngine(QObject* parent = nullptr);

    bool darkTheme() const { return m_darkTheme; }
    void setDarkTheme(bool dark);

    bool reducedMotion() const { return m_reducedMotion; }
    void setReducedMotion(bool reduced);

    bool compactNavigation() const { return m_compactNav; }
    void setCompactNavigation(bool compact);

    QColor canvasBg() const;
    QColor surfacePrimary() const;
    QColor surfaceElevated() const;
    QColor surfaceHeader() const;
    QColor borderSubtle() const;
    QColor borderStrong() const;
    QColor textPrimary() const;
    QColor textSecondary() const;
    QColor textMuted() const;

    QColor statusNominal() const;
    QColor statusWarning() const;
    QColor statusCritical() const;
    QColor statusInfo() const;

    QColor accentPrimary() const { return m_tokens.primary; }
    QColor accentOnText() const { return m_tokens.onAccentText; }
    QColor accentReadable() const { return m_tokens.readableOnSurface; }
    QColor accentBorder() const { return m_tokens.activeBorder; }
    QColor accentSurfaceTint() const { return m_tokens.surfaceTint; }

    QString accentHex() const { return m_accentHex; }
    int accentR() const { return m_rgb.r; }
    int accentG() const { return m_rgb.g; }
    int accentB() const { return m_rgb.b; }
    double accentHue() const { return m_hsv.h; }
    double accentSat() const { return m_hsv.s; }
    double accentVal() const { return m_hsv.v; }
    double accentHslLight() const { return m_hsl.l; }
    double contrastOnSurface() const { return m_tokens.contrastOnSurface; }
    double contrastOnAccent() const { return m_tokens.contrastOnAccent; }
    bool wcagAaPass() const { return m_tokens.wcagAaNormal; }
    bool wcagAaaPass() const { return m_tokens.wcagAaaNormal; }

    QStringList accentSwatches() const { return m_swatches; }
    void setAccentSwatches(const QStringList& list);

    int transitionDurationMs() const { return m_reducedMotion ? 0 : 140; }

    Q_INVOKABLE bool setAccentFromHex(const QString& hexStr);
    Q_INVOKABLE void setAccentFromRgb(int r, int g, int b);
    Q_INVOKABLE void setAccentFromHsv(double h, double s, double v);
    Q_INVOKABLE void setAccentFromHsl(double h, double s, double l);
    Q_INVOKABLE void saveCurrentAccentToSwatches();

Q_SIGNALS:
    void themeChanged();
    void accentChanged();
    void motionChanged();
    void layoutChanged();
    void swatchesChanged();

private:
    void recomputeAccentTokens();

    bool m_darkTheme = true;
    bool m_reducedMotion = false;
    bool m_compactNav = false;

    core::RgbColor m_rgb{59, 158, 255};
    core::HsvColor m_hsv{210.0, 0.77, 1.0};
    core::HslColor m_hsl{210.0, 1.0, 0.62};
    QString m_accentHex = QStringLiteral("#3B9EFF");
    core::AccentTokenSet m_tokens;
    QStringList m_swatches;
};

} // namespace acc::app
