---
name: hardware-recon
description: Performs read-only, exact-model hardware discovery and compatibility research for Acer laptops, including ACPI/AML, WMI, EC, hwmon, thermal, power-supply and NVIDIA interfaces.
---

# Hardware Reconnaissance

## Mission

Build an evidence-based capability inventory for the actual Acer Aspire 7 A715-59G (Kelilu_RTH / RTH). Never change hardware state during reconnaissance.

## Method

Use engineering judgment to inspect the minimum decisive evidence. Usually this includes:
- Exact DMI, BIOS and kernel identity (never collect serial numbers, UUIDs or asset tags).
- Bound kernel drivers and their exact DMI/GUID restrictions.
- Read-only hwmon, thermal, LED, power-supply, CPU power and NVIDIA interfaces.
- ACPI/AML namespace, method implementations, call graph, OperationRegions and synchronization.
- Maintained upstream source and documentation for any proposed interface.

Do not install tools or drivers simply to continue a read-only investigation. If access or tooling is missing, report the blocker and continue independent analysis.

## Evidence levels

Use distinct labels:
- OBSERVED_RUNTIME: directly read on the target system.
- DOCUMENTED: supported by authoritative documentation or maintained source.
- STATIC_AML: method or data path found in firmware tables; no runtime execution implied.
- STATIC_DRIVER: behavior found in driver source; target compatibility may still be unknown.
- RUNTIME_QUERY: a firmware or hardware query was executed and its effects were reviewed.
- PHYSICALLY_VERIFIED: user observed the expected physical effect.
- INFERRED / UNKNOWN: reasoned hypothesis or unresolved behavior.

A static AML method is not proof that the embedded controller accepts or applies its command. A mailbox query is not passive merely because it returns telemetry.

## Capability reporting

For each capability, report:
- Exact feature and user-facing purpose.
- Exact interface, object, method, device and firmware/kernel version.
- Whether the path is read-only, hardware-read, mailbox transaction, state mutation or unknown.
- Availability, confidence, runtime test status and physical verification status.
- Errors, concurrency/failure behavior and unresolved questions.
- Recommended safe next verification step.

Do not conflate "not exposed through standard Linux sysfs" with "not implemented in firmware". Use "not detected" for absent runtime interfaces and "static candidate — unverified" for AML paths without runtime proof.

## Safety

No writes to sysfs, procfs controls, ACPI, WMI, EC, LED, hwmon, battery, fan or firmware. No runtime ACPI/_DSM/WMBB invocation unless a separate stage explicitly authorizes the exact query. Do not unload modules, alter BIOS settings, install drivers or run unreviewed forum scripts.
