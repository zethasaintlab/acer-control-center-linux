// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "core/ColorScience.h"

#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace acc::core {

namespace {

double srgbChannelToLinear(double cNormalized) {
    if (cNormalized <= 0.04045) {
        return cNormalized / 12.92;
    }
    return std::pow((cNormalized + 0.055) / 1.055, 2.4);
}

double wrapHue(double h) {
    double mod = std::fmod(h, 360.0);
    if (mod < 0.0) {
        mod += 360.0;
    }
    return mod;
}

} // namespace

RgbColor ColorScience::clampRgb(int r, int g, int b) {
    return RgbColor{
        std::clamp(r, 0, 255),
        std::clamp(g, 0, 255),
        std::clamp(b, 0, 255)
    };
}

HsvColor ColorScience::rgbToHsv(const RgbColor& rgb) {
    const RgbColor c = clampRgb(rgb.r, rgb.g, rgb.b);
    const double r = c.r / 255.0;
    const double g = c.g / 255.0;
    const double b = c.b / 255.0;

    const double maxC = std::max({r, g, b});
    const double minC = std::min({r, g, b});
    const double delta = maxC - minC;

    double h = 0.0;
    if (delta > 1e-6) {
        if (maxC == r) {
            h = 60.0 * std::fmod(((g - b) / delta), 6.0);
        } else if (maxC == g) {
            h = 60.0 * (((b - r) / delta) + 2.0);
        } else {
            h = 60.0 * (((r - g) / delta) + 4.0);
        }
        h = wrapHue(h);
    }

    const double s = (maxC <= 1e-6) ? 0.0 : (delta / maxC);
    const double v = maxC;
    return HsvColor{h, std::clamp(s, 0.0, 1.0), std::clamp(v, 0.0, 1.0)};
}

RgbColor ColorScience::hsvToRgb(const HsvColor& hsv) {
    const double h = wrapHue(hsv.h);
    const double s = std::clamp(hsv.s, 0.0, 1.0);
    const double v = std::clamp(hsv.v, 0.0, 1.0);

    const double c = v * s;
    const double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
    const double m = v - c;

    double rPrime = 0.0;
    double gPrime = 0.0;
    double bPrime = 0.0;

    if (h < 60.0) {
        rPrime = c; gPrime = x; bPrime = 0.0;
    } else if (h < 120.0) {
        rPrime = x; gPrime = c; bPrime = 0.0;
    } else if (h < 180.0) {
        rPrime = 0.0; gPrime = c; bPrime = x;
    } else if (h < 240.0) {
        rPrime = 0.0; gPrime = x; bPrime = c;
    } else if (h < 300.0) {
        rPrime = x; gPrime = 0.0; bPrime = c;
    } else {
        rPrime = c; gPrime = 0.0; bPrime = x;
    }

    return clampRgb(
        static_cast<int>(std::lround((rPrime + m) * 255.0)),
        static_cast<int>(std::lround((gPrime + m) * 255.0)),
        static_cast<int>(std::lround((bPrime + m) * 255.0))
    );
}

HslColor ColorScience::rgbToHsl(const RgbColor& rgb) {
    const RgbColor c = clampRgb(rgb.r, rgb.g, rgb.b);
    const double r = c.r / 255.0;
    const double g = c.g / 255.0;
    const double b = c.b / 255.0;

    const double maxC = std::max({r, g, b});
    const double minC = std::min({r, g, b});
    const double delta = maxC - minC;

    double h = 0.0;
    if (delta > 1e-6) {
        if (maxC == r) {
            h = 60.0 * std::fmod(((g - b) / delta), 6.0);
        } else if (maxC == g) {
            h = 60.0 * (((b - r) / delta) + 2.0);
        } else {
            h = 60.0 * (((r - g) / delta) + 4.0);
        }
        h = wrapHue(h);
    }

    const double l = (maxC + minC) * 0.5;
    const double s = (delta <= 1e-6) ? 0.0 : (delta / (1.0 - std::abs(2.0 * l - 1.0)));
    return HslColor{h, std::clamp(s, 0.0, 1.0), std::clamp(l, 0.0, 1.0)};
}

RgbColor ColorScience::hslToRgb(const HslColor& hsl) {
    const double h = wrapHue(hsl.h);
    const double s = std::clamp(hsl.s, 0.0, 1.0);
    const double l = std::clamp(hsl.l, 0.0, 1.0);

    const double c = (1.0 - std::abs(2.0 * l - 1.0)) * s;
    const double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
    const double m = l - c * 0.5;

    double rPrime = 0.0;
    double gPrime = 0.0;
    double bPrime = 0.0;

    if (h < 60.0) {
        rPrime = c; gPrime = x; bPrime = 0.0;
    } else if (h < 120.0) {
        rPrime = x; gPrime = c; bPrime = 0.0;
    } else if (h < 180.0) {
        rPrime = 0.0; gPrime = c; bPrime = x;
    } else if (h < 240.0) {
        rPrime = 0.0; gPrime = x; bPrime = c;
    } else if (h < 300.0) {
        rPrime = x; gPrime = 0.0; bPrime = c;
    } else {
        rPrime = c; gPrime = 0.0; bPrime = x;
    }

    return clampRgb(
        static_cast<int>(std::lround((rPrime + m) * 255.0)),
        static_cast<int>(std::lround((gPrime + m) * 255.0)),
        static_cast<int>(std::lround((bPrime + m) * 255.0))
    );
}

QString ColorScience::rgbToHex(const RgbColor& rgb) {
    const RgbColor c = clampRgb(rgb.r, rgb.g, rgb.b);
    return QStringLiteral("#%1%2%3")
        .arg(c.r, 2, 16, QLatin1Char('0'))
        .arg(c.g, 2, 16, QLatin1Char('0'))
        .arg(c.b, 2, 16, QLatin1Char('0'))
        .toUpper();
}

bool ColorScience::parseHexToRgb(const QString& hexString, RgbColor& outRgb) {
    QString trimmed = hexString.trimmed();
    if (trimmed.startsWith(QLatin1Char('#'))) {
        trimmed.remove(0, 1);
    }
    if (trimmed.size() == 3) {
        QString expanded;
        expanded.reserve(6);
        for (const QChar ch : trimmed) {
            expanded.append(ch);
            expanded.append(ch);
        }
        trimmed = expanded;
    }
    static const QRegularExpression kHexRegex(QStringLiteral("^[0-9A-Fa-f]{6}$"));
    if (!kHexRegex.match(trimmed).hasMatch()) {
        return false;
    }

    bool okR = false, okG = false, okB = false;
    const int r = trimmed.mid(0, 2).toInt(&okR, 16);
    const int g = trimmed.mid(2, 2).toInt(&okG, 16);
    const int b = trimmed.mid(4, 2).toInt(&okB, 16);
    if (!okR || !okG || !okB) {
        return false;
    }
    outRgb = clampRgb(r, g, b);
    return true;
}

double ColorScience::relativeLuminance(const QColor& color) {
    if (!color.isValid()) {
        return 0.0;
    }
    const double r = srgbChannelToLinear(color.redF());
    const double g = srgbChannelToLinear(color.greenF());
    const double b = srgbChannelToLinear(color.blueF());
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double ColorScience::contrastRatio(const QColor& c1, const QColor& c2) {
    const double l1 = relativeLuminance(c1);
    const double l2 = relativeLuminance(c2);
    const double lighter = std::max(l1, l2);
    const double darker = std::min(l1, l2);
    return (lighter + 0.05) / (darker + 0.05);
}

AccentTokenSet ColorScience::deriveAccessibleAccentTokens(const QColor& rawAccent, bool darkTheme) {
    const QColor validAccent = rawAccent.isValid() ? rawAccent : QColor(QStringLiteral("#3B9EFF"));
    const QColor surfaceBg = darkTheme ? QColor(QStringLiteral("#13161B")) : QColor(QStringLiteral("#FFFFFF"));
    const QColor darkInk(QStringLiteral("#0D0F12"));
    const QColor lightInk(QStringLiteral("#FFFFFF"));

    AccentTokenSet tokens;
    tokens.primary = validAccent;

    // 1. Choose foreground text color on top of a solid accent button
    const double crDarkInk = contrastRatio(validAccent, darkInk);
    const double crLightInk = contrastRatio(validAccent, lightInk);
    if (crDarkInk >= crLightInk) {
        tokens.onAccentText = darkInk;
        tokens.contrastOnAccent = crDarkInk;
    } else {
        tokens.onAccentText = lightInk;
        tokens.contrastOnAccent = crLightInk;
    }

    // 2. Ensure readableOnSurface achieves >= 4.5:1 against surfacePrimary
    HslColor hsl = rgbToHsl(RgbColor{validAccent.red(), validAccent.green(), validAccent.blue()});
    QColor candidate = validAccent;
    double crSurface = contrastRatio(candidate, surfaceBg);
    for (int step = 0; step < 60 && crSurface < 4.6; ++step) {
        if (darkTheme) {
            hsl.l = std::min(0.95, hsl.l + 0.015);
        } else {
            hsl.l = std::max(0.12, hsl.l - 0.015);
        }
        const RgbColor adj = hslToRgb(hsl);
        candidate = QColor(adj.r, adj.g, adj.b);
        crSurface = contrastRatio(candidate, surfaceBg);
    }
    tokens.readableOnSurface = candidate;
    tokens.contrastOnSurface = crSurface;
    tokens.wcagAaNormal = (crSurface >= 4.5);
    tokens.wcagAaaNormal = (crSurface >= 7.0);

    // 3. Ensure activeBorder achieves >= 3.0:1 against surfacePrimary
    HslColor borderHsl = rgbToHsl(RgbColor{validAccent.red(), validAccent.green(), validAccent.blue()});
    QColor borderCandidate = validAccent;
    double crBorder = contrastRatio(borderCandidate, surfaceBg);
    for (int step = 0; step < 50 && crBorder < 3.1; ++step) {
        if (darkTheme) {
            borderHsl.l = std::min(0.90, borderHsl.l + 0.015);
        } else {
            borderHsl.l = std::max(0.18, borderHsl.l - 0.015);
        }
        const RgbColor adj = hslToRgb(borderHsl);
        borderCandidate = QColor(adj.r, adj.g, adj.b);
        crBorder = contrastRatio(borderCandidate, surfaceBg);
    }
    tokens.activeBorder = borderCandidate;

    // 4. Subtle surface tint
    QColor tint = validAccent;
    tint.setAlphaF(darkTheme ? 0.14f : 0.10f);
    tokens.surfaceTint = tint;

    return tokens;
}

} // namespace acc::core
