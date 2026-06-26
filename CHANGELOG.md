# Changelog

## [2.5.0] - 2026-06-26

### Added
- **Lọc card mạng ảo khi Aggregate**: Thêm logic phân biệt và lọc các card mạng ảo (VPN, Hyper-V, Docker, v.v.) khi chọn "All Interfaces" (Tất cả giao diện) để tránh bị tính đè cộng dồn lưu lượng (double-counting). Tự động fallback sử dụng tất cả card mạng hoạt động nếu không phát hiện card vật lý nào (ví dụ khi chạy trong máy ảo).

## [2.4.2] - 2026-06-13

### Fixed
- **Floating Window**: Thêm cơ chế duy trì trạng thái luôn nổi ở trên cùng (topmost) định kỳ 150ms và tự động ẩn khi phát hiện có ứng dụng/trò chơi đang chạy ở chế độ toàn màn hình (fullscreen/borderless) hoặc xem video toàn màn hình để tránh làm gián đoạn trải nghiệm người dùng.

## [2.4.1] - 2026-06-13

### Fixed
- **Overlay Taskbar**: Sửa lỗi overlay bị ẩn khi người dùng tương tác, nhấn chuột phải vào màn hình Desktop (Progman/WorkerW) hoặc Taskbar chính/phụ.

## [2.4.0] - 2026-06-13

### Added
- **Bộ Cài Đặt (Inno Setup)**: Thiết kế bộ cài đặt hệ thống `NetPulse_Setup.exe` tự động cấu hình và tạo Scheduled Task quyền quản trị cao nhất lúc đăng nhập để tự chạy không hiển thị UAC prompt.
- **Tính Năng Bảo Vệ Portable Mode**: Ẩn hoàn toàn tùy chọn chế độ di động và tạo file INI khi ứng dụng đang chạy từ thư mục hệ thống `Program Files` để tránh lỗi ghi đè dữ liệu hoặc Access Denied.
- **Tài Liệu Quy Trình Phát Hành**: Bổ sung hướng dẫn chi tiết quy trình gộp nhánh, đóng gói bản nén di động ZIP và tạo Release.

## [2.3.0] - 2025-12-22

### Added
- **Unified Theming System**: Introduced a new, robust `ThemeColors` architecture to standardize and manage application themes, enabling easy expansion and consistent styling.
- **13 Theme Presets**: The application now features a comprehensive collection of 13 high-quality built-in themes:
  - **Dark**: Standard Dark, Dracula, Cyberpunk (Neon), Nord, Forest (Matcha), OLED Black.
  - **Light**: Standard Light, Solarized Light, Morning Mist (Blue), Soft Paper (Warm/Sepia), Mint Fresh (Green), Lavender (Purple), Rose Pink.
- **Check for Updates**: New menu item in the Tray Icon context menu to manually check for application updates.

### Changed
- **Codebase Cleanup**: Removed unused ImGui library (~500KB) and legacy build artifacts for a cleaner, smaller repository.
- **Updated .gitignore**: Added `temp_build/` to prevent CMake artifacts from being tracked.
- **README**: Added complete list of 13 theme presets.

### Fixed
- **Dark Theme Polish**: Resolved remaining dark mode issues for consistent UI rendering across all dialogs.

## [2.2.0] - 2025-12-20

### Improved
- **Native Dark Mode Menus**: Transitioned Tray Menu from legacy Owner-Draw to Native Windows Menu to resolve white border issues and improve Windows 11 compatibility.
- **Dashboard Tooltips**: Completely rewritten chart tooltips using Window Subclassing to support full Dark Mode (Dark background, Light text, Custom border), removing the default system style.

### Fixed
- **UI Consistency**: Fixed various themed controls (Buttons, Edit Boxes) to respect Dark Mode settings immediately without restart.
- **Code Cleanup**: Removed legacy owner-draw code and unused headers.

## [2.1.0] - 2025-12-18

### Added
- **Detailed Connection Log (Mini DPI)**: Real-time monitoring of active TCP/UDP connections. Accessible via Tray Icon.
- **Portable Mode**: Zero-dependency mode. Automatically activates if `netpulse.ini` exists next to the executable.
- **Interactive Charts**: Hover tooltips for detailed stats and click-to-drill-down (Monthly -> Daily).

### Changed
- **Chart Visuals**: Optimized date labels to prevent overflow using abbreviated formats.
- **Localization**: Full translation support for Connection Log dialog.

## [2.0.0] - 2025-12-16

### Added
- **Integrated Bandwidth Speed Test**: New dedicated dialog for testing download, upload, and ping latency. Includes history tracking and dark theme support.
- **Floating Window Position Safety**: Automatic reset logic to ensure the floating widget remains visible when changing DPI settings or monitor configurations.

### Changed
- **High-DPI Support**: Switched application DPI awareness to `GDI Scaling` mode to resolve text blurriness on high-resolution displays (2K/4K).
- **Dark Theme Overhaul**:
  - Unified dark theme across all dialogs (About, Speed Test, Settings).
  - Modernized window frames (removed white/classic borders in dark mode).
  - Implemented custom owner-draw rendering for buttons to ensure correct contrast.

### Fixed
- **Floating Window Visibility**: Resolved issue where the floating widget would disappear off-screen due to DPI coordinate mismatches.
- **Dialog Frame Rendering**: Fixed inconsistent title bar and border rendering in the About and Speed Test dialogs.
- **Theme Conflicts**: Removed redundant theme application logic (`ApplyToDialog`) that caused visual glitches.

## [1.5.0] - 2025-12-14

### Added
- **Integrated Bandwidth Speed Test**: Perform download, upload, and ping tests directly within the application.
- **Full Localization**: Complete translation support for all UI elements across 5 languages.
- **GDI Resource Optimization**: Comprehensive audit and cleanup of GDI objects for better stability.

### Changed
- **UI Standardization**: Unified Dark Mode theming for buttons, lists, and tabs using `DialogThemeHelper`.
- **Unit Testing**: Updated test suite for better isolation and API alignment.
- **CI/CD**: Fixed build pipeline reliability issues.

### Fixed
- Reserved linker errors in build system.
- Corrected various minor UI inconsistencies in Dark Mode.

## [1.4.0] - 2025-12-13

### Added
- **VPN & Proxy Detection (Phase 3)**: Automatically detects VPN connections and proxy settings.
  - Shows VPN/Proxy indicator in Floating Window with color-coded status.
  - Displays current public IP address with periodic updates (configurable interval).
  - Detects popular VPN adapters: OpenVPN, WireGuard, NordVPN, Tailscale, etc.
- **Sparkline Visualization**: Real-time network traffic graph in Floating Window.
- **Tray Icon Animation**: Visual pulse effect when traffic exceeds configurable threshold.
- **Chart Export**: Export Dashboard network chart as BMP image (located next to CSV export).
- **History Export**: Export detailed network usage history to CSV.
- **Time Range Settings**: Configurable sparkline history duration (30s, 1m, 5m) via Settings > Display.

### Changed
- **UI Refinement**: Moved sparkline settings from context menu to main Settings dialog for better accessibility and localization.
- **Localization**: Comprehensive translation support for all new features in 5 languages (EN, VI, JA, KO, ZH-CN).

## [1.3.1] - 2025-12-10

### Added
- **Run as Administrator Auto-Start**: New checkbox option to start with Windows using elevated privileges via Task Scheduler.
- **Enhanced Debug Logging**: Replaced ephemeral debug prints with a persistent file-based logging system (`%LOCALAPPDATA%\NetworkMonitor\NetworkMonitor.log`) for easier troubleshooting.

### Fixed
- **Settings Dialog Reliability**: Rewrote "dirty check" logic to use value-based comparison instead of event-based tracking. Settings are now only saved when actual values change.
- **Phantom UAC Prompts**: Decoupled Auto-Start logic from general settings saving. UAC prompts now only appear when enabling/disabling Admin Auto-Start, not when changing language or theme.
- **Dark Theme Checkboxes**: Fixed multiple issues with owner-draw checkboxes in dark mode (unresponsive clicks, paint coalescing lag, missing handler for Data Usage checkbox).

### Changed
- **IConfigProvider Interface**: Updated `SetAutoStart` signature to support admin mode parameter.

## [1.3.0] - 2025-12-09

### Added
- **Per-App Usage**: Monitor network usage per application (process).
- **Floating Widget**: Desktop widget for quick stats overview.
- **System Monitor**: CPU/RAM usage monitoring.
- **Data Usage Alerts**: Monthly quota tracking and alerts.

### Changed
- **Dialog Management**: Improved single-instance behavior for Per-App and About dialogs.

### Fixed
- **Critical Crash**: Fixed assertion failure in LanguageManager when changing languages.

## [1.2.0] - 2025-12-07

### Added
- **Expanded Language Support**: Added Japanese, Korean, and Chinese (Simplified).
- **Dark Theme Polish**: Implemented native-like dark mode for scrollbars, ComboBox dropdown lists, and checkboxes.
- **Localization**: Full internationalization of remaining hardcoded UI strings (buttons, tabs, dialog labels).

### Changed
- **Brightness Control**: 
  - Optimized response time for F5/F6 shortcuts (native-like performance).
  - Completely removed annoyance popup notifications during brightness changes.
- **Codebase Helper**: Removed redundant file banner headers from all source files for cleaner code.

### Fixed
- **Settings Persistence**: Fixed critical bug where selected language was not saving/loading correctly on restart.
- **UI Consistency**: Fixed visual artifacts in dark mode for standard Windows controls.

## [1.1.0] - 2025-12-06

### Added
- **Ping Monitor**: Real-time latency display on overlay with color coding
  - Green (<100ms), Yellow (100-200ms), Red (>200ms or timeout)
  - Configurable ping target (IP/domain) and interval
- **Keyboard Shortcut**: `Win+Shift+N` to toggle overlay (customizable)
- **Connection Notifications**: Balloon notification on network connect/disconnect
- **Settings UI**: New Advanced section for ping and hotkey configuration

### Changed
- Enhanced CI workflow with Debug/Release matrix builds
- Updated README with new features

## [1.0.0] - 2025-11-22

### Added
- Initial release
- System tray icon with traffic monitoring
- Taskbar overlay showing download/upload speeds
- Dashboard with today/monthly statistics
- History logging to SQLite
- Settings dialog with language support (EN/VI)
- Dark theme support
- Auto-start with Windows option
