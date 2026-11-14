# Hardware Reconnaissance & Capability Audit — Acer Aspire 7 A715-59G (`Kelilu_RTH`)

**Audit Date:** 2026-11-08  
**Target Platform:** Fedora Linux 45 (`7.2.15-300.fc45.x86_64`)  
**Target Machine:** Acer Aspire 7 `A715-59G` (`board_name: RTH`, `board_vendor: Kelilu_RTH`, BIOS `INSYDE Corp. V1.17` `06/30/2026`)

---

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
| Discrete GPU | NVIDIA GeForce RTX 3050 6GB Laptop GPU `GA107BM [10de:25ac]` (`Subsys 1025:1900`, `nvidia 615.78.08`) | `lspci -nnk`, `nvidia-smi` |
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
- `PNP0C09:00` (`\_SB_.PC00.LPCB.EC__`, bound to `acpi-ec`)
- `PNP0C0A:00` (`\_SB_.BAT0`, bound to `acpi-battery`)
- `ACPI0003:00` (`\_SB_.AC__`, bound to `ac`)
- `PNP0C14:00` (`\_SB_.PC00.WMI1`, bound to `acpi-wmi`)
- `PNP0C14:01` (`\_SB_.WMI_`, bound to `acpi-wmi`)
- `INT340E:00` (`\_SB_.PTID`) plus `processor_thermal_device_pci` (`00:04.0`)

---

## 3. Acer Software & Linux Project Comparison

| Original Feature | Purpose | A715-59G Hardware Support | Linux (`7.2.15-300.fc45`) Support | Implementation Feasibility |
|---|---|---|---|---|
| **System Telemetry Overview** (Acer Control Center / AcerSense) | Real-time CPU, GPU, memory, storage, thermal & battery overview | **Yes** | **Fully Supported** (`coretemp`, `/proc`, `nvme`, `BAT0`, `powercap`, NVML) | **Implemented (Read-Only + RAPL via Daemon)** |
| **Performance Profiles** (Silent / Normal / Performance) | Coordinate system thermal/power profile and CPU P-state policy | **Yes** | **Fully Supported** via Fedora `tuned` / `tuned-ppd` (`net.hadess.PowerProfiles`) and `intel_pstate` (`scaling_governor`, `energy_performance_preference`, `no_turbo`, `min/max_perf_pct`) | **Implemented (Full Read/Write with Readback)** |
| **Thermal & Fan Control** (NitroSense / PredatorSense inspiration) | Monitor sensor temperatures, fan RPM, and configure fan curves | **Thermal sensors Yes**; EC (`PNP0C09:00`) controls dual cooling fans automatically | **Thermal Sensors Read-Only Supported** (`hwmon1..6`, `thermal_zone0..7`); **Fan RPM & PWM Not Detected** in `/sys/class/hwmon` | **Thermal Monitoring + Safe Fan Curve Studio:** Live thermal tracking + curve editor that binds to `hwmon/pwm*` when present/mocked and clearly reports `Firmware Auto (No kernel PWM bound)` on physical `Kelilu_RTH` |
| **Keyboard Lighting Studio** | Control backlight brightness, timeout, or zone RGB colors | **SKU-Dependent** (`A715-59G` commonly ships with single-color backlight controlled by EC/`Fn` shortcuts; some Clevo ODM boards use `ABBC0F6D`) | **Not Detected in `/sys/class/leds/`** (`no *kbd_backlight*` class device exposed by default kernel) | **Full Unrestricted Color Picker + Hardware-Aware Routing:** Full HSV/HSL/RGB/Hex Studio and multi-zone profile persistence; routes to `/sys/class/leds/*kbd_backlight*` when detected/mocked and displays explicit `Not Detected` hardware badge when absent |
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
| `cap.thermal.fans` | Physical Fan RPM & Manual/Curve PWM Override | `/sys/class/hwmon/*/fan*_input`, `/sys/class/hwmon/*/pwm*` | Absent | `NotDetected` | Firmware EC manages fans automatically. UI shows live thermal telemetry + clearly labeled Curve Preview/Simulation mode unless a verified `hwmon` PWM device is present or mocked. |
| `cap.gpu.nvidia` | NVIDIA RTX 3050 6GB Telemetry, Clocks, P-State & Throttling | Dynamic `dlopen("libnvidia-ml.so.1")` (`0000:01:00.0`) | User RO | `ReadOnly` | Verified (`driver 615.78.08`, `6144 MiB`, `60W` default / `75W` max). Includes `P8` idle power rail-gate anomaly detection (`>1.5x max_limit` in `P8` flagged as `Rail Gated`). |
| `cap.battery.telemetry` | Battery Charge %, Voltage, Current, Wattage & Est. Capacity Ratio | `/sys/class/power_supply/BAT0/*`, `/sys/class/power_supply/AC/online` | `0444` | `ReadOnly` | Verified (`charge_full 1956000` / `charge_full_design 3410000` → `57.4%` fuel-gauge capacity ratio; serial redacted). |
| `cap.battery.limit` | Battery Charge End Threshold (e.g. 80% Care Mode) | `/sys/class/power_supply/BAT0/charge_control_end_threshold` | Absent | `NotDetected` | Supported in HAL & daemon when sysfs node exists; disabled with clear hardware explanation on `Kelilu_RTH`. |
| `cap.backlight.display` | Internal eDP Panel Brightness | `/sys/class/backlight/intel_backlight` (`max_brightness: 48000`) & `org.freedesktop.login1.Session.SetBrightness` | `0644` / D-Bus | `FullySupported` | Verified (`intel_backlight` on `card1-eDP-1`). |
| `cap.keyboard.lighting` | Physical Keyboard Backlight & RGB Zones | `/sys/class/leds/*kbd_backlight*` (`brightness`, `max_brightness`, `multi_intensity`) | Absent (`ABBC0F6D` unverified) | `NotDetected` | Full unrestricted HSV/HSL/RGB/Hex Color Picker & Profile Studio implemented; binds to `/sys/class/leds/*kbd_backlight*` when present/mocked and never fabricates physical hardware writes when absent. |
