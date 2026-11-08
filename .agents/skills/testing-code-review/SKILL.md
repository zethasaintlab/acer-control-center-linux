---
name: testing-code-review
description: Plans tests and reviews code for correctness, regression risk, security, maintainability, accessibility, and hardware edge cases in the Acer Linux control-center project. Use before merging or declaring a feature complete.
---

# Testing and Code Review Skill

## Review order
1. Compare implementation against the approved requirements and capability matrix.
2. Check correctness, error handling, boundary values, stale/missing telemetry, timeouts, cancellation, and suspend/resume.
3. Check privilege boundaries, user-controlled inputs, path handling, process execution, D-Bus/Polkit permissions, and secret leakage.
4. Check that unsupported hardware states are represented honestly and do not enable unsafe controls.
5. Check UI state consistency, accessibility, keyboard navigation, contrast, loading/error/empty states, and responsive layouts.
6. Check performance: polling frequency, thread usage, memory growth, unnecessary process spawning, and UI responsiveness.
7. Check configuration migrations, corrupted settings, profile import/export validation, and backwards compatibility.
8. Check packaging paths, file permissions, service lifecycle, upgrades, and uninstall behavior.

## Test strategy
- Unit-test pure domain logic without hardware.
- Mock sysfs, hwmon, power supply, WMI, D-Bus, and NVIDIA adapters.
- Test invalid and missing files, permission denial, device disappearance, timeouts, malformed values, and unsupported models.
- Test dangerous control adapters only with mocks or dedicated safe fixtures.
- Do not automate EC writes or dangerous hardware operations against the physical laptop.
- Add GUI tests for keyboard navigation, focus, accessible labels, loading, unsupported, and error states.
- Record commands and exact results. Separate tests run locally, in CI, in containers/VMs, and on physical hardware.

## Review reporting
Report findings by severity: Blocker, High, Medium, Low, Informational. Each finding must include file/line, impact, reproducibility/evidence, and a concrete remediation. Do not report hypothetical style preferences as bugs. State what was not tested.
