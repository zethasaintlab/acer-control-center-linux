Name:           acer-control-center
Version:        1.0.0
Release:        7%{?dist}
Summary:        Native Hardware Telemetry & Control Center for Acer Aspire 7 A715-59G

# Component license breakdown for files shipped in the binary RPM:
# - Application binaries & docs: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception (see LICENSE)
# - Clean-room optional CLV0001 driver source (driver/acer_clv0001_kbd.c): GPL-2.0-or-later (see COPYING.GPL-2.0)
# - AppStream metainfo XML (io.github.zethasaintlab.acer-control-center.metainfo.xml): CC0-1.0
License:        GPL-2.0-or-later AND (GPL-3.0-or-later WITH GPL-3.0-linking-source-exception) AND CC0-1.0
URL:            https://github.com/zethasaintlab/acer-control-center-linux
# Local source archive generated from the active repository tree (see README.md / docs/FEDORA_45_PACKAGING.md)
# until a tagged upstream release tarball includes the current branch changes.
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.26
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  desktop-file-utils
BuildRequires:  systemd-rpm-macros
BuildRequires:  pkgconfig(Qt6Core) >= 6.5
BuildRequires:  pkgconfig(Qt6Gui) >= 6.5
BuildRequires:  pkgconfig(Qt6Widgets) >= 6.5
BuildRequires:  pkgconfig(Qt6Qml) >= 6.5
BuildRequires:  pkgconfig(Qt6Quick) >= 6.5
BuildRequires:  pkgconfig(Qt6QuickControls2) >= 6.5
BuildRequires:  pkgconfig(Qt6DBus) >= 6.5
BuildRequires:  pkgconfig(Qt6Test) >= 6.5
BuildRequires:  pkgconfig(polkit-gobject-1)

Requires:       polkit
Requires:       dbus-common
Requires:       hicolor-icon-theme
Recommends:     tuned-ppd
Recommends:     qt6-qtwayland

%description
Acer Control Center — Linux Edition is a native C++23 and Qt 6 Quick desktop
hardware management application engineered for the Acer Aspire 7 A715-59G
(board RTH / Kelilu_RTH) on Fedora Linux 45 (KDE Plasma 6 & Hyprland Wayland).
It provides real-time CPU, memory, thermal, battery, and optional NVIDIA NVML
telemetry, verified Intel P-State and TuneD/PPD performance profiles,
interactive fan curve visualization, a capability-aware Keyboard Lighting Studio
(clean-room CLV0001 ACPI _DSM preset and Stage 17 static 24-bit custom RGB
backend, /sys/class/leds sysfs, and unrestricted sRGB/HSV/HSL Preview Studio
independent of application accent), clean-room out-of-tree CLV0001 driver
source for optional operator build, and privacy-redacted hardware diagnostics.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.zethasaintlab.acer-control-center.desktop
%ctest --output-on-failure

%post
%systemd_post acer-control-daemon.service

%preun
%systemd_preun acer-control-daemon.service

%postun
%systemd_postun_with_restart acer-control-daemon.service

%files
%license LICENSE COPYING.GPL-2.0
%doc README.md docs/HARDWARE_AUDIT_A715_59G.md docs/ARCHITECTURE_ADR.md docs/DESIGN_SYSTEM.md docs/FEDORA_45_PACKAGING.md docs/ACPI_RUNTIME_INVESTIGATION.md docs/RELEASE_READINESS.md docs/HOST_INTEGRATION_A715_59G.md
%{_bindir}/acer-control-center
%{_libexecdir}/acer-control-daemon
%{_unitdir}/acer-control-daemon.service
%{_datadir}/applications/io.github.zethasaintlab.acer-control-center.desktop
%{_datadir}/metainfo/io.github.zethasaintlab.acer-control-center.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/io.github.zethasaintlab.acer-control-center.svg
%{_datadir}/dbus-1/interfaces/io.github.zethasaintlab.AcerControlCenter1.xml
%{_datadir}/dbus-1/system.d/io.github.zethasaintlab.AcerControlCenter1.conf
%{_datadir}/dbus-1/system-services/io.github.zethasaintlab.AcerControlCenter1.service
%{_datadir}/polkit-1/actions/io.github.zethasaintlab.acercontrol.policy
%dir %{_datadir}/acer-control-center
%dir %{_datadir}/acer-control-center/driver
%{_datadir}/acer-control-center/driver/acer_clv0001_kbd.c
%{_datadir}/acer-control-center/driver/Makefile
%{_datadir}/acer-control-center/driver/dkms.conf

%changelog
* Mon Nov 23 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-7
- Add Stage 31 host integration evidence (docs/HOST_INTEGRATION_A715_59G.md) and close release readiness
* Mon Nov 23 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-6
- Reconcile PROJECT_STATE.md and RELEASE_READINESS.md with Stage 29–30.2 release evidence (%%doc refresh)
* Mon Nov 23 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-5
- Finalize composite SPDX License tag for application, CLV0001 driver source, and AppStream metadata
- Add canonical GPL-2.0 license text (COPYING.GPL-2.0) and include it under %%license
- Ship Stage 23A/23B ACPI investigation and Stage 28 release readiness docs under %%doc
* Fri Nov 20 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-4
- Add FirmwareAutomatic fan capability modeling and read-only tachometer/PWM safety gating
- Reconcile BAT0 cycle_count provenance (NOT REPORTED when unreported) and harden charge-limit rounding/rollback
- Implement Endfield-inspired global bimodal UI redesign (Dark Tactical HUD & Light CAD Schematic)
- Add multi-resolution visual QA capture, contact sheet, and expanded unit/GUI capability tests
* Fri Nov 20 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-3
- Extend CLV0001 ACPI _DSM 0x67 driver, HAL, daemon, controller, and Keyboard Studio for static 24-bit custom RGB with capability discovery
* Mon Nov 16 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-2
- Integrate clean-room CLV0001 (\_SB_.DCHU) ACPI _DSM 0x67 keyboard preset backend and optional driver sources
- Harden /proc/driver/nvidia telemetry non-fabrication and CPU governor/EPP rollback validation
* Sun Nov 08 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-1
- Initial Fedora 45 RPM release of Acer Control Center Linux Edition (v1.0.0)
