# Fedora Linux 45 Packaging, Installation & Privilege Architecture

**Target Distribution:** Fedora Linux 45 (`fc45`)  
**Target Desktop Environments:** KDE Plasma 6 (Wayland) & Hyprland (Wayland)  
**Target Hardware:** Acer Aspire 7 A715-59G (`board_name: RTH`, `board_vendor: Kelilu_RTH`)  
**Package License:** `GPL-3.0-or-later WITH GPL-3.0-linking-source-exception`

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
```bash
rpm -qip build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-2.fc45.x86_64.rpm
rpm -qlp build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-2.fc45.x86_64.rpm
rpm -qRp build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-2.fc45.x86_64.rpm
```

### Installing & Removing the RPM on Fedora 45
```bash
# Install
sudo dnf install ./build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-2.fc45.x86_64.rpm

# Enable or check D-Bus activated service status
systemctl status acer-control-daemon.service

# Uninstall cleanly (preserves user settings in ~/.config/acer-control-center/)
sudo dnf remove acer-control-center
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
| `io.github.zethasaintlab.acercontrol.set-keyboard-lighting` | `yes` | Validated preset/brightness writes to `CLV0001` (`clv0001_kbd_command`, write-only `_DSM 0x67`) or `/sys/class/leds/*kbd_backlight*` |
| `io.github.zethasaintlab.acercontrol.set-battery-limit` | `auth_admin_keep` | Validated writes (`[50..100]`) to `/sys/class/power_supply/BAT0/charge_control_end_threshold` with readback verification |
| `io.github.zethasaintlab.acercontrol.set-fan-mode` | `auth_admin_keep` | Validated writes (`[20..100]%`) to `/sys/class/hwmon/*/pwm1` & `pwm1_enable` with automatic fallback to `pwm1_enable=2` on error |

---

## 5. Optional Clean-Room `CLV0001` Kernel Driver & Physical Validation Runbook

RPM installation **never** compiles or loads out-of-tree kernel modules during `%post` scriptlets. Operators who choose to validate or use `CLV0001` keyboard backlight preset control on `Aspire A715-59G` (`Kelilu_RTH`) follow this controlled opt-in procedure.

### 5.1 Prerequisites & Read-Only Preflight
- Target machine matches exact DMI fields (`Acer` / `Aspire A715-59G` / `RTH` / `Kelilu_RTH`) and exposes `/sys/bus/acpi/devices/CLV0001:00` (`\_SB_.DCHU`).
- Kernel headers (`kernel-devel` matching `uname -r`) are installed and the CMake project + module are built without root:
```bash
# 1. Read-only hardware preflight before loading module:
./build/acer-control-center --dump-telemetry-json

# 2. Build out-of-tree kernel module against the running kernel:
mkdir -p build/driver-build
cp driver/acer_clv0001_kbd.c driver/Makefile build/driver-build/
make -C "/lib/modules/$(uname -r)/build" M="$PWD/build/driver-build" modules
modinfo build/driver-build/acer_clv0001_kbd.ko
```

### 5.2 Operator-Authorized Module Load & Expected Topology
Loading the module alone performs only read-only DMI/HID/ACPI checks and sysfs creation — it executes **zero** `_DSM 0x67` firmware writes.
```bash
# Requires explicit operator root authorization:
sudo insmod build/driver-build/acer_clv0001_kbd.ko

# Verify kernel log & binding:
dmesg | tail -n 15
ls -la /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_*
readlink -f /sys/bus/acpi/devices/CLV0001:00/driver
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_presets
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state
```
Expected initial posture:
- `/sys/bus/acpi/devices/CLV0001:00/driver` -> `.../bus/platform/drivers/acer_clv0001_kbd`.
- `clv0001_kbd_command` is `-rw-------` (`0600`) owned by `root:root`; `clv0001_kbd_state` and `clv0001_kbd_presets` are `-r--r--r--` (`0444`) owned by `root:root`.
- `clv0001_kbd_state` reports `status=uninitialized`, `preset=none`, `brightness_level=0`, `payload_grb=00 00 00 F0`, `acpi_return_type=none`, `physical_readback=unsupported`.

### 5.3 Safe Initial Preset, Brightness, Error & Rate-Limit Verification
```bash
# Safe initial cyan preset at level 2 (50% -> 0x5F scale -> GRB 7E 00 63 F0):
printf 'cyan 2\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state

# Brightness step 4 (100% -> FF FF FF F0) and backlight off (00 00 00 F0):
printf 'white 4\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
sleep 0.1
printf 'off 0\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
cat /sys/bus/acpi/devices/CLV0001:00/clv0001_kbd_state

# Verify fail-closed rejection of invalid preset (-EINVAL) and 40 ms rate limit (-EBUSY):
printf 'invalid_preset 4\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command || echo "Rejected with -EINVAL as expected"
```
Expected status after an accepted write: `status=accepted_no_hw_readback`, `physical_readback=unsupported`, `acpi_return_type` equal to `integer` / `buffer` / `package`.

### 5.4 End-to-End Daemon & Unprivileged GUI Verification
```bash
# Install updated unit/daemon or restart system service after RPM install:
sudo systemctl restart acer-control-daemon.service
systemctl status acer-control-daemon.service

# Launch unprivileged GUI and confirm CLV0001 detection:
./build/acer-control-center --dump-telemetry-json
./build/acer-control-center
```
In `04 // KEYBOARD STUDIO`, confirm the routing header reads `CLV0001 ACPI _DSM 0x67 (17 PRESETS · 5 LEVELS · NO HW READBACK)`, select presets (`cyan`, `red`, `white`) and discrete levels (`0..4`), click `SEND CLV0001 _DSM 0x67 PRESET (NO HW READBACK)`, and verify the status banner reports ACPI acceptance without fabricating physical readback.

### 5.5 Module Removal & Rollback Strategy
```bash
# Return keyboard LEDs to off (optional) and unload module cleanly:
printf 'off 0\n' | sudo tee /sys/devices/LNXSYSTM:00/LNXSYBUS:00/CLV0001:00/clv0001_kbd_command
sudo rmmod acer_clv0001_kbd
lsmod | grep acer_clv0001_kbd || echo "acer_clv0001_kbd unloaded cleanly"
```
If any failure occurs during physical testing, unloading via `sudo rmmod acer_clv0001_kbd` removes sysfs nodes immediately; standard EC/Fn keyboard hotkeys remain untouched, and no persistent boot module configuration or resume ACPI write is installed by default.
