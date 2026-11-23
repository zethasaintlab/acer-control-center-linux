---
name: hardware-safety
description: Reviews or designs privileged Linux laptop hardware controls, especially ACPI/AML, WMI, EC, fan, battery charging, keyboard lighting, CPU and NVIDIA operations.
---

# Hardware Safety

## Safety model

Assume undocumented firmware operations can hang the EC, leave a controller in an unexpected mode, interfere with other drivers, break suspend/resume or compromise thermal/battery protections. Prefer standard kernel interfaces or a narrow, independently verified model-specific driver.

## Evidence required before runtime use

For any model-specific ACPI/WMI/EC path:
1. Identify the exact DMI, board and BIOS.
2. Inspect the full static AML or driver call graph.
3. Classify every read, mailbox write, OperationRegion write, SMI, notification and state mutation.
4. Verify argument encoding, return type, input bounds and error handling.
5. Account for ACPI method serialization, ACPI mutexes, driver-local locks, shared EC mailboxes and interleaving with existing keyboard operations.
6. Establish exact Linux caller ownership and a maintainable kernel/userspace boundary.
7. Define a safe failure response and recovery plan.

Static method presence is not runtime proof. A query using ECMD/FCMD/FDAT or another command mailbox is a hardware transaction, not a passive read. A zero-write method may still read hardware-backed regions.

## Required control design

- Identify the exact supported interface and driver.
- Detect capabilities before exposing controls.
- Validate typed arguments in the backend.
- Never expose raw EC offsets, arbitrary ACPI method IDs, WMI command IDs, shell commands or unrestricted sysfs paths.
- Verify resulting state where a trustworthy readback exists.
- Define rollback and firmware-automatic fallback before enabling a state-changing fan feature.
- Battery threshold setters must validate supported discrete values, threshold ordering, enable state and failure/rollback semantics.
- Keep read-only monitoring independently available when controls are not supported.

## Privilege boundary

Keep GUI and normal application components unprivileged. Prefer the smallest kernel interface and a typed, allowlisted D-Bus/Polkit service. Do not give userspace arbitrary firmware invocation or raw mailbox access.

## Hard prohibitions

Never guess or write undocumented EC registers. Never use /dev/mem, /dev/port, ioperm or arbitrary port I/O as a product feature. Never use another Acer model's WMI GUID or driver quirk as proof of compatibility. Never run physical hardware writes in automated tests. Physical validation requires separate explicit user authorization for each operation class and a known recovery path.

## Failure

On timeout, invalid readback, permission denial, device disappearance, suspend/resume or unknown firmware state, report failure/unknown rather than success. Do not retry dangerous writes indefinitely. Preserve firmware automatic operation whenever a verified interface provides a safe restore path.
