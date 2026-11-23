# Architecture Decision Record & Security Specification

**Document ID:** ADR-001  
**Date:** 2026-11-08  
**Status:** Approved  
**Project:** Acer Control Center — Linux Edition (`GPL-3.0-or-later`)

---

## 1. Technology Selection (ADR-001)

### Decision
Build **Acer Control Center — Linux Edition** using:
- **Language:** Modern **C++23** (`-std=c++23`, `-Wall -Wextra -Wpedantic`)
- **GUI Framework:** **Qt 6.11 (`QtQuick` / `QML`, `QtQuickControls2`)** with custom C++ `QQuickPaintedItem` precision visual instruments
- **IPC Boundary:** **QtDBus (`Qt6::DBus`)** over the Linux System Bus (with session-bus / local mock support for automated testing)
- **Authorization:** **Polkit (`polkit-gobject-1`)** inside the privileged service binary (`acer-control-daemon`)
- **NVIDIA GPU Integration:** Optional runtime dynamic loading of `libnvidia-ml.so.1` via `dlopen()` / `dlsym()` using clean-room ABI declarations
- **Build System & Packaging:** **CMake 4.3 + Ninja** and native **Fedora 45 RPM (`rpmbuild`)**

### Rationale & Alternatives Evaluated
1. **Qt 6.11 / QML (`C++23`) — Selected:**
   - Native Wayland rendering (`qt6-qtwayland`) tested across both **KDE Plasma 6** and **Hyprland** at 144Hz.
   - All required development headers (`qt6-qtbase-devel`, `qt6-qtdeclarative-devel`, `qt6-qtshadertools-devel`, `polkit-devel`, `dbus-devel`, `cli11-devel`, `sqlite-devel`) are native Fedora 45 packages already installed in the build environment.
   - Enables bespoke non-template hardware instruments (`TelemetryStripChart`, `HsvColorWheel`, `FanCurveEditor`, `KeyboardLayoutVisualizer`) without webview bloat.
2. **GTK 4 / Libadwaita (`C++` or `Rust`) — Rejected:**
   - Libadwaita's opinionated GNOME styling conflicts with KDE Plasma 6 and the **Precision Hardware Atelier** visual identity.
   - Rust `gtk4-rs` / `relm4` requires external Cargo crate vendoring not present in the offline Fedora 45 build root.
3. **Rust + Dioxus / Electron / Tauri — Rejected:**
   - WebView-based UIs violate our native desktop requirement, increase memory footprint, and often suffer from hybrid-GPU compositing artifacts under Wayland.

---

## 2. License & Third-Party Dependency Audit

**Project License:** `GPL-3.0-or-later` (with GNU GPL v3 Section 7 Additional Permission for optional runtime `dlopen` of system graphics driver libraries).

| Dependency | Fedora 45 Package | Upstream License | Compatibility with `GPL-3.0-or-later` |
|---|---|---|---|
| Qt 6.11 (`Core`, `Gui`, `Quick`, `QuickControls2`, `DBus`, `Sql`, `Test`, `QuickTest`) | `qt6-qtbase-devel`, `qt6-qtdeclarative-devel` | `LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only` | **Compatible** |
| Polkit (`polkit-gobject-1`) | `polkit-devel` | `LGPL-2.0-or-later` | **Compatible** |
| D-Bus (`libdbus-1`) | `dbus-devel` | `AFL-2.1 OR GPL-2.0-or-later` | **Compatible** |
| CLI11 | `cli11-devel` | `BSD-3-Clause` | **Compatible** |
| SQLite 3 | `sqlite-devel` | `Public Domain` | **Compatible** |
| NVIDIA Management Library (`libnvidia-ml.so.1`) | `xorg-x11-drv-nvidia-cuda-libs` (Optional runtime `dlopen` only) | Proprietary system driver library | **Compatible:** Not linked at build time; zero NVIDIA headers distributed; loaded optionally at runtime via `dlopen` under explicit GPLv3 Section 7 permission. |

**Graceful Degradation Guarantee:** If `libnvidia-ml.so.1` is absent, unreadable, or fails `nvmlInit_v2()`, `NvmlGpuAdapter` catches the condition cleanly during capability discovery, marks `cap.gpu.nvidia` as `NotDetected` (or falls back to `/sys/bus/pci/devices/0000:01:00.0` metadata), and never prevents the application from launching.

---

## 3. Eight-Layer Modular Architecture

```text
+-----------------------------------------------------------------------------------+
| 1. Presentation Layer (QML Screens + C++ QQuickPaintedItem Instruments)           |
+-----------------------------------------------------------------------------------+
| 2. Application / ViewModel Layer (QObjects, ThemeEngine, SystemTray, Login1)      |
+-----------------------------------------------------------------------------------+
| 3. Settings & Profile Persistence (~/.config/acer-control-center/, Schema v1)     |
+-----------------------------------------------------------------------------------+
| 4. Diagnostics & Redacted Logging (Ring Buffer + Redacted Markdown/JSON Exporter) |
+-----------------------------------------------------------------------------------+
| 5. Hardware Capability Discovery (DMI, WMI, hwmon, BAT0, leds, pstate, NVML)      |
+---------------------------------+-------------------------------------------------+
| 6. Read-Only Telemetry Adapters | 7. State-Changing Control Adapters              |
|    (/proc, hwmon, BAT0, NVML)   |    (Tuned/PPD, Login1, Daemon D-Bus Client)     |
+---------------------------------+-------------------------------------------------+
                                  | D-Bus IPC (io.github.zethasaintlab.AcerControl1)
+---------------------------------v-------------------------------------------------+
| 8. Privileged Service Boundary (acer-control-daemon + Polkit + Readback Verifier) |
+-----------------------------------------------------------------------------------+
```

### Strict Architectural Invariants
1. **Unprivileged GUI:** `acer-control-center` refuses to run if `geteuid() == 0` (unless `--allow-root-test` is explicitly set in isolated container tests), preventing accidental root GUI execution.
2. **Asynchronous Polling:** All telemetry collection executes on a dedicated background worker (`TelemetryCoordinator`) with configurable intervals (`500 ms`, `1000 ms`, `2000 ms`, `5000 ms`) and pauses automatically upon receiving `org.freedesktop.login1.Manager.PrepareForSleep(true)`.
3. **Mock Hardware Injection (`ACC_SYSFS_ROOT`):** Every path lookup in discovery, telemetry, and control adapters resolves through `SysfsRootResolver`, allowing unit tests, integration tests, and GUI tests to run against deterministic mock hardware trees (`A715-59G Physical Mirror`, `Full RGB + PWM + ChargeLimit Mock`, and `Fault/Permission-Denied Mock`) without touching `/sys`.

---

## 4. Security & Privilege Boundary Specification

### 4.1 D-Bus Service (`io.github.zethasaintlab.AcerControlCenter1`)
Object path: `/io/github/zethasaintlab/AcerControlCenter1`

| Method | Arguments | Returns | Polkit Action ID | Default Policy (`active` local seat) |
|---|---|---|---|---|
| `GetRaplPowerSnapshot` | *(none)* | `(u64 pkg_uj, u64 core_uj, u64 uncore_uj, u64 psys_uj, i64 ts_us, b avail)` | `io.github.zethasaintlab.acercontrol.read-rapl` | `yes` (`allowUserInteraction = false`) |
| `SetCpuPerformanceTuning` | `(s governor, s epp, b turbo, u32 min_pct, u32 max_pct)` | `(b applied, s readback_json, s error)` | `io.github.zethasaintlab.acercontrol.set-power-profile` | `auth_admin_keep` (`allowUserInteraction = true`) |
| `SetKeyboardLighting` | `(s mode, u32 brightness, s hex_zones_csv)` | `(b applied, s readback_json, s error)` | `io.github.zethasaintlab.acercontrol.set-keyboard-lighting` | `yes` (`allowUserInteraction = true`) |
| `SetBatteryChargeLimit` | `(u32 end_threshold_pct)` | `(b applied, u32 readback_pct, s error)` | `io.github.zethasaintlab.acercontrol.set-battery-limit` | `auth_admin_keep` (`allowUserInteraction = true`) |
| `SetFanControlMode` | `(s mode, u32 duty_pct)` | `(b applied, s readback_json, s error)` | `io.github.zethasaintlab.acercontrol.set-fan-mode` | `auth_admin_keep` (`allowUserInteraction = true`) |

### 4.2 Input Validation, Systemd Sandboxing & Readback Verification Rules
- **No Shell Execution:** `acer-control-daemon` never calls `system()`, `popen()`, or `/bin/sh`. It opens allowlisted sysfs paths using `QFile` (`O_WRONLY | O_CLOEXEC`) after verifying canonical paths (`QFileInfo::canonicalFilePath()`) under `/sys/devices/` (or the active mock root).
- **Zero Linux Capabilities & Narrow Mount Namespace (`acer-control-daemon.service`):**
  - `CapabilityBoundingSet=` (empty) and `AmbientCapabilities=` (empty): all target sysfs files are `0400` or `0644` owned by `root:root`, so the daemon reads and writes them via standard VFS owner DAC bits with `CapEff=0`, `CapPrm=0`, `CapBnd=0`.
  - `ProtectSystem=strict`, `ProtectKernelTunables=yes`, and `ReadWritePaths=-/sys/devices/system/cpu/intel_pstate/no_turbo -/sys/devices/system/cpu/intel_pstate/min_perf_pct -/sys/devices/system/cpu/intel_pstate/max_perf_pct -/sys/devices/system/cpu/cpufreq -/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00` ensure the mount namespace leaves the rest of `/sys` read-only.
- **Allowlisted Arguments:**
  - `governor` $\in \{\text{"powersave"}, \text{"performance"}\}$ (verified against `scaling_available_governors`).
  - `epp` $\in \{\text{"default"}, \text{"performance"}, \text{"balance\_performance"}, \text{"balance\_power"}, \text{"power"}\}$ (verified against `energy_performance_available_preferences`).
  - `min_pct`, `max_pct` $\in [10, 100]$ with `min_pct <= max_pct`.
  - `end_threshold_pct` $\in [50, 100]$.
- **Mandatory Readback:** Readback-capable sysfs controls are reopened and verified after a write. A successful matching readback sets `physicalStateVerified = true`. Controls without hardware readback must report their actual verification limitation; CLV0001 uses `accepted_no_hw_readback` as documented in ADR-002.
- **Safe Rollback:** If a multi-attribute operation partially fails, the daemon restores the pre-operation snapshot captured prior to the write and returns an explicit error code.


---

## Addendum — Hardware Evidence, Async Compliance & Design Direction (2026-11-23)

ADR-001 remains the approved application architecture: C++23, Qt 6/QML, QtDBus, Polkit and Fedora RPM packaging. This addendum does not authorize a toolkit or privilege-boundary migration.

**Stage 24 Async Runtime Foundation (implemented):**
The GUI runtime now enforces ADR-001 §3 Invariant #2:
- **Worker-thread telemetry (`acc::app::TelemetryCoordinator` / `TelemetryWorker`):** `hal::TelemetryCollector::collectSnapshot(activeProfileId, &hints)` executes on a dedicated worker `QThread` owned by `TelemetryCoordinator` (`m_workerThread`), keeping sysfs/proc/NVML reads and delta state off the GUI thread. `TelemetryCoordinator` pre-fetches daemon reachability (`NameHasOwner`), `GetRaplPowerSnapshot`, and `net.hadess.PowerProfiles` `ActiveProfile` asynchronously on the GUI thread via `QDBusConnection::asyncCall()` + `QDBusPendingCallWatcher`, coalesces overlapping ticks, drops stale sequence generations (`m_minValidSeq`) across hardware-mode resets, and cleanly joins the worker thread on destruction.
- **Non-blocking control transport (`acc::app::IAsyncControlTransport` / `DbusControlTransport`):** Physical privileged operations (`SetKeyboardLighting`, `SetCpuPerformanceTuning`, `SetFanControlMode`, `SetBatteryChargeLimit`, `org.freedesktop.login1.Session.SetBrightness`, and `net.hadess.PowerProfiles` profile switches) dispatch via `QDBusConnection::asyncCall()` + `QDBusPendingCallWatcher` (`AsyncControlOutcome`), eliminating `QDBus::Block` calls from `ControlCenterController` on the GUI thread. (The synchronous `QDBus::Block` helper in `hal::ControlOperations::applyDisplayBrightness` remains only in the direct HAL helper used by CLI/unit tests or mock mode, outside the GUI thread path.)
- **Explicit Stage 25 UI Runtime Contract (`acc::app::ControlCenterController`):**
  - Freshness properties & enum (`FreshnessState`: `Unavailable`, `Fresh`, `Stale`): `telemetryFreshness` (`"unavailable"` | `"fresh"` | `"stale"`), `telemetryFreshnessState`, `telemetryAvailable`, `telemetryStale`, `telemetryRefreshing`, `telemetrySuspended`, `telemetryAgeMs`, `lastTelemetryTimestampMs`, `telemetryErrorMessage`, signal `telemetryFreshnessChanged()`.
  - Privileged auth/control properties & enum (`AuthState`: `Idle`, `Pending`, `Authorized`, `Denied`, `DaemonUnavailable`, `Failed`): `controlOperationPending`, `controlOperationName`, `controlAuthState` (`"idle"` | `"pending"` | `"authorized"` | `"denied"` | `"daemon_unavailable"` | `"failed"`), `controlAuthStateEnum`, `controlLastErrorCode`, `controlLastErrorMessage`, signals `controlOperationChanged()` and `controlOperationFinished(operation, applied, authState, errorCode, message)`.
  - Lazy workspace instantiation in `src/ui/qml/Main.qml`: `workspaceArea` hosts a single `Loader` (`id: workspaceLoader`, `objectName: "workspaceLoader"`) bound to `mainWindow.navItems[ctrl.currentTabIndex].source` instead of eagerly instantiating all 8 workspace views.
  - Capability separation preserved: single-zone `CLV0001` static RGB (`kbdSupportsArbitraryRgb`) remains strictly distinct from spatial multi-zone RGB (`kbdRgbSupported`).

**Stage 25 Endfield-Inspired Structural UI Rebuild (implemented):**
- **Responsive Shell (`src/ui/qml/Main.qml`, `minimumWidth: 680`, `minimumHeight: 540`):** Uses a 3-tier adaptive navigation shell (`222px` grouped side rail >= 1160px, `64px` compact icon rail 820–1159px or `theme.compactNavigation`, `40px` horizontal tab strip < 820px), manual async `SYNC` trigger, global `TelemetryStateBanner`, and lazy single-view `workspaceLoader`.
- **Reusable Component System & Bimodal Tokens (`src/ui/qml/`, `ThemeEngine`):** Added `CommandModule`, `TelemetryMetricTile`, `TelemetryStateBanner`, and `CadKeyValueRow`; extended `ThemeEngine` with `surfaceSunken`, `surfaceOverlay`, and `hairlineRule` while preserving WCAG 2.2 contrast calculation and keyboard/accent independence; redesigned all 8 workspace QML views (`OverviewView`, `PerformanceView`, `ThermalView`, `KeyboardStudioView`, `BatteryView`, `GpuView`, `DiagnosticsView`, `SettingsView`) around task-driven split layouts that natively render Stage 24 `fresh` / `stale` / `unavailable` and `pending` / `denied` / `daemon_unavailable` / `failed` states. See `docs/DESIGN_SYSTEM.md` for full token and component details.

**Stage 26 Validation & Evidence Boundary (2026-11-23):**
- *Real-service & live-host verified:* Live Fedora 45 Hyprland Wayland startup (`QT_QPA_PLATFORM=wayland`, `xwayland=false`), `acer-control-daemon` read-only system D-Bus methods (`GetDaemonVersion`, `GetRaplPowerSnapshot`), `net.hadess.PowerProfiles` `ActiveProfile` reconciliation on physical startup, read-only host telemetry parity (`coretemp`, `intel_pstate`, `BAT0`, `nvme`, `acpitz`, `nvidia 620.70.06` NVML), and clean RPM packaging (`rpmbuild -bb` + staging install to `/usr/lib/systemd/system`).
- *Mock/simulation verified:* Multi-capability fan/battery/CLV0001 write flows, `TestDenyAll` session-bus Polkit denial (`ERR_POLKIT_UNAUTHORIZED`), stale/suspend/failure freshness transitions, and responsive breakpoint layout checks (`680×540` through `1920×1080`) in `test_core_and_hal` and `test_gui_atelier`.
- *Unverified / unauthorized:* Live interactive Polkit prompt cancellation/granting from GUI, physical suspend/resume wakeup cycle, `acer_clv0001_kbd` reload (`CLV0001:00` currently unbound), Stage 23A.1/23A.2 AML probes (strictly unloaded/unauthorized), and firmware fan/FlexiCharger writes (locked disabled).

The 2026-11-23 offline AML audit identified static Clevo WMBB / CLV0001 DCHU paths for fan control and FlexiCharger. These paths are not runtime-verified and are not implemented by ADR-001's generic standard sysfs fan/battery adapters. No undocumented firmware execution or writes are authorized. See docs/HARDWARE_AUDIT_A715_59G.md and docs/PROJECT_STATE.md for the current capability posture.

CLV0001 keyboard integration is documented separately in ADR-002 and remains narrowly scoped to `_DSM 0x67`. Any expansion to DCHU fan queries/controls or FlexiCharger requires its own architecture and safety review. Do not treat an existing keyboard-only driver as a generic _DSM transport or reuse a raw firmware dispatcher from userspace.

---

## 5. ADR-002 — Clean-Room `CLV0001` (`\_SB_.DCHU`) Keyboard Integration via `platform_driver` + Sysfs + D-Bus/Polkit (2026-11-16; updated for Stage 17/18 2026-11-20)

- **Context:** The stock Fedora 45 Linux kernel (`7.3.3-300.fc45.x86_64`) exposes `ACPI` device `CLV0001:00` (`\_SB_.DCHU`) without binding any upstream driver, leaving `/sys/class/leds/` empty for keyboard lighting. Physical testing on `A715-59G` (`Kelilu_RTH`) confirmed ACPI `_DSM` UUID `93f224e4-fbdc-4bbf-add6-db71bdc0afad`, rev `0`, fn `0x67`, 4-byte GRB payload `[G, R, B, 0xF0]` controls 17 named presets (Stage 15), static 24-bit `rgb <R> <G> <B> <level>` (Stage 17/18: `#12A7E3` level `4` -> `A7 12 E3 F0`), and 5 discrete brightness steps (`0..4` → `0x00, 0x2F, 0x5F, 0x8F, 0xBF`).
- **Options Considered:**
  1. *Shell out to `alg-rgb` CLI or open `/dev/alg_rgb` (`0666`):* Rejected. Violates no-shell-execution and least-privilege Polkit boundaries, relies on third-party `install.sh` global mutations, and conflicts with `GPL-2.0`-only upstream source SPDX vs this project's `GPL-3.0-or-later`.
  2. *Legacy `acpi_driver` (`acpi_bus_register_driver`):* Rejected / non-viable. Removed in Fedora 45 kernel `7.3.3-300.fc45.x86_64` headers.
  3. *Clean-room kernel `platform_driver` with `.acpi_match_table` + `sysfs` on ACPI kobject + `Clv0001KeyboardBackend` via `acer-control-daemon` (Chosen):* Complies cleanly with Linux 7.3 kernel APIs, enforces strict allowlists, keeps root ACPI evaluation isolated in kernel/Polkit-daemon space, and cleanly preserves `GPL-2.0-or-later` / `GPL-3.0-or-later` license compatibility.
- **Architectural Decisions & Invariants:**
  1. `driver/acer_clv0001_kbd.c` validates exact DMI identity (`Acer` / `Aspire A715-59G` / `RTH` / `Kelilu_RTH` with zero override parameter), HID (`CLV0001`), ACPI path (`\_SB.DCHU` / `\_SB_.DCHU`), `_DSM` existence, performs no automatic `_DSM` writes on resume, and exposes `0600`/`0444` sysfs attributes (`clv0001_kbd_command`, `clv0001_kbd_state`, `clv0001_kbd_presets`) on `&adev->dev.kobj` so canonical paths remain confined under `/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/`.
  2. Only `_DSM` function `0x67` is callable; unverified functions (`0x68`, `0x69`, `0x6A`) are prohibited. Both kernel and userspace enforce strict input validation (17 presets or `rgb <0..255> <0..255> <0..255> <0..4>` single-zone static commands) and rate-limiting (`40 ms` minimum interval / `25 Hz` ceiling in kernel & default HAL). Temporal animations (`breathing`, `wave`) and per-zone spatial splitting remain excluded/unsupported.
  3. Because `_DSM 0x67` does not expose hardware readback, `ControlResult` explicitly records `physicalStateVerified = false` and `verificationStatusId = "accepted_no_hw_readback"`, distinguished in diagnostics and UI from `verified_physical_readback`.
  4. Capability discovery reads `supports_custom_rgb=1` from `clv0001_kbd_presets`: arbitrary static 24-bit RGB is routed only when the bound Stage 17+ driver advertises `supports_custom_rgb=1`; legacy preset-only modules still fail closed with `ERR_CLV0001_PRESET_REQUIRED`, while preview colors remain decoupled from `ThemeEngine`.
- **Non-Destructive Physical Operator Verification Procedure (After Explicit Approval):**
  1. Build module in `build/driver-build/` (`make -C /lib/modules/$(uname -r)/build M=$PWD/build/driver-build modules`) — already verified cleanly on `7.3.3-300.fc45.x86_64`.
  2. Operator loads module explicitly (`sudo insmod build/driver-build/acer_clv0001_kbd.ko`) and restarts/installs updated `acer-control-daemon`.
  3. Launch unprivileged `./build/acer-control-center` as normal user, switch to `04 // KEYBOARD STUDIO`, select presets (`red`, `cyan`, `white`) and brightness levels (`1..4`, `0`), and confirm physical LED response and `clv0001_kbd_state` status transitions.
