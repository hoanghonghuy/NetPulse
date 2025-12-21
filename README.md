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

### CMake

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