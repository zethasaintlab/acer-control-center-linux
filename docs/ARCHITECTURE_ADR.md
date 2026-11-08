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
| `GetRaplPowerSnapshot` | *(none)* | `(u64 pkg_uj, u64 core_uj, u64 uncore_uj, u64 psys_uj, i64 ts_us)` | `io.github.zethasaintlab.acercontrol.read-power` | `yes` |
| `SetCpuPerformanceTuning` | `(s governor, s epp, b turbo, u32 min_pct, u32 max_pct)` | `(b applied, s readback_json, s error)` | `io.github.zethasaintlab.acercontrol.manage-performance` | `auth_admin_keep` |
| `SetKeyboardLighting` | `(s mode, u8 brightness, s hex_zones_csv)` | `(b applied, s readback_json, s error)` | `io.github.zethasaintlab.acercontrol.manage-keyboard` | `yes` (active console user) |
| `SetBatteryChargeLimit` | `(u8 end_threshold_pct)` | `(b applied, u8 readback_pct, s error)` | `io.github.zethasaintlab.acercontrol.manage-battery` | `auth_admin_keep` |
| `SetFanControlMode` | `(s mode, u8 duty_pct)` | `(b applied, s readback_json, s error)` | `io.github.zethasaintlab.acercontrol.manage-fans` | `auth_admin_keep` |

### 4.2 Input Validation & Readback Verification Rules
- **No Shell Execution:** `acer-control-daemon` never calls `system()`, `popen()`, or `/bin/sh`. It opens allowlisted sysfs paths using `QFile` (`O_WRONLY | O_CLOEXEC`) after verifying canonical paths under the configured sysfs root.
- **Allowlisted Arguments:**
  - `governor` $\in \{\text{"powersave"}, \text{"performance"}\}$ (verified against `scaling_available_governors`).
  - `epp` $\in \{\text{"default"}, \text{"performance"}, \text{"balance\_performance"}, \text{"balance\_power"}, \text{"power"}\}$ (verified against `energy_performance_available_preferences`).
  - `min_pct`, `max_pct` $\in [10, 100]$ with `min_pct <= max_pct`.
  - `end_threshold_pct` $\in [50, 100]$.
- **Mandatory Readback:** After writing any sysfs attribute, the daemon re-opens and reads back the attribute immediately. Only if the readback matches the requested state does `applied` return `true`.
- **Safe Rollback:** If a multi-attribute operation partially fails, the daemon restores the pre-operation snapshot captured prior to the write and returns an explicit error code.
