// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "app/ThemeEngine.h"

namespace acc::app {

ThemeEngine::ThemeEngine(QObject* parent)
    : QObject(parent) {
    m_swatches = {
        QStringLiteral("#FFEA00"),
        QStringLiteral("#A4E61D"),
        QStringLiteral("#F5D328"),
        QStringLiteral("#2596FF"),
        QStringLiteral("#F5B82E"),
        QStringLiteral("#9B6BFF"),
        QStringLiteral("#E06C65"),
        QStringLiteral("#68C938")
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
    return m_darkTheme ? QColor(QStringLiteral("#131616")) : QColor(QStringLiteral("#CFD5D9"));
}
QColor ThemeEngine::surfacePrimary() const {
    return m_darkTheme ? QColor(QStringLiteral("#222524")) : QColor(QStringLiteral("#D8DCDF"));
}
QColor ThemeEngine::surfaceElevated() const {
    return m_darkTheme ? QColor(QStringLiteral("#2B302F")) : QColor(QStringLiteral("#EDF0F2"));
}
QColor ThemeEngine::surfaceHeader() const {
    return m_darkTheme ? QColor(QStringLiteral("#181B1A")) : QColor(QStringLiteral("#F4F6F7"));
}
QColor ThemeEngine::surfaceRecessed() const {
    return m_darkTheme ? QColor(QStringLiteral("#4A5056")) : QColor(QStringLiteral("#B8C0C6"));
}
QColor ThemeEngine::surfaceSunken() const {
    return m_darkTheme ? QColor(QStringLiteral("#151818")) : QColor(QStringLiteral("#C5CDD2"));
}
QColor ThemeEngine::surfaceOverlay() const {
    return m_darkTheme ? QColor(QStringLiteral("#333837")) : QColor(QStringLiteral("#F7F9FA"));
}
QColor ThemeEngine::hairlineRule() const {
    return m_darkTheme ? QColor(QStringLiteral("#2E3433")) : QColor(QStringLiteral("#B6BFC6"));
}
QColor ThemeEngine::surfaceCadLight() const {
    return QColor(QStringLiteral("#D8DCDF"));
}
QColor ThemeEngine::surfacePillLight() const {
    return QColor(QStringLiteral("#F2F4F5"));
}
QColor ThemeEngine::surfaceSelectedDark() const {
    return QColor(QStringLiteral("#242629"));
}
QColor ThemeEngine::textOnLightCad() const {
    return QColor(QStringLiteral("#23272A"));
}
QColor ThemeEngine::textOnSelectedDark() const {
    return QColor(QStringLiteral("#F0F2F2"));
}
QColor ThemeEngine::darkInk() const {
    return QColor(QStringLiteral("#141618"));
}
QColor ThemeEngine::borderCadSubtle() const {
    return QColor(QStringLiteral("#9DA7B0"));
}
QColor ThemeEngine::borderSubtle() const {
    return m_darkTheme ? QColor(QStringLiteral("#3E4443")) : QColor(QStringLiteral("#AEB7BE"));
}
QColor ThemeEngine::borderStrong() const {
    return m_darkTheme ? QColor(QStringLiteral("#5C6463")) : QColor(QStringLiteral("#7C868E"));
}
QColor ThemeEngine::textPrimary() const {
    return m_darkTheme ? QColor(QStringLiteral("#F0F2F2")) : QColor(QStringLiteral("#23272A"));
}
QColor ThemeEngine::textSecondary() const {
    return m_darkTheme ? QColor(QStringLiteral("#B8C0C4")) : QColor(QStringLiteral("#3E454B"));
}
QColor ThemeEngine::textMuted() const {
    return m_darkTheme ? QColor(QStringLiteral("#8C9398")) : QColor(QStringLiteral("#5D656C"));
}

QColor ThemeEngine::statusNominal() const {
    return m_darkTheme ? QColor(QStringLiteral("#A4E61D")) : QColor(QStringLiteral("#4A7A00"));
}
QColor ThemeEngine::statusWarning() const {
    return m_darkTheme ? QColor(QStringLiteral("#F5B82E")) : QColor(QStringLiteral("#A86400"));
}
QColor ThemeEngine::statusCritical() const {
    return m_darkTheme ? QColor(QStringLiteral("#E06C65")) : QColor(QStringLiteral("#C5283D"));
}
QColor ThemeEngine::statusInfo() const {
    return m_darkTheme ? QColor(QStringLiteral("#2596FF")) : QColor(QStringLiteral("#0066CC"));
}
QColor ThemeEngine::brandYellow() const {
    return QColor(QStringLiteral("#FFEA00"));
}
QColor ThemeEngine::industrialAmber() const {
    return QColor(QStringLiteral("#F5D328"));
}
QColor ThemeEngine::telemetryLime() const {
    return m_darkTheme ? QColor(QStringLiteral("#A4E61D")) : QColor(QStringLiteral("#4A7A00"));
}
QColor ThemeEngine::telemetryLimeBar() const {
    return QColor(QStringLiteral("#A4E61D"));
}
QColor ThemeEngine::technicalCyan() const {
    return m_darkTheme ? QColor(QStringLiteral("#2596FF")) : QColor(QStringLiteral("#0066CC"));
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
