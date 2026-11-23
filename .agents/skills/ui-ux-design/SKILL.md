---
name: ui-ux-design
description: Designs and critiques the Acer Control Center's industrial-technical desktop experience, from information architecture and navigation through components, interactions and visual system.
---

# Desktop Product Design Skill

## Product objective

Create a native Linux hardware command center for the Acer Aspire 7 A715-59G. Its interface should feel like a deliberate technical instrument: clear operational hierarchy, purposeful information density, precise controls and a coherent industrial identity.

The visual reference is the project's Endfield research repository:
https://github.com/zethasaintlab/Endfield-Design-System

Use its bimodal industrial-technical language:
- Dark mode: tactical HUD, instrument bays, restrained contrast, technical status and operational focus.
- Light mode: clean CAD/engineering surfaces, precise dividers, calm workspace and readable annotations.
- Industrial geometry: mostly sharp structural edges, disciplined panel boundaries, selective pill-shaped actions.
- Technical motifs: indexed section labels, topological/contour references, registration marks and restrained hazard striping where they communicate hierarchy or caution.

The reference informs design decisions; it is not a screen template. Do not copy Endfield artwork, logos, character assets, proprietary screenshots or exact game UI compositions.

## Functional reference

Study Div Acer Manager Max for workspace responsibilities, dashboard content, hardware-setting workflows and capability-aware presentation:
https://github.com/PXDiv/Div-Acer-Manager-Max

DAMX is not a visual target, driver dependency or proof of A715-59G compatibility. Its Linuwu-Sense backend targets different Acer WMI interfaces and must not be installed or reused as a hardware authority for this machine.

## Redesign before styling

A color-only reskin is not an acceptable redesign. Work in this order:

1. Understand user tasks and the existing implementation.
2. Reconcile workspace ownership and information architecture.
3. Design navigation, global status context and screen-level composition.
4. Define interaction flows and all important UI states.
5. Establish reusable components and design tokens.
6. Apply the visual language and refine motion/decoration.
7. Validate actual screens at realistic sizes and with real/empty/error data.

The current eight workspace responsibilities are a starting point, not a requirement to retain their exact names or layout:
- Command Deck / overview.
- Performance.
- Thermal Management.
- Keyboard Studio.
- Power & Battery.
- GPU Center.
- Diagnostics.
- System Configuration.

Combine or split workspaces only when user tasks and information ownership justify it. Avoid navigation bloat and duplicate controls.

## Design language

Use the palette in docs/DESIGN_SYSTEM.md as a reference-derived starting point. Semantic roles must drive color meaning; a status must not be communicated by hue alone. Suggested reference colors include Endfield yellow, amber and lime accents, technical cyan, industrial dark/light surfaces, and explicit warning/error semantics. Recheck every token for contrast in both themes.

Typography should have clear roles:
- Extended/display face for rare high-level identity moments.
- Technical sans for workspace and section headings.
- Neutral sans for labels and explanatory copy.
- Tabular numerals for telemetry that changes over time.
- Monospace for units, paths, firmware identifiers and metadata.

Prefer the installed system families Inter Display, Inter and JetBrains Mono when available. Keep fallback behavior deterministic.

Structural surfaces should be disciplined and readable. Use decorative marks only when they establish grouping, orientation, warning or technical context. Do not place a technical motif in every empty area.

## Interaction and capability states

Design and test:
- Initial loading and service connection.
- Available and verified hardware capability.
- Read-only capability.
- Unavailable or unsupported hardware.
- Static firmware candidate / unverified capability.
- Preview or mock-only interaction.
- Stale telemetry.
- Permission denied, failed operation and verified success.

Never imply that a preview changed physical hardware. Never show simulated readings as live telemetry. Fan controls remain preview-only while the physical actuator is unverified. Battery charge-limit controls remain disabled while the physical interface is unverified.

Keyboard RGB and application theme accent are independent systems.

## Accessibility and desktop behavior

Support keyboard navigation, visible focus, logical tab order, accessible names, contrast, text scaling, localization growth, reduced motion and useful error text. Design both dark and light themes intentionally. Validate at the project's practical tiled-window sizes as well as 1920×1080; avoid fixed layouts that only work at the design screenshot size.

## Review standard

A screen is not complete because it looks attractive in a screenshot. Check task completion, information hierarchy, truthful hardware state, empty/loading/error states, accessibility, responsive behavior and consistency with the reusable component system.
