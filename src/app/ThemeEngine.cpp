// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/ThemeEngine.h"

namespace acc::app {

ThemeEngine::ThemeEngine(QObject* parent)
    : QObject(parent) {
    m_swatches = {
        QStringLiteral("#3B9EFF"),
        QStringLiteral("#2EBE7E"),
        QStringLiteral("#E5A024"),
        QStringLiteral("#E04F5F"),
        QStringLiteral("#00B8D9"),
        QStringLiteral("#D97706"),
        QStringLiteral("#84CC16"),
        QStringLiteral("#94A3B8")
    };
    recomputeAccentTokens();
}

void ThemeEngine::setDarkTheme(bool dark) {
    if (m_darkTheme != dark) {
        m_darkTheme = dark;
        recomputeAccentTokens();
        Q_EMIT themeChanged();
        Q_EMIT accentChanged();
    }
}

void ThemeEngine::setReducedMotion(bool reduced) {
    if (m_reducedMotion != reduced) {
        m_reducedMotion = reduced;
        Q_EMIT motionChanged();
    }
}

void ThemeEngine::setCompactNavigation(bool compact) {
    if (m_compactNav != compact) {
        m_compactNav = compact;
        Q_EMIT layoutChanged();
    }
}

QColor ThemeEngine::canvasBg() const {
    return m_darkTheme ? QColor(QStringLiteral("#0D0F12")) : QColor(QStringLiteral("#F3F5F8"));
}
QColor ThemeEngine::surfacePrimary() const {
    return m_darkTheme ? QColor(QStringLiteral("#13161B")) : QColor(QStringLiteral("#FFFFFF"));
}
QColor ThemeEngine::surfaceElevated() const {
    return m_darkTheme ? QColor(QStringLiteral("#191D24")) : QColor(QStringLiteral("#EAEEF4"));
}
QColor ThemeEngine::surfaceHeader() const {
    return m_darkTheme ? QColor(QStringLiteral("#101318")) : QColor(QStringLiteral("#E3E8F0"));
}
QColor ThemeEngine::borderSubtle() const {
    return m_darkTheme ? QColor(QStringLiteral("#232832")) : QColor(QStringLiteral("#D2D9E4"));
}
QColor ThemeEngine::borderStrong() const {
    return m_darkTheme ? QColor(QStringLiteral("#343C4A")) : QColor(QStringLiteral("#B4BFCE"));
}
QColor ThemeEngine::textPrimary() const {
    return m_darkTheme ? QColor(QStringLiteral("#ECEFF4")) : QColor(QStringLiteral("#12161F"));
}
QColor ThemeEngine::textSecondary() const {
    return m_darkTheme ? QColor(QStringLiteral("#9AA4B5")) : QColor(QStringLiteral("#4A5568"));
}
QColor ThemeEngine::textMuted() const {
    return m_darkTheme ? QColor(QStringLiteral("#646E80")) : QColor(QStringLiteral("#718096"));
}

QColor ThemeEngine::statusNominal() const {
    return m_darkTheme ? QColor(QStringLiteral("#2EBE7E")) : QColor(QStringLiteral("#168A56"));
}
QColor ThemeEngine::statusWarning() const {
    return m_darkTheme ? QColor(QStringLiteral("#E5A024")) : QColor(QStringLiteral("#B87503"));
}
QColor ThemeEngine::statusCritical() const {
    return m_darkTheme ? QColor(QStringLiteral("#E04F5F")) : QColor(QStringLiteral("#C5283D"));
}
QColor ThemeEngine::statusInfo() const {
    return m_darkTheme ? QColor(QStringLiteral("#3B9EFF")) : QColor(QStringLiteral("#0066CC"));
}

void ThemeEngine::setAccentSwatches(const QStringList& list) {
    m_swatches = list;
    Q_EMIT swatchesChanged();
}

void ThemeEngine::recomputeAccentTokens() {
    m_accentHex = core::ColorScience::rgbToHex(m_rgb);
    m_tokens = core::ColorScience::deriveAccessibleAccentTokens(QColor(m_rgb.r, m_rgb.g, m_rgb.b), m_darkTheme);
}

bool ThemeEngine::setAccentFromHex(const QString& hexStr) {
    core::RgbColor parsed;
    if (!core::ColorScience::parseHexToRgb(hexStr, parsed)) {
        return false;
    }
    m_rgb = parsed;
    m_hsv = core::ColorScience::rgbToHsv(m_rgb);
    m_hsl = core::ColorScience::rgbToHsl(m_rgb);
    recomputeAccentTokens();
    Q_EMIT accentChanged();
    return true;
}

void ThemeEngine::setAccentFromRgb(int r, int g, int b) {
    m_rgb = core::ColorScience::clampRgb(r, g, b);
    m_hsv = core::ColorScience::rgbToHsv(m_rgb);
    m_hsl = core::ColorScience::rgbToHsl(m_rgb);
    recomputeAccentTokens();
    Q_EMIT accentChanged();
}

void ThemeEngine::setAccentFromHsv(double h, double s, double v) {
    m_hsv = core::HsvColor{h, s, v};
    m_rgb = core::ColorScience::hsvToRgb(m_hsv);
    m_hsl = core::ColorScience::rgbToHsl(m_rgb);
    recomputeAccentTokens();
    Q_EMIT accentChanged();
}

void ThemeEngine::setAccentFromHsl(double h, double s, double l) {
    m_hsl = core::HslColor{h, s, l};
    m_rgb = core::ColorScience::hslToRgb(m_hsl);
    m_hsv = core::ColorScience::rgbToHsv(m_rgb);
    recomputeAccentTokens();
    Q_EMIT accentChanged();
}

void ThemeEngine::saveCurrentAccentToSwatches() {
    if (!m_swatches.contains(m_accentHex)) {
        m_swatches.prepend(m_accentHex);
        while (m_swatches.size() > 12) {
            m_swatches.removeLast();
        }
        Q_EMIT swatchesChanged();
    }
}

} // namespace acc::app
