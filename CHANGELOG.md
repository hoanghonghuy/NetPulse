# Changelog

All notable changes to NetworkMonitor will be documented in this file.

## [1.4.0] - 2025-12-13

### Added
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
