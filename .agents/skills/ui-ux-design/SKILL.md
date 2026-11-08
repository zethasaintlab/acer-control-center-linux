---
name: ui-ux-design
description: Creates and critiques high-quality desktop UX and visual systems for a native Linux hardware-control application, with strict anti-template and anti-AI-slop criteria. Use when designing screens, components, typography, colors, layout, interaction, or visual reviews.
---

# Desktop UI/UX Design Skill

## Design intent
Design a premium, original hardware instrument for a technically literate user. It should feel deliberate, calm, precise, and native to a desktop—not like a generic SaaS admin panel or AI-generated dashboard.

## Research before styling
Consult current and relevant primary guidance:
- KDE Human Interface Guidelines: `https://develop.kde.org/hig/`
- GNOME Human Interface Guidelines: `https://developer.gnome.org/hig/`
- Nielsen Norman Group usability heuristics: `https://www.nngroup.com/articles/ten-usability-heuristics/`
- WCAG: `https://www.w3.org/WAI/standards-guidelines/wcag/`

Extract applicable principles and turn them into concrete design decisions. Do not merely name-drop these references.

## Anti-AI-slop rejection checklist
Reject:
- Purple/blue gradients used by default or without meaning.
- Excessive glassmorphism, blur, glow, neon, and ornamental borders.
- Cards nested in cards or card-based layouts without information-architecture justification.
- Huge decorative headings, excessive pills/badges, arbitrary icon styles, emoji navigation.
- Fake charts, fake telemetry, ornamental gauges, random percentages, and ungrounded status indicators.
- Excessive rounded corners, generic dashboard templates, inconsistent spacing, and meaningless animation.
- UI that looks like disconnected components assembled from a template library.

## Required design process
1. Identify user tasks and information priority for each screen.
2. Define navigation and a consistent layout grid before adding decorative styling.
3. Define typography roles, spacing tokens, neutral surfaces, semantic colors, and one customizable accent system.
4. Design real states: loading, unavailable, unsupported, stale, permission denied, error, and success.
5. Design keyboard and pointer interactions; ensure visible focus and sensible tab order.
6. Check contrast, text scaling, localization length, reduced motion, and small-window behavior.
7. Use a consistent icon family and restrained motion.
8. Review every screen at 1920×1080 and at a smaller practical window size.
9. Show actual telemetry as data; use preview labels for mock/design-only content.
10. Capture screenshots and review alignment, density, clipping, typography, and contrast before calling a screen complete.

## Color requirements
- Application theme accent is always freely customizable using RGB/HSV/HSL/HEX values.
- Prevent custom accent choices from making text or controls inaccessible; provide contrast-aware variants or a warning.
- Physical keyboard LED color is a separate feature and is shown only when verified hardware support exists.
- Never imply that changing application accent changes physical keyboard LEDs.
- Dark and light themes must both be intentionally designed, not simple inversion.

## Visual direction
A restrained “Precision Hardware Atelier” direction is a starting point, not a rigid template: neutral foundation, precise typography, purposeful density, subtle surface differentiation, strong alignment, readable numeric telemetry, and minimal ornament.

## Review output
For each proposed screen, provide:
- User task and information hierarchy.
- Layout rationale.
- Component inventory.
- Interaction states.
- Accessibility checks.
- Anti-AI-slop checklist results.
- Screenshots or a precise description of unresolved visual issues.
