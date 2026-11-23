---
trigger: always_on
description: "Outcome-oriented autonomy and approval boundaries for Acer Control Center engineering."
---

# Autonomous Engineering Workflow

## Operating principle
Own the engineering outcome, not a prescribed command sequence. Understand the task, inspect the relevant source of truth, choose an efficient approach, and use appropriate tools. Do not wait for the user to specify routine investigation steps or implementation details.

Prefer a short, direct follow-up for incremental work. Use a fuller engineering brief for a new stage, major architecture/design work, or safety-critical operation. Avoid repeating instructions already established in AGENTS.md, project docs or the current task.

## Proceed autonomously
- Inspect repository files, Git state, history and existing architecture.
- Investigate relevant upstream documentation and source.
- Edit repository code, tests, documentation and design assets within the authorized task.
- Run project-local, non-destructive build, test, formatting and analysis commands.
- Use mocks and fixtures for hardware development.
- Make reasonable implementation decisions and explain material trade-offs in the completion report.

## Approval remains required
- Any root, sudo, pkexec, Polkit-authorized or outside-sandbox operation.
- Installing/removing system packages or changing services, udev, kernel modules, boot configuration or global system settings.
- Any write to hardware, firmware, ACPI, WMI, EC, fan, battery, keyboard LED, CPU/GPU control or other physical device.
- Any runtime ACPI/WMI/_DSM/WMBB/EC method execution not separately authorized.
- Destructive operations, permission/ownership changes, partition/filesystem operations, credential access, or unrelated personal-data access.
- Git commits, pushes, merges, rebases, resets, cleans, branch changes that could discard work, releases and tags.
- Changes that broaden project permission rules or weaken system security.

## Hardware and evidence invariants
1. Read-only discovery is the default.
2. Separate AML static evidence, kernel interface discovery, runtime readback and physical verification.
3. A query that writes an EC mailbox is not a passive read, even if it returns telemetry.
4. A method or interface discovered on a related laptop is not proof of A715-59G compatibility.
5. Unsupported or unverified controls must fail closed and explain the reason.
6. Keep automatic firmware thermal control intact unless a narrowly verified interface and fallback are established.
7. Automated tests must never write to physical hardware.

## Git and workspace integrity
Inspect status and diffs before editing. Preserve uncommitted work. Do not reset, force-push, rewrite history or overwrite user changes. Keep unrelated files untouched. A requested repository edit does not imply permission to mutate system state.

## Completion
State the outcome, important design/engineering decisions, files changed, verification actually performed, remaining risks and recommended next step. Distinguish implemented, mock-tested, runtime-verified and physically verified behavior.
