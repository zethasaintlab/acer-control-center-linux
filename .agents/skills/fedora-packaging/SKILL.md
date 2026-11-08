---
name: fedora-packaging
description: Builds, packages, validates, and documents desktop applications for Fedora Linux 45 using RPM, desktop files, D-Bus, Polkit, systemd, and Fedora-native dependencies. Use for installation, release, packaging, and Fedora integration work.
---

# Fedora 45 Packaging Skill

## Baseline
Fedora Linux 45 is the primary target. Verify the actual Fedora release, current Fedora 45 package names, repository availability, policy, and dependency versions before writing a spec or installation script.

## Required approach
1. Identify the selected language/toolkit and runtime requirements from the approved architecture decision.
2. Prefer Fedora-packaged dependencies when practical. Do not add COPRs, third-party repositories, or external package sources without explicit approval and a documented reason.
3. Produce a reproducible RPM build and a clear spec file. Follow current Fedora packaging guidelines.
4. Keep build-time and runtime dependencies separate and minimal.
5. Use proper desktop entry, icon, metainfo/appdata where applicable, MIME integration only if needed, and uninstall-safe file locations.
6. Keep user settings in user-owned standard configuration/data locations. Do not delete user data on package removal.
7. Install system services and Polkit policies only when justified. Ensure service permissions are least-privilege and avoid broad sudoers rules.
8. Do not install development dependencies or alter the user's default shell/system configuration as part of the application package.
9. Test install, upgrade, downgrade if supported, removal, and clean reinstall in an isolated Fedora 45 environment where possible.
10. Document exactly what requires root and why.

## Validation
- Build the RPM from a clean source tree.
- Inspect RPM metadata and dependency closure.
- Run package linting tools available for the target environment.
- Verify installed file paths and permissions.
- Verify desktop entry launch and icon resolution.
- Verify service activation, authorization, logs, and removal behavior if applicable.
- Test under KDE Plasma Wayland and Hyprland Wayland.
- Do not claim Fedora 45 compatibility based only on a Fedora 44 build.

## Safety
Never use `curl | sudo bash`, unpinned remote installers, or scripts that modify repositories/system services without review. Do not use `dnf remove` or broad package replacement as a cleanup shortcut. Explain package transactions before executing them.
