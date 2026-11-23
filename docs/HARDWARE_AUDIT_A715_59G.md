# Hardware Reconnaissance & Capability Audit — Acer Aspire 7 A715-59G (`Kelilu_RTH`)

**Initial Audit Date:** 2026-11-08
**CLV0001 & Linux 7.3 Re-Audit Date:** 2026-11-16
**Target Platform:** Fedora Linux 45 (verified live on `7.3.3-300.fc45.x86_64`; initial baseline `7.2.15-300.fc45.x86_64`)
**Target Machine:** Acer Aspire 7 `A715-59G` (`board_name: RTH`, `board_vendor: Kelilu_RTH`, BIOS `INSYDE Corp. V1.17` `06/30/2026`)

---

> **Evidence freshness note (2026-11-23):** This document's initial reconnaissance was captured on 2026-11-08 using kernel 7.2.15. The later offline ACPI/AML findings in Section 5 supersede the original "no firmware path" inference for fan and charge control. They do not establish runtime support or physical operation.

## 1. Non-Sensitive DMI & Platform Identity

All hardware discovery was conducted strictly in read-only mode without capturing serial numbers, UUIDs, or asset tags.

| DMI / System Attribute | Observed Value | Source |
|---|---|---|
| `sys_vendor` | `Acer` | `/sys/class/dmi/id/sys_vendor` |
| `product_name` | `Aspire A715-59G` | `/sys/class/dmi/id/product_name` |
| `product_version` | `1.17` | `/sys/class/dmi/id/product_version` |
| `board_name` | `RTH` | `/sys/class/dmi/id/board_name` |
| `board_vendor` | `Kelilu_RTH` | `/sys/class/dmi/id/board_vendor` |
| `bios_vendor` | `INSYDE Corp.` | `/sys/class/dmi/id/bios_vendor` |
| `bios_version` | `V1.17` | `/sys/class/dmi/id/bios_version` |
| `bios_date` | `06/30/2026` | `/sys/class/dmi/id/bios_date` |
| `chassis_type` | `10` (Notebook) | `/sys/class/dmi/id/chassis_type` |
| CPU | `13th Gen Intel(R) Core(TM) i7-13620H` (10C/16T: 6P + 4E, max 4900 MHz) | `/proc/cpuinfo`, `lscpu` |
| Integrated GPU | Intel Raptor Lake-P `[UHD Graphics] [8086:a7a8]` (`i915` bound) | `lspci -nnk` |
| Discrete GPU | NVIDIA GeForce RTX 3050 6GB Laptop GPU `GA107BM [10de:25ac]` (`Subsys 1025:1900`, observed live `nvidia 620.70.06` / `P8 6W/25W`; earlier `615.78.08`) | `lspci -nnk`, `nvidia-smi`, `/proc/driver/nvidia` |
| Storage | 2× Phison Electronics PS5019-E19 PCIe4 NVMe (`nvme0`, `nvme1`) | `lspci -nnk`, `/sys/class/hwmon` |
| ODM Board Footprint | O2 Micro SD Host Controller `[1217:8621]` carries subsystem `CLEVO/KAPOK Computer Device [1558:a554]` | `lspci -nnk` |

---

## 2. WMI, ACPI & Embedded Controller Architecture

### 2.1 Why `acer_wmi` Does Not Bind on `A715-59G` (`Kelilu_RTH`)
In-tree `acer_wmi` (`/lib/modules/7.2.15-300.fc45.x86_64/kernel/drivers/platform/x86/acer-wmi.ko.xz`) matches only three WMI GUID aliases:
- `wmi:676AA15E-6A47-4D9F-A2CC-1E6D18D14026`
- `wmi:6AF4F258-B401-42FD-BE91-3D4AC2D7C0D3`
- `wmi:67C3371D-95A3-4C37-BB61-DD47B491DAAB`

On the `Aspire A715-59G` (`Kelilu_RTH`), `/sys/bus/wmi/devices/` exposes a completely different WMI table inherited from its Clevo/Kapok ODM design:
- `ABBC0F6B-8EA1-11D1-00A0-C90629100000-2` (`CLEVO_WMI_EVENT_GUID` — unbound)
- `ABBC0F6C-8EA1-11D1-00A0-C90629100000-3` (`CLEVO_WMI_EMAIL_GUID` — unbound)
- `ABBC0F6D-8EA1-11D1-00A0-C90629100000-1` (`CLEVO_WMI_METHOD_GUID` — unbound)
- `F6CB5C3C-9CAE-4EBD-B577-931EA32A2CC0-0` (`mxm_wmi` — bound)

### 2.2 Relevant ACPI Devices Observed
- `CLV0001:00` (`\_SB_.DCHU`, ACPI `_STA` `status=15` = `0x0F` [Present | Enabled | UI-Visible | Functioning] — exposed keyboard ACPI `_DSM` target; no upstream Linux driver bound)
- `CLV0002:00` (`\_SB_.DCHP` live / `\_SB_.DCHV` in mock fixture, ACPI `_STA` `status=15` — sibling ODM ACPI device; unbound, out of scope)
- `PNP0C09:00` (`\_SB_.PC00.LPCB.EC0_` / `EC__`, bound to `acpi-ec`)
- `PNP0C0A:00` (`\_SB_.BAT0`, bound to `acpi-battery`)
- `ACPI0003:00` (`\_SB_.AC__`, bound to `ac`)
- `PNP0C14:00` (`\_SB_.PC00.WMI1`, bound to `acpi-wmi`)
- `PNP0C14:01` (`\_SB_.WMI_`, bound to `acpi-wmi`)
- `INT340E:00` (`\_SB_.PTID`) plus `processor_thermal_device_pci` (`00:04.0`)

### 2.3 Clean-Room `CLV0001` (`\_SB_.DCHU`) Keyboard Investigation & `alg-cli` Analysis (2026-11-16)
Read-only sysfs inspection on the target `A715-59G` confirmed `CLV0001:00` (`hid=CLV0001`, `path=\_SB_.DCHU`, ACPI `_STA` `status=15` [`0x0F`], modalias `acpi:CLV0001:`) while `/sys/class/leds/` contains no `*kbd_backlight*` node. Note that ACPI `_STA` `status=15` reflects BIOS ACPI namespace presence only and is distinct from `clv0001DriverBound` (`false` when unbound) and `clv0001_kbd_state` (`status=uninitialized` vs. `status=accepted_no_hw_readback`, `physical_readback=unsupported`). Hardware behavior was cross-referenced against user-confirmed physical testing on this machine and source inspection of upstream [`24kaushik/alg-cli@038d628`](https://github.com/24kaushik/alg-cli/tree/038d6289dd38f92d9c321d87ee6203fc63767143) and fork [`zethasaintlab/alg-cli@acb9d92`](https://github.com/zethasaintlab/alg-cli/tree/acb9d92f0da836d8addf69604ab13b5d9c4b2304):

| Item | Evidence Status | Findings |
|---|---|---|
| ACPI `_DSM` UUID, Revision & Static Color Function | **User-Confirmed Physical Behavior (Stage 15 Presets + Stage 17 Arbitrary RGB)** + Source Evidence | UUID `93f224e4-fbdc-4bbf-add6-db71bdc0afad`, rev `0`, fn `0x67`, 4-byte GRB payload `[G, R, B, 0xF0]` controls backlight on/off, 17 named presets (Stage 15: `off`, `red`, `orange`, `yellow`, `lime`, `light-green`, `green`, `green-cyan`, `cyan`, `light-blue`, `blue`, `violet`, `magenta`, `pink`, `flesh`, `bluish-white`, `white`), arbitrary static 24-bit RGB `rgb <R> <G> <B> <brightness_level>` (Stage 17: verified `#12A7E3` level `4` -> `A7 12 E3 F0`), and 5 discrete brightness scales (`0..4` → `0x00, 0x2F, 0x5F, 0x8F, 0xBF` normalized over `0xBF`). |
| Hardware Readback | **Source Evidence + Physical Posture** | `_DSM 0x67` is write-only (`accepted_no_hw_readback`); firmware exposes no readback getter for currently lit keyboard color/brightness. |
| Secondary Functions (`0x68`, `0x69`, `0x6A`) | **Unverified Hypothesis (Prohibited)** | Appear in `alg_rgb.c` / `cli/alg-rgb.c`; not physically verified on `A715-59G` and permanently excluded from this repository. |
| Fork `alg-rgb-engine` ABI Mismatch | **Source-Code Evidence** (`zethasaintlab/alg-cli`) | `engine/alg-rgb-engine` writes `"rgb {r} {g} {b} {brightness}\n"`, while `kernel/alg_rgb.c` parses `"%31s %d"` against the 17-preset table and returns `-EINVAL` for `"rgb"`. Reproduced in unit tests. |
| Third-Party Licensing & Packaging | **Source-Code Evidence** | `alg-cli` `LICENSE` prose states GPLv2-or-later while `alg_rgb.c`/`alg-rgb.c` declare `SPDX-License-Identifier: GPL-2.0`, and `install.sh` creates `0666` `/dev/alg_rgb` and alters `/etc/modules-load.d/`. Resolved by authoring our own clean-room `GPL-2.0-or-later` module (`driver/acer_clv0001_kbd.c`). |
| Linux `7.3` Kernel API Adaptation | **Local Observation** (`/usr/src/kernels/7.3.3-300.fc45.x86_64`) | `struct acpi_driver` / `acpi_bus_register_driver` are removed in Linux 7.3; clean-room driver binds via `struct platform_driver` + `.acpi_match_table`, enforces exact `Acer`/`Aspire A715-59G`/`RTH`/`Kelilu_RTH` DMI matching and 40 ms (`25 Hz`) rate limit without automatic resume `_DSM` writes, and attaches `0600`/`0444` sysfs attributes (`clv0001_kbd_{command,state,presets}`) to `&adev->dev.kobj` under `/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/`. |


### 2.4 Stage 14 Controlled Physical Hardware Validation Record (2026-11-16)
Controlled Stage 14 execution on the target **Acer Aspire 7 A715-59G (`RTH` / `Kelilu_RTH`)** running Fedora 45 (`7.3.3-300.fc45.x86_64`) with packaged RPM `acer-control-center-1.0.0-2.fc45.x86_64` (DNF5 transaction `63`, `rpm -V` exit `0`) and hardened `acer-control-daemon.service` (`MainPID=57721`, `CapabilityBoundingSet=` empty, `ProtectSystem=strict`, `ProtectKernelTunables=yes`, `ReadWritePaths=-/sys/devices/system/cpu/intel_pstate/no_turbo -/sys/devices/system/cpu/intel_pstate/min_perf_pct -/sys/devices/system/cpu/intel_pstate/max_perf_pct -/sys/devices/system/cpu/cpufreq -/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00`):

1. **Module Bind & Initial Posture (Phase B):** `acer_clv0001_kbd` loaded cleanly (`vermagic: 7.3.3-300.fc45.x86_64 SMP preempt mod_unload`), bound `/sys/bus/platform/devices/CLV0001:00/driver` -> `/sys/bus/platform/drivers/acer_clv0001_kbd`, logged `acer_clv0001_kbd CLV0001:00: bound CLV0001 (\_SB.DCHU) _DSM 0x67 keyboard backlight sysfs interface`, and initialized `clv0001_kbd_state` to `status=uninitialized`, `preset=none`, `brightness_level=0`, `payload_grb=00 00 00 F0`, `acpi_return_type=none`, `physical_readback=unsupported`.
2. **Pre-ACPI Negative Validation (Userspace & Kernel Sysfs `Phase C`):**
   - Unprivileged D-Bus calls (`SetKeyboardLighting` signature `sus`) rejected `"preset:invalid_preset"` (`ERR_INVALID_CLV0001_PRESET`), `"static"` + `"#123456"` (`ERR_CLV0001_PRESET_REQUIRED`), and brightness `150` (`ERR_INVALID_BRIGHTNESS`) with `bss false` and `verification_status="rejected_before_execution"`.
   - Direct root writes to `/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command` (`-rw-------`) for `invalid_preset 4`, `cyan 9`, and `cyan 2 extra` all returned `Invalid argument` (`-EINVAL`), preserving `status=uninitialized` and `acpi_return_type=none`.
3. **Sequential Physical ACPI `_DSM 0x67` Writes (`Phase D`, D-Bus -> Polkit -> Daemon -> Sysfs -> Driver):**
   - `cyan 2` (`50%`): `bss true ... "OK"`, `status=accepted_no_hw_readback`, `preset=cyan`, `brightness_level=2`, `payload_grb=7E 00 63 F0`, `acpi_return_type=integer`, `physical_readback=unsupported`.
   - `white 4` (`100%`): `bss true ... "OK"`, `status=accepted_no_hw_readback`, `preset=white`, `brightness_level=4`, `payload_grb=FF FF FF F0`, `acpi_return_type=integer`, `physical_readback=unsupported`.
   - `off 0` (`0%`): `bss true ... "OK"`, `status=accepted_no_hw_readback`, `preset=off`, `brightness_level=0`, `payload_grb=00 00 00 F0`, `acpi_return_type=integer`, `physical_readback=unsupported`.
   - Back-to-back rate-limit test (`"preset:red" 50 "red"` -> `00 7E 00 F0`, followed immediately by `"preset:blue" 50 "blue"`): second call returned `bss false` with `ERR_RATE_LIMITED` (`rejected_before_execution`), then cleanly restored to `off 0` (`00 00 00 F0`).
4. **Unprivileged Qt Quick GUI (`Main.qml` + `ControlCenterController`) E2E (`Phase E`):**
   - Verified `Main.qml` loaded in Physical Hardware mode (`backend=clv0001_acpi_dsm`, `connected=true`, `supportsPresetColors=true`, `supportsArbitraryRgb=false`, `physicalReadback=false`).
   - Verified `selectKbdPreset("cyan")` + `setKbdBrightnessLevel(3)` -> `preset=cyan`, `brightness_level=3`, `payload_grb=BE 00 95 F0`, `acpi_return_type=integer`; arbitrary hex `#123456` rejected with `ERR_CLV0001_PRESET_REQUIRED` leaving hardware state untouched; `snapToNearestKbdPreset()` -> `bluish-white` level `3` (`5F 2F 5F F0`); and final restore `off 0` (`00 00 00 F0`).
   - `/usr/bin/acer-control-center --dump-telemetry-json` reported `clv0001LastStatus="accepted_no_hw_readback"`, `clv0001LastAcpiReturnType="integer"`, and `physicalReadbackSupported=false`; `journalctl -k` recorded zero ACPI/kernel errors or warnings across the session.
5. **Scope of Physical Confirmation:** Kernel and daemon telemetry prove `acpi_evaluate_dsm()` (`_DSM` UUID `93f224e4-fbdc-4bbf-add6-db71bdc0afad`, rev `0`, fn `0x67`) accepted each command and returned an ACPI `integer` object (`status=accepted_no_hw_readback`), while `physical_readback=unsupported`. Subsequent Stage 15 operator testing physically verified all 17 named color presets on hardware.

### 2.5 Stage 17 Static Arbitrary RGB Physical Hardware Verification Record (2026-11-20)
Following Stage 15 preset verification, controlled operator-executed Stage 17 verification tested non-preset arbitrary 24-bit static RGB through `acer_clv0001_kbd` on the exact target **Acer Aspire 7 A715-59G (`RTH` / `Kelilu_RTH`)**:

- **Test identifier:** Stage 17 arbitrary RGB physical verification
- **Driver artifact tested:** `build/driver-build-stage17/acer_clv0001_kbd.ko` (`srcversion: A86A12F0F4BF3B06F90363E`, `vermagic: 7.3.3-300.fc45.x86_64 SMP preempt mod_unload `, SHA-256 `aa81bdcf7c1af3c293906d2cd4111ad97119726210e4d788be700bf6d7e0efae`)
- **Requested input:** `rgb 18 167 227 4`
- **RGB:** `#12A7E3` (`R=18`, `G=167`, `B=227`, brightness level `4`)
- **Expected GRB payload:** `A7 12 E3 F0`
- **Physical result:** `OPERATOR-CONFIRMED VISIBLE COLOR CHANGE`
- **Interpretation:**
  - The Stage 17 kernel driver was loaded and bound to `CLV0001:00` (`\_SB_.DCHU`).
  - The command `rgb 18 167 227 4` was accepted through `clv0001_kbd_command` / ACPI `_DSM` function `0x67` with payload `A7 12 E3 F0`.
  - The operator visually observed the physical keyboard change color to `#12A7E3`.
  - `physical_readback` remains `unsupported` (`0`); successful ACPI return alone does not independently verify hardware LED state.
  - This confirms one arbitrary, non-preset static RGB operation on the exact `A715-59G` hardware without claiming exhaustive 24-bit gamut testing, multi-zone spatial split control, or temporal animation.

### 2.6 Stage 18 Custom RGB GUI Integration & Verification Record (2026-11-20 — Audit Log Gap Noted)
Stage 18 extended the `CLV0001` pipeline across `Clv0001KeyboardBackend`, `ControlOperations`, `DaemonServiceObject` (`SetKeyboardLighting`), `ControlCenterController`, and `KeyboardStudioView` (`1.0.0-3.fc45`), advertising `supports_custom_rgb=1` via `clv0001_kbd_presets` while keeping legacy preset-only modules fail-closed (`ERR_CLV0001_PRESET_REQUIRED`):

- **Operator physical test:** The operator physically tested Stage 18 GUI custom static RGB on the target `Acer Aspire A715-59G` (`Kelilu_RTH`) using `#12A7E3` at brightness level `4` (`rgb 18 167 227 4` -> GRB payload `A7 12 E3 F0`).
- **Evidence distinction:** Physical LED illumination was confirmed by operator visual observation, firmware acceptance is reported as `accepted_no_hw_readback`, and `physical_readback` remains `unsupported` (`_DSM 0x67` is write-only). HAL, daemon, and controller custom-RGB paths are also exercised in automated mock unit/GUI tests (`test_core_and_hal`, `test_gui_atelier`).
- **Documentation gap:** Unlike Stage 14, a verbatim Stage 18 machine capture log (`clv0001_kbd_state`, `busctl`/daemon log, `rpm -V`) was not archived in this document at the time of testing. Treat this as a missing audit artifact to capture cleanly if re-validated, not as contradicting the operator-confirmed Stage 18 physical test. On the current host snapshot (`2026-11-23`), `acer_clv0001_kbd` is unloaded (`CLV0001:00 unbound`).

---

---

## 3. Acer Software & Linux Project Comparison

| Original Feature | Purpose | A715-59G Hardware Support | Linux (`7.2.15-300.fc45`) Support | Implementation Feasibility |
|---|---|---|---|---|
| **System Telemetry Overview** (Acer Control Center / AcerSense) | Real-time CPU, GPU, memory, storage, thermal & battery overview | **Yes** | **Fully Supported** (`coretemp`, `/proc`, `nvme`, `BAT0`, `powercap`, NVML) | **Implemented (Read-Only + RAPL via Daemon)** |
| **Performance Profiles** (Silent / Normal / Performance) | Coordinate system thermal/power profile and CPU P-state policy | **Yes** | **Fully Supported** via Fedora `tuned` / `tuned-ppd` (`net.hadess.PowerProfiles`) and `intel_pstate` (`scaling_governor`, `energy_performance_preference`, `no_turbo`, `min/max_perf_pct`) | **Implemented (Full Read/Write with Readback)** |
| **Thermal & Fan Control** (NitroSense / PredatorSense inspiration) | Monitor sensor temperatures, fan RPM, and configure fan curves | **Thermal sensors Yes**; EC (`PNP0C09:00`) controls dual cooling fans automatically | **Thermal Sensors Read-Only Supported** (`hwmon1..6`, `thermal_zone0..7`); **Fan RPM & PWM Not Detected** in `/sys/class/hwmon` | **Thermal Monitoring + Safe Fan Curve Studio:** Live thermal tracking + curve editor that binds to `hwmon/pwm*` when present/mocked and clearly reports `Firmware Auto (No kernel PWM bound)` on physical `Kelilu_RTH` |
| **Keyboard Lighting Studio** | Control backlight brightness, timeout, or RGB color | **`CLV0001` Static Presets + Static Custom RGB** (`\_SB_.DCHU` via `_DSM 0x67` accepts 17 presets & Stage 17 arbitrary RGB at 5 brightness levels; per-zone split, temporal effects & readback unsupported) | **No `*kbd_backlight*` in `/sys/class/leds/`**; clean-room `driver/acer_clv0001_kbd.c` exposes `clv0001_kbd_{command,state,presets}` with explicit `supports_custom_rgb=1` capability discovery | **Capability-Aware Routing:** (1) `/sys/class/leds/*kbd_backlight*` RW with readback, (2) Clean-room `CLV0001` 17 presets + optional Stage 17 static 24-bit `rgb` (`accepted_no_hw_readback`), or (3) Unrestricted HSV/HSL/RGB Studio Preview when unbound |
| **Battery Care & Charge Threshold** (80% Charge Limit) | Prevent battery wear by capping charge at 80% and tracking capacity ratio | **Telemetry Yes**; Charge limit depends on ODM EC driver | **Telemetry & Capacity Ratio Read-Only Supported** (`BAT0` capacity, voltage, current, full-vs-design capacity ratio); `charge_control_end_threshold` **Not Detected** | **Telemetry/History Implemented + Conditional Charge Limit:** Charge limit slider binds to standard `charge_control_end_threshold` and remains safely disabled with explanation when absent; fuel-gauge capacity ratio (`charge_full / charge_full_design`) is presented with its estimation limitations and never prescribes forced discharge calibration |
| **NVIDIA GPU Center** | Monitor RTX 3050 clocks, VRAM, utilization, temperature, P-state, throttling | **Yes** (`RTX 3050 6GB Laptop GPU`) | **Read-Only Supported** via `libnvidia-ml.so.1` & `nvidia-powerd` | **Implemented via Clean-Room Dynamic NVML Loader** with `P8` rail-gating anomaly detection |
| **Hardware Diagnostics** | Inspect live capabilities, driver bindings, WMI GUIDs, and export redacted logs | **Yes** | **Fully Supported** | **Implemented** |

### Existing Linux Projects Audited
1. **`fladirm/asense` (`https://github.com/fladirm/asense`):** Rust + Dioxus GUI + D-Bus daemon targeting Predator/Nitro laptops via `acer_wmi` gaming interfaces and `ENEK5130` controllers. Its `acer_wmi` assumptions do not apply to the Clevo-derived `Kelilu_RTH` board on the `A715-59G`.
2. **`stanlysilas/acer-control-center` (`https://github.com/stanlysilas/acer-control-center`):** GTK 4 GUI + D-Bus daemon targeting ODM-built Acer ALG laptops (`AL15G-53`). Demonstrates the necessity of D-Bus privilege separation on ODM-built Acer laptops, though we avoid any unverified raw EC writes.

---

## 4. Complete Hardware Capability Matrix (`A715-59G / Kelilu_RTH`)

| Capability ID | User-Facing Feature | Exact Interface / API Observed | Permissions | Capability State | Verification & Safety Notes |
|---|---|---|---|---|---|
| `cap.cpu.temp` | CPU Package & Core Temperatures (10 cores) | `/sys/class/hwmon/hwmon5/temp*_input` (`coretemp`) | `0444` | `ReadOnly` | Verified live (`42°C–50°C`). |
| `cap.cpu.load` | Total & Per-Thread CPU Utilization (16 threads) | `/proc/stat` | `0444` | `ReadOnly` | Verified live delta computation. |
| `cap.cpu.freq` | Per-Core Clock Frequency & Bounds | `/sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq` | `0444` | `ReadOnly` | Verified (`400 MHz – 4900 MHz`). |
| `cap.cpu.power` | CPU Package, Core, Uncore & Platform Power (Watts) | `/sys/class/powercap/intel-rapl:0/energy_uj` (`package-0`, `core`, `uncore`, `psys`) | `0400` (`root`) | `ReadOnly` (Privileged) | Read safely via `acer-control-daemon` (`GetRaplPowerSnapshot`) with microjoule wrap-around handling. |
| `cap.cpu.pstate` | CPU Governor, EPP, Turbo Boost, Min/Max Perf % | `/sys/devices/system/cpu/cpu*/cpufreq/{scaling_governor,energy_performance_preference}`, `/sys/devices/system/cpu/intel_pstate/{no_turbo,min_perf_pct,max_perf_pct}` | `0644` (`root`) | `FullySupported` | Controlled via `acer-control-daemon` with strict allowlist validation and immediate sysfs readback. |
| `cap.power.profile` | System Power Profile (`Quiet`, `Balanced`, `Performance`) | D-Bus `net.hadess.PowerProfiles` (`tuned-ppd`) & `com.redhat.tuned` | D-Bus | `FullySupported` | Verified (`tuned-adm` / `tuned-ppd` active on Fedora 45). |
| `cap.mem.telemetry` | RAM Usage, Available Memory & Linux PSI Pressure | `/proc/meminfo`, `/proc/pressure/{cpu,memory,io}` | `0444` / `0666` | `ReadOnly` | Verified (`16040568 kB` total; PSI `some`/`full` averages). |
| `cap.thermal.sensors` | NVMe0, NVMe1, Wi-Fi & ACPI Board Thermal Zones | `hwmon3` (`nvme1`), `hwmon4` (`nvme0`), `hwmon6` (`iwlwifi_1`), `thermal_zone0..7` (`acpitz`, `SEN2`, `SEN3`, `TCPU`, `x86_pkg_temp`) | `0444` | `ReadOnly` | Verified across all 7 hwmon devices and 8 thermal zones. |
| `cap.thermal.fans` | Physical Fan RPM & Manual/Curve PWM Override | `/sys/class/hwmon/*/fan*_input`, `/sys/class/hwmon/*/pwm*` | Absent | `NotDetected` (`FirmwareAutomatic`) | Firmware EC manages dual fans automatically. UI shows live thermal telemetry + clearly labeled Curve Preview/Simulation mode unless a verified `hwmon` PWM device is present or mocked. Static AML fan paths (`SCMD 0x68/0x69/0x79`, `PK0C`/`PK0D`/`PK0E`) remain runtime-unverified and locked. |
| `cap.gpu.nvidia` | NVIDIA RTX 3050 6GB Telemetry, Clocks, P-State & Throttling | Dynamic `dlopen("libnvidia-ml.so.1")` (`0000:01:00.0`) & `/proc/driver/nvidia` | User RO | `ReadOnly` | Verified live (`nvidia 620.70.06`, `P8`, `6W/25W`, `6144 MiB`; previously `615.78.08`). When NVML is unavailable, `/proc/driver/nvidia` fallback reports `telemetryMeasuredByNvml=false` and `powerLimitsMeasured=false` without fabricating VRAM or TGP values. |
| `cap.battery.telemetry` | Battery Charge %, Voltage, Current, Wattage & Est. Capacity Ratio | `/sys/class/power_supply/BAT0/*`, `/sys/class/power_supply/AC/online` | `0444` | `ReadOnly` | Verified (`charge_full 1956000` / `charge_full_design 3410000` → `57.4%` fuel-gauge capacity ratio; serial redacted). |
| `cap.battery.limit` | Battery Charge End Threshold (e.g. 80% Care Mode) | `/sys/class/power_supply/BAT0/charge_control_end_threshold` | Absent | `NotDetected` | Supported in HAL & daemon when standard sysfs node exists; disabled with clear hardware explanation on `Kelilu_RTH`. Stage 23A read-only probes (`GCMD 0x77`, `PK04 0x1E`) runtime-observed `OGNS` NVS shadow `(BAEE>>1=1, BSCP=70, BTCP=80)` and `FCSC`/`FCTC` constants only; live EC readback/enforcement and setters (`PK04 0x1F`, `SCMD 0x76`, `ECMD 0xCB`) remain unverified and disabled. |
| `cap.backlight.display` | Internal eDP Panel Brightness | `/sys/class/backlight/intel_backlight` (`max_brightness: 48000`) & `org.freedesktop.login1.Session.SetBrightness` | `0644` / D-Bus | `FullySupported` | Verified (`intel_backlight` on `card1-eDP-1`). |
| `cap.keyboard.lighting` | Physical Keyboard Backlight, `CLV0001` Presets & Static Custom RGB | `/sys/class/leds/*kbd_backlight*` OR `/sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_command` | `0600` (`root`, D-Bus/Polkit) / Unbound on stock kernel | `PartiallySupported` (when bound) / `NotDetected` (unbound) | Clean-room `CLV0001` backend supports 17 presets (Stage 15 verified), Stage 17 static 24-bit RGB (`supports_custom_rgb=1` advertised in `clv0001_kbd_presets`; `#12A7E3` level `4` physically confirmed), and 5 levels (`0..4`) over `_DSM 0x67` with strict DMI/HID/`_STA` checks and `accepted_no_hw_readback`; falls back cleanly to Preview Mode when unbound or preset-only on older modules. |



## Stage 21.1 Read-Only Fan, WMI & Battery Forensics Addendum (2026-11-20) — Historical Snapshot


> **Historical status:** This record reflects the investigation state on 2026-11-20, before the Stage 22/22.1 offline AML audit. Its pre-disassembly hypotheses and proposed inspection steps are superseded by Section 5 below. Use Section 5 for current firmware safety and capability conclusions.


### 1. Confirmed Kernel & Hardware State (`Acer Aspire A715-59G` / `Kelilu_RTH`, INSYDE `V1.17`)
- **Fan Interface Confirmation:** Zero `fan*_input`, `pwm*`, or `PNP0C0B` fan cooling devices exist in `/sys/class/hwmon/hwmon0..6` (`AC`, `acpitz`, `BAT0`, `nvme0`, `nvme1`, `coretemp`, `iwlwifi_1`) or `/sys/class/thermal/`. The Embedded Controller `PNP0C09:00` (`\_SB_.PC00.LPCB.EC__`) manages dual cooling fans exclusively in **`FIRMWARE AUTOMATIC`** mode.
- **Battery Cycle-Count Reconciliation:** `/sys/class/power_supply/BAT0/uevent` exposes `POWER_SUPPLY_CYCLE_COUNT=0` alongside `POWER_SUPPLY_CHARGE_FULL=1970000` uAh vs `POWER_SUPPLY_CHARGE_FULL_DESIGN=3410000` uAh (`57.8%` estimated capacity ratio). Linux `drivers/acpi/battery.c` initializes `cycle_count = 0` when the firmware uses legacy `_BIF` or when `_BIX` / the Smart Battery IC omits cycle count (`0` or `0xFFFF`). Therefore, `BAT0/cycle_count == 0` on `Kelilu_RTH` cannot be distinguished from an unreported cycle counter without raw ACPI `_BIX` table inspection and is classified and displayed conservatively as **`NOT REPORTED`** rather than fabricating `0 cycles`.
- **Battery Charge Threshold:** `charge_control_end_threshold`, `charge_control_start_threshold`, `charge_behaviour`, `charge_type`, and `charge_control_limit` are absent under `/sys/class/power_supply/BAT0/`. Status: **`BATTERY TELEMETRY ONLY — CHARGE LIMIT NOT DETECTED`**.

### 2. Bounded Read-Only Scope — ACPI Tables, `WMBB`, & `ABBC0F6D`
- **ACPI Table Permissions:** `/sys/firmware/acpi/tables/{DSDT,SSDT1..SSDT17}` exist on disk with strict `0400 root:root` permissions (547,290-byte `DSDT`). Unprivileged inspection cannot read AML definitions.
- **Observed Platform Nodes:**
  - `CLV0001:00` (`\_SB_.DCHU`, status `15`): Bound by clean-room Stage 17+ `acer_clv0001_kbd` solely for verified keyboard backlight `_DSM 0x67` commands. Zero fan or battery methods are assumed or invoked on `\_SB_.DCHU`.
  - `CLV0002:00` (`\_SB_.DCHP`, status `15`): Present and unbound in ACPI; semantics unverified without `DSDT`/`SSDT` AML disassembly.
  - `ABBC0F6D-8EA1-11D1-00A0-C90629100000-1` (`object_id: BB` / `\_SB_.WMI_.WMBB`): Unbound Clevo/Kapok WMI device GUID (alongside `ABBC0F6B` `notify_id: D0` and `ABBC0F6C` `notify_id: D1`).
- **Upstream Clevo / TUXEDO Driver Context:** In mainline Linux and out-of-tree Clevo/TUXEDO drivers (`clevo_acpi`, `clevo-xsm-wmi`, `tuxedo-drivers`), `WMBB` (`ABBC0F6D`) commonly implements OEM-specific WMI methods (including fan RPM/duty queries or EC bridges on certain Clevo chassis). However, **exact-board DMI identity `Acer / Aspire A715-59G / Kelilu_RTH (RTH)` is NOT upstreamed in Fedora 45 kernel drivers (`acer_wmi`, `acerhdf`, `uniwill_laptop`, `system76_acpi`)**, and `WMBB` method numbers vary across ODM/InsYde BIOS generations.
- **Distinction — Confirmed Absence vs. Open Hypothesis:**
  - **Confirmed Fact:** No standard kernel Linux `hwmon` fan tachometer, `pwm*`, `power_supply` charge-limit sysfs interface, or ACPI `PNP0C0B` fan actuator is exposed on this host (`FIRMWARE AUTOMATIC — NO EXPOSED CONTROL`).
  - **Open Hypothesis (Unverified):** Read-only fan tachometer getters or battery charge-limit methods *may* exist inside `DSDT`/`SSDT` (`\_SB_.WMI_.WMBB` / `\_SB_.DCHU` / `\_SB_.DCHP`), but verifying them safely requires offline AML disassembly first — never speculative WMI/EC calls.

### 3. Historical Offline-Inspection Preparation (Superseded by Stage 22/22.1)
To inspect `DSDT`/`SSDT` definitions read-only without invoking firmware methods or writing EC registers, the operator may run:
```bash
sudo mkdir -p /tmp/acpi-audit-a715-59g && sudo cp /sys/firmware/acpi/tables/DSDT /sys/firmware/acpi/tables/SSDT* /tmp/acpi-audit-a715-59g/
sudo chmod 0644 /tmp/acpi-audit-a715-59g/DSDT /tmp/acpi-audit-a715-59g/SSDT*
iasl -d /tmp/acpi-audit-a715-59g/DSDT /tmp/acpi-audit-a715-59g/SSDT*
grep -rn -E "WMBB|DCHU|DCHP|ABBC0F6D|FANG|FANR|PFAN|_BIX|BATT" /tmp/acpi-audit-a715-59g/*.dsl
```

---

## 5. Addendum — Stage 22 / 22.1 Offline AML Findings

**Audit date:** 2026-11-23  
**Target:** Acer Aspire A715-59G / Kelilu_RTH / RTH / INSYDE V1.17  
**Kernel reported by the operator:** 7.3.3-300.fc45.x86_64  
**Method:** Offline static disassembly of DSDT plus SSDT1–SSDT17 using ACPICA. No AML method, _DSM, WMI command, EC register or mailbox operation was executed during these stages.

### 5.1 Corrected conclusion

The absence of standard Linux fan and battery-threshold sysfs interfaces remains true. However, the original assumption that no vendor firmware path exists was too strong.

Stage 22 identified vendor-specific AML control and query paths behind:
- Clevo WMBB: ABBC0F6D-8EA1-11D1-00A0-C90629100000, object ID BB.
- CLV0001 / DCHU: _DSM UUID 93f224e4-fbdc-4bbf-add6-db71bdc0afad.
- EC mailbox helper ECMD using EC81 FCMD/FDAT/FBUF/FBF1–FBF3.
- SystemMemory EC.RAM and EC.RAM3 regions.

These are static findings only. The embedded EC firmware's acceptance, response encoding, fan channel mapping and physical effects remain unverified.

### 5.2 Fan findings

Static AML paths include:
- Manual duty request: SCMD 0x68 -> ECMD 0xC1 for channels 0x01–0x04.
- Automatic restore request: SCMD 0x69 -> ECMD 0xC1 with 0xFF and selected channel IDs.
- Fan mode: SCMD 0x79 subcommand 0x01 -> ECMD 0xD7.
- Dynamic curve/slope parameters: SCMD 0x79 subcommand 0x07 -> ECMD 0xCC.
- Curve table access: PK0D / PK0E and EC.RAM3 fields for three declared fan groups.
- RPM/thermal paths: PK0C and GCMD 0x63, 0x64, 0x6E–0x71.

Important Stage 22.1 corrections:
- WMI.PK0D performs direct SystemMemory reads and sets its response field to zero; DCHU.PK0D also performs an ECMD 0xD7 mailbox query and leaves the command uncleared.
- WMI.PK0C and DCHU.PK0C are active mailbox-query transactions, not passive reads. Their ECMD call counts and response fields differ.
- GCMD 0x63/0x64/0x6E–0x71 write EC mailbox command/data registers even when their purpose is telemetry.
- SCMD 0x68 sends four channel writes from one packed argument, has no AML readback and defines no watchdog. A partial or incorrectly packed request is not a safe single-fan control.
- SCMD 0x69 requests automatic restore, but AML cannot prove the private EC firmware's restore semantics.
- SCMD 0x79 contains side-effecting profile/SMI paths; subcommand 0x07 has a division-by-zero edge case for certain inputs.
- Direct EC.RAM/RAM3 accesses bypass the same PATM mailbox-lock scope used by ECMD and can race with firmware updates.

**Product posture:** firmware automatic. Do not expose physical manual fan control or invoke telemetry mailbox methods until the exact path, concurrency behavior, readback and recovery strategy are separately verified and authorized.

### 5.3 Battery / FlexiCharger findings

Static AML paths include:
- Getter GCMD 0x77, which returns BAEE, BSCP and BTCP from GNVS/OGNS NVS.
- DCHU.PK04 subcommand 0x1E, which returns the current NVS values and static preset arrays.
- Setter DCHU.PK04 subcommand 0x1F and SCMD 0x76 paths, which update NVS and may send ECMD 0xCB.

Static preset arrays reported:
- Start-charge values (FCSC): 40, 50, 60, 70, 80, 90 percent.
- Stop-charge values (FCTC): 60, 70, 80, 90, 100 percent.

Important limitations:
- GCMD 0x77 reads NVS shadow state, not direct live EC readback.
- PK04 0x1F writes NVS before or independently of successful EC command completion and lacks threshold-order/range validation and rollback.
- DCHU.SCMD and WMI.SCMD have materially different conditional behavior.
- BAT0 does not expose the standard charge-threshold node on the reported physical system.
- The standard _BST charge-limiting status bit is not a threshold-setting interface.
- Stage 22.2 corrected an earlier Stage 22.1 misstatement about `BAT0._BTP`: `BAT0._BTP` (`DSDT/DSDT.dsl:95677-95701`) is **not** an empty stub; when `ECOK` is set it sends a standard ACPI battery trip-point EC mailbox command (`ECMD { 0x05, 0x00, 0xF2, 0x00, 0xC0, hi, lo, 0x00 }`), which is unrelated to Clevo FlexiCharger (`ECMD 0xCB`).

**Product posture:** battery telemetry only. Do not expose or invoke a charge-limit setter until runtime behavior, EC acceptance, constraints, readback and rollback are proven.

### 5.4 Candidate zero-mailbox queries and Stage 22.2 closure

Stage 22.2 closed the static AML review of the initial read-only candidate set (`DSDT` + `SSDT1–SSDT17`, offline `iasl -d` disassembly) behind `\_SB_.DCHU._DSM` (UUID `93f224e4-fbdc-4bbf-add6-db71bdc0afad`, `Arg1` revision `0`, `Arg3` as `Package { Buffer }`):

1. **`DCHU.GCMD 0x77`** (`DSDT/DSDT.dsl:102800-102874, 105881-105891, 106591-106597`):
   - **Call contract:** `Arg2 = 0x77`, `Arg3 = Package { Buffer(>=4) }` (mandatory because `GCMD` executes `ARGP = DerefOf (Arg2 [Zero])` before testing `Arg1`).
   - **Return:** ACPI `Integer`: bits `[7:0] = (BAEE >> 1)`, `[15:8] = BSCP`, `[23:16] = BTCP`.
   - **Access path:** Pure `OGNS` ACPI NVS (`SystemMemory 0x428D6018`) read; no `ECOK` check, zero `EC.RAM`, zero `EC81`, zero `PATM`.
2. **`DCHU.PK04 0x1E`** (`DSDT/DSDT.dsl:102800-102830, 102965-102970, 103011-103029, 103597-103621, 103766`):
   - **Call contract:** `Arg2 = 0x04`, `Arg3 = Package { Buffer([0]=0x1E, ...) }`.
   - **Return:** `256`-byte `Buffer` (`0x0100`): when `ECOK == 1`, `[0] = (BAEE >> 1)`, `[1] = BSCP`, `[2] = BTCP`, `[16..31] = FCSC` (`40, 50, 60, 70, 80, 90`, zero-padded), `[32..47] = FCTC` (`60, 70, 80, 90, 100`, zero-padded); when `ECOK == 0`, zero-filled `Buffer (0x0100)`.
   - **Access path:** `ECOK`-gated (`IntObj`) read of `OGNS` NVS (`SystemMemory 0x428D6018`) and static AML constant buffers; zero `EC.RAM`, zero `EC81`, zero `PATM`.
3. **`DCHU.PK04 0x08`** (`DSDT/DSDT.dsl:102800-102830, 102965-102970, 103011-103029, 103277-103369, 103766`):
   - **Call contract:** `Arg2 = 0x04`, `Arg3 = Package { Buffer([0]=0x08, ...) }`.
   - **Return:** `256`-byte `Buffer` (`0x0100`): when `ECOK == 1`, packs `W000` (`[0..1] = 0x0001`), `W002` (`[2..5] = PSF1`), `W006` (`[6..9] = modified PSF2`), `W00A` (`[10..11] = modified PSF4`), `W00C` (`[12] = PDDS flag`), `W00D` (`[13..14] = PSF5 & mask`), `W00F` (`[15..18] = modified CCF0`), `W013` (`[19..22] = modified CCF1`), `W017` (`[23..26] = 0x0800`); when `ECOK == 0`, zero-filled `Buffer (0x0100)`.
   - **Access path:** `ECOK`-gated (`IntObj`) read of `OGNS` NVS (`PSF0..PSF6`, `CCF0..CCF1`, `LKFG`), ACPI globals (`OSYS`, `PPMF`, `OSCP`), trivial `WMI.PSTG ()` stub (`DSDT/DSDT.dsl:108258-108261`, returns `Zero`), conditional single-byte `^^PC00.LPCB.EC.RINF` read when `(PSF6 & 0x0F)` (`RINF` is at byte offset **`0xDB`** / physical `0xFE0B01DB` of `OperationRegion (RAM, SystemMemory, 0xFE0B0100, 0x0100)` at `DSDT/DSDT.dsl:100739, 100862`, **not** `EC81`; supersedes the earlier `0xDC` offset note), conditional `^^PC00.GFX0.PDDS (0x0300)` (`SSDT4/SSDT4.dsl:2044-2093`, reading Intel IGD OpRegion `IGDM` `SystemMemory ASLB` under `NoLock`), and `CondRefOf (\_SB.PC00.PEG1.PEGP.EJFG)`. Zero `EC81`, zero `ECMD`, zero `PATM`.

**Excluded from the initial Stage 23 candidate set:**
- **`DCHU.PK07`** (`DSDT/DSDT.dsl:103769-103782`): `NotSerialized`, `ECOK`-gated 16-bit read of `EC.RAM.BMD0` (`SystemMemory 0xFE0B0100` at `DSDT/DSDT.dsl:100842`). Although zero-`EC81`/zero-`ECMD`, `BMD0` has no other AML writers or consumers across all 18 tables; `PK07` remains excluded from the initial Stage 23 candidate set unless new evidence establishes a concrete need.
- **`ECMD` / write-path operations:** `PK0C`, `DCHU.PK0D`, `GCMD 0x63/0x64/0x6E–0x71`, all fan setters (`SCMD 0x68/0x69/0x79`, `PK0E`), `BAT0._BTP`, and FlexiCharger setters (`PK04 0x1F`, `SCMD 0x76`) are not zero-write candidates and remain strictly excluded.

### 5.5 Stage 22.2 static closure & Stage 23 authorization boundary

Stage 22.2 resolved all five open static-analysis questions:
1. **ACPI 6.6 `BAT0` inventory (`DSDT/DSDT.dsl:95497-95879`):** `_STA` (`95501`), `_BIX` (`95594`, conditionally compiled under `If ((PSF0 & 0x00010000))` at `95527`; `UPBX` reads `EC.RAM.CYCN` at `95581`), `_BIF` (`95663`), `_BTP` (`95677-95701`, EC mailbox-backed `ECMD 0xF2/0xC0` standard ACPI battery trip-point method — **not** an empty stub and **not** a FlexiCharger control), and `_BST` (`95867`, setting bit 3 `0x08` when `(OSCP & 0x00080000) && (BAEE & 0x02)` at `95732-95738`).
2. **`ECOK` object & semantics:** `Name (ECOK, Zero)` inside `\_SB_.PC00.LPCB.EC` (`DSDT/DSDT.dsl:100605`) — a named ACPICA `IntObj`, **not** a `FieldUnitObj` or EC register (`SSDT1/SSDT1.dsl:44`'s `External(..., FieldUnitObj)` is a per-table `iasl` heuristic). Initialized to `Zero` and updated only once across all 18 tables in `EREG` (`100632`, `If ((PSF0 & One))` -> `ECOK = One`), invoked by `EC._REG (Arg0=0x03, Arg1=One)` (`100718`). Reading `ECOK` performs zero hardware I/O.
3. **Distinct synchronization mechanisms & boundaries:**
   - **`DCHU._DSM` (`Serialized`, `102800`):** ACPICA method-level serialization across callers of `DCHU._DSM` only (`GCMD`, `SCMD`, `PK04`, `CPKG` are `Serialized`; `CC30`, `PK07`, `PK0C`, `PK0D`, `PK0E`, `PK11`, `OCWR` are `NotSerialized`). Does **not** acquire `EC.PATM`.
   - **`WMBB` / `PATM` (`Serialized`, `100176-100312`):** `\_SB_.WMI_.WMBB` explicitly acquires `EC.PATM` (`Mutex (PATM, 0x00)`, `100604`, timeout `0x0064` ms) across every subcommand dispatch.
   - **`ECMD` / `PATM` (`NotSerialized`, `101104-101168`):** Acquires `EC.PATM` (`0x03E8` ms) directly around `EC81` `EmbeddedControl` mailbox registers `FCMD..FBF3` (likewise `ECRD`/`ECWT` at `101075-101102` acquire `PATM` around single accesses).
   - **`acer_clv0001_kbd` mutex (`driver/acer_clv0001_kbd.c:355-492`):** Holds Linux kernel `ctx->lock` across sysfs rate-limiting and `acpi_evaluate_dsm(..., 0x67, ...)` (`SCMD 0x67` -> `ECMD` under `PATM`), serializing sysfs keyboard command writers only (not external `WMBB` or ACPI callers).
   - **Boundary consequence:** Evaluating `GCMD 0x77`, `PK04 0x1E`, or `PK04 0x08` through `DCHU._DSM` never touches `PATM` or `EC81`, cannot deadlock `ctx->lock` or `PATM`, and is serialized against `SCMD 0x67` at `DCHU._DSM` entry; multi-field `EC.RAM`/`EC.RAM3`/`OGNS` writers remain unserialized against `WMBB` and EC SRAM updates.
4. **Data-layer distinction (NVS shadow vs. direct `SystemMemory` vs. live EC state):**
   - **NVS shadow values (`OGNS`, `SystemMemory 0x428D6018`):** Boot-time/OS-mutable BIOS shared memory (`PSF0..6`, `BAEE`, `BSCP`, `BTCP`, `CCF0..1`, `LKFG`) read by `GCMD 0x77`, `PK04 0x1E`, and `PK04 0x08`; can differ from live EC state or reflect BIOS defaults rather than hardware acceptance.
   - **Direct shared `SystemMemory` reads (`EC.RAM 0xFE0B0100`, `IGDM ASLB`):** Shared SRAM / IGD OpRegion reads (`EC.RAM.RINF` in `PK04 0x08`, `IGDM.CPDL..CPL8` in `SSDT4.PDDS`) performed without `EC81` port I/O or `ECMD` handshakes.
   - **Live EC state (`EC81` `FCMD..FBF3` via `ECMD`):** Active EC firmware registers/commands; never invoked by `GCMD 0x77`, `PK04 0x1E`, or `PK04 0x08`. Live EC acceptance and behavior for FlexiCharger (`ECMD 0xCB`) and fans (`ECMD 0xC1`/`0xD7`/`0xC0`) remain unverified.

### 5.7 Stage 23A Runtime Closure & Stage 23B.1 Safety Block (Stage 28 Summary)

Stage 23 runtime investigation outcomes are consolidated in full in [docs/ACPI_RUNTIME_INVESTIGATION.md](ACPI_RUNTIME_INVESTIGATION.md):

- **Stage 23A.1.3 (`GCMD 0x77` — Operator-Reported Runtime Result):** External single-shot probe (`stage23a1_gcmd77_probe_v3.ko`, SHA-256 `aebcbb90f5a6dd42a86b451442f1e961cf0f3f0dfad4d1674fff6cc3572d3eca`) evaluated fresh `_STA = 0x0F` and `_DSM 0x77` -> `AE_OK`, returning `Integer 0x00504601` (`BAEE >> 1 = 0x01`, `BSCP = 70`, `BTCP = 80`).
- **Stage 23A.2.2 (`PK04 0x1E` — Verbatim Captured Log):** External single-shot probe (`stage23a2_pk04_1e_probe_v2.ko`, SHA-256 `09ef7e509eec1173f9bbfc2cfa46c1dc9edd49d6def5e214ffede7bd20a97385`) returned `AE_OK` and `256`-byte `Buffer` (`POPULATED_CASE_1E`: `01 46 50`, matching `FCSC` `40..90` and `FCTC` `60..100`, all unassigned bytes zero).
- **Epistemic scope:** Both Stage 23A probes confirm **cross-query `OGNS` NVS shadow consistency (`1, 70, 80`) only**. Live EC state (`EC81`/`ECMD`) and physical battery charge-limit enforcement remain **unverified**, and FlexiCharger setters (`PK04 0x1F`, `SCMD 0x76`, `ECMD 0xCB`) remain **locked/disabled**.
- **Stage 23B.1 / 23B.1.1 (`PK04 0x08` — STOP):** Classified `STATIC_READ_ONLY_BUT_RUNTIME_RACE_OR_POINTER_VALIDITY_UNRESOLVED` and closed without probe build or execution due to unreserved `EC.RAM.RINF` (`0xFE0B01DB`, omitted by `PNP0C02:02`/E820) and unguarded `NoLock` `IGDM`/`ASLB` reads concurrent with `i915`. All remaining `ECMD`, fan, and FlexiCharger setter paths remain unauthorized.

### 5.6 Evidence location

The operator's private offline audit directory (`/home/zeth/acpi-audit-a715-59g-WcgNEm`, with individual per-table `iasl -d` outputs under `individual-disassembly-20261123-123004/`) is outside this repository. Combined `iasl -da` failed with `AE_ALREADY_EXISTS` (`SADX`, `\GPOM`), leaving the top-level `DSDT.dsl` empty (`0` bytes), whereas all 18 individual table disassemblies (`DSDT` + `SSDT1–SSDT17`) completed with `exit=0` and populated `AML_SYMBOL_INDEX.txt` (`5,037` lines). Exact AML line references in §5.3–§5.5 are anchored to those individual `.dsl` files.
