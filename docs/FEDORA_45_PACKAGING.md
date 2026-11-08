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
rpm -qip build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-1.fc45.x86_64.rpm
rpm -qlp build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-1.fc45.x86_64.rpm
rpm -qRp build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-1.fc45.x86_64.rpm
```

### Installing & Removing the RPM on Fedora 45
```bash
# Install
sudo dnf install ./build/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-1.fc45.x86_64.rpm

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

---

## 4. Privilege & Polkit Security Model

1. **GUI Never Runs as Root:** `acer-control-center` enforces an early `geteuid() == 0` guard in `main()` before `QApplication` initialization and refuses to launch as `root`.
2. **No Shell Execution:** Neither the GUI nor `acer-control-daemon` invokes `/bin/sh`, `system()`, `popen()`, or `QProcess`.
3. **Polkit Action Matrix:**

| Polkit Action ID | Default (`allow_active`) | Operation |
|---|---|---|
| `io.github.zethasaintlab.acercontrol.read-rapl` | `yes` | Read `/sys/class/powercap/intel-rapl:0/energy_uj` (`0400` root-only on Linux 6.x/7.x) |
| `io.github.zethasaintlab.acercontrol.set-power-profile` | `auth_admin_keep` | Validated writes to `intel_pstate` / `cpufreq` (`scaling_governor`, `energy_performance_preference`, `no_turbo`, `min/max_perf_pct`) with readback verification and rollback |
| `io.github.zethasaintlab.acercontrol.set-keyboard-lighting` | `yes` | Validated writes to `/sys/class/leds/*kbd_backlight*` (`brightness`, `multi_intensity`) with readback verification |
| `io.github.zethasaintlab.acercontrol.set-battery-limit` | `auth_admin_keep` | Validated writes (`[50..100]`) to `/sys/class/power_supply/BAT0/charge_control_end_threshold` with readback verification |
| `io.github.zethasaintlab.acercontrol.set-fan-mode` | `auth_admin_keep` | Validated writes (`[20..100]%`) to `/sys/class/hwmon/*/pwm1` & `pwm1_enable` with automatic fallback to `pwm1_enable=2` on error |
