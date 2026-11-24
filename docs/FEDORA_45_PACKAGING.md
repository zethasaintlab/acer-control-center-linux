# Fedora Linux 45 Packaging, Installation & Privilege Architecture

**Target Distribution:** Fedora Linux 45 (`fc45`)  
**Target Desktop Environments:** KDE Plasma 6 (Wayland) & Hyprland (Wayland)  
**Target Hardware:** Acer Aspire 7 A715-59G (`board_name: RTH`, `board_vendor: Kelilu_RTH`)  
**Package License:** `GPL-2.0-or-later AND (GPL-3.0-or-later WITH GPL-3.0-linking-source-exception) AND CC0-1.0`

---

## 1. Fedora 45 Native Dependencies

All build and runtime dependencies are satisfied from standard Fedora 45 repositories (zero third-party COPR repositories required for building or running the application):

### Build-Time Dependencies (`BuildRequires`)
```bash
sudo dnf install \
    cmake \
    ninja-build \
    gcc-c++ \
    rpm-build \
    desktop-file-utils \
    systemd-rpm-macros \
    qt6-qtbase-devel \
    qt6-qtdeclarative-devel \
    polkit-devel
```

### Runtime Dependencies (`Requires` / `Recommends`)
- `qt6-qtbase`, `qt6-qtdeclarative`, `qt6-qtwayland` (Wayland client integration for KDE Plasma 6 & Hyprland)
- `polkit`, `dbus-common`, `hicolor-icon-theme`
- `tuned-ppd` (recommended; provides `net.hadess.PowerProfiles` D-Bus service on Fedora 45)
- `nvidia-driver-cuda-libs` (`libnvidia-ml.so.1`, optional; loaded dynamically at runtime via `dlopen` with full graceful fallback if absent)

---

## 2. Building Binary & Source RPMs Locally

You can build the Fedora 45 `.rpm` and `.src.rpm` packages in an unprivileged local build tree (`build/rpmbuild`) without root privileges:

```bash
mkdir -p build/rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

# Create source archive matching the spec Version (1.0.0)
tar --exclude='./build' --exclude='./.git' \
    --transform='s,^\.,acer-control-center-1.0.0,' \
    -czf build/rpmbuild/SOURCES/acer-control-center-1.0.0.tar.gz .

cp packaging/fedora/acer-control-center.spec build/rpmbuild/SPECS/

# Build SRPM and binary RPM (runs %check with desktop-file-validate and ctest)
rpmbuild --define "_topdir $(pwd)/build/rpmbuild" -ba build/rpmbuild/SPECS/acer-control-center.spec
```

### Inspecting the Resulting RPM
*(Replace `1.0.0-4` with the built release from `packaging/fedora/acer-control-center.spec`; release progression is `1.0.0-1` baseline -> `1.0.0-2` CLV0001 presets -> `1.0.0-3` Stage 18 custom RGB -> `1.0.0-4` Stage 21/21.1 WIP & hardening.)*
```bash
rpm -qip build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-4.fc45.x86_64.rpm
rpm -qlp build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-4.fc45.x86_64.rpm
rpm -qRp build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-4.fc45.x86_64.rpm
```

### 2.3 Complete Fedora 45 (DNF5) RPM Installation, Verification & Rollback Procedure

Fedora 45 uses `dnf5` (`dnf5 5.4.7.0` observed on host). Because `acer-control-center` is installed directly from local `.rpm` files (`@commandline` repository) rather than a remote Fedora mirror, rollback must use a preserved local prior `.rpm` (for example `1.0.0-3.fc45` or `1.0.0-2.fc45`) rather than assuming `dnf5 history undo` can fetch an older local package from a remote repo.

#### Step 1 — Record Pre-Upgrade Baseline & Verify Rollback Artifact (Read-Only)
```bash
# Record currently installed NEVRA, build timestamp, and installed file digests:
rpm -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH} (built %{BUILDTIME:date})\n' acer-control-center
rpm -qi acer-control-center
rpm -q --dump acer-control-center | grep -E '/usr/(bin/acer-control-center|libexec/acer-control-daemon|lib/systemd/system/acer-control-daemon.service)'

# Record DNF5 transaction history for acer-control-center:
dnf5 history list --contains-pkgs=acer-control-center

# Record active systemd unit, live sandbox properties, and running D-Bus daemon version:
systemctl status acer-control-daemon.service --no-pager
systemctl cat acer-control-daemon.service --no-pager
systemctl show -p MainPID,ActiveState,SubState,FragmentPath,ReadWritePaths,CapabilityBoundingSet acer-control-daemon.service
busctl call io.github.zethasaintlab.AcerControlCenter1 /io/github/zethasaintlab/AcerControlCenter1 io.github.zethasaintlab.AcerControlCenter1 GetDaemonVersion

# Verify both the previous working RPM (e.g. 1.0.0-3.fc45) and target RPM (1.0.0-4.fc45) exist locally:
ls -la build/rpmbuild*/RPMS/x86_64/acer-control-center-1.0.0-*.fc45.x86_64.rpm
```

#### Step 2 — Upgrade to Target Release & Verify Effective Systemd / Daemon State (Operator-Authorized)
```bash
# Upgrade (or install on clean system) via DNF5:
sudo dnf5 upgrade ./build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-4.fc45.x86_64.rpm

# Record the new DNF5 transaction ID:
dnf5 history list --contains-pkgs=acer-control-center

# Reload systemd manager configuration and restart the privileged daemon:
sudo systemctl daemon-reload
sudo systemctl restart acer-control-daemon.service

# Verify installed NEVRA, RPM file integrity, and effective systemd sandbox configuration:
rpm -q acer-control-center
rpm -V acer-control-center
systemctl status acer-control-daemon.service --no-pager
systemctl show -p MainPID,ActiveState,SubState,FragmentPath,ReadWritePaths,CapabilityBoundingSet,ProtectSystem,ProtectKernelTunables acer-control-daemon.service
busctl call io.github.zethasaintlab.AcerControlCenter1 /io/github/zethasaintlab/AcerControlCenter1 io.github.zethasaintlab.AcerControlCenter1 GetDaemonVersion
```
Expected post-upgrade invariants:
- `rpm -q acer-control-center` outputs the target NEVRA (`acer-control-center-1.0.0-4.fc45.x86_64`) and `rpm -V acer-control-center` returns exit `0` with empty output.
- `systemctl show` confirms `ReadWritePaths` includes `-/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00`, `CapabilityBoundingSet` is empty (`CapabilityBoundingSet=`), and `ActiveState=active`.
- `GetDaemonVersion` returns `s "1.0.0-fc45"`.

#### Step 3 — Deterministic RPM Rollback Procedure (If Upgrade or Daemon Fails)
If the upgrade introduces a unit/daemon regression or RPM verification failure, roll back explicitly against the preserved local working RPM (do **not** rely on `dnf5 history undo` alone for `@commandline` local RPMs):
```bash
# Downgrade offline to preserved working prior RPM (e.g., 1.0.0-3.fc45):
sudo dnf5 downgrade ./build/rpmbuild-stage18/RPMS/x86_64/acer-control-center-1.0.0-3.fc45.x86_64.rpm
sudo systemctl daemon-reload
sudo systemctl restart acer-control-daemon.service
rpm -q acer-control-center
systemctl status acer-control-daemon.service --no-pager
```
If complete package removal is required instead (preserves user config in `~/.config/acer-control-center/`):
```bash
sudo dnf5 remove acer-control-center
sudo systemctl daemon-reload
```

---

## 3. Installed File Layout

| Installed Path | Purpose |
|---|---|
| `/usr/bin/acer-control-center` | Unprivileged Qt 6 Quick / Wayland desktop GUI |
| `/usr/libexec/acer-control-daemon` | Privileged D-Bus & Polkit hardware control daemon |
| `/usr/lib/systemd/system/acer-control-daemon.service` | Hardened `systemd` service unit (`Type=dbus`) |
| `/usr/share/dbus-1/system.d/io.github.zethasaintlab.AcerControlCenter1.conf` | D-Bus system bus ownership and method-call policy |
| `/usr/share/dbus-1/system-services/io.github.zethasaintlab.AcerControlCenter1.service` | D-Bus system bus on-demand service activation |
| `/usr/share/dbus-1/interfaces/io.github.zethasaintlab.AcerControlCenter1.xml` | Introspection XML contract |
| `/usr/share/polkit-1/actions/io.github.zethasaintlab.acercontrol.policy` | Fine-grained Polkit action definitions |
| `/usr/share/applications/io.github.zethasaintlab.acer-control-center.desktop` | Desktop launcher (`StartupWMClass=acer-control-center`) |
| `/usr/share/metainfo/io.github.zethasaintlab.acer-control-center.metainfo.xml` | AppStream software center metadata |
| `/usr/share/icons/hicolor/scalable/apps/io.github.zethasaintlab.acer-control-center.svg` | Scalable vector application icon |
| `/usr/share/acer-control-center/driver/{acer_clv0001_kbd.c,Makefile,dkms.conf}` | Clean-room `CLV0001` (`GPL-2.0-or-later`) kernel module source & opt-in DKMS metadata |

---

## 4. Privilege & Polkit Security Model

1. **GUI Never Runs as Root:** `acer-control-center` enforces an early `geteuid() == 0` guard in `main()` before `QApplication` initialization and refuses to launch as `root`.
2. **No Shell Execution:** Neither the GUI nor `acer-control-daemon` invokes `/bin/sh`, `system()`, `popen()`, or `QProcess`.
3. **Hardened Systemd Sandbox (`acer-control-daemon.service`):** Runs with `CapabilityBoundingSet=` (empty), `AmbientCapabilities=` (empty), `NoNewPrivileges=yes`, `ProtectSystem=strict`, `ProtectHome=yes`, `ProtectKernelTunables=yes`, `PrivateDevices=yes`, `PrivateNetwork=yes`, and `ReadWritePaths=-/sys/devices/system/cpu/intel_pstate/no_turbo -/sys/devices/system/cpu/intel_pstate/min_perf_pct -/sys/devices/system/cpu/intel_pstate/max_perf_pct -/sys/devices/system/cpu/cpufreq -/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00`.
4. **Polkit Action Matrix:**

| Polkit Action ID | Default (`allow_active`) | Operation |
|---|---|---|
| `io.github.zethasaintlab.acercontrol.read-rapl` | `yes` | Read `/sys/class/powercap/intel-rapl:0/energy_uj` (`0400` root-only on Linux 6.x/7.x) |
| `io.github.zethasaintlab.acercontrol.set-power-profile` | `auth_admin_keep` | Validated writes to `intel_pstate` / `cpufreq` (`scaling_governor`, `energy_performance_preference`, `no_turbo`, `min/max_perf_pct`) with readback verification and rollback |
| `io.github.zethasaintlab.acercontrol.set-keyboard-lighting` | `yes` | Validated preset/static-RGB/brightness writes to `CLV0001` (`clv0001_kbd_command`, write-only `_DSM 0x67`) or `/sys/class/leds/*kbd_backlight*` |
| `io.github.zethasaintlab.acercontrol.set-battery-limit` | `auth_admin_keep` | Validated writes (`[50..100]`) to `/sys/class/power_supply/BAT0/charge_control_end_threshold` with readback verification (blocked on host by `ProtectSystem=strict` & absent sysfs node) |
| `io.github.zethasaintlab.acercontrol.set-fan-mode` | `auth_admin_keep` | Validated writes (`[20..100]%`) to verified `/sys/class/hwmon/*/pwm1` & `pwm1_enable` with fallback to `pwm1_enable=2` (blocked on host by `ProtectSystem=strict` & absent sysfs node) |

---

## 5. Optional Clean-Room `CLV0001` Kernel Driver & Physical Validation Runbook

RPM installation **never** compiles or loads out-of-tree kernel modules during `%post` scriptlets. Operators who choose to validate or use `CLV0001` keyboard backlight preset control on `Aspire A715-59G` (`Kelilu_RTH`) must follow this staged procedure separating read-only checks from state-changing host operations.

### 5.1 State & Status Terminology (Four Distinct Layers)
Never conflate ACPI namespace presence, driver binding, software command state, and physical LED illumination:

| Layer | Field / Source | Meaning & Allowed Values |
|---|---|---|
| **1. ACPI Device Status (`_STA`)** | `clv0001Status` / `/sys/bus/acpi/devices/CLV0001:00/status` | Decimal representation of firmware ACPI AML `_STA` return bits (`15` = `0x0F`: bit 0 `0x01` present, bit 1 `0x02` enabled, bit 2 `0x04` UI-visible, bit 3 `0x08` functioning properly). `15` means the BIOS ACPI device is present and enabled even when **no** driver is bound. |
| **2. Driver Binding State** | `clv0001DriverBound` / `/sys/bus/acpi/devices/CLV0001:00/driver` | `false` when unbound; `true` once `acer_clv0001_kbd` binds `CLV0001:00` and exposes `clv0001_kbd_{command,state,presets}`. |
| **3. Software Command & ACPI Eval State** | `clv0001LastStatus` / `status=` in `clv0001_kbd_state` | `uninitialized` (loaded cleanly, zero `_DSM` writes yet); `accepted_no_hw_readback` (`acpi_evaluate_dsm()` returned `integer`, `buffer`, or `package` — **does NOT confirm physical LED illumination**); `error:rate_limited` (`-EBUSY`, rejected `< 40 ms` before `_DSM`); `error:acpi_eval_failed` (`-EIO`, ACPI returned `NULL`); `error:unexpected_return_type` (`-EPROTO`). |
| **4. Physical LED Readback** | `physical_readback=unsupported` in `clv0001_kbd_state` | Permanently `unsupported` (`false`) because `CLV0001` `_DSM 0x67` is a write-only firmware setter. Actual physical keyboard illumination is confirmed **solely by human visual observation**. |

### 5.2 Stage 1 — Read-Only Physical Preflight (Zero Host State Changes)
All commands in Stage 1 are strictly read-only and must pass before loading `acer_clv0001_kbd.ko` or writing sysfs:
```bash
# 1. Verify running kernel release matches compiled module vermagic:
uname -r
modinfo -F vermagic build/driver-build/acer_clv0001_kbd.ko
modinfo -F license,alias,srcversion build/driver-build/acer_clv0001_kbd.ko

# 2. Check kernel lockdown posture (module load fails if Secure Boot lockdown enforces signed modules):
cat /sys/kernel/security/lockdown

# 3. Verify no competing keyboard/Clevo module is loaded:
lsmod | grep -E "acer_clv0001_kbd|alg_rgb|clevo" || echo "Clean: no competing module loaded"

# 4. Verify exact 4-field DMI identity (module fails closed with -ENODEV if any field differs):
head -n 1 /sys/class/dmi/id/{sys_vendor,product_name,board_name,board_vendor}

# 5. Verify CLV0001 ACPI node presence, namespace path, ACPI _STA, and unbound platform posture:
head -n 1 /sys/bus/acpi/devices/CLV0001:00/{hid,path,status}
readlink -e /sys/bus/acpi/devices/CLV0001:00/physical_node
readlink -e /sys/bus/platform/devices/CLV0001:00/driver || echo "Unbound as expected"

# 6. Verify installed RPM, daemon version, and effective systemd ReadWritePaths:
rpm -q acer-control-center
systemctl show -p MainPID,ActiveState,SubState,ReadWritePaths,CapabilityBoundingSet acer-control-daemon.service
busctl call io.github.zethasaintlab.AcerControlCenter1 /io/github/zethasaintlab/AcerControlCenter1 io.github.zethasaintlab.AcerControlCenter1 GetDaemonVersion

# 7. Run read-only application JSON preflight:
./build/acer-control-center --dump-telemetry-json
```
Preflight pass criteria:
- `uname -r` (`7.3.3-300.fc45.x86_64`) matches `modinfo -F vermagic`, and `/sys/kernel/security/lockdown` shows `[none] ...` (or module is signed if `integrity`/`confidentiality` is active).
- DMI reads `Acer`, `Aspire A715-59G`, `RTH`, `Kelilu_RTH`; `CLV0001:00` reports `hid=CLV0001`, `path=\_SB_.DCHU`, `status=15`, and no competing driver is bound.
- Before running D-Bus/GUI physical writes in Stage 4, ensure `acer-control-center` has been upgraded to `1.0.0-2.fc45` (Section 2.3 Step 2) and `systemctl show -p ReadWritePaths` includes `-/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00`.

### 5.3 Stage 2 — Operator-Authorized Module Load & Binding Verification (Zero Firmware Writes)
Loading `acer_clv0001_kbd.ko` performs only read-only DMI/HID/ACPI checks and sysfs creation — it executes **zero** `_DSM 0x67` firmware calls.
```bash
# Requires explicit operator root authorization:
sudo insmod build/driver-build/acer_clv0001_kbd.ko

# Inspect kernel log, platform driver binding, sysfs permissions, and initial uninitialized state:
dmesg | tail -n 15
readlink -e /sys/bus/platform/devices/CLV0001:00/driver
ls -la /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_*
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_presets
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
```
Expected post-load posture:
- `/sys/bus/platform/devices/CLV0001:00/driver` -> `/sys/bus/platform/drivers/acer_clv0001_kbd`.
- `/sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_*` (symlinked at `/sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_*`): `clv0001_kbd_command` is `-rw-------` (`0600`) owned by `root:root`; `clv0001_kbd_state` and `clv0001_kbd_presets` are `-r--r--r--` (`0444`) owned by `root:root`.
- `clv0001_kbd_state` reports `status=uninitialized`, `preset=none`, `brightness_level=0`, `payload_grb=00 00 00 F0`, `acpi_return_type=none`, `physical_readback=unsupported`.

### 5.4 Stage 3 — Negative Validation (Rejected Before Any ACPI Firmware Operation)
Verify that invalid inputs are rejected inside `clv0001_kbd_command_store()` (`-EINVAL`) **before** any `acpi_evaluate_dsm()` call:
```bash
# All three writes must fail with 'Invalid argument' (-EINVAL):
printf 'invalid_preset 4\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command || echo "PASS: invalid preset rejected (-EINVAL)"
printf 'cyan 9\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command || echo "PASS: out-of-range brightness rejected (-EINVAL)"
printf 'cyan 2 extra\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command || echo "PASS: extra token rejected (-EINVAL)"

# Confirm state remains untouched (zero _DSM calls occurred):
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
```
Pass criterion: `clv0001_kbd_state` must still report `status=uninitialized`, `preset=none`, `payload_grb=00 00 00 F0`, and `acpi_return_type=none`.

### 5.5 Stage 4 — Controlled State-Changing Firmware Writes & Visual Confirmation
Every command below evaluates `_DSM 0x67` and modifies hardware keyboard illumination state:
```bash
# 1. Safe initial state-changing write: cyan at discrete level 2 (scale 0x5F -> GRB 7E 00 63 F0):
printf 'cyan 2\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
# [HUMAN VISUAL CHECK]: Confirm keyboard backlight is physically lit in cyan at ~50% brightness.

# 2. Brightness & preset step check: white at level 4 (scale 0xBF -> GRB FF FF FF F0):
sleep 0.1
printf 'white 4\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
# [HUMAN VISUAL CHECK]: Confirm keyboard backlight turns bright white.

# 3. Legitimate state-changing backlight-off write ("off 0" sends GRB 00 00 00 F0 via _DSM 0x67):
sleep 0.1
printf 'off 0\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
# [HUMAN VISUAL CHECK]: Confirm keyboard backlight turns completely off.

# 4. Optional rate-limit (-EBUSY) verification (< 40 ms back-to-back writes):
sleep 0.1
printf 'red 2\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command && \
  printf 'blue 2\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command || \
  echo "Rate-limit triggered as expected (-EBUSY)"
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
```
Then verify end-to-end daemon D-Bus/Polkit execution and the unprivileged GUI (requires `1.0.0-2.fc45` RPM upgrade and daemon restart from Section 2.3):
```bash
# D-Bus verification through Polkit-gated acer-control-daemon:
sleep 0.1
busctl call io.github.zethasaintlab.AcerControlCenter1 \
    /io/github/zethasaintlab/AcerControlCenter1 \
    io.github.zethasaintlab.AcerControlCenter1 \
    SetKeyboardLighting ssu "preset:cyan" 75 "cyan"

# Launch unprivileged GUI and inspect CLV0001 Studio:
./build/acer-control-center --dump-telemetry-json
acer-control-center
```
In `04 // KEYBOARD STUDIO`, confirm `CLV0001` status shows write-only `_DSM 0x67` with no physical readback (`supports_custom_rgb=1` on Stage 17+ modules, or `17 PRESETS ONLY` on legacy modules), apply presets (`cyan`, `red`, `white`, `off`) and optional static 24-bit RGB (`#12A7E3` level `4` on Stage 17+ modules), confirm legacy preset-only modules reject non-preset hex with `ERR_CLV0001_PRESET_REQUIRED`, and verify the status banner never claims physical readback.

### 5.6 Explicit Failure & Abort Conditions
**Stop testing immediately if any condition below occurs. Do NOT issue repeated physical writes after an unexpected result.**

| Condition | Observable Symptom | Immediate Operator Action |
|---|---|---|
| Kernel / `vermagic` or lockdown failure | `insmod` returns `Invalid module format` (`-ENOEXEC`), `Operation not permitted`, or `Key was rejected by service`. | Abort before load; inspect `uname -r`, `modinfo`, and `/sys/kernel/security/lockdown`. Rebuild against matching kernel headers or sign/enroll MOK if integrity lockdown requires signed modules. |
| Unexpected device binding | Preflight `readlink -e /sys/bus/platform/devices/CLV0001:00/driver` already points to a non-empty driver before `insmod`. | Abort; do not load `acer_clv0001_kbd` on top of an unexpected binding. |
| Driver probe failure | `insmod` succeeds or returns `-ENODEV`, `/sys/bus/platform/devices/CLV0001:00/driver` is absent, or `clv0001_kbd_*` sysfs files do not appear. | Abort; capture `dmesg \| tail -n 30` and DMI fields; run `sudo rmmod acer_clv0001_kbd` if module remains loaded. |
| ACPI evaluation failure or wrong return type | `clv0001_kbd_state` shows `status=error:acpi_eval_failed` or `status=error:unexpected_return_type`. | Abort immediately (do not retry writes); capture `clv0001_kbd_state` and `dmesg`, then unload via `sudo rmmod acer_clv0001_kbd`. |
| Unexpected kernel error / warning | Any `BUG:`, `WARNING:`, lockdep splat, or `ACPI Error:` / `AE_*` message appears in `dmesg`. | Abort immediately; capture `dmesg`, return keyboard state only if safe or unload via `sudo rmmod acer_clv0001_kbd`. |
| Systemd / D-Bus / Polkit failure | `SetKeyboardLighting` fails with `AccessDenied` / `Read-only file system` / unknown preset error due to stale `1.0.0-1` daemon or missing `ReadWritePaths`. | Abort D-Bus/GUI step; verify `rpm -q`, `systemctl show`, and `journalctl -u acer-control-daemon.service` before retrying. |
| Physical LED inconsistency | `status=accepted_no_hw_readback` is reported, but LEDs fail to illuminate, change to wrong color, fail to dim across levels `0..4`, or fail to turn off on `off 0`. | Stop further writes; capture machine state and visual mismatch verbatim; unload `sudo rmmod acer_clv0001_kbd` and open a firmware investigation before further testing. |

### 5.7 Evidence Collection Procedure (Machine-Observed vs. Human-Observed)
Collect machine evidence after Stage 2, Stage 3, and Stage 4 in one session:
```bash
{
  echo "=== KERNEL & MODULE ==="
  uname -a
  lsmod | grep acer_clv0001_kbd || true
  modinfo build/driver-build/acer_clv0001_kbd.ko
  echo "=== DMESG (CLV0001 / ACPI) ==="
  dmesg | grep -iE "clv0001|acer_clv0001|dchu|acpi error" | tail -n 40
  echo "=== SYSFS BINDING & STATE ==="
  readlink -e /sys/bus/platform/devices/CLV0001:00/driver || true
  ls -la /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_* || true
  cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state || true
  echo "=== RPM, SYSTEMD & DAEMON ==="
  rpm -q acer-control-center
  systemctl status acer-control-daemon.service --no-pager
  systemctl show -p ReadWritePaths,CapabilityBoundingSet acer-control-daemon.service
  journalctl -u acer-control-daemon.service -n 30 --no-pager
  echo "=== DIAGNOSTICS JSON ==="
  ./build/acer-control-center --dump-telemetry-json
} | tee clv0001-physical-validation-log.txt
```
*(Inspect and redact any personal paths/logs before publishing externally.)*

Distinguish evidence sources strictly in the final validation record:

| Evidence Category | Verification Authority | What It Establishes |
|---|---|---|
| Kernel release, `modinfo`, `dmesg`, `clv0001_kbd_state`, `busctl` reply, GUI status banner | **Machine-Observed** | Module vermagic, bind/probe health, sysfs permissions, input rejection (`-EINVAL`/`-EBUSY`), and ACPI `_DSM 0x67` return status (`accepted_no_hw_readback` with `physical_readback=unsupported`). |
| Keyboard color match (`cyan 2`, `white 4`), dimming response (`0..4`), backlight turn-off (`off 0`) | **Human Visual Confirmation Only** | Physical illumination of the A715-59G keyboard LED matrix (firmware provides zero hardware readback). |

### 5.8 Module Removal & Full Host Rollback Strategy
```bash
# 1. Optionally turn keyboard backlight off before unload (only if physical writes succeeded):
printf 'off 0\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command

# 2. Unload kernel module cleanly (removes clv0001_kbd_* sysfs attributes via platform remove):
sudo rmmod acer_clv0001_kbd
lsmod | grep acer_clv0001_kbd || echo "acer_clv0001_kbd unloaded cleanly"
readlink -e /sys/bus/platform/devices/CLV0001:00/driver || echo "CLV0001:00 unbound cleanly"

# 3. Optional RPM rollback to previous working 1.0.0-1.fc45 release:
sudo dnf5 downgrade ./build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-1.fc45.x86_64.rpm
sudo systemctl daemon-reload
sudo systemctl restart acer-control-daemon.service
```
No persistent `/etc/modules-load.d/` or DKMS entry is registered by the RPM itself (`AUTOINSTALL="no"` in `driver/dkms.conf`), so unloading returns the kernel to stock unbound posture immediately. Notice that once `acer_clv0001_kbd.ko` is installed under `/lib/modules/<running-kver>/extra/` and `depmod -a` is run, Fedora's standard udev rule (`/usr/lib/udev/rules.d/80-drivers.rules`) resolves modalias `acpi:CLV0001:` via `modules.alias` (`alias acpi*:CLV0001:* acer_clv0001_kbd`, verified via `modprobe -R`) for that exact kernel only; booting into a newer upgraded kernel will leave `CLV0001:00` unbound unless rebuilt.

### 5.9 Future Cross-Kernel Driver Distribution Strategy (DKMS vs. Fedora `akmods`)

Because installing `/lib/modules/<kver>/extra/acer_clv0001_kbd.ko` manually does not survive Fedora kernel updates, automated cross-kernel packaging requires choosing either DKMS or Fedora `akmods`:

| Criterion | Option A — Opt-In DKMS (`acer-control-center-kmod-dkms` + `driver/dkms.conf`) | Option B — Fedora `akmods` (`acer-control-center-kmod` / `kmodtool` SRP) |
|---|---|---|
| **Fedora 45 Ecosystem Fit** | Supported via Fedora `dkms` package (already stubbed by `driver/dkms.conf` + `driver/Makefile`). | Native Fedora/RPM Fusion standard; `akmodsd` produces versioned `kmod-acer-clv0001-<kver>` RPMs managed by DNF. |
| **Operational Complexity** | Low: reuses existing `driver/dkms.conf` (changing `AUTOINSTALL="yes"` only inside an opt-in subpackage, keeping base RPM source-only). | Higher: requires separate `-kmod-common`, `akmod-*`, `kmodtool` RPM macro specs, and COPR/Fedora kmod build conventions. |
| **Kernel Upgrade Behavior** | `dkms autoinstall` service builds on boot/install when `kernel-devel-<new-kver>` matches. | Builds binary `kmod-*.rpm` via `akmods` systemd/DNF hook and installs it into the RPM DB. |
| **RPM DB Ownership & Rollback** | Built `.ko` sits outside RPM file tracking (`/var/lib/dkms/...` & `/lib/modules/.../extra/`); removed cleanly by `dkms remove` / `%preun`. | Cleanest RPM DB ownership (`rpm -qf` owns every `.ko`); uninstall/rollback happens through DNF. |
| **Secure Boot MOK Signing** | Built-in `dkms.conf` + `/etc/dkms/framework.conf` MOK key enrollment (`/var/lib/dkms/mok.pub`). | Built-in `/etc/pki/akmods/` `kmodgenca` MOK enrollment. |
| **API Breakage Failure Handling** | Fails build into `/var/lib/dkms/.../build/make.log`; leaves system booting safely with `CLV0001` in Preview Mode. | Fails akmod build log; leaves system booting safely with `CLV0001` in Preview Mode. |

**Canonical Recommendation:** Use **Option B (`akmods`) as the canonical long-term Fedora / COPR distribution path** once published in an external Fedora repository (so generated `.ko` artifacts are RPM-owned, cleanly auditable, and rollback-safe via DNF), while keeping the main `acer-control-center` RPM **source-only** (`AUTOINSTALL="no"` in `driver/dkms.conf`, zero automatic kernel compilation or module loading unless the operator explicitly installs a separate `akmod` or opt-in DKMS package).
