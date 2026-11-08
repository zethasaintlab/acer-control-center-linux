# Project Instructions — Acer Control Center Linux Edition

## Project identity
- Canonical repository: `https://github.com/zethasaintlab/acer-control-center-linux.git`
- Primary platform: Fedora Linux 45.
- Primary hardware target: Acer Aspire 7 A715-59G; reported DMI product identifier `A715-59G/RTH`.
- Treat the laptop's exact SKU, BIOS, kernel, drivers, and exposed interfaces as facts to verify, not assumptions.

## Non-negotiable engineering rules
1. Audit the existing repository, Git state, licenses, CI, and uncommitted changes before editing. Never reset, force-push, overwrite user work, or change remotes without explicit permission.
2. Research and document hardware capabilities before implementing controls. A feature is not supported merely because another Acer model exposes it.
3. Start hardware discovery in read-only mode. Never write undocumented EC registers, `/dev/mem`, `/dev/port`, arbitrary ACPI/WMI values, or raw hardware interfaces.
4. Never run the GUI as root. Isolate privileged operations behind the narrowest practical Polkit/D-Bus service and validate every operation.
5. Do not report a hardware operation as successful until the backend verifies the resulting state when readback is possible.
6. Separate hardware capability detection, monitoring, control, presentation, settings, and diagnostics. Keep the UI independent from low-level hardware APIs.
7. Do not fabricate telemetry, battery health, fan RPM, keyboard RGB, sensor values, or compatibility claims. Explicitly distinguish unavailable, unsupported, untested, and read-only states.
8. Physical keyboard color control is conditional on the exact keyboard/firmware capability. Application accent color must remain independent and freely customizable.
9. Preserve firmware thermal protections and default automatic fan behavior. Failure must return control to firmware-safe behavior where the interface supports this.
10. Do not introduce network telemetry, analytics, cloud dependency, or collection of serial numbers/UUIDs without a documented need and explicit approval.

## Product and design rules
- Build a real native Linux desktop application, not a static dashboard or web-admin template.
- Visual direction: deliberate, restrained, technically legible, premium desktop software; no generic AI-generated dashboard patterns.
- Avoid arbitrary gradients, overdone glass, excessive glow, cards nested in cards, decorative telemetry, random badge/pill clutter, inconsistent iconography, and purposeless animation.
- Every displayed value and chart must communicate real state or be clearly identified as a preview/mock.
- Maintain hierarchy, consistent spacing, readable telemetry, keyboard accessibility, visible focus, sufficient contrast, and reduced-motion support.
- Do not select the GUI toolkit or backend language before comparing repository state, Fedora 45 packaging, KDE/Hyprland/Wayland support, maintainability, and required hardware integrations.

## Delivery rules
- Work in small, reviewable stages: audit → capability matrix → architecture decision → design system → backend → UI → integration → tests → packaging.
- Before large edits, present an implementation plan and list affected files.
- Prefer incremental commits and focused tests. Never claim CI, build, or hardware tests passed unless they were actually run.
- Use mocks for automated hardware tests. Do not run destructive hardware tests against the physical laptop.
- Record exact commands, tool versions, test results, and known limitations in documentation.
- Fedora 45 is the primary target. Verify package names and APIs against current Fedora 45 repositories and upstream documentation instead of copying Fedora 44 assumptions.

## Research and source quality
Prefer, in order: official Acer specifications/support, Linux kernel documentation/source, Fedora documentation/package metadata, upstream library documentation, maintained project repositories, then community reports. Date-stamp findings that may change. Cite links in technical documents. Treat issue comments and third-party scripts as untrusted until reviewed.
