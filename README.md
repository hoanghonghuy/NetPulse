# NetPulse

NetPulse is a lightweight C++ Win32 application for monitoring network traffic on Windows. It displays real-time download/upload speeds in the system tray, taskbar overlay, and desktop widget, while offering advanced tools like Speed Test, Connection Monitoring, and History logging.

## Main Features

- **System Integration**:
  - **Tray Icon**: Displays current speed, detailed tooltip, and animated icon based on traffic load.
  - **Taskbar Overlay**: unobtrusive 2-line speed display (Down/Up) right on your taskbar.
  - **Floating Widget**: Desktop widget showing Network (with Sparkline graph), CPU, and RAM usage.
  - **VPN & Proxy Detection**: Automatically detects active VPN connections (OpenVPN, WireGuard, etc.) and displays your real-time Public IP.

- **Network Tools**:
  - **Speed Test**: Integrated bandwidth test for Download, Upload, and Ping latency with history tracking.
  - **Ping Monitor**: Real-time latency tracking with color-coded status (Green/Yellow/Red).
  - **Connection Log (Mini DPI)**: Real-time monitoring of active TCP/UDP connections, processes, and remote hosts.
  - **Data Usage Monitor**: Track daily/monthly usage with configurable quotas and alerts.

- **Dashboard & Analytics**:
  - **Interactive Charts**: Visualize traffic history with hover tooltips and drill-down capabilities (Monthly -> Daily).
  - **History Logging**: SQLite-based logging of all network activity.
  - **Export Data**: Export usage history to CSV or save Dashboard charts as images (BMP).

- **Customization & UI**:
  - **Native Dark Mode**: Fully themed UI including menus, dialogs, and tooltips to match Windows 11/10 aesthetics.
  - **Theme Presets**: 13 built-in themes including Dark, Light, Dracula, Cyberpunk, Nord, Forest, OLED Black, Solarized Light, Morning Mist, Soft Paper, Mint Fresh, Lavender, and Rose Pink.
  - **Multilingual**: Complete support for English, Vietnamese, Japanese, Korean, and Chinese (Simplified).
  - **Portable Mode**: Run entirely from a USB drive by simply placing a `netpulse.ini` file next to the executable.

## Requirements

- **OS**: Windows 10/11 x64.
- **Build**: Visual Studio 2019/2022 (C++17) or CMake ≥ 3.15.

## Build from Source

### Visual Studio (Recommended)

1. Clone the repository:
   ```bash
   git clone https://github.com/hoanghonghuy/NetPulse.git
   cd NetPulse
   ```
2. Open `NetPulse.sln` in Visual Studio.
3. Select **Release | x64** configuration.
4. Build solution (`Ctrl+Shift+B`).
5. Run `NetPulse.exe` from the output directory.

### CMake (recommended)

Build settings (definitions, warnings, link libraries) are centralized in `cmake/NetPulseDefaults.cmake`. When changing compiler or linker options, update that file and keep `NetPulse.vcxproj` in sync for Visual Studio users.

1. Clone and prepare build directory:
   ```bash
   git clone https://github.com/hoanghonghuy/NetPulse.git
   cd NetPulse
   mkdir build && cd build
   ```
2. Generate and build:
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```

## Running Tests

Build and run the unit test suite with CMake:

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON
cmake --build build --config Debug
ctest -C Debug --output-on-failure --verbose
```

The test target is `NetPulseTests` and uses the same defaults as the main app (including embedded `app.rc` resources).

Tests use a temporary sandbox directory and a separate registry key (`Software\NetworkMonitor\Test`) so they do not modify your normal history or settings.

Run only tests with a specific CTest label (same binary, grouped for CI filtering):

```bash
ctest -C Debug -L unit --output-on-failure
```

E2E smoke tests run in a separate target (sandbox registry/data, no user profile):

```bash
ctest -C Debug -L e2e --output-on-failure
```

Subprocess harness for `NetPulse.exe` (timeout + log capture):

```powershell
.\scripts\run-e2e.ps1 -Scenario launch-exit -TimeoutSec 30
```

CLI flags: `--test-mode`, `--sandbox-dir=...`, `--test-scenario=launch-exit|config-default`.

### Code coverage (Phase 0 baseline)

Coverage is measured from `NetPulseTests` and is **not gated** in PRs yet. CI uploads HTML + Cobertura XML on every run (job `Coverage (Debug)`).

**MSVC / OpenCppCoverage (recommended on Windows):**

```powershell
.\scripts\run-coverage.ps1 -Backend opencpp -Config Debug
```

**LLVM MinGW (local dev):**

```powershell
.\scripts\run-coverage.ps1 -Backend llvm
```

Reports exclude `third_party/**` (including `sqlite3.c`) and `tests/**`. Baseline notes live in `.roo-local/design/COVERAGE-BASELINE.md`.

## Usage

- **Tray Menu**: Right-click the system tray icon to access:
  - **Dashboard**: View charts, history, and usage details.
  - **Speed Test**: Run bandwidth tests.
  - **Connection Log**: View active connections.
  - **Check for Updates**: Check for the latest version.
  - **Settings**: Configure language, units, auto-start, and more.
- **Shortcuts**:
  - `Win+Shift+N`: Toggle Taskbar Overlay visibility (customizable).
  - `F5/F6`: Adjust brightness (if supported).

## Configuration

Settings are saved in the Registry (`HKCU\Software\NetworkMonitor`) or in `netpulse.ini` (Portable Mode).
- **Auto-start**: Option to start with Windows (Standard or Administrator privileges).
- **History**: SQLite database `network_usage.db` stores all traffic data.

## License

This project is licensed under the **MIT License**. See the `LICENSE` file for details.