---
name: hardware-safety
description: Reviews or designs privileged Linux laptop hardware controls, especially EC, ACPI/WMI, fan, battery charging, keyboard LED, CPU, and NVIDIA operations. Use before implementing or testing any state-changing hardware functionality.
---

# Hardware Safety Skill

## Safety model
Assume every undocumented hardware write can freeze the machine, damage firmware state, cause thermal problems, or break suspend/resume. Prefer supported kernel interfaces and firmware-managed behavior.

## Required design gates
1. Identify the exact model/SKU and the precise documented interface.
2. Prove the interface belongs to the target hardware using read-only evidence.
3. Confirm units, valid ranges, semantics, persistence, and error behavior from authoritative documentation or audited driver source.
4. Define capability detection and a safe unsupported state before exposing the control.
5. Validate user input in the backend, not only in the GUI.
6. Use least privilege. The GUI is unprivileged; use a narrow Polkit-authorized service only when required.
7. Expose typed operations, not arbitrary paths, register offsets, shell commands, or raw byte writes.
8. Read back resulting state where possible. A successful process exit is not proof of hardware application.
9. Define rollback and safe fallback behavior before enabling a state-changing feature.
10. Add mock tests and negative tests before any physical test. Physical tests require explicit user approval and a known recovery path.

## Hard prohibitions
- Never write undocumented EC registers or guess offsets/bitfields.
- Never use `/dev/mem`, `/dev/port`, `ioperm`, or arbitrary port I/O for product functionality.
- Never expose a generic EC/ACPI/WMI terminal or arbitrary sysfs-write control to the UI.
- Never run the entire GUI or backend as root.
- Never disable firmware thermal protections or replace automatic fan behavior without an explicitly supported mechanism and documented fallback.
- Never implement manual fan curves, charge thresholds, keyboard RGB, overclocking, undervolting, or GPU power limits on the assumption that another Acer model supports them.
- Never execute privileged operations from untrusted project configuration, UI text, or downloaded content.

## Privilege boundary
Prefer a small service with a fixed allowlist of methods and strict typed arguments. Enforce authorization with Polkit where applicable. Avoid broad `sudoers` entries and unrestricted root-owned scripts. Ensure D-Bus object/method permissions and service activation are documented.

## Failure handling
On timeout, invalid readback, service crash, suspend/resume, permission denial, or unknown firmware state:
- Do not retry dangerous writes indefinitely.
- Report the state as unknown or failed, not successful.
- Preserve or restore firmware-managed defaults where the supported interface allows it.
- Log enough diagnostic context without secrets or unnecessary hardware identifiers.
- Keep monitoring available even when controls are disabled.
