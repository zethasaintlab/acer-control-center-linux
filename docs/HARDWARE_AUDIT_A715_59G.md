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
| `cap.keyboard.lighting` | Physical Keyboard Backlight & RGB Zones | `/sys/class/leds/*kbd_backlight*` (`brightness`, `max_brightness`, `multi_intensity`) | Absent (`ABBC0F6D` `object_id=BB` unbound) | `NotDetected` | Full unrestricted HSV/HSL/RGB/Hex Color Picker & Profile Studio implemented; routes to Backend A (`/sys/class/leds/*kbd_backlight*`, distinguishing Monochrome vs Multicolor RGB), Backend B (`ClevoKeyboardBackend` `WMBB` `0x67` verified bridge in mock/driver fixtures), or Backend C (`FirmwareHotkeyOnly` Preview Mode on physical `Kelilu_RTH`). |

---

## 5. Stage 12 — Keyboard Backlight Reverse Engineering & Multi-Backend Architecture (`Kelilu_RTH`)

### 5.1 End-to-End Execution Path (`GUI -> Controller -> D-Bus -> Polkit -> Daemon -> HAL -> Hardware`)
1. **GUI (`src/ui/qml/KeyboardStudioView.qml`):** Renders the interactive 4-zone chassis deck visualizer, unrestricted HSV/HSL wheel, sRGB sliders, `#RRGGBB` hex input, custom swatches, and hardware capability/domain separation banner.
2. **Controller (`src/app/ControlCenterController.cpp`):** Maintains independent keyboard lighting state (`m_currentKbdProfile`, `m_kbdRgb`, `m_kbdHsv`, `m_kbdHsl`) completely decoupled from `ThemeEngine::accentHex()`. When `applyKeyboardLightingToHardware()` is invoked in physical mode, it issues a synchronous D-Bus method call `SetKeyboardLighting(mode, brightness_pct, zones_hex_csv)` to `io.github.zethasaintlab.AcerControlCenter1`.
3. **Polkit & Daemon (`src/daemon/DaemonServiceObject.cpp`, `src/daemon/PolkitAuthorizer.cpp`):** Verifies the caller against Polkit action `io.github.zethasaintlab.acercontrol.set-keyboard-lighting` (`allow_active=yes`, `allow_any=no`, `allow_inactive=no`). Denied callers fail closed (`ERR_POLKIT_UNAUTHORIZED`) with zero hardware writes.
4. **HAL Discovery & Control (`src/hal/HardwareDiscovery.cpp`, `src/hal/ControlOperations.cpp`, `src/hal/ClevoKeyboardBackend.cpp`):**
   - Evaluates the 3-tier backend priority (`Backend A: LinuxLedsMulticolorRgb / LinuxLedsMonochrome` $\rightarrow$ `Backend B: ClevoWmiMethodBb` $\rightarrow$ `Backend C: FirmwareHotkeyOnly`).

### 5.2 Physical WMI & ACPI Topology Discovered on `A715-59G` (`Kelilu_RTH`, BIOS `V1.17`)

| Bus / Device Path | Attributes Observed | Kernel Driver Binding | Functional Role |
|---|---|---|---|
| `/sys/bus/wmi/devices/ABBC0F6B-8EA1-11D1-00A0-C90629100000-2` | `DEVTYPE=event`, `notify_id=D0`, `instance_count=1`, `expensive=0` (under `\_SB_.WMI_`) | Unbound | Clevo WMI hotkey/EC event notification block (`0xD0`) |
| `/sys/bus/wmi/devices/ABBC0F6C-8EA1-11D1-00A0-C90629100000-3` | `DEVTYPE=event`, `notify_id=D1`, `instance_count=1`, `expensive=0` (under `\_SB_.WMI_`) | Unbound | Clevo WMI secondary event notification block (`0xD1`) |
| `/sys/bus/wmi/devices/ABBC0F6D-8EA1-11D1-00A0-C90629100000-1` | `DEVTYPE=method`, `object_id=BB`, `instance_count=1`, `expensive=0` (under `\_SB_.WMI_`) | Unbound (no in-tree module in `modules.alias`) | Maps to ACPI control method `\_SB_.WMI_.WMBB(instance, method_id, arg)` |
| `/sys/bus/wmi/devices/F6CB5C3C-9CAE-4EBD-B577-931EA32A2CC0-0` | `DEVTYPE=method`, `object_id=MX`, `instance_count=1` (under `\_SB_.PC00.WMI1`) | Bound to `mxm-wmi` | NVIDIA MXM GPU WMI method (`WMMX`) |
| `/sys/bus/acpi/devices/CLV0001:00` | `hid=CLV0001`, `path=\_SB_.DCHU`, `status=15` | Unbound | Clevo DCHU Windows Control Center ACPI bridge |
| `/sys/bus/acpi/devices/CLV0002:00` | `hid=CLV0002`, `path=\_SB_.DCHP`, `status=15` | Unbound | Clevo DCHP Windows Control Center ACPI bridge |
| `/sys/bus/acpi/devices/10251783:00` | `hid=10251783`, `path=\_SB_.PC00.AEVD` | Unbound | Acer vendor ACPI event device (`AEVD`) |
| `/sys/bus/acpi/devices/PNP0C09:00` | `hid=PNP0C09`, `path=\_SB_.PC00.LPCB.EC__`, `GPE=0x6e`, `ports=0x62/0x66` | Bound to `acpi-ec` | Embedded Controller managing thermal fans & `Fn` keyboard illumination |
| `/sys/bus/acpi/devices/MSFT0001:00` | `hid=MSFT0001` (`PNP0303`), `path=\_SB_.PC00.LPCB.PS2K` | Bound to `i8042` / `atkbd` | Internal PS/2 keyboard (`input4::capslock`, `numlock`, `scrolllock` only) |

### 5.3 Clevo WMBB (`ABBC0F6D-8EA1-11D1-00A0-C90629100000`) Protocol & Hardware Safety Gate
- Cross-referencing upstream Linux Clevo drivers (`tuxedocomputers/tuxedo-drivers` `clevo_wmi.c` / `clevo_keyboard.c`, `Lekensteyn/acpi-stuff`, `gtozzi/clevo-backlight`), Clevo `WMBB` (`object_id=BB`) uses:
  - **Method ID `0x67` (`CLEVO_CMD_SET_KB_RGB_LEDS`):** Accepts a 32-bit `uint32_t` sub-command word:
    - Zone 0 (Left): `0xF0000000 | (B << 16) | (R << 8) | G` (`BRG` byte order)
    - Zone 1 (Center): `0xF1000000 | (B << 16) | (R << 8) | G`
    - Zone 2 (Right): `0xF2000000 | (B << 16) | (R << 8) | G`
    - Brightness (`0..255`): `0xF4000000 | (brightnessByte & 0xFF)`
    - Mode / State: `0x00000000` (Off), `0x10000000` (Static On), `0x1002A000` (Breathing), `0x33010000` (Wave/Cycle)
- **Why Physical Execution of `WMBB` Remains Disabled by Default on `Kelilu_RTH`:**
  1. Linux `wmi.c` does not expose a user-space method execution node under `/sys/bus/wmi/devices/ABBC0F6D-8EA1-11D1-00A0-C90629100000-1/` unless a kernel driver binds to the GUID.
  2. Official Acer `Aspire 7 A715-59G` regional SKUs ship with either a **monochrome white single-zone** backlight (controlled by EC `Fn` shortcuts) or a **multi-zone** backlight. Without decompiling the root-only DSDT (`0400`) or installing out-of-tree kernel modules (`acpi_call` / `tuxedo-drivers`), issuing raw ACPI/WMI writes on physical hardware is prohibited by our Hardware Write Safety policy.
  3. Therefore, `ClevoKeyboardBackend` (`src/hal/ClevoKeyboardBackend.h` / `.cpp`) implements the complete, bounds-checked `0x67` packet encoder and strict DMI (`Acer` / `Aspire A715-59G` / `RTH` / `Kelilu_RTH`) + BIOS (`V1.17`) + WMI (`ABBC0F6D`, `object_id=BB`, `DEVTYPE=method`) gate, keeping physical hardware in safe `FirmwareHotkeyOnly` / Preview Mode while enabling deterministic verification against mock WMI response fixtures.

