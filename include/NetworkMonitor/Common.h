// ============================================================================
// File: Common.h
// Description: Common definitions, constants, and structures for NetworkMonitor
// Author: NetworkMonitor Project
// ============================================================================

#ifndef NETWORK_MONITOR_COMMON_H
#define NETWORK_MONITOR_COMMON_H

// ============================================================================
// WINDOWS HEADERS - PHẢI THEO THỨ TỰ NÀY
// ============================================================================

// Prevent minimal windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Prevent winsock.h (version 1) from being included
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

// Include winsock2 TRƯỚC windows.h
#include <winsock2.h>
#include <ws2tcpip.h>

// Include windows.h
#include <windows.h>

// Include winternl.h để có NTSTATUS definition
#include <winternl.h>

// ============================================================================
// STANDARD C++ HEADERS
// ============================================================================

#include <string>
#include <cstdint>

// ============================================================================
// CONSTANTS
// ============================================================================

// Application Information
#define APP_NAME L"NetworkMonitor"
#define APP_VERSION L"1.0.0"
#define APP_WINDOW_CLASS L"NetworkMonitorWindowClass"

// Update Intervals (milliseconds)
constexpr UINT UPDATE_INTERVAL_FAST = 1000;      // 1 second
constexpr UINT UPDATE_INTERVAL_NORMAL = 2000;    // 2 seconds
constexpr UINT UPDATE_INTERVAL_SLOW = 5000;      // 5 seconds

// Default Settings
constexpr UINT DEFAULT_UPDATE_INTERVAL = UPDATE_INTERVAL_NORMAL;
constexpr int DEFAULT_HISTORY_AUTO_TRIM_DAYS = 0;
constexpr int MAX_HISTORY_AUTO_TRIM_DAYS = 365;

// Message IDs
#define WM_TRAYICON (WM_USER + 1)
#define WM_UPDATE_STATS (WM_USER + 2)

// Menu IDs
#define IDM_SETTINGS 1001
#define IDM_ABOUT 1002
#define IDM_EXIT 1003
#define IDM_AUTOSTART 1004
#define IDM_UPDATE_FAST 1005
#define IDM_UPDATE_NORMAL 1006
#define IDM_UPDATE_SLOW 1007
#define IDM_SHOW_TASKBAR_OVERLAY 1008
#define IDM_DASHBOARD 1009

// Tray Icon ID
#define ID_TRAY_ICON 2001

// Timer IDs
#define TIMER_UPDATE_NETWORK 3001
#define TIMER_PING 3002

// Hotkey IDs
#define HOTKEY_TOGGLE_OVERLAY 4001

// ============================================================================
// NAMESPACE DECLARATION
// ============================================================================

namespace NetworkMonitor
{

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Network speed units
enum class SpeedUnit
{
    BytesPerSecond,     // B/s
    KiloBytesPerSecond, // KB/s
    MegaBytesPerSecond, // MB/s
    MegaBitsPerSecond   // Mbps
};

// Application UI language
enum class AppLanguage
{
    SystemDefault = 0,
    English = 1,
    Vietnamese = 2,
    Japanese = 3,
    Korean = 4,
    ChineseSimplified = 5
};

// Application theme mode
enum class ThemeMode
{
    SystemDefault = 0,
    Light = 1,
    Dark = 2
};

// Network statistics for a single interface
struct NetworkStats
{
    std::wstring interfaceName;      // Interface name (e.g., "Ethernet", "Wi-Fi")
    std::wstring interfaceDesc;      // Interface description
    ULONG64 bytesReceived;           // Total bytes received
    ULONG64 bytesSent;               // Total bytes sent
    ULONG64 prevBytesReceived;       // Previous bytes received (for delta calculation)
    ULONG64 prevBytesSent;           // Previous bytes sent (for delta calculation)
    double currentDownloadSpeed;     // Current download speed (bytes/sec)
    double currentUploadSpeed;       // Current upload speed (bytes/sec)
    double peakDownloadSpeed;        // Peak download speed (bytes/sec)
    double peakUploadSpeed;          // Peak upload speed (bytes/sec)
    bool isActive;                   // Is interface active?
    DWORD lastUpdateTime;            // Last update timestamp (GetTickCount)

    NetworkStats()
        : bytesReceived(0)
        , bytesSent(0)
        , prevBytesReceived(0)
        , prevBytesSent(0)
        , currentDownloadSpeed(0.0)
        , currentUploadSpeed(0.0)
        , peakDownloadSpeed(0.0)
        , peakUploadSpeed(0.0)
        , isActive(false)
        , lastUpdateTime(0)
    {
    }
};

// Application configuration
struct AppConfig
{
    UINT updateInterval;             // Update interval in milliseconds
    SpeedUnit displayUnit;           // Display unit for speed
    bool autoStart;                  // Auto-start with Windows
    bool enableLogging;              // Enable history logging
    bool debugLogging;               // Enable debug logging to file
    bool darkTheme;
    ThemeMode themeMode;             // Theme selection mode
    int historyAutoTrimDays;
    AppLanguage language;            // UI language
    std::wstring selectedInterface;  // Selected interface name (empty = all)
    bool enableConnectionNotification; // Show notification on connect/disconnect
    std::wstring pingTarget;         // Ping target IP/domain (default: 8.8.8.8)
    UINT pingIntervalMs;             // Ping interval in milliseconds (default: 5000)
    UINT hotkeyModifier;             // Hotkey modifier (MOD_WIN | MOD_SHIFT, etc.)
    UINT hotkeyKey;                  // Hotkey virtual key code (default: 'N')
    int overlayFontSize;             // Overlay font size (default: 13)
    COLORREF overlayDownloadColor;   // Overlay download text color (default: cyan)
    COLORREF overlayUploadColor;     // Overlay upload text color (default: green)

    AppConfig()
        : updateInterval(DEFAULT_UPDATE_INTERVAL)
        , displayUnit(SpeedUnit::KiloBytesPerSecond)
        , autoStart(false)
        , enableLogging(true)
        , debugLogging(false)
        , darkTheme(false)
        , themeMode(ThemeMode::SystemDefault)
        , historyAutoTrimDays(DEFAULT_HISTORY_AUTO_TRIM_DAYS)
        , language(AppLanguage::SystemDefault)
        , selectedInterface(L"")
        , enableConnectionNotification(true)
        , pingTarget(L"8.8.8.8")
        , pingIntervalMs(5000)
        , hotkeyModifier(MOD_WIN | MOD_SHIFT)
        , hotkeyKey('N')
        , overlayFontSize(13)
        , overlayDownloadColor(RGB(0, 255, 255))   // Cyan
        , overlayUploadColor(RGB(0, 255, 0))       // Green
    {
    }
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_COMMON_H

