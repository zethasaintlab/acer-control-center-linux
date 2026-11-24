# Host Integration Evidence — Acer Aspire 7 A715-59G (`acer-control-center-1.0.0-6.fc45`)

- **Target Hardware:** Acer Aspire 7 `A715-59G` (`board_name: RTH`, `board_vendor: Kelilu_RTH`, BIOS `INSYDE Corp. V1.17`)
- **Host OS / Kernel:** Fedora Linux 45 (`7.3.3-300.fc45.x86_64`, `x86_64`)
- **Desktop Environment:** Hyprland on Wayland (`WAYLAND_DISPLAY=wayland-1`)
- **Validated Installed Package:** `acer-control-center-1.0.0-6.fc45.x86_64`
- **Validation Date:** 2026-11-23 (Stage 31.1 post-install verification of user-installed `1.0.0-6.fc45`)

> **Candidate Identity Note:** This document records live host validation performed against the unsigned local RPM candidate `acer-control-center-1.0.0-6.fc45.x86_64`. Later documentation-only RPM refreshes (`1.0.0-7.fc45`) package this report under `%doc` without altering binaries, daemon behavior, D-Bus/Polkit definitions, or driver sources.

---

## A. Installation Provenance

- **Installed NVR:** `acer-control-center-1.0.0-6.fc45.x86_64`
- **Candidate Artifact:** `build/release-stage30-3/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-6.fc45.x86_64.rpm`
- **Candidate SHA-256:** `8fe0351a566c0a3a098a9ebdbf9ffa3e9b999fa81811de52841a667a662ea7ae`
- **Verified Rollback Artifact (Unused):** `build/rpmbuild-stage18/RPMS/x86_64/acer-control-center-1.0.0-3.fc45.x86_64.rpm` (`SHA-256`: `8e47940ff3adb594b249a4d78d5f6527793ee91036956fa9a1bd47a79c2b7f1d`)
- **DNF5 Transaction ID:** `66` (`Status: Ok`)
- **Transaction Command & Timestamp:** `dnf5 install build/release-stage30-3/rpmbuild/RPMS/x86_64/acer-control-center-1.0.0-6.fc45.x86_64.rpm` *(workstation path prefix redacted)* executed by `UID 1000` (`[username redacted]`) from `2026-11-23 13:38:48` to `13:38:50 UTC` (`20:38:48–20:38:50 WIB`)
- **Package Action:** Upgraded `acer-control-center-0:1.0.0-3.fc45.x86_64` (`@System`) to `acer-control-center-0:1.0.0-6.fc45.x86_64` (`@commandline`)
- **Provenance Distinction:** The DNF5 upgrade transaction was initiated directly by the user (`UID 1000`) prior to Stage 31 execution, **not** by the Stage 31/31.1 validation agent. Stage 31 stopped at its `1.0.0-3.fc45` baseline gate, and Stage 31.1 validated the resulting live `1.0.0-6.fc45` installation without executing another package transaction.

---

## B. RPM Integrity Verification

- **Package Metadata Match (`rpm -q` vs. `rpm -qp`):** Exact match on `Name` (`acer-control-center`), `Epoch` (`(none)`), `Version` (`1.0.0`), `Release` (`6.fc45`), `Arch` (`x86_64`), `BUILDTIME` (`1795440434`), and `License` (`GPL-2.0-or-later AND (GPL-3.0-or-later WITH GPL-3.0-linking-source-exception) AND CC0-1.0`).
- **File-by-File Comparison (`FILENAMES`, `FILEMODES`, `FILEUSERNAME:FILEGROUPNAME`, `FILEDIGESTS`):** `FILE_METADATA_EXACT_MATCH` between installed files and the candidate RPM.
- **`rpm -V acer-control-center`:** Passed (`exit 0`, clean output).
- **License & Documentation Coverage:** `rpm -qL` confirmed `/usr/share/licenses/acer-control-center/COPYING.GPL-2.0` and `/usr/share/licenses/acer-control-center/LICENSE`; `rpm -qd` confirmed all `7` expected `%doc` files present.
- **Rollback:** Not required and not executed.

---

## C. `systemd` Service & D-Bus Read-Only Validation

- **Service State (`acer-control-daemon.service`):** `enabled` (`UnitFilePreset=disabled`) and `active (running)` (`MainPID=68297`, `Result=success`, `ExecMainStatus=0`).
- **Executable Path:** `/usr/libexec/acer-control-daemon` (`User=root` via `/usr/share/dbus-1/system-services/io.github.zethasaintlab.AcerControlCenter1.service`).
- **Upgrade Journal Continuity (`journalctl -u acer-control-daemon.service`):** Clean `Stopping` -> `Stopped` (`Deactivated successfully`) -> `Starting` -> `Started` transition at `Nov 23 20:38:49 WIB` with zero crashes, capability errors, or permission failures.
- **D-Bus System Bus Ownership (`GetNameOwner`):** Owned at `:1.7005`.
- **Read-Only D-Bus Method Results:**
  - `io.github.zethasaintlab.AcerControlCenter1.GetDaemonVersion` (`(s)`): returned `('1.0.0-fc45',)`.
  - `io.github.zethasaintlab.AcerControlCenter1.GetRaplPowerSnapshot` (`(ttttxb)`): succeeded and matched declared signature (example sample: `(uint64 159595805483, uint64 83739064497, uint64 4574858161, uint64 12391813918, int64 1795441480893000, true)`).
- **Write Method Boundary:** Zero D-Bus write or hardware-control methods were invoked.

---

## D. Polkit Policy Validation

- **Installed Policy File:** `/usr/share/polkit-1/actions/io.github.zethasaintlab.acercontrol.policy` (`-rw-r--r--`, `root:root`, `2698` bytes).
- **XML Syntax Check:** `xmllint --noout` passed (`POLKIT_XML_VALID`).
- **Registered Action IDs & Default Authorization Rules (`pkaction`):**
  - `io.github.zethasaintlab.acercontrol.read-rapl`: `yes` (active session)
  - `io.github.zethasaintlab.acercontrol.set-keyboard-lighting`: `yes` (active session)
  - `io.github.zethasaintlab.acercontrol.set-power-profile`: `auth_admin_keep`
  - `io.github.zethasaintlab.acercontrol.set-battery-limit`: `auth_admin_keep`
  - `io.github.zethasaintlab.acercontrol.set-fan-mode`: `auth_admin_keep`
- **Scope Distinction:** Policy syntax validity and `pkaction` registration/defaults are verified; an interactive desktop authentication dialog (`auth_admin_keep`) was **not** triggered or tested.

---

## E. Wayland GUI & Live Telemetry Validation

- **Live GUI Launch (`WAYLAND_DISPLAY=wayland-1`, Hyprland):** `/usr/bin/acer-control-center` (`PID 70223`) mapped natively as a Wayland surface (`mapped: true`, `xwayland: false`, `size: [907, 1048]`, title `Acer Control Center — Linux Edition (Acer Aspire A715-59G (v1.17))`) with zero stdout/stderr warnings or errors.
- **Process Termination Detail:** Sending `hyprctl dispatch closewindow` triggered the application's system-tray/minimize-to-tray handling rather than terminating the process. A subsequent `SIGTERM` exited the process with status `143` (not a normal `0` shutdown code), leaving no lingering GUI process.
- **8-Workspace Navigation & Theme Rendering:** Exercised via `/usr/bin/acer-control-center --capture-screenshots` under an isolated `XDG_CONFIG_HOME` on the live Wayland display across all `8` workspace views (`01-overview` through `08-settings`, Dark and Light themes); completed with `exit 0`.
- **Real-Host Read-Only Telemetry (`--dump-telemetry-json`):** Exited `0` with empty stderr and reported `hardwareMode: Physical Hardware`, `mockActive: false`, `daemonReachable: true`, `Acer Aspire A715-59G (v1.17)` / `RTH // Kelilu_RTH`, `Intel i7-13620H`, `CLV0001 DETECTED (KERNEL DRIVER UNBOUND — PREVIEW MODE)`, `bat_limit_supported: false`, and `fan_manual_supported: false`.

---

## F. User Configuration Integrity

- `$XDG_CONFIG_HOME/acer-control-center/settings.json` *(standard user config directory)* remained completely untouched and byte-identical before and after validation:
  - **Size:** `8434` bytes (`mode 644`, `mtime 2026-11-16 09:59:36 +0700`)
  - **SHA-256:** `dc6b1d7c98c4232739730347e79511e40c2032dcfd0b54922c72564fda99691b`

---

## G. Hardware Safety & Residual Limitations

- **Zero Hardware Mutations During Stage 31.1 Package Validation:** Zero ACPI, AML, EC, WMI, firmware, or sysfs write operations occurred during Stage 31/31.1 RPM validation; `acer_clv0001_kbd` was not installed or loaded during Stage 31.1 (`lsmod` confirmed unloaded).
- **Subsequent Stage 36–37 CLV0001 Host Activation (2026-11-24):** Outside the `1.0.0-6.fc45` RPM payload itself, Stage 36 built `acer_clv0001_kbd.ko` (`sha256: 02ff5bf35f8795c9295461c0eeffca0905e6e2b434fba0dd93f0bff6f436ef3d`) for `7.3.3-300.fc45.x86_64`, installed it under `/lib/modules/7.3.3-300.fc45.x86_64/extra/`, bound `CLV0001:00` (`\_SB_.DCHU`), and executed one `cyan 4` command (`status=accepted_no_hw_readback`, `preset=cyan`, `brightness_level=4`, `payload_grb=FF 00 C8 F0`, `acpi_return_type=integer`, `physical_readback=unsupported`). Operator confirmed keyboard illumination (exact hue unconfirmed). Same-kernel reboot autoload (`modprobe -R "acpi:CLV0001:"` -> `acer_clv0001_kbd`) is inferred from udev `80-drivers.rules` rather than reboot-tested, and cross-kernel persistence remains future packaging work.
- **Retained Hardware & Operator-Gated Limits:** Battery charge-limit (`cap.battery.limit`) and manual fan PWM (`cap.thermal.fans`) remain unavailable (`NotDetected` / `FirmwareAutomatic`); interactive desktop Polkit authentication and physical `PrepareForSleep` suspend/resume transitions remain unverified; full hardware certification has not been established.
