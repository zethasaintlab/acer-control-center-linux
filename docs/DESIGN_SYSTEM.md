# Visual Design System — "Precision Hardware Atelier"

**Project:** Acer Control Center — Linux Edition  
**Design Direction:** Precision Hardware Atelier  
**Primary Display Target:** `1920×1080` @ `144Hz` (scaled cleanly down to `1120×720` minimum window size)

---

## 1. Research Foundation & Translated Design Rules

Our visual language synthesizes five authoritative sources into concrete, testable rules:

1. **Nielsen Norman Group (10 Usability Heuristics):**
   - **Visibility of System Status:** Every metric tile and control displays its live hardware provenance (`coretemp`, `NVML 01:00.0`, `BAT0`, `tuned-ppd`) and capability badge (`LIVE`, `READ-ONLY`, `RAIL GATED (P8)`, `NOT DETECTED`, `MOCK PREVIEW`).
   - **Error Prevention & Recovery:** Unavailable hardware writes (`Fan Curve Override`, `80% Charge Limit`, `Physical Keyboard LED` on `Kelilu_RTH`) never fail silently; controls explain *why* the kernel interface is unavailable and separate preview/profile editing from physical writes.
2. **KDE Human Interface Guidelines:**
   - Persistent left navigation rail (`220px` standard / `64px` compact mode) with 8 top-level workspaces and direct keyboard shortcuts (`Alt+1` .. `Alt+8`).
   - Native system tray integration (`QSystemTrayIcon` / StatusNotifierItem) for quick profile switching and thermal readout.
3. **GNOME Human Interface Guidelines:**
   - Strict visual hierarchy: maximum 3 data points per summary cell; progressive disclosure for per-core/per-zone details.
4. **Apple Human Interface Guidelines:**
   - Tabular monospace numerals (`tabular-nums`) for all live counters, frequencies, temperatures, and voltages so digits never jitter horizontally during `500 ms` updates.
   - Restrained, purposeful transitions (`140 ms` cubic-bezier) that automatically collapse to `0 ms` when `reducedMotion` is enabled.
5. **WCAG 2.2 AA/AAA Accessibility:**
   - Automated relative luminance ($L$) and contrast ratio ($CR = \frac{L_1 + 0.05}{L_2 + 0.05}$) computation on every custom Application Accent Color selection, ensuring $\ge 4.5:1$ text contrast and $\ge 3.0:1$ non-text control contrast across both Dark and Light themes.
   - High-visibility `2px` keyboard focus outlines (`focusRingColor`) on all interactive controls, including custom canvas widgets.

---

## 2. Anti-AI-Slop Rejection Checklist

Every screen and component is audited against these strict prohibitions:
- **No purple-blue or random gradients:** All structural surfaces are flat, matte architectural planes.
- **No glassmorphism, neon glow, or drop-shadow soup:** Depth is communicated through crisp `1px` structural hairlines (`borderSubtle`, `borderStrong`) and luminance steps.
- **No cards nested inside cards:** Workspaces use an architectural instrument-bay grid divided by `1px` hairlines and indexed technical headers (`01 // CPU & PLATFORM`, `02 // DISCRETE GPU`).
- **No emoji icons or mixed icon weights:** Custom `1.5px` stroke vector iconography rendered via QML `Canvas` / SVG paths.
- **No fabricated telemetry or ornamental gauges:** Every chart plots real time-series ring buffers with labeled axes (`°C`, `W`, `%`, `MHz`).

---

## 3. Color System & Dual Color Architecture

### 3.1 Surface & Semantic Palette
| Token Role | Dark Theme ("Graphite Instrument") | Light Theme ("Anodized Aluminum") | Usage |
|---|---|---|---|
| `canvasBg` | `#0D0F12` | `#F3F5F8` | Primary window background |
| `surfacePrimary` | `#13161B` | `#FFFFFF` | Primary instrument bay surface |
| `surfaceElevated` | `#191D24` | `#EAEEF4` | Interactive control / input background |
| `surfaceHeader` | `#101318` | `#E3E8F0` | Section header bar & navigation rail |
| `borderSubtle` | `#232832` | `#D2D9E4` | `1px` structural grid hairline |
| `borderStrong` | `#343C4A` | `#B4BFCE` | Active control / input boundary |
| `textPrimary` | `#ECEFF4` | `#12161F` | Primary headings & numeric readouts |
| `textSecondary` | `#9AA4B5` | `#4A5568` | Technical labels, units & provenance |
| `textMuted` | `#646E80` | `#718096` | Disabled / unsupported explanation text |
| `statusNominal` | `#2EBE7E` | `#168A56` | Normal thermal/power state (`< 75°C`) |
| `statusWarning` | `#E5A024` | `#B87503` | Elevated thermal/power state (`75–87°C`) |
| `statusCritical` | `#E04F5F` | `#C5283D` | Thermal/power throttling or critical alert (`>= 88°C`) |
| `statusInfo` | `#3B9EFF` | `#0066CC` | Read-only / informational state badge |

### 3.2 Dual Color System (Strict Separation)
1. **Application Theme Accent (`SettingsViewModel.accentColor`):**
   - Freely customizable across the entire sRGB gamut via HSV/HSL wheel, RGB sliders, Hex input, and saved swatches.
   - `ThemeEngine` computes WCAG relative luminance and derives:
     - `accentPrimary`: User's selected color.
     - `accentAccessibleText`: Contrast-safe foreground (`#0D0F12` or `#FFFFFF`) guaranteed $\ge 4.5:1$ against `accentPrimary`.
     - `accentSurfaceTint`: Low-opacity (`14%` Dark / `10%` Light) tint for active navigation items and chart area fills.
     - `accentReadableOnSurface`: Luminance-clamped variant guaranteed $\ge 4.5:1$ against `surfacePrimary` for inline text/numbers.
2. **Physical Keyboard Lighting Color (`KeyboardStudioViewModel`):**
   - Controls the lighting profile for the physical keyboard (1-zone or 4-zone sRGB + brightness + mode).
   - **Never coupled to the Application Theme Accent** (though a user may explicitly click "Copy App Accent Swatch" if desired).
   - Displays a permanent hardware routing indicator showing whether changes are written to `/sys/class/leds/*kbd_backlight*`, routed to `Mock Hardware Test Mode`, or held in `Profile Design & Preview Mode (Hardware LED Not Detected)`.

---

## 4. Typography & Spatial Grid

### Typography Scale
- **Primary UI Font:** System sans-serif (`Inter`, `Noto Sans`, or `Cantarell`)
- **Technical / Telemetry Font:** System monospace (`JetBrains Mono`, `IBM Plex Mono`, or `monospace`) with `font.features: { "tnum": 1 }`
- **Roles:**
  - `Hero Metric` (`28px` Mono Bold): Primary temperature, wattage, or percentage readout.
  - `Section Index` (`11px` Mono SemiBold, uppercase, `0.08em` tracking): e.g., `01 // PROCESSOR TELEMETRY`.
  - `Workspace Title` (`18px` Sans SemiBold): Screen header title.
  - `Control Label` (`13px` Sans Medium): Setting and control title.
  - `Body / Explanation` (`12px` Sans Regular): Hardware capability notes and descriptions.
  - `Caption / Provenance` (`11px` Mono Regular): Sensor sysfs path, polling interval, or unit label.

### Spatial System (`8px` Base Unit)
- Window minimum dimensions: `1120×720`; default target: `1360×860` (fits comfortably inside `1920×1080` floating or tiled in Hyprland/KDE).
- Navigation Rail: `220px` width (`64px` collapsed).
- Top Status Bar: `48px` height (shows DMI model `Aspire A715-59G // Kelilu_RTH`, active profile badge, CPU/GPU live pills, polling rate indicator, and Mock/Physical mode badge).
- Section Padding: `16px`; Grid Gap: `1px` hairline structural borders or `12px` control spacing.

---

## 5. Complete Navigation & Screen Inventory

1. **`01 // OVERVIEW` (Main Dashboard):**
   - 4-quadrant live telemetry bay (CPU Package & Cores, NVIDIA RTX 3050 GPU, Memory & PSI Pressure, Thermal & Power Summary) + dual 60-second synchronized strip charts + quick Operating Profile selector.
2. **`02 // PERFORMANCE` (Performance Center):**
   - Operating Profile switcher (`Quiet`, `Balanced`, `Performance`, `Custom`) wired to `tuned-ppd` + `intel_pstate` (`governor`, `EPP`, `Turbo Boost`, `Min/Max Perf %`) + AC/Battery automatic switching rules + per-core frequency bar rack.
3. **`03 // THERMAL` (Thermal & Fan Center):**
   - Live multi-sensor temperature matrix (`coretemp`, `TCPU`, `SEN2`, `SEN3`, `nvme0`, `nvme1`, `iwlwifi_1`, `GPU`) + interactive 2D piecewise Fan Curve Editor (`°C` vs `PWM %`) with explicit `Firmware Auto / Preview Mode` vs `Hardware PWM` status banner.
4. **`04 // KEYBOARD STUDIO` (Keyboard Lighting Studio):**
   - Interactive scale vector visualizer of the Acer Aspire 7 15.6" keyboard (with numpad & 4-zone segmentation) + **Unrestricted Color Picker** (2D HSV/HSL wheel, RGB `0–255` sliders & spinboxes, `#RRGGBB` hex input, live sRGB preview, saved custom swatches) + lighting profile manager (`Save`, `Load`, `Rename`, `Delete`).
5. **`05 // BATTERY & POWER` (Battery & Power Center):**
   - Real-time charge state, discharge/charge wattage (`V × I`), wear health analysis (`charge_full` vs `charge_full_design`), rolling charge/power history chart, AC adapter status, battery notifications, and conditional Charge Limit (`80% Care Mode`) control.
6. **`06 // GPU` (NVIDIA GPU Center):**
   - Dedicated RTX 3050 6GB instrument panel: Graphics/SM/Memory clock gauges, VRAM allocation bar, P-state indicator (`P0`–`P8`), power limits (`60W` default / `75W` max) with P8 rail-gate detection, and decoded NVML throttle reason flags.
7. **`07 // DIAGNOSTICS` (Hardware Diagnostics):**
   - Full Capability Matrix table with live state badges, DMI/WMI/driver inspector, live switches for **Physical Hardware Mode** vs **Mock Hardware Test Mode** (for safe verification of unsupported/unverified controls), application log viewer, and **Export Redacted Diagnostic Report** (Markdown/JSON).
8. **`08 // SETTINGS` (Personalization & System Preferences):**
   - Dark / Light theme toggle, **Unrestricted Application Theme Accent Color Picker** with live WCAG 2.2 contrast ratio meter, telemetry polling rate selector (`500 ms` – `5000 ms`), reduced-motion toggle, compact navigation toggle, system tray preferences, and profile import/export.
