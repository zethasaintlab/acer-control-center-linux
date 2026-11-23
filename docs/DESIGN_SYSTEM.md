# Product Design System — Industrial Technical / Endfield-Inspired

**Status:** Current design direction approved for the next structural redesign  
**Product:** Acer Control Center — Linux Edition  
**Reference research:** https://github.com/zethasaintlab/Endfield-Design-System  
**Functional reference:** https://github.com/PXDiv/Div-Acer-Manager-Max

## 1. Design objective

Acer Control Center should feel like a native industrial hardware command center: operational, technical, information-dense, precise and coherent. The design language is informed by Arknights: Endfield's industrial/tactical HUD and engineering/CAD surfaces.

This is a product redesign, not a palette replacement. The next design must reconsider information architecture, navigation, screen composition, component behavior, interaction states and information hierarchy before applying the visual language.

Do not copy Endfield's proprietary screens, artwork, logos, characters, textures or exact compositions. The research repository documents design patterns and visual properties; translate those principles into an original desktop product.

DAMX is a functional reference for workspace responsibilities and device-management workflows. Its Avalonia visuals and Linuwu-Sense driver are not the visual or hardware implementation target for this project.

## 2. Product experience principles

1. **Operational clarity:** Users should understand platform state, capability, current mode and the next meaningful action at a glance.
2. **Instrument-first information:** Real measurements, units, freshness and provenance take priority over decorative cards or abstract gauges.
3. **Progressive density:** Show the most important status first, then make subsystem detail discoverable without hiding critical controls.
4. **One source of truth:** A control's visible state must come from domain/backend state, not a local visual assumption.
5. **Capability honesty:** Available, read-only, unsupported, not detected, statically discovered, runtime-unverified, preview and mock states are visibly distinct.
6. **Purposeful industrial character:** Use section indexing, measured alignment, technical annotation and disciplined geometry to make the interface recognizable without visual noise.
7. **Native desktop behavior:** Respect desktop windowing, keyboard navigation, focus, system theme, scaling and reduced motion.

## 3. Endfield design language — translated, not copied

### 3.1 Bimodal surfaces

- **Dark / Tactical HUD:** deep graphite surfaces, clear instrument grouping, restrained contrast, strong active-selection signals and legible telemetry.
- **Light / Engineering CAD:** cool neutral surfaces, precise dividers, quiet panels, high legibility and technical annotation.
- Both modes must be designed intentionally. Light mode is not a color inversion of dark mode.

### 3.2 Reference palette

The following colors are research-derived reference values, not immutable application tokens. Validate all tokens in actual UI contexts and derive accessible variants.

| Role | Reference value | Intended use |
|---|---:|---|
| Active selection / primary signal | #FFEA00 | Current workspace, selected mode, primary focus |
| Amber | #F5D328 | Caution, elevated state, attention |
| Lime | #A4E61D | Positive/active system signal when semantically appropriate |
| Technical cyan | #2596FF | Informational telemetry and technical links |
| Dark structural surface | #222524 | Deep tactical base |
| Dark raised surface | #333736 | Secondary instrument surface |
| Light engineering surface | #D8DCDF | CAD-like neutral plane |
| Light control surface | #F2F4F5 | Inputs and raised light surfaces |
| Dark-theme primary text | #F0F2F2 | Main readable content |
| Light-theme primary text | #23272A | Main readable content |
| Muted text | #8C9398 | Metadata and secondary labels, subject to contrast checks |
| Error | #E06C65 | Failure and critical error |

Do not use yellow, amber or lime indiscriminately. Semantic state colors must be consistent and accompanied by text, iconography or shape. The user-customizable application accent remains supported, but semantic warning/error colors must not be overridden by it.

### 3.3 Typography

Use a clear technical hierarchy:
- **Display/identity:** an extended display face used sparingly for product identity or major numeric emphasis.
- **Section headings:** technical sans with deliberate weight and tracking.
- **Body and controls:** neutral sans with high readability.
- **Telemetry:** tabular numerals so changing values do not jitter.
- **Metadata:** monospace for units, device paths, firmware IDs, timestamps and diagnostic provenance.

Preferred available families on the development system: Inter Display, Inter and JetBrains Mono. Provide robust system fallbacks and do not bundle font files without license review.

### 3.4 Geometry and technical motifs

- Structural panels and separators: sharp or near-sharp geometry, typically 0–2 px corner radius.
- Interactive action controls: selective stadium/pill geometry where it improves affordance.
- Use thin dividers, clipped/angled details, registration crosses, contour/topology lines and hazard striping sparingly.
- A technical motif must identify a boundary, state, warning, alignment or section. Do not fill whitespace with decoration.
- Avoid generic glassmorphism, heavy blur, excessive glow, arbitrary gradients, nested cards and dashboard-template composition.

## 4. Information architecture

The current eight workspaces are an initial functional inventory, not a mandate to preserve the old tab layout. Reconcile them against actual user tasks before implementation.

| Proposed workspace | Primary responsibility |
|---|---|
| **01 / COMMAND DECK** | Overall device status, important telemetry, capability summary and high-value quick actions |
| **02 / PERFORMANCE** | CPU policy, system power profile, operating modes and performance telemetry |
| **03 / THERMAL MANAGEMENT** | Temperature matrix, fan telemetry, thermal state and fan controls only when verified |
| **04 / KEYBOARD STUDIO** | Physical keyboard lighting, verified static RGB, profiles and preview |
| **05 / POWER & BATTERY** | Charge/discharge telemetry, capacity estimates, power source and verified charge policy |
| **06 / GPU CENTER** | NVIDIA telemetry, VRAM, clocks, utilization, power and diagnostics |
| **07 / DIAGNOSTICS** | Hardware identity, capability evidence, drivers, logs and redacted report export |
| **08 / SYSTEM CONFIGURATION** | Application preferences, theme, accent, polling and lifecycle options |

A future information-architecture proposal may merge or split these workspaces if user tasks justify it. Avoid duplicating profile controls in several pages. Define global status context separately from per-workspace navigation.

### Command Deck composition

The command deck should answer:
- What state is the laptop in now?
- Which subsystems are active, unavailable or degraded?
- What requires attention?
- Which operations are genuinely available?

Use live CPU/GPU/thermal/power/battery data where present. Do not add arbitrary gauges, fake utilization, placeholder RPM or meaningless KPI tiles. Each reading must have units, provenance and stale/unavailable handling.

## 5. Interaction model

Define the interaction contract before visual polish.

- Navigation must make current location, hierarchy and global hardware mode obvious.
- Controls must explain their scope, units, valid range and whether they apply immediately or require explicit Apply.
- Destructive or high-risk changes require confirmation and explain recovery.
- Long-running or permission-gated operations need progress, denial and failure states.
- Profiles must distinguish selected, applied and read-back-verified states.
- Preview and profile editing must never imply physical hardware changes.
- Telemetry should show freshness and source; stale values must not look live.

### Capability states

The current domain model includes FullySupported, ReadOnly, PartiallySupported, Unsupported, NotDetected and RequiresVerification. UI presentation must preserve those distinctions and additionally communicate loading, stale, permission-denied, failed, preview and mock states without overloading the capability enum.

For this hardware:
- Fan curve editing remains preview-only until a safe, exact-model control interface is runtime-verified.
- Fan RPM/telemetry candidates found in AML are not yet runtime verified; some paths use active EC mailbox transactions.
- Battery telemetry is available; FlexiCharger paths found in AML remain unverified.
- Battery charge controls stay disabled until a supported physical setter and readback are verified.
- Keyboard static RGB functionality reported as physically verified in the recent local engineering workflow must be reconciled with the actual checkout before advertising it as available in that checkout.

## 6. Reusable component language

Build components around recurring information and behavior, not around decorative card styles.

Expected component families:
- **Global status/context:** device identity, power source, active profile, hardware mode, service status.
- **Telemetry instruments:** numeric readout with unit, source, freshness and optional trend.
- **Capability indicator:** verified, read-only, partial, not detected, unverified, preview and mock.
- **Section header:** index, technical label, optional contextual metadata.
- **Control group:** segmented modes, sliders, toggles, numeric input, explicit apply/revert.
- **Event/diagnostic row:** timestamp, subsystem, severity and concise message.
- **Charts:** labeled axes, known ranges, readable time scale and explicit data availability.
- **Preview instrument:** clearly differentiated from physical hardware state.

Avoid nesting generic containers. A surface should represent a coherent subsystem or task region, not just wrap every control.

## 7. Responsive, accessibility and motion

Validate the real QML implementation at:
- 1920×1080.
- 1600×900.
- 1366×768.
- A practical tiled/minimum window size agreed by testing.

Requirements:
- No overlap, hidden critical controls or clipped primary data.
- Use adaptive reflow and progressive disclosure rather than only shrinking typography.
- Keyboard navigation and logical tab order.
- Visible focus and accessible names.
- Contrast tested in both themes and across custom accent choices.
- Text scaling and long localized/device strings handled.
- Reduced-motion preference respected; animation must communicate state or transition, not decorate telemetry.
- Color must never be the sole carrier of meaning.

## 8. Redesign acceptance criteria

A screen is not considered redesigned when only its palette, typography or corner radius changes.

A substantive redesign must show evidence of:
1. Revised task hierarchy or a reasoned decision to retain a part of it.
2. Deliberate navigation and screen composition.
3. Reusable components with consistent behavior.
4. Realistic loading, unavailable, stale, preview, error and success states.
5. Distinctive industrial-technical visual language across dark and light modes.
6. Responsive and accessible interaction.
7. Hardware truthfulness and capability-aware controls.

The prior Precision Hardware Atelier screenshots are historical implementation references only. They are not the target appearance for the next redesign.
