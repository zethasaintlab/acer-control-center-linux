// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include <QColor>
#include <QString>

namespace acc::core {

struct RgbColor {
    int r = 59;
    int g = 158;
    int b = 255;
};

struct HsvColor {
    double h = 210.0; // [0.0, 360.0)
    double s = 0.77;  // [0.0, 1.0]
    double v = 1.0;   // [0.0, 1.0]
};

struct HslColor {
    double h = 210.0; // [0.0, 360.0)
    double s = 1.0;   // [0.0, 1.0]
    double l = 0.62;  // [0.0, 1.0]
};

struct AccentTokenSet {
    QColor primary;
    QColor onAccentText;       // Guaranteed >= 4.5:1 against primary
    QColor readableOnSurface;  // Guaranteed >= 4.5:1 against surfacePrimary
    QColor activeBorder;       // Guaranteed >= 3.0:1 against surfacePrimary
    QColor surfaceTint;        // Subtle translucent tint
    double contrastOnSurface = 4.5;
    double contrastOnAccent = 4.5;
    bool wcagAaNormal = true;
    bool wcagAaaNormal = false;
};

class ColorScience {
public:
    static RgbColor clampRgb(int r, int g, int b);
    static HsvColor rgbToHsv(const RgbColor& rgb);
    static RgbColor hsvToRgb(const HsvColor& hsv);
    static HslColor rgbToHsl(const RgbColor& rgb);
    static RgbColor hslToRgb(const HslColor& hsl);

    static QString rgbToHex(const RgbColor& rgb);
    static bool parseHexToRgb(const QString& hexString, RgbColor& outRgb);

    // WCAG 2.1 relative luminance in [0.0, 1.0]
    static double relativeLuminance(const QColor& color);

    // WCAG 2.1 contrast ratio in [1.0, 21.0]
    static double contrastRatio(const QColor& c1, const QColor& c2);

    // Computes contrast-safe derived accent tokens for Dark or Light theme surfaces
    static AccentTokenSet deriveAccessibleAccentTokens(const QColor& rawAccent, bool darkTheme);
};

} // namespace acc::core
