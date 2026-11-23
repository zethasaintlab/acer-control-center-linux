# Release Readiness & QA Assessment — Acer Control Center (`1.0.0-7.fc45`)

- **Assessment Date:** 2026-11-23 (Stage 32 final release closure; initial Stage 28 closure at `e5d6fc1`)
- **Committed Baseline:** `feat/clv0001-keyboard-integration` at `74a34f7` (Stage 29 documentation/spec checkpoint)
- **Host-Integrated Package:** `acer-control-center-1.0.0-6.fc45.x86_64` (Stage 30.3 candidate installed via user DNF5 transaction `#66` and validated in Stage 31.1; see [docs/HOST_INTEGRATION_A715_59G.md](HOST_INTEGRATION_A715_59G.md))
- **Uncommitted Local Closure Overlays:** Stage 30.2 license compliance (`COPYING.GPL-2.0`, composite SPDX `License:` + `%license`), Stage 30.3 reconciliation, and Stage 32 closure (`docs/HOST_INTEGRATION_A715_59G.md`, `Release: 7%{?dist}`)
- **Target Platform:** Acer Aspire 7 `A715-59G` (`RTH` / `Kelilu_RTH`, `INSYDE Corp. V1.17`), Fedora Linux 45 (`7.3.3-300.fc45.x86_64`), Wayland (`Hyprland` / `KDE Plasma`)
- **Application Version / Packaging Release:** `1.0.0` / `7%{?dist}`

---

## 1. Release Posture Verdict

**Verdict:** **Locally built, packaging-verified (`1.0.0-7.fc45.x86_64`), and host-integrated on the target Acer Aspire A715-59G (`1.0.0-6.fc45.x86_64` verified in Stage 31.1). Ready for repository review and Stage 32 commit authorization.**

The package remains an **unsigned, locally built, unpublished** RPM candidate (not a signed Fedora/Copr release) and is **not** hardware-certified for unverified fan PWM/tachometer overrides or battery FlexiCharger threshold setters, both of which remain fail-closed (`NotDetected` / `FirmwareAutomatic`) by design. The `1.0.0-7.fc45` candidate refreshes `%doc` only and carries no binary/code changes relative to host-verified `1.0.0-6.fc45`.

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
| **Fedora 45 Packaging, License & Host Integration** | Stages 30–30.3 verified clean `git archive` (`74a34f7`), composite SPDX `License:`, canonical `COPYING.GPL-2.0` (`edaef632...`), `CTest 2/2`, desktop/AppStream validation, and zero `rpmlint` license errors across `1.0.0-4..6`. Stage 31.1 verified user-installed `1.0.0-6.fc45` on host (`rpm -V`, `systemd`, D-Bus, Polkit XML/`pkaction`, Wayland GUI, 8-view navigation, `settings.json` intact). Stage 32 adds `HOST_INTEGRATION_A715_59G.md` under `%doc` (`1.0.0-7.fc45`). | Host-integrated (`1.0.0-6.fc45`, unsigned/unpublished); `1.0.0-7.fc45` locally verified |

*Release-engineering & host-integration progression:*
1. **Stage 29 (`74a34f7`):** Committed Stage 28/28.1 documentation and `1.0.0-4` spec checkpoint.
2. **Stages 30–30.3 (`build/release-stage30/` .. `build/release-stage30-3/`):** Clean `git archive` rebuild (`1.0.0-4`), composite SPDX license audit, canonical FSF `COPYING.GPL-2.0` under `%license` (`1.0.0-5`), and documentation reconciliation (`1.0.0-6`).
3. **Stage 31 Preflight & Stage 31.1 Host Integration (`acer-control-center-1.0.0-6.fc45.x86_64`):** User DNF5 transaction `#66` (`2026-11-23 20:38:48–20:38:50 WIB`) upgraded `1.0.0-3` -> `1.0.0-6`; Stage 31.1 verified `rpm -V`, `acer-control-daemon.service` restart/enablement, D-Bus `GetDaemonVersion` (`1.0.0-fc45`) & `GetRaplPowerSnapshot`, Polkit policy/actions, Wayland GUI launch/navigation, real-host telemetry, and untouched `settings.json` (`docs/HOST_INTEGRATION_A715_59G.md`).
4. **Stage 32 (`build/release-stage32/`):** Added `docs/HOST_INTEGRATION_A715_59G.md`, reconciled project docs, and built final `1.0.0-7.fc45` documentation-refreshed candidate (`8` `%doc` files).

*`rpmlint` triage (verified on `1.0.0-4.fc45` and `1.0.0-5.fc45`):* zero license diagnostics; `no-manual-page-for-binary acer-control-center` is advisory for a GUI desktop app; `invalid-url Source0` is an intentional local-tarball choice until a tagged upstream release tarball exists; `devel-file-in-non-devel-package /usr/share/acer-control-center/driver/acer_clv0001_kbd.c` is intentional packaging of optional out-of-tree DKMS kernel module source (not `-devel` library headers); `%description` `spelling-error` alerts on `sys`/`leds`/`sysfs`/`sRGB` are technical-term false positives.

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
| **Host Package State (`1.0.0-6.fc45` installed; `1.0.0-7.fc45` built locally)** | Host runs verified `1.0.0-6.fc45` (upgraded from `1.0.0-3` via user DNF5 transaction `#66`); `1.0.0-7.fc45` adds `%doc HOST_INTEGRATION_A715_59G.md` only and is not installed. | Unnecessary host transaction avoided since binaries/runtime are identical to `1.0.0-6.fc45`. |
| **Live `acer_clv0001_kbd` Reload Post-Stage 26** | `acer_clv0001_kbd` is unloaded (`CLV0001:00` unbound; UI in Preview Mode). | Kernel module `insmod`/DKMS build on live host requires operator action. |
| **Live Interactive Desktop Polkit Prompt (`auth_admin_keep`)** | XML and `pkaction` defaults verified; interactive password dialog not clicked autonomously. | Requires live desktop authentication dialog interaction by human operator. |
| **Live `PrepareForSleep` Suspend/Resume Cycle** | Implemented and unit-verified (`TelemetryCoordinator` pause/resume wiring). | Triggering host suspend/resume mutates host power state. |

---

## 3. Recommended Pre-Commit / Maintainer Checklist

1. Review Stage 32 working-tree changes (`COPYING.GPL-2.0`, `docs/HOST_INTEGRATION_A715_59G.md`, `docs/PROJECT_STATE.md`, `docs/RELEASE_READINESS.md`, `packaging/fedora/acer-control-center.spec`) and authorize the single local commit (`docs(release): close host integration and release readiness`).
2. Keep `backup/pre-hydrogen-20261123-100354`, `stash@{0}`, and `stash@{1}` intact until remote publication policy is decided.
3. Preserve external diagnostic probes in the private external ACPI audit workspace outside the source repository; do not ship or invoke `PK04 0x08` or unverified `ECMD` setters.
