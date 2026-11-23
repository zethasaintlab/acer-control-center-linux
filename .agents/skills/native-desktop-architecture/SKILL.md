---
name: native-desktop-architecture
description: Maintains the approved Qt/C++ native Linux desktop architecture, including QML, D-Bus, Polkit, capability discovery, telemetry, control boundaries and Fedora packaging.
---

# Native Desktop Architecture

## Approved baseline

ADR-001 already selects C++23, Qt 6 / QtQuick-QML, QtDBus, CMake and Fedora RPM packaging. Do not reopen a toolkit bake-off or migrate to GTK, Avalonia, Electron, Tauri or a webview without a concrete requirement and a new Architecture Decision Record.

The application must remain a native Linux desktop application with KDE Plasma and Hyprland Wayland support.

## Architectural boundaries

Keep responsibilities explicit:
1. QML presentation and reusable visual components.
2. Application/domain state and use cases.
3. Hardware capability discovery.
4. Read-only telemetry adapters.
5. Typed state-changing control adapters.
6. Privileged D-Bus/Polkit service.
7. Settings and profile persistence.
8. Diagnostics and redacted logging.

The UI must not read arbitrary sysfs files or invoke hardware commands directly. Keep the UI responsive; sensor polling, process execution and D-Bus calls must not block the GUI thread.

## Capability-first operation

Keep monitoring independent from control. Model available, read-only, partial, unsupported, not-detected and requires-verification states honestly. Distinguish stale data, preview/mock data, permission errors and failed hardware operations.

Never expose an operation just because a control exists in QML or a method name exists in AML. The backend owns capability gates and argument validation.

## Linux integration

Prefer documented kernel interfaces and existing system services. Avoid competing power-policy daemons. Use D-Bus and Polkit only where the privilege boundary requires them. Version service contracts and return explicit errors/readback.

## Persistence and reliability

Use versioned configuration schemas and validate imports. Preserve user settings through upgrades and uninstall. Handle missing devices, service restart, sleep/resume, timeouts and malformed sysfs values explicitly.

## Changes that need an ADR

Create or amend an ADR before introducing a new runtime, privilege boundary, long-lived service, kernel driver dependency, public hardware protocol or architecture-level dependency.
