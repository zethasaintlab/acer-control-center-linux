---
name: native-desktop-architecture
description: Designs and implements maintainable native Linux desktop applications with Wayland support, KDE/Hyprland compatibility, hardware backends, D-Bus, Polkit, settings, and service boundaries. Use for architecture or implementation decisions in this repository.
---

# Native Linux Desktop Architecture

## First decision: choose technology from evidence
Before selecting a toolkit, compare at least Qt 6/QML, GTK 4/libadwaita, and a justified Rust-native option against:
- KDE Plasma integration and visual consistency.
- Hyprland and Wayland behavior.
- Fedora 45 packaging and dependency availability.
- Sensor visualization and custom controls.
- Accessibility, localization, testing, maintainability, and contributor familiarity.
- Separation between UI and low-level hardware operations.

Do not assume Qt is selected merely because it was mentioned as a candidate. Record a short Architecture Decision Record (ADR), including alternatives and trade-offs.

## Architectural boundaries
Keep these responsibilities separate:
1. Presentation/UI.
2. Application/domain state and use cases.
3. Hardware capability discovery.
4. Read-only telemetry adapters.
5. State-changing control adapters.
6. Privileged service boundary.
7. Settings/profiles.
8. Diagnostics/logging.

The UI must consume typed domain models and must not read arbitrary sysfs files or invoke hardware commands directly.

## Runtime behavior
- Do not block the UI thread with sensor polling, process execution, or D-Bus calls.
- Use asynchronous I/O or worker tasks with bounded concurrency and timeouts.
- Poll only sensors that exist; handle missing, stale, unavailable, and invalid readings explicitly.
- Avoid excessive polling. Use configurable intervals and suspend-aware lifecycle handling.
- Keep monitoring and controls independently degradable.
- Make service availability and hardware capability visible in the domain state.

## Linux integration
Prefer documented kernel interfaces and existing system services. Check whether power-profiles-daemon, tuned, UPower, NVIDIA services, or firmware already own a control. Do not create competing daemons or duplicate policy engines.

Use D-Bus and Polkit only where their boundaries are justified. Keep the GUI unprivileged. Design typed, versioned service methods and explicit error codes.

## Persistence
- Use a documented, versioned configuration schema.
- Validate imported/exported profiles.
- Never store credentials or secrets in project settings.
- Make migrations backward-compatible where practical.
- Preserve user settings through package upgrades and uninstall unless the user explicitly asks to remove them.

## Engineering quality
- Prefer explicit types and small modules.
- Avoid hidden global state and broad exception swallowing.
- Document public backend interfaces.
- Add unit and integration tests around adapters and domain logic.
- Use mocks for hardware-dependent tests.
- Write an ADR before introducing a new runtime, service, or dependency with significant operational impact.
