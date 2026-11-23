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

## 2. Repository snapshot — reconciled 2026-11-23

Repository: https://github.com/zethasaintlab/acer-control-center-linux

- **`origin/main` (`fde5b4c`):** Merges PR #1 (`docs/hydrogen-workflow-design-realignment`) on top of `c456e1c` (`v1.0.0`). Contains the public governance/design docs without the `CLV0001` driver or Stage 17–21.1 local implementation.
- **Active local branch (`feat/clv0001-keyboard-integration`):** Rebased onto `fde5b4c` at `e1a90bb`, advanced through `5ee5e9c` (Stages 17–21.1 + docs reconciliation), `fba0a0c` (Stage 22.2 static AML audit), `b854a2c` (Stages 24–25 async runtime & structural Endfield-inspired UI), and `HEAD` `e5d6fc1` (Stage 26 integration & validation), with Stage 28 consolidated closure docs in the uncommitted working tree. Pre-rebase history (`a4531b7`) is preserved on `backup/pre-hydrogen-20261123-100354`.
- **Host runtime snapshot (`7.3.3-300.fc45.x86_64`):** `acer-control-center-1.0.0-3.fc45.x86_64` remains installed on host (unchanged); `acer-control-daemon.service` is `active` (`CapabilityBoundingSet=` empty, `ReadWritePaths` scoped to `intel_pstate`, `cpufreq`, and `CLV0001:00`); `acer_clv0001_kbd` is unloaded (`CLV0001:00` unbound); `packaging/fedora/acer-control-center.spec` defines version `1.0.0`, release `4%{?dist}`.

## 3. Application architecture and current implementation status

The active `feat/clv0001-keyboard-integration` working tree uses:
- C++23 and Qt 6 / QtQuick-QML (`CMake` + Fedora RPM packaging).
- An unprivileged desktop GUI (`geteuid() == 0` guard) and a hardened `acer-control-daemon` D-Bus/Polkit service.
- Clean-room `CLV0001` (`\_SB_.DCHU`) `_DSM 0x67` kernel driver (`driver/acer_clv0001_kbd.c`) and HAL backend (`Clv0001KeyboardBackend`) supporting 17 verified presets, 5 discrete levels (`0..4`), and Stage 17/18 static 24-bit custom RGB (`supports_custom_rgb=1`).
- Stage 21.1 hardware-honesty fixes already present in the staged HAL: `findFirstFanPwmHwmonPath()` requires `isVerifiedFanDriverName("acer_ec_fans")` plus both `pwm1` and `pwm1_enable`, `probeAllCapabilities()` classifies the A715-59G fan posture as `FirmwareAutomatic` (`PNP0C09:00`), and `BAT0/cycle_count == 0` is conservatively reported as `NOT REPORTED`.
- Generic `charge_control_end_threshold` support in HAL/daemon remains disabled (`NotDetected`) on `Kelilu_RTH` because `BAT0` does not expose it and Clevo FlexiCharger AML paths remain runtime-unverified.

### Historical UI & architectural gaps from Stage 21 WIP (resolved in Stages 24–25)
- **Historical UI redesign gap (Stage 21 WIP, resolved in Stage 25):** Preliminary Stage 21 QML kept the legacy 8-tab Atelier shell without freshness/Polkit states; Stage 25 replaced it with the responsive Endfield-inspired shell and component suite (see §9 and `docs/DESIGN_SYSTEM.md`).
- **Historical ADR-001 async invariant violation (resolved in Stage 24):** Prior to Stage 24, `ControlCenterController` invoked `TelemetryCollector::collectSnapshot()` and `QDBus::Block` calls on the GUI thread; Stage 24 moved collection onto `TelemetryCoordinator` and non-blocking `QDBusPendingCallWatcher` (`AsyncControlClient`).
- **Historical eager workspace instantiation (resolved in Stage 24):** Prior to Stage 24, `Main.qml` eagerly instantiated all 8 workspace views; `Main.qml` now uses lazy single-view `workspaceLoader` instantiation.

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
- Stage 14/15 physically verified all 17 keyboard presets and levels `0..4` on the exact A715-59G (archived in `docs/HARDWARE_AUDIT_A715_59G.md` §2.4).
- Stage 17 physically verified one arbitrary static 24-bit RGB color (`#12A7E3`, level `4`, payload `A7 12 E3 F0`) via `build/driver-build-stage17/acer_clv0001_kbd.ko` (`docs/HARDWARE_AUDIT_A715_59G.md` §2.5).
- Stage 18 integrated custom static RGB into the GUI/HAL/daemon pipeline and was physically tested by the operator using `#12A7E3` at brightness level `4`; `docs/HARDWARE_AUDIT_A715_59G.md` §2.6 records the missing verbatim machine capture log as a documentation gap. `physical_readback` remains `unsupported` (`accepted_no_hw_readback`), and per-zone split is unsupported (single-zone uniform backlight).
- False-RGB temporal animation is deferred.
- On the live host (`2026-11-23`), `acer_clv0001_kbd` is unloaded (`CLV0001:00 unbound`).

### Fan
- Standard hwmon fan RPM/PWM sysfs nodes and PNP0C0B fan devices were not detected in the reported physical system.
- Stage 22 static AML found vendor-specific paths for fan mode, curve data, manual duty and RPM/thermal queries.
- Stage 22.1 clarified important side effects and asymmetries: DCHU PK0C and DCHU PK0D issue EC mailbox transactions; WMI PK0D is a direct SystemMemory read; SCMD 0x68 writes duty to four channels and has no AML watchdog/readback; 0x69 requests automatic restore.
- Static discovery does not prove EC firmware acceptance, RPM encoding, channel count or safe fallback.
- Product posture remains firmware automatic; no fan control or fan query has been runtime-authorized.

### Battery
- **BAT0 & FlexiCharger posture:** `BAT0` telemetry is available (`cycle_count=0` labeled `NOT REPORTED`); standard `charge_control_end_threshold` is absent (`NotDetected`). Stage 23A.1.3 (`GCMD 0x77`) and Stage 23A.2.2 (`PK04 0x1E`) external single-shot probes observed matching `OGNS` ACPI NVS shadow values (`BAEE >> 1 = 0x01`, `BSCP = 70`, `BTCP = 80`) and firmware `FCSC` (`40..90`) / `FCTC` (`60..100`) constant arrays (`docs/ACPI_RUNTIME_INVESTIGATION.md`). Because live EC register state (`ECMD 0xCB`) and physical charge-limit enforcement remain unverified and `PK04 0x1F` lacks threshold validation and rollback, product posture remains **battery telemetry only** (`cap.battery.limit` disabled on physical `A715-59G`).

## 5. ACPI investigation stage

### Completed
- Stage 22: DSDT and SSDT1–SSDT17 extracted and statically disassembled offline. No AML methods were invoked.
- Stage 22.1: source-line evidence and method side effects reviewed. It identified differences between WMBB and DCHU, EC mailbox writes in query paths, EC/NVS access distinctions and synchronization concerns.
- Stage 22.2: closed all five offline AML analysis items against the 18 individual `.dsl` tables (`docs/HARDWARE_AUDIT_A715_59G.md` §5.3–§5.6):
  - Corrected `BAT0._BTP` (`DSDT/DSDT.dsl:95677-95701`) from "empty stub" to an EC mailbox-backed (`ECMD 0xF2/0xC0`) ACPI battery trip-point method (unrelated to FlexiCharger), and confirmed conditional `BAT0._BIX` (`PSF0 & 0x00010000`, `UPBX` reading `EC.RAM.CYCN`).
  - Resolved `EC.ECOK` (`DSDT/DSDT.dsl:100605`) as an AML `IntObj` (`Name (ECOK, Zero)`) initialized and updated through `EC._REG (0x03, One)` -> `EREG`, clarifying `SSDT1.dsl:44`'s `FieldUnitObj` heuristic.
  - Documented the distinct synchronization mechanisms across `DCHU._DSM` (`Serialized` method entry only), `WMBB`/`PATM`, `ECMD`/`PATM`, and `acer_clv0001_kbd` `ctx->lock` (sysfs writers only).
  - Verified exact contracts and access paths for `DCHU.GCMD 0x77` (requires `Package{Buffer}` in `Arg3`; `OGNS` NVS only), `DCHU.PK04 0x1E` (`ECOK`-gated `OGNS` NVS + static `FCSC`/`FCTC`), and `DCHU.PK04 0x08` (`ECOK`-gated `OGNS` NVS, `WMI.PSTG ()` stub, conditional single-byte `SystemMemory` `EC.RAM.RINF`, and `SSDT4.PDDS` `IGDM` read), preserving the distinction between NVS shadow values, direct `SystemMemory` reads, and unverified live EC state.
  - Kept `PK07` excluded from the initial Stage 23 candidate set unless new evidence establishes a concrete need.

- **Stage 23A.1.3 (`GCMD 0x77` — Operator-Reported Runtime Result):** External single-shot probe (`stage23a1_gcmd77_probe_v3.ko`, SHA-256 `aebcbb90...`) passed 6-field exact DMI guard, symbol provenance audit (`22/22`), `_STA = 0x0F`, and `_DSM 0x77` (`AE_OK`, `Integer 0x00504601`, decoded `OGNS` NVS `BAEE>>1=1, BSCP=70, BTCP=80`); unloaded cleanly (`docs/ACPI_RUNTIME_INVESTIGATION.md` §2).
- **Stage 23A.2.2 (`PK04 0x1E` — Runtime Verified, Verbatim Log Captured):** Confirmed `SCLT`/`TCLT` are `BufferField`s (not methods); external single-shot probe (`stage23a2_pk04_1e_probe_v2.ko`, SHA-256 `09ef7e50...`) passed preflight, `_STA = 0x0F`, `_DSM 0x04/0x1E` (`AE_OK`, 256B `POPULATED_CASE_1E`: `01 46 50`, `FCSC`/`FCTC` constants, zero padding, zero ACPICA Global Lock warnings) and unloaded cleanly (`docs/ACPI_RUNTIME_INVESTIGATION.md` §3). Both Stage 23A probes verify **`OGNS` NVS shadow consistency only** — not live EC state or physical charging limits.
- **Stage 23B.1 / 23B.1.1 (`PK04 0x08` — Closed Safety-Blocked / STOP):** Classified `STATIC_READ_ONLY_BUT_RUNTIME_RACE_OR_POINTER_VALIDITY_UNRESOLVED`. Explicitly **stopped** before probe creation due to unreserved `EC.RAM.RINF` (`0xFE0B01DB`, omitted by `PNP0C02:02`/E820; supersedes Stage 22.2's `0xDC` note) and unguarded `NoLock` `IGDM`/`SANV.ASLB` reads concurrent with `i915` (`docs/ACPI_RUNTIME_INVESTIGATION.md` §5).

### Consolidated Subsystem Capability & Evidence Summary (Stage 28)

| Subsystem | Live/Repo Posture | Primary Evidence Class |
|---|---|---|
| CPU Telemetry & RAPL Power | Live via `coretemp`, `/proc`, daemon RAPL | Runtime verified (`A715-59G`) |
| CPU Performance Tuning & Profiles | Live RW via daemon + `tuned-ppd` / `intel_pstate` | Runtime verified (daemon D-Bus + readback) |
| Thermal Sensors | Live RO (`hwmon` / `thermal_zone`) | Runtime verified (`A715-59G`) |
| Fan Tachometer & PWM Override | `FirmwareAutomatic` (`NotDetected`; Curve Preview / `--mock-hardware full` only) | Safety-locked (`NotDetected`; static AML paths unverified) |
| Keyboard Backlight & Static RGB | `CLV0001` 17 presets + static 24-bit RGB (`accepted_no_hw_readback`); Preview when unbound | Physically verified (presets & Stage 17 RGB; module currently unloaded) |
| Battery Telemetry | Live RO (`BAT0`; `cycle_count=0` -> `NOT REPORTED`) | Runtime verified (`A715-59G`) |
| Battery Charge Threshold / FlexiCharger | Disabled (`NotDetected`; `--mock-hardware full` only; Stage 23A read-only NVS only) | Stage 23A `OGNS` NVS only; EC/physical unverified; setters locked |
| `PK04 0x08` System/Platform Status | Probe build/execution explicitly **STOPPED** | Safety-blocked (`STATIC_READ_ONLY_BUT_RUNTIME_RACE_OR_POINTER_VALIDITY_UNRESOLVED`) |
| NVIDIA RTX 3050 GPU | Live RO via dynamic `dlopen` NVML (fallback `/proc/driver/nvidia`) | Runtime verified (`nvidia 620.70.06` / graceful no-NVML fallback) |

### Safety Locks Retained
Do **not** build, load, or invoke `PK04 0x08`, `PK07`, `PK0C`, `DCHU.PK0D`, `GCMD 0x63/0x64/0x6E–0x71`, `SCMD 0x68/0x69/0x76/0x79`, `PK0E`, `PK04 0x1F`, or `BAT0._BTP` on physical hardware. Standalone Stage 23A `.ko` probes live strictly in the external audit workspace and are not packaged or shipped.

## 6. Deferred work

- False-RGB animation and temporal color cycling.
- Any physical fan duty, fan curve or fan mode writes.
- Any physical FlexiCharger setter.
- General-purpose or arbitrary ACPI/WMI/EC execution interface.
- DAMX driver installation or wholesale reuse.

## 7. Next actions & Release Readiness

See [docs/RELEASE_READINESS.md](RELEASE_READINESS.md) for the complete Stage 28 assessment:
1. Review and commit the Stage 28 documentation updates when ready (all prior commits through `e5d6fc1` and `backup/pre-hydrogen-20261123-100354` remain intact).
2. Operator-gated live validations outside automated CI/QA remain optional before tagging/upgrading host RPM (`1.0.0-4.fc45`): live interactive desktop Polkit dialog test, live `systemd` suspend/resume `PrepareForSleep` cycle, and optional live `acer_clv0001_kbd` reload test.
3. Keep physical fan control, battery FlexiCharger setters, and `PK04 0x08` disabled/blocked unless future hardware-safety evidence overturns the current blockers.

## 8. Confidence vocabulary

Use these labels consistently:
- **Physically verified:** observed hardware behavior on this exact laptop.
- **Runtime verified:** real interface/method was invoked and its result/side effects were checked; physical effect may still be unverified.
- **Standard interface detected:** exact Linux kernel interface exists on this running system.
- **Static AML candidate:** firmware source contains a path, but runtime behavior is not proven.
- **Preview/mock:** simulated or design-only, never physical.
- **Not detected:** no matching runtime interface found; this does not prove firmware lacks a hidden path.
- **Unsupported:** authoritative evidence establishes incompatibility or absence, not merely a missing sysfs node.

---

## 9. Stage 24–26 Implementation Snapshot (Committed at `b854a2c` / `e5d6fc1`)

- **Asynchronous runtime (Stage 24):** `TelemetryCoordinator` + `TelemetryWorker` move sysfs/proc/NVML reads off the GUI thread; `DbusControlTransport` eliminates `QDBus::Block` from GUI interactions; explicit `FreshnessState` and `ControlAuthState` states are exposed via `ControlCenterController` alongside lazy single-view `workspaceLoader` instantiation in `Main.qml`.
- **Structural UI rebuild (Stage 25):** Replaced legacy Atelier pages with a responsive Endfield-inspired 3-tier shell (`minimumWidth: 680`, `minimumHeight: 540`) and reusable components (`CommandModule`, `TelemetryMetricTile`, `TelemetryStateBanner`, `CadKeyValueRow`).
- **Integration & safety posture (Stage 26):** Native Wayland execution verified under Fedora 45 Hyprland (`7.3.3-300.fc45.x86_64`, `acer-control-daemon` system D-Bus `GetDaemonVersion`/`GetRaplPowerSnapshot`, `nvidia 620.70.06` NVML); `MODE` reconciles cleanly with live `tuned-ppd`; `CMakeLists.txt` systemd unit path and `.desktop` metadata fixed; isolated `rpmbuild -bb` (`1.0.0-4.fc45.x86_64`), staging install, `appstreamcli`, `desktop-file-validate`, `test_core_and_hal` (18/18), and `test_gui_atelier` (9/9) pass. *(Historical Stage 26 snapshot note: Stage 23A.1.3 and Stage 23A.2.2 single-shot read-only probes were later executed separately as recorded in §5, while fan PWM, FlexiCharger setters, and `PK04 0x08` remain locked.)*
