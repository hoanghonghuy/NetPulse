# NetworkMonitor

NetworkMonitor is a lightweight C++ Win32 application for monitoring network traffic on Windows. It displays real-time download/upload speeds in the system tray and taskbar, and logs history to SQLite for statistics.

## Main Features

- **Tray icon**: displays current speed, detailed tooltip, icon changes according to load level (idle / active / high).
- **Taskbar overlay**: 2 lines of text (Down / Up) located next to the system tray area, always displaying real-time speed.
- **Speed Test**: Integrated tool to measure download/upload bandwidth and latency.
- **Ping Monitor**: Real-time latency display on overlay with color coding (green <100ms, yellow 100-200ms, red >200ms, "---" on timeout).
- **Keyboard Shortcut**: Press `Win+Shift+N` to toggle overlay visibility (customizable in Settings).
- **Connection Notifications**: Optional balloon notification when network connects/disconnects.
- **Multiple Network Interfaces**: can monitor all interfaces or select a specific interface.
- **Dashboard**:
  - Total traffic **Today / This month**.
  - List of recent samples (time, interface, download, upload).
  - Simple charts based on historical data.
- **History logging (SQLite)**:
  - Record usage by interval to the file `network_usage.db` next to `NetworkMonitor.exe`.
  - Automatically clean history by the number of days configured.
  - Dialog "Manage history" to delete all or keep only the last 30 / 90 days.
- **Settings**:
  - Update cycle: Fast (1s) / Normal (2s) / Slow (5s).
  - Display units: B/s, KB/s, MB/s, Mbps.
  - Autostart with Windows.
  - Turn on/off history recording.
  - Select the interface to monitor.
  - Select UI language: System / English / Vietnamese.
  - Configure ping target (IP/domain) and interval.
  - Customize hotkey for overlay toggle.
  - Enable/disable connection notifications.
- **Per-App Usage**: Monitor network usage per application (process). **Requires Administrator privileges.**
- **Floating Widget**: Desktop widget for quick stats overview (Network, CPU, RAM).
- **System Monitor**: Real-time CPU and RAM usage monitoring.
- **Data Usage Alerts**: Monthly quota tracking and configurable alerts.
- **Multilingual (i18n)**:
  - All menus, dialogs, message boxes use STRINGTABLE.
  - Supports: English, Vietnamese, Japanese, Korean, Chinese (Simplified).
- **Performance**:
  - Very light background application, CPU almost 0%, RAM only a few MB.

## Requirements

- **OS**: Windows 10/11 (Win7+ can build but not tested thoroughly).
- **Architecture**: x64.
- **Build**:
  - Visual Studio 2019/2022 with Windows 10 SDK (C++17).
  - Or CMake ≥ 3.15 + MSVC.

## Build from source

### Visual Studio (recommended)

1. Clone repo:

```bash
git clone https://github.com/hoanghonghuy/NetworkMonitor.git
cd NetworkMonitor
```

2. Open `NetworkMonitor.sln` in Visual Studio.
3. Select the **Release | x64** configuration.
4. Build solution (`Ctrl+Shift+B`).
5. Run `NetworkMonitor.exe` in the output folder.

### CMake

1. Clone the repo and create a build folder:

```bash
git clone https://github.com/hoanghonghuy/NetworkMonitor.git
cd NetworkMonitor
mkdir build
cd build
```

2. Generate & build (example for VS 2022 x64):

```bash
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

3. Run `NetworkMonitor.exe` in `build/Release`.

### Run tests with CMake

To enable and run the test suite using CMake (Debug configuration):

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build --config Debug
cd build
ctest -C Debug --output-on-failure
```

## Usage

- After running, the application is located in the **system tray**.
- **Right‑click** tray icon to open the menu:
  - Update Interval.
  - Start with Windows.
  - Dashboard…
  - Settings…
  - About…
  - Exit.
- **Keyboard shortcut**: Press `Win+Shift+N` to toggle overlay visibility.
- **Dashboard**:
  - Open from the context menu.
  - **Refresh** button to update data.
  - **Manage history…** button to clean the DB (delete all / keep 30 / 90 days).
- **Settings**:
  - Adjust update interval, display unit, logging, auto‑start, interface and language.
  - Configure ping target, interval, and hotkey in the Advanced section.
  - Changing the Language will apply to the texts retrieved from the resource (menu, dialog, message).

## Architecture overview

The codebase is organized into clear layers:

- **src/entry**
  - `main.cpp`: Win32 `WinMain` entry. Creates a single-instance mutex and runs `NetworkMonitor::Application`.

- **src/core**
  - `Application`: main controller that owns the hidden message window, timer, `ConfigManager`, `NetworkMonitorClass`, `TrayIcon`, `TaskbarOverlay` and drives the app lifecycle (`Initialize`, `Run`, `Cleanup`).
  - `NetworkMonitorClass`: collects per-interface traffic statistics using Win32/IP Helper APIs.
  - `ConfigManager`: loads/saves settings (registry) into `AppConfig`.
  - `HistoryLogger`: singleton that logs samples to SQLite and exposes queries used by the dashboard.
  - `PingMonitor`: ICMP ping monitor using Windows ICMP API.
  - `NetworkCalculator`, `Utils` helpers.

- **src/ui**
  - `TrayIcon`: creates the system tray icon, context menu and routes menu commands back to `Application::OnMenuCommand`.
  - `TaskbarOverlay`: draws the 2-line speed overlay window near the taskbar and forwards right-clicks to the tray menu.

- **src/ui/dialogs**
  - `SettingsDialog`: modal settings dialog; edits `AppConfig` through `ConfigManager`.
  - `DashboardDialog`: shows Today/This month totals, chart and recent samples, backed by `HistoryLogger`.
  - `HistoryDialog`: simple dialog that calls `HistoryLogger::DeleteAll` / `TrimToRecentDays`.

- **resources**
  - `app.rc`, `resource.h`, icons, manifest and all STRINGTABLEs (EN/VI) for menus and dialogs.

High-level data flow:

1. `WinMain` → `Application::Initialize()` sets up window class, hidden window, timer and components.
2. Timer (`WM_TIMER`) → `Application::OnTimer()` updates `NetworkMonitorClass`, logs samples via `HistoryLogger` and refreshes tray icon & overlay.
3. Tray menu commands → `Application::OnMenuCommand()` (update interval, settings, dashboard, history, about, exit).
4. Dialogs are created by `Application` as needed and operate purely on `AppConfig`/`HistoryLogger`, keeping UI logic separate from core.

## Configuration & Data

- **Registry**: save configuration under key

```text
HKEY_CURRENT_USER\Software\NetworkMonitor
```

Includes: `UpdateInterval`, `DisplayUnit`, `EnableLogging`, `HistoryAutoTrimDays`,
`Language`, `SelectedInterface`, `PingTarget`, `PingIntervalMs`, `HotkeyModifier`, `HotkeyKey`, and auto‑start status (Run key).

- **History**:
  - SQLite file: `network_usage.db` placed next to `NetworkMonitor.exe`.
  - Table `usage(timestamp, interface, bytes_down, bytes_up)` + index by `timestamp`.
  - Automatically trim by `HistoryAutoTrimDays` if value > 0.

- **Auto‑start**:
  - Use `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` with value name `NetworkMonitor`.

## License

The project is released under the **MIT** license. See the `LICENSE` file for details.