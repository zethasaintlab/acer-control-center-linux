Name:           acer-control-center
Version:        1.0.0
Release:        1%{?dist}
Summary:        Native Hardware Telemetry & Control Center for Acer Aspire 7 A715-59G

License:        GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
URL:            https://github.com/zethasaintlab/acer-control-center-linux
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

%description
Acer Control Center — Linux Edition is a native C++23 and Qt 6 Quick desktop
hardware management application engineered for the Acer Aspire 7 A715-59G
(board RTH / Kelilu_RTH) on Fedora Linux 45 (KDE Plasma 6 & Hyprland Wayland).
It provides real-time CPU, memory, thermal, battery, and optional NVIDIA NVML
telemetry, verified Intel P-State and TuneD/PPD performance profiles,
interactive fan curve visualization, an unrestricted sRGB/HSV/HSL Keyboard
Lighting Studio independent of the application accent color, and
privacy-redacted hardware diagnostics.

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
%license LICENSE
%doc README.md docs/HARDWARE_AUDIT_A715_59G.md docs/ARCHITECTURE_ADR.md docs/DESIGN_SYSTEM.md docs/FEDORA_45_PACKAGING.md
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

%changelog
* Sun Nov 08 2026 zethasaintlab <zethasaintlab@users.noreply.github.com> - 1.0.0-1
- Initial Fedora 45 RPM release of Acer Control Center Linux Edition (v1.0.0)
