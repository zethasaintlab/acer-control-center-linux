# ACPI Runtime & Static Safety Investigation — Acer Aspire A715-59G (`Kelilu_RTH`)

- **Target Machine:** Acer Aspire 7 `A715-59G` (`sys_vendor: Acer`, `product_name: Aspire A715-59G`, `board_name: RTH`, `board_vendor: Kelilu_RTH`, `bios_vendor: INSYDE Corp.`, `bios_version: V1.17`)
- **Target OS / Kernel:** Fedora Linux 45 (`7.3.3-300.fc45.x86_64`)
- **Target ACPI Scope:** `\_SB_.DCHU` (`HID CLV0001`, `_DSM` UUID `93f224e4-fbdc-4bbf-add6-db71bdc0afad`)
- **Last Consolidated:** 2026-11-23 (Stage 28)

> **Boundary Notice:** The standalone diagnostic kernel probe modules summarized here (`stage23a1_gcmd77_probe_v3.ko` and `stage23a2_pk04_1e_probe_v2.ko`) were single-shot operator-authorized diagnostic probes built and run in an external audit workspace outside this repository. Their `.ko` binaries are **not** included in this repository or RPM package, and **no** physical FlexiCharger, fan, or unverified ACPI/WMI controls are enabled in `acer-control-center` or `acer-control-daemon`.

---

## 1. Epistemic Classification Vocabulary

To avoid conflating ACPI transport return values with live physical hardware enforcement, this investigation strictly separates seven evidence tiers:

| Evidence Class | Meaning in This Project |
|---|---|
| **Physically verified** | Direct physical device behavior observed on this exact laptop (e.g., visual keyboard backlight illumination at `#12A7E3` level `4`). |
| **Runtime verified (Verbatim Log)** | Standalone probe or driver invoked the live interface on `7.3.3-300.fc45.x86_64`, and complete verbatim `journalctl -k` output was archived. |
| **Operator-reported runtime result** | Standalone probe executed under explicit one-shot operator authorization and returned decoded output/exit status, without a saved verbatim `journalctl -k` dump in the audit report. |
| **Static AML candidate** | Offline `iasl -d` disassembly (`DSDT` + `SSDT1–SSDT17`) identified the AML path, arguments, and locking behavior; runtime behavior is unproven. |
| **Safety-blocked (Unresolved)** | Static read/race/resource-ownership analysis identified unresolved runtime safety risks; probe build and execution are explicitly stopped. |
| **Not detected** | Standard Linux kernel sysfs/hwmon/power-supply ABI is absent on this host. |
| **Mock / Preview** | Deterministic software fixture (`--mock-hardware`) or visual UI preview only. |

---

## 2. Stage 23A.1.3 — One-Shot `DCHU._DSM` `GCMD 0x77` Probe

### Artifact & Static Guard Provenance
- **Module:** `stage23a1_gcmd77_probe_v3.ko` (external private audit workspace; not shipped in repo)
- **SHA-256:** `aebcbb90f5a6dd42a86b451442f1e961cf0f3f0dfad4d1674fff6cc3572d3eca`
- **Fail-closed 6-field exact DMI guard:**
  - `sys_vendor = "Acer"`, `product_name = "Aspire A715-59G"`, `board_name = "RTH"`, `board_vendor = "Kelilu_RTH"`, `bios_vendor = "INSYDE Corp."`, `bios_version = "V1.17"`
- **Symbol provenance audit (Stage 23A.1.3.1):** All 22 external symbols resolved against target kernel `Module.symvers` (`GPL`-compatible), including `__ubsan_handle_out_of_bounds`.
- **Call contract:** Fresh `acpi_bus_get_status_handle` (`_STA` check requiring `0x0f`), followed by one `DCHU._DSM` (`rev=0`, `func=0x77`, `Arg3 = Package { Buffer(256){0} }`) evaluation dispatching internally to `DCHU.GCMD`.

### Runtime Outcome (**Operator-Reported Runtime Result**)
*Provenance note:* Recorded from explicit operator-authorized single-run execution on `7.3.3-300.fc45.x86_64`; a verbatim `journalctl -k` log capture was not separately archived for `v3`.

- **Six-field DMI identity:** Passed
- **ACPI metadata preflight:** `CLV0001` (`\_SB_.DCHU`) cached `_STA` bits 0–3 all set (`present=1`, `enabled=1`, `show_in_ui=1`, `functional=1`, `dynamic_status=1`)
- **Fresh `_STA`:** `0x000000000000000f` (`15`)
- **`_DSM` transport status:** `AE_OK (0x00000000)`
- **Returned AML type:** `ACPI_TYPE_INTEGER`
- **Raw `GCMD` return integer:** `0x00504601` (`5259457` decimal)
- **Decoded `OGNS` ACPI NVS shadow fields (`DSDT.dsl:106591-106597`):**
  - `BAEE >> 1` (bits `[7:0]`): `0x01`
  - `BSCP` (bits `[15:8]`): `70` (`0x46`)
  - `BTCP` (bits `[23:16]`): `80` (`0x50`)
  - `byte3` (bits `[31:24]`): `0x00`
  - `upper32` (bits `[63:32]`): `0x00000000`
- **Lifecycle:** `insmod` exit `0`; `rmmod stage23a1_gcmd77_probe_v3` succeeded; both probe and `acer_clv0001_kbd` unloaded afterward.

---

## 3. Stage 23A.2.2 — One-Shot `DCHU._DSM` `PK04 0x1E` Probe

### Artifact & Static Call-Graph Verification (Stage 23A.2.1 / 23A.2.2)
- **Module:** `stage23a2_pk04_1e_probe_v2.ko` (external private audit workspace; not shipped in repo)
- **SHA-256:** `09ef7e509eec1173f9bbfc2cfa46c1dc9edd49d6def5e214ffede7bd20a97385`
- **Static AML closures prior to runtime approval:**
  - `SCLT` and `TCLT` (`DSDT.dsl:103607-103620`) confirmed as named `CreateField` `BufferField` targets inside local `OBUF` — **not** AML methods. Zero helper AML methods are invoked beyond `_DSM` -> `PK04`.
  - `ECOK` (`DSDT.dsl:100605`) confirmed as an `IntObj` read (zero hardware I/O).
  - `OGNS` (`SystemMemory 0x428D6018`) uses `Field (OGNS, AnyAcc, Lock, Preserve)`: ACPICA acquires/releases the ACPI Global Lock per individual field access (`BAEE`, `BSCP`, `BTCP`), making the multi-field read **non-atomic** across calls and void/fail-open in AML if Global Lock acquisition were ever contended.
  - Pure-C parser self-test (`test_pk04_1e_response`) passed `8/8` cases before build; `23/23` external kernel symbols audited clean.

### Runtime Outcome (**Runtime Verified — Verbatim Kernel Log Captured**)
Captured verbatim from `journalctl -k` on `Nov 23 18:55:47` under `7.3.3-300.fc45.x86_64`:

```text
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: exact 6-field DMI identity verified: sys_vendor='Acer' product_name='Aspire A715-59G' board_name='RTH' board_vendor='Kelilu_RTH' bios_vendor='INSYDE Corp.' bios_version='V1.17'
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: metadata preflight passed: target=\_SB.DCHU hid=CLV0001 dynamic_status=1 cached_status={present=1,enabled=1,ui=1,func=1}
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: fresh _STA passed: raw_sta=0x000000000000000f (15) required_bits=0x0f
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: acpi_eval_status=AE_OK (0x00000000) return_type=buffer length=256
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: raw_buf: 00000000: 01 46 50 00 00 00 00 00 00 00 00 00 00 00 00 00  .FP.............
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: raw_buf: 00000010: 28 32 3c 46 50 5a 00 00 00 00 00 00 00 00 00 00  (2<FPZ..........
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: raw_buf: 00000020: 3c 46 50 5a 64 00 00 00 00 00 00 00 00 00 00 00  <FPZd...........
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: raw_buf: 00000030: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
... (offsets 0x0040–0x00f0 all zero: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00) ...
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: classification=POPULATED_CASE_1E ogns_nvs_shadow={baee_shr1=0x01 (1), bscp=70 (0x46), btcp=80 (0x50)} fcsc_const_0x10_0x1f=[28 32 3c 46 50 5a 00 00 00 00 00 00 00 00 00 00] fctc_const_0x20_0x2f=[3c 46 50 5a 64 00 00 00 00 00 00 00 00 00 00 00] unassigned_nonzero={0x03_0x0f=0, 0x30_0xff=0} note="OGNS ACPI NVS shadow and firmware-defined constant arrays only; live EC state unverified"
Nov 23 18:55:47 nonexist kernel: stage23a2_pk04_1e_probe_v2: unloaded standalone Stage 23A.2.2 PK04(0x1E) probe v2
```

- **Decoded buffer summary (`POPULATED_CASE_1E`):**
  - Byte `0x00` (`BAEE >> 1`): `0x01` (`1`)
  - Byte `0x01` (`BSCP`): `70` (`0x46`)
  - Byte `0x02` (`BTCP`): `80` (`0x50`)
  - Bytes `0x10..0x1F` (`FCSC` start presets): `40, 50, 60, 70, 80, 90` (`0x28 0x32 0x3C 0x46 0x50 0x5A`), zero-padded
  - Bytes `0x20..0x2F` (`FCTC` stop presets): `60, 70, 80, 90, 100` (`0x3C 0x46 0x50 0x5A 0x64`), zero-padded
  - Padding (`0x03..0x0F`, `0x30..0xFF`): All `0x00` (`unassigned_nonzero = 0`)
  - ACPICA Global Lock errors/warnings in log: None observed
  - Lifecycle: `insmod` exit `0`, `rmmod` exit `0`; probe and `acer_clv0001_kbd` unloaded afterward.

---

## 4. Stage 23A Cross-Query Synthesis & Epistemic Boundaries

1. **ACPI transport success (`AE_OK`):** Both `DCHU._DSM` (`0x77`) and `DCHU._DSM` (`0x04`, subcommand `0x1E`) executed cleanly on `\_SB_.DCHU` with `AE_OK`.
2. **Valid AML return objects:** Firmware returned a non-sentinel `Integer` (`0x00504601`) and a zero-padded `256`-byte `Buffer` (`POPULATED_CASE_1E`).
3. **Cross-query NVS consistency (`BAEE >> 1 = 1`, `BSCP = 70`, `BTCP = 80`):** Both independent AML paths read identical values from the `OGNS` ACPI NVS shadow region (`0x428D6018`).
4. **Firmware constant arrays:** `PK04 0x1E` returned the exact BIOS-defined `FCSC` (`40..90`) and `FCTC` (`60..100`) constant preset bytes observed statically in `DSDT`.
5. **Live EC state NOT observed:** Neither probe executed `ECMD` or read `EC81` mailbox registers. Whether the Embedded Controller tracks or honors `(70, 80)` dynamically in firmware was **not** observed.
6. **Physical charging-limit enforcement NOT tested:** Physical battery charging behavior at `70%` / `80%` on `BAT0` was **not** tested.
7. **Setter paths remain strictly LOCKED / UNAUTHORIZED:** FlexiCharger write paths (`PK04 0x1F`, `SCMD 0x76`, `ECMD 0xCB`) write `OGNS` NVS independently of EC command completion and lack threshold-order validation and rollback (`docs/HARDWARE_AUDIT_A715_59G.md` §5.3). Battery charge limiting remains disabled (`cap.battery.limit = NotDetected`) in `acer-control-center` unless `BAT0/charge_control_end_threshold` exists.

---

## 5. Stage 23B.1 / 23B.1.1 — `PK04 0x08` Safety Closure (**STOP**)

**Final Classification:** `STATIC_READ_ONLY_BUT_RUNTIME_RACE_OR_POINTER_VALIDITY_UNRESOLVED`

**Engineering Decision:** **STOP.** Do **not** build, load, or execute a kernel probe or product code path for `DCHU.PK04` subcommand `0x08`.

### Complete Static AML Call-Graph Closure
- **Maximum conditional AML call count (`7`):** `DCHU._STA`, `DCHU._DSM`, `DCHU.CC30`, `DCHU.PK04`, `DCHU.PSTG`, `\_SB_.WMI.PSTG ()` (`DSDT.dsl:108258-108261`, returns `Zero`), and conditional `^^PC00.GFX0.PDDS (0x0300)` (`SSDT4.dsl:2044-2093`).
- **`Notify` operations in `Case (0x08)` and helpers:** `0`
- **`EC81` / `ECMD` / `PATM` usage:** `0`

### Unresolved Runtime Blockers Identified in Stage 23B.1 / 23B.1.1

1. **Unreserved `EC.RAM.RINF` `SystemMemory` Address (`0xFE0B01DB`):**
   - When `(PSF6 & 0x0F)` is non-zero, `PK04 0x08` performs a conditional 8-bit `ByteAcc` read of `^^PC00.LPCB.EC.RINF` inside `OperationRegion (RAM, SystemMemory, 0xFE0B0100, 0x0100)` (`DSDT.dsl:100739, 100862`).
   - Exact `DSDT/DSDT.dsl:100857-100863` field layout: `Offset (0xD7)` is followed by four contiguous 8-bit fields (`DTHL` at `0xD7`, `DTBP` at `0xD8`, `AIRP` at `0xD9`, `WINF` at `0xDA`), placing `RINF` at **byte offset `0xDB`** (bit offset `0x06D8`, physical address **`0xFE0B01DB`**) before a 1-byte gap (`0xDC`) and `Offset (0xDD)` (`INF2`). This supersedes Stage 22.2's earlier `0xDC` offset note.
   - OS memory-resource cross-check (`iomem`, `/sys/firmware/memmap`, ACPI `PNP0C02` `_CRS`): `0xFE0B0100–0xFE0B01FF` lies inside an **unreserved E820 hole** (`[gap 0xd0000000–0xfed1ffff]`) and is **omitted** by `PNP0C02:02` (`\_SB_.PRRE`), which reserves `0xfe050000–0xfe0affff` and `0xfe0d0000–0xfe0fffff` around `0xfe0b0000–0xfe0cffff` without reserving `0xFE0B0100`. No ACPI resource or kernel driver claims ownership of `0xFE0B01DB`.

2. **Intel IGD OpRegion (`IGDM` / `SANV.ASLB`) `NoLock` & Pointer-Validity Hazard:**
   - Conditionally executes `^^PC00.GFX0.PDDS (0x0300)` (`SSDT4.dsl:2044-2093`), which maps `OperationRegion (IGDM, SystemMemory, ASLB, ...)` (`SANV.ASLB = 0x428E2418` observed in static table state) and reads offsets `0x140–0x15F` (`CPDL..CPL8`) inside `Field (IGDM, AnyAcc, NoLock, Preserve)`.
   - `SSDT4.PDDS` does **not** check `ASLB != 0` or OpRegion signature validity prior to dereferencing `IGDM`, while `i915` concurrently maps the IGD OpRegion (`MEMREMAP_WB`) under its own lock (`NoLock` in AML leaves concurrent reads unserialized against GPU/firmware updates or stale `ASLB` state).

### Continuing Exclusions
- `DCHU.PK07` (`EC.RAM.BMD0` in `0xFE0B0100`) remains excluded.
- All `ECMD`/write-path operations (`PK0C`, `DCHU.PK0D`, `GCMD 0x63/0x64/0x6E–0x71`, fan setters `SCMD 0x68/0x69/0x79`, `PK0E`, `BAT0._BTP`, and FlexiCharger setters `PK04 0x1F`, `SCMD 0x76`) remain excluded and unauthorized.
