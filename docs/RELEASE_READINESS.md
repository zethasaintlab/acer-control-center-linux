# Release Readiness & QA Assessment — Acer Control Center (`1.0.0-4.fc45`)

- **Assessment Date:** 2026-11-23 (Stage 28 — Engineering Closure & Release Readiness)
- **Target Branch / Commit Baseline:** `feat/clv0001-keyboard-integration` at `e5d6fc1` (with Stage 28 documentation updates)
- **Target Platform:** Acer Aspire 7 `A715-59G` (`RTH` / `Kelilu_RTH`, `INSYDE Corp. V1.17`), Fedora Linux 45 (`7.3.3-300.fc45.x86_64`), Wayland (`Hyprland` / `KDE Plasma`)
- **Application Version / Packaging Release:** `1.0.0` / `4%{?dist}`

---

## 1. Release Posture Verdict

**Verdict:** **Functionally complete for verified capabilities on the Acer Aspire A715-59G, locally buildable (`CMake`/`Ninja`), RPM-packageable (`1.0.0-4.fc45.x86_64`), and ready for external engineering/packaging review.**

It is **not** hardware-certified for unverified fan PWM/tachometer overrides or battery FlexiCharger threshold setters, both of which remain fail-closed (`NotDetected` / `FirmwareAutomatic`) by design.

---

## 2. Classification Matrix (Verified vs. Mock vs. Safety-Blocked vs. Operator-Gated)

### A. Verified & Working (Code, Automated Tests, Host Runtime & Local Packaging)

| Area | Verification Scope & Evidence | Confidence Class |
|---|---|---|
| **Core, HAL, Security & Mock Suite (`test_core_and_hal`)** | Automated `ctest` unit/mock tests (CPU/RAPL/BAT0/hwmon/NVML parsing, DMI/HID validation, sysfs canonicalization, path traversal/argument injection defense, `--mock-hardware {full,clv0001,mirror}`). | Verified (Local `ctest`) |
| **QML UI & Keyboard Navigation (`test_gui_atelier`)** | Automated Qt Quick test suite covering 3-tier shell, responsive layout (`>= 680x540`), lazy `workspaceLoader` view switching, stale/offline state banners, and keyboard focus. | Verified (Local `ctest`) |
| **Async Runtime & Non-Blocking D-Bus (Stage 24/26)** | Background `TelemetryCoordinator` + `TelemetryWorker`; non-blocking `DbusControlTransport`; `FreshnessState` & `ControlAuthState` tracking; `geteuid() == 0` GUI guard. | Runtime verified (Wayland Hyprland + daemon D-Bus `GetDaemonVersion`/`GetRaplPowerSnapshot`) |
| **Privileged Daemon & Sandbox (`acer-control-daemon.service`)** | Active on host (`systemd`); `CapabilityBoundingSet=` empty, `NoNewPrivileges=yes`, `ProtectSystem=strict`, `ReadWritePaths` scoped to `intel_pstate`, `cpufreq`, `CLV0001:00`. | Runtime verified on host (`systemd` active) |
| **Telemetry & Power Profiles** | CPU (`coretemp`, `intel_pstate`, `cpufreq`, RAPL via daemon), RAM/PSI (`/proc/meminfo`, `/proc/pressure/*`), NVMe/ACPI sensors, `BAT0` telemetry (`cycle_count=0` conservatively labeled `NOT REPORTED`), eDP `intel_backlight`, `tuned-ppd` (`net.hadess.PowerProfiles`), and dynamic `dlopen` NVML (`nvidia 620.70.06` / graceful non-NVML fallback). | Runtime verified on `A715-59G` |
| **Keyboard Backlight (`CLV0001` Static Presets + Static 24-bit RGB)** | `driver/acer_clv0001_kbd.c` cleanly exposes `clv0001_kbd_{command,state,presets}` (`supports_custom_rgb=1`); 17 presets (`0..4`) physically verified (Stage 14/15); static 24-bit RGB (`#12A7E3` level `4`) physically verified (Stage 17/18, noted gap: Stage 18 verbatim log not saved); falls back cleanly to Preview when `CLV0001:00` is unbound. | Physically verified (presets & Stage 17 RGB) / Preview when unbound |
| **Stage 23A Read-Only AML NVS Queries (External Audit Only)** | One-shot external probes `GCMD 0x77` (Stage 23A.1.3, operator-reported) and `PK04 0x1E` (Stage 23A.2.2, verbatim `journalctl -k` log) both returned `AE_OK` and matching `OGNS` NVS shadow `(BAEE>>1=1, BSCP=70, BTCP=80)` + `FCSC`/`FCTC` constants. **Not** shipped as runtime product code. | Runtime verified (NVS shadow only; live EC state unverified) |
| **Fedora 45 Packaging & AppStream Metadata** | Clean `DESTDIR` staging install (`--prefix /usr`), `desktop-file-validate`, `appstreamcli validate --no-net`, isolated `rpmbuild -ba` (`1.0.0-4.fc45`) generating `x86_64` main, `debuginfo`, `debugsource`, and `.src.rpm` packages without touching the host RPM database. `rpmlint` shows no runtime/layout blockers (see note below). | Verified locally (isolated build/staging) |

*`rpmlint` triage (`acer-control-center-1.0.0-4.fc45.x86_64.rpm`, `.src.rpm`, `.spec`):* `no-manual-page-for-binary acer-control-center` is advisory for a GUI desktop app (optional future CLI man page improvement); `invalid-url Source0` is an intentional local-tarball choice until a tagged upstream release tarball exists; `devel-file-in-non-devel-package /usr/share/acer-control-center/driver/acer_clv0001_kbd.c` is intentional packaging of optional out-of-tree DKMS kernel module source (not `-devel` library headers); `%description` `spelling-error` alerts on `sys`/`leds`/`sysfs`/`sRGB` are technical-term false positives.

### B. Mock-Only Capabilities (Explicitly Labeled Non-Physical on `A715-59G`)

- **Fan PWM Duty & Interactive Curve Writing:** Only bound under `--mock-hardware full` (`acer_ec_fans` mock hwmon device with `pwm1` / `pwm1_enable`) or preview mode (`FirmwareAutomatic` on physical `A715-59G`).
- **Battery Charge End Threshold Slider (`charge_control_end_threshold`):** Only active under `--mock-hardware full` where a mock `BAT0/charge_control_end_threshold` node is synthesized. Disabled with explanation on physical `Kelilu_RTH`.
- **Keyboard Lighting Simulation (`clv0001` / `full` mock modes):** Software sysfs simulation in `--mock-hardware` modes; Studio Preview mode when hardware node is unbound.

### C. Intentionally Retained Physical Hardware Safety Limits

- **No physical fan override:** No standard `hwmon` fan RPM/PWM sysfs interface exists; vendor `SCMD 0x68/0x69/0x79` and `PK0C`/`PK0D`/`PK0E` paths have unverified EC firmware semantics, 4-channel packing without AML watchdog, or mailbox side effects (`docs/HARDWARE_AUDIT_A715_59G.md` §5.2).
- **No physical battery FlexiCharger control:** `PK04 0x1F` / `SCMD 0x76` / `ECMD 0xCB` write `OGNS` NVS independently of EC command completion and lack threshold validation and rollback (`docs/HARDWARE_AUDIT_A715_59G.md` §5.3).
- **No keyboard hardware readback or false-RGB temporal animation:** `_DSM 0x67` is write-only (`accepted_no_hw_readback`); per-zone split and software color-cycling timers remain excluded.

### D. Safety-Blocked Investigations

- **`DCHU.PK04 0x08` (`STATIC_READ_ONLY_BUT_RUNTIME_RACE_OR_POINTER_VALIDITY_UNRESOLVED`):** Explicitly stopped prior to probe construction due to unreserved `EC.RAM.RINF` `SystemMemory` (`0xFE0B01DB`) outside `PNP0C02:02`/`E820` and unguarded `NoLock` `IGDM`/`SANV.ASLB` reads concurrent with `i915` (`docs/ACPI_RUNTIME_INVESTIGATION.md` §5).
- **`DCHU.PK07` (`BMD0`):** Excluded (no other AML readers/writers).

### E. Operator-Gated Live Checks (Require Explicit Operator Action Outside Unattended QA)

| Gated Item | Current State | Why Gated |
|---|---|---|
| **Host RPM Upgrade (`1.0.0-3` -> `1.0.0-4.fc45`)** | Host still runs `acer-control-center-1.0.0-3.fc45.x86_64`. | Installing/upgrading RPMs mutates host system state (`dnf`/`rpm`). |
| **Live `acer_clv0001_kbd` Reload Post-Stage 26** | `acer_clv0001_kbd` is unloaded (`CLV0001:00` unbound). | Kernel module `insmod`/DKMS build on live host requires operator action. |
| **Live Interactive Desktop Polkit Prompt (`auth_admin_keep`)** | Verified via unit/D-Bus error paths and daemon start; interactive password dialog not clicked autonomously. | Requires live desktop authentication dialog interaction by human operator. |
| **Live `PrepareForSleep` Suspend/Resume Cycle** | Implemented and unit-verified (`TelemetryCoordinator` pause/resume wiring). | Triggering host suspend/resume mutates host power state. |

---

## 3. Recommended Pre-Tag Checklist for Maintainer

1. Review Stage 28 documentation updates (`git status`, `git diff`) and commit when satisfied (keeping `backup/pre-hydrogen-20261123-100354` intact until remote branch policy is settled).
2. Optionally perform operator-gated live host checks: `sudo rpm -Uvh ...1.0.0-4.fc45.x86_64.rpm`, load `acer_clv0001_kbd`, test one interactive Polkit-authorized `intel_pstate` profile change and one `systemd` suspend/resume cycle under Wayland.
3. Preserve external diagnostic probes (`/home/zeth/acpi-audit-a715-59g-WcgNEm/`) outside the source repository; do not ship or invoke `PK04 0x08` or unverified `ECMD` setters.
