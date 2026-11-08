---
trigger: always_on
description: "Project-scoped autonomy, safety, and approval boundaries for Acer Control Center Linux development."
---

# Acer Control Center — Autonomous Workflow Policy

## Operating mode
- Use Antigravity's **Auto** permission preset for this project when the Terminal Sandbox is available.
- Do not switch to Turbo, disable the sandbox, bypass the permission engine, or request blanket access.
- This file is project guidance, not an enforcement boundary. The Antigravity permission engine and OS permissions remain authoritative.

## Work that should proceed autonomously
- Inspect and edit files inside this repository.
- Create tests, fixtures, mock hardware backends, documentation, and design assets inside the repository.
- Run read-only inspection commands and project-local lint, formatting, build, and test commands inside the sandbox.
- Research official documentation on domains explicitly allowed in the permission lists.
- Diagnose failures, revise project code, and repeat safe project-local tests.
- Use mock interfaces for hardware development until the physical interface has been verified.

## Operations requiring explicit approval
- Any command requiring `sudo`, `pkexec`, root privileges, Polkit authorization, or access outside the sandbox.
- Installing/removing system packages, changing system services, udev rules, kernel modules, boot settings, or global configuration.
- Writing to `/sys`, `/proc/sys`, `/dev`, ACPI/WMI, Embedded Controller, firmware, fan, battery, or keyboard hardware interfaces.
- Reading or writing credentials, private keys, browser profiles, tokens, or unrelated personal files.
- Destructive file operations, recursive deletion, changing permissions/ownership/capabilities, or modifying partitions/filesystems.
- Git commits, pushes, resets, cleans, rebases, merges, checkout/switch operations that could discard work, and release/tag operations.
- Opening browser pages that can perform authenticated actions, and any MCP operation not explicitly trusted.

## Hard safety invariants
1. Start hardware discovery in read-only mode. No undocumented EC register writes, `/dev/mem`, `/dev/port`, or arbitrary ACPI/WMI writes.
2. Never run the GUI or ordinary application components as root.
3. Never execute hardware writes as part of automated tests against the physical laptop. Use mocks.
4. Preserve firmware thermal protections and automatic fan fallback.
5. Never claim physical hardware support based only on mocks, another Acer model, or a similarly named SKU.
6. Do not report an operation successful until verified through a supported readback or a clear status response.
7. Never use shell tricks, encoded payloads, command substitution, `eval`, or indirect execution to bypass permission prompts.
8. If an operation is blocked or requires approval, continue independent safe work and explain exactly what is waiting and why.
9. Never broaden a permission grant merely to avoid a prompt. Request the narrowest required action and target.
10. Do not access data outside the project unless it is necessary, explicitly authorized, and limited to the required path.

## Git and user data
- Inspect `git status` and existing diffs before editing.
- Never overwrite uncommitted user changes.
- Do not rewrite history or force-push.
- Keep generated build output inside the project build directory or temporary directory.
- Do not read `.env`, private keys, tokens, or unrelated secret files.

## Completion reporting
- Distinguish implemented, mock-tested, physically tested, unverified, and unsupported behavior.
- Report commands and tests actually run. Never fabricate successful builds, tests, hardware validation, or CI results.
