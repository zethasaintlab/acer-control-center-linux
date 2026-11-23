# Hardware Reconnaissance & Capability Audit — Acer Aspire 7 A715-59G (`Kelilu_RTH`)

**Audit Date:** 2026-11-08  
**Target Platform:** Fedora Linux 45 (`7.2.15-300.fc45.x86_64`)  
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
- Stage 22.1 found BAT0._BTP is an empty stub in this BIOS build; do not treat it as a FlexiCharger control.

**Product posture:** battery telemetry only. Do not expose or invoke a charge-limit setter until runtime behavior, EC acceptance, constraints, readback and rollback are proven.

### 5.4 Candidate zero-mailbox queries and pending safety review

Stage 22.1 identified these as candidates for a separate, explicitly authorized runtime review:
- DCHU GCMD 0x77.
- DCHU PK04 0x1E.
- DCHU PK04 0x08.
- Optionally PK07 if its BMD0 value is needed.

PK04 0x08 may conditionally read EC.RINF. PK04 0x1E is gated by ECOK; Stage 22.2 must resolve the exact ECOK object/access type. DCHU._DSM serialization and the interaction with driver-local keyboard locks and WMBB/PATM must also be reviewed.

PK0C, DCHU.PK0D, GCMD 0x63/0x64/0x6E, all fan setters, PK0E and FlexiCharger setters are not zero-write candidates.

### 5.5 Pending work and authorization

Stage 22.2 was not completed in the reported workflow. It must:
1. Correct the ACPI 6.6 battery method inventory.
2. Resolve ECOK's declaration and hardware-access semantics.
3. Verify Serialized method semantics and synchronization boundaries.
4. Validate exact DCHU function/package contracts and output types.
5. Finalize the zero-mailbox candidate matrix.

Stage 23 runtime queries are not authorized by this static audit. A separate user authorization is required before evaluating any DCHU _DSM function.

### 5.6 Evidence location

The operator's private offline audit directory is outside this repository. It contains the extracted DSDT/SSDT binaries, disassembly outputs and reports. Do not assume those binary artifacts are committed here. Exact AML line references should be taken from the operator's Stage 22/22.1 report and rechecked against the current private audit files before implementation.
