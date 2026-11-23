# Acer Control Center — Agent Instructions

## Project identity
A native Linux hardware command center for the Acer Aspire 7 A715-59G (Kelilu_RTH / RTH), primarily targeting Fedora Linux 45, KDE Plasma and Hyprland on Wayland.

- Repository: https://github.com/zethasaintlab/acer-control-center-linux
- Hardware target: Acer Aspire A715-59G; board Kelilu_RTH / RTH; INSYDE BIOS V1.17.
- Approved implementation stack in ADR-001: C++23, Qt 6 / QML, CMake, D-Bus and Polkit.
- Visual direction: Endfield-inspired industrial-technical interface. This means structural UX and information architecture, not recoloring the previous interface.
- Functional reference: Div Acer Manager Max (DAMX). Study its workspaces and device-management workflows; do not assume its Linuwu-Sense driver or hardware support applies to this laptop.

## Read the current source of truth
Before substantial work, consult:
1. docs/PROJECT_STATE.md — latest reconciled project and hardware status.
2. docs/WORKFLOW.md — collaboration, autonomy, stage and handoff conventions.
3. docs/DESIGN_SYSTEM.md — current product UX and visual direction.
4. docs/HARDWARE_AUDIT_A715_59G.md — model-specific evidence and safety posture.
5. docs/ARCHITECTURE_ADR.md — approved application and privilege architecture.

When sources conflict, prefer fresh repository/runtime evidence, then the dated hardware audit and project-state record. Report contradictions; do not silently choose the most convenient source.

## Engineering principles
- Be outcome-oriented and exercise engineering judgment. Determine the appropriate investigation and implementation approach rather than waiting for step-by-step commands.
- Inspect the actual checkout, branch, HEAD, working tree and relevant code before relying on handoff notes. Preserve all user changes.
- Keep the GUI unprivileged. Keep privileged operations behind narrow typed D-Bus/Polkit interfaces.
- Separate capability discovery, telemetry, state-changing controls, domain state, presentation, persistence and diagnostics.
- Never fabricate telemetry, capability, readback, hardware support or test results.
- Distinguish observed, documented, statically inferred, runtime-observed and physically verified facts.
- A capability found in AML, a mock, a similarly named device or another Acer model is not proof that it works on this machine.
- Use mocks for automated hardware tests. Physical testing requires a separate, explicit authorization for the exact operation.

## Hardware safety
- Begin hardware investigation read-only.
- Do not guess EC offsets or execute undocumented ACPI, WMI, _DSM, WMBB or EC commands.
- Do not use /dev/mem, /dev/port, raw port I/O or arbitrary sysfs paths as product interfaces.
- Preserve firmware-managed thermal behavior. Do not expose fan controls until the exact interface, semantics, failure behavior and safe fallback are verified.
- Keep battery charge controls disabled until a supported interface and the A715-59G implementation are runtime-verified.
- Stage 22/22.1 discovered static vendor AML paths. This does not authorize runtime method invocation. The initial zero-mailbox query candidates still require Stage 22.2 closure and separate Stage 23 authorization.
- Never run the GUI as root or broaden permissions to avoid an approval prompt.

## Product and design
- Treat docs/DESIGN_SYSTEM.md as the current design contract.
- Redesign information architecture, navigation, screen composition, component language and interaction states before applying visual styling.
- Endfield is a design-language reference, not a source of copied art, assets, logos or UI screens.
- DAMX is a functional workflow reference, not a driver or compatibility authority.
- Keep application theme accent and physical keyboard lighting independent.
- Every metric and control must communicate its source, freshness and capability state. Clearly label preview, mock, unavailable, unverified and physical modes.
- Preserve accessibility, keyboard operation, focus visibility, contrast, reduced motion and practical tiled-window behavior.

## Delivery
- Work in focused, reviewable increments. Plan broadly only when the task is genuinely architectural or high-risk.
- Run relevant project-local build, lint and tests when available. Report exactly what was run and what was not.
- Do not claim a physical test, Fedora compatibility, successful build or CI pass without evidence.
- Do not commit, push, merge, reset, force-push or discard user work without explicit authorization.
- For handoffs, report changed files, evidence, tests, limitations and one recommended next step.
