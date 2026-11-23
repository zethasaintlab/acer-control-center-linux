# Project State & Hydrogen Handoff

**Last reconciled:** 2026-11-23  
**Purpose:** One concise current-state index for the next engineering session.  
**Important:** This document deliberately distinguishes the public GitHub snapshot from the more recent local engineering results described in the current project handoff.

## 1. Product direction

Acer Control Center is a native Linux hardware command center for the Acer Aspire 7 A715-59G (Kelilu_RTH / RTH), primarily targeting Fedora 45, KDE Plasma and Hyprland on Wayland.

Current design direction:
- Industrial-technical desktop experience inspired by the project's Endfield Design System research.
- Structural redesign: information architecture, navigation, screen composition, components and interactions must change where necessary; a color-only reskin is not acceptable.
- DAMX is a functional reference for workspace responsibilities, not a visual target, driver dependency or hardware compatibility source.
- Preserve the Qt 6 / QML / C++23 architecture approved in ADR-001 unless a concrete reason and new ADR justify changing it.

## 2. Public repository snapshot — verified during this audit

Repository: https://github.com/zethasaintlab/acer-control-center-linux

At the time of this audit:
- Default branch: main
- Main HEAD: c456e1c99d7c6174804fcbb7ec47a7c3ecd43092
- Main commit date: 2026-11-14
- Main commit message: merge of release/v1.0.0-final-hardening
- Public branches observed: main, release/v1.0.0-final-hardening, feature/clevo-keyboard-backlight.
- feature/clevo-keyboard-backlight is a Stage 12-era branch whose latest observed commit is 9904fabc3be01804b5221260767b8b32ce047edd (2026-11-14). It is not the later Stage 17/18 implementation described in recent local engineering reports.

The public main tree contains 99 tracked files and the Qt/QML application, C++ HAL, D-Bus/Polkit service, tests, packaging, agent rules and docs. It does not contain the later driver/acer_clv0001_kbd.c path referenced by the recent local Stage 17/18 handoff.

Recent local engineering context reports a branch named feat/clv0001-keyboard-integration at commit a4531b711a2c626832d4d212b8cf219d6b302f16 and a physically verified Stage 18 custom-RGB integration. That branch/commit was not present among the public GitHub branches inspected in this audit. Treat this as a local-workspace handoff claim until the next session inspects the actual checkout and reconciles its Git history. Do not overwrite or reset that local work to match public main.

## 3. Application architecture and current public-main implementation

The public main snapshot uses:
- C++23 and Qt 6 / QtQuick-QML.
- CMake and Fedora RPM packaging.
- An unprivileged desktop GUI.
- A D-Bus daemon with narrow Polkit actions for privileged controls.
- Hardware discovery, telemetry, control operations, configuration, diagnostics and mock sysfs roots.
- Eight QML workspaces in Main.qml: Overview, Performance, Thermal & Fans, Keyboard Studio, Battery & Power, NVIDIA GPU, Diagnostics and Settings.

The current public-main visual implementation is the older Precision Hardware Atelier layout: top status bar, left navigation rail, bottom status strip and workspace pages composed from instrument panels/grids. It is the historical baseline, not the intended final Endfield-inspired design.

A public-main backend review identified a fan capability detection gap: findFirstFanPwmHwmonPath() accepts either fan1_input OR pwm1, and probeAllCapabilities() promotes any matching path to FullySupported. The project safety contract requires a known verified driver and a complete actuator interface (including the required enable semantics) before exposing control. This needs a focused implementation review before any fan control can be enabled.

The generic charge threshold implementation currently targets the standard BAT0 charge_control_end_threshold sysfs attribute. It does not implement the newly discovered Clevo FlexiCharger AML protocol.

## 4. Hardware facts and capability posture

Target:
- Acer Aspire A715-59G / board RTH / Kelilu_RTH.
- INSYDE BIOS V1.17 dated 2026-06-30.
- Recent local hardware reports used Fedora 45 kernel 7.3.3-300.fc45.x86_64.
- ACPI EC PNP0C09:00.
- CLV0001 / DCHU and CLV0002 / DCHP.
- Clevo-family WMBB method GUID ABBC0F6D-8EA1-11D1-00A0-C90629100000.
- DCHU _DSM UUID 93f224e4-fbdc-4bbf-add6-db71bdc0afad.

### Keyboard
- Stage 15 physically verified all 17 keyboard presets on the exact A715-59G.
- Stage 17 physically verified one arbitrary static RGB color through the custom kernel interface.
- Stage 18 physically verified end-to-end custom static RGB from the GUI on the local engineering workspace.
- False-RGB temporal animation is deferred.
- The latest local implementation and tests must be reconciled with the actual checkout before Hydrogen changes or advertises these paths.

### Fan
- Standard hwmon fan RPM/PWM sysfs nodes and PNP0C0B fan devices were not detected in the reported physical system.
- Stage 22 static AML found vendor-specific paths for fan mode, curve data, manual duty and RPM/thermal queries.
- Stage 22.1 clarified important side effects and asymmetries: DCHU PK0C and DCHU PK0D issue EC mailbox transactions; WMI PK0D is a direct SystemMemory read; SCMD 0x68 writes duty to four channels and has no AML watchdog/readback; 0x69 requests automatic restore.
- Static discovery does not prove EC firmware acceptance, RPM encoding, channel count or safe fallback.
- Product posture remains firmware automatic; no fan control or fan query has been runtime-authorized.

### Battery
- BAT0 telemetry is available; standard charge_control_end_threshold is not exposed.
- Stage 22 static AML found a Clevo FlexiCharger getter/setter path, discrete start/stop preset arrays and ECMD 0xCB transport.
- Stage 22.1 found that the getter reads GNVS/OGNS NVS shadow values rather than live EC readback; PK04 0x1F writes NVS before attempting ECMD and lacks threshold ordering/range validation and rollback.
- The static preset arrays are start 40, 50, 60, 70, 80, 90 percent and stop 60, 70, 80, 90, 100 percent. They do not prove live EC acceptance.
- Product posture remains battery telemetry only; no charge-limit firmware call is authorized.

## 5. ACPI investigation stage

### Completed
- Stage 22: DSDT and SSDT1–SSDT17 extracted and statically disassembled offline. No AML methods were invoked.
- Stage 22.1: source-line evidence and method side effects reviewed. It identified differences between WMBB and DCHU, EC mailbox writes in query paths, EC/NVS access distinctions and synchronization concerns.

### Pending
- Stage 22.2: correct the ACPI 6.6 battery-method inventory, resolve the exact ECOK object/access type, verify Serialized method semantics and finalize the exact Tier-1 query contracts.
- Stage 23: only after Stage 22.2 closure and separate explicit authorization, consider the narrowly scoped zero-mailbox DCHU queries. The candidates are DCHU GCMD 0x77, DCHU PK04 0x1E and DCHU PK04 0x08 (optionally PK07 if justified).
- Stage 23 is not authorized by this handoff.

Do not invoke PK0C, DCHU PK0D, GCMD 0x63/0x64/0x6E, SCMD 0x68/0x69/0x76/0x79, PK0E or PK04 0x1F on physical hardware. Do not call WMI, DCHU _DSM, WMBB or EC methods under the previous static-only authorization.

## 6. Deferred work

- False-RGB animation and temporal color cycling.
- Any physical fan duty, fan curve or fan mode writes.
- Any physical FlexiCharger setter.
- General-purpose or arbitrary ACPI/WMI/EC execution interface.
- DAMX driver installation or wholesale reuse.

## 7. Immediate next actions

1. Hydrogen must inspect the active local checkout, branch, HEAD, uncommitted changes and the available local Stage 17/18 artifacts before reconciling with public GitHub.
2. Complete Stage 22.2 only within its static-only scope. Do not broaden the prior authorization.
3. After evidence closure, request separate operator authorization before any Stage 23 runtime query.
4. Begin UI redesign from information architecture and task workflows, using docs/DESIGN_SYSTEM.md. Do not begin with palette replacement.
5. Review fan capability detection and existing generic charge-limit controls against the evidence before changing any hardware-facing implementation.

## 8. Confidence vocabulary

Use these labels consistently:
- **Physically verified:** observed hardware behavior on this exact laptop.
- **Runtime verified:** real interface/method was invoked and its result/side effects were checked; physical effect may still be unverified.
- **Standard interface detected:** exact Linux kernel interface exists on this running system.
- **Static AML candidate:** firmware source contains a path, but runtime behavior is not proven.
- **Preview/mock:** simulated or design-only, never physical.
- **Not detected:** no matching runtime interface found; this does not prove firmware lacks a hidden path.
- **Unsupported:** authoritative evidence establishes incompatibility or absence, not merely a missing sysfs node.
