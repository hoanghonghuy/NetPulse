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
#define APP_VERSION L"1.3.0"
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
    bool autoStartAsAdmin;           // Auto-start as Administrator (via Task Scheduler)
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
    
    // Data Usage Alerts
    bool enableDataUsageAlerts;      // Enable data usage alert feature
    double dataQuotaGB;              // Monthly data quota in GB (0 = disabled)
    int dataAlertThreshold1;         // First alert threshold percentage (default: 80)
    int dataAlertThreshold2;         // Second alert threshold percentage (default: 100)

    // Floating Window
    bool showFloatingWindow;         // Show floating desktop widget
    int floatingWindowX;             // Floating window X position
    int floatingWindowY;             // Floating window Y position
    BYTE floatingWindowOpacity;      // Floating window opacity (0-255)
    bool floatingShowNetwork;        // Show network speed in floating window
    bool floatingShowCPU;            // Show CPU usage in floating window
    bool floatingShowRAM;            // Show RAM usage in floating window

    AppConfig()
        : updateInterval(DEFAULT_UPDATE_INTERVAL)
        , displayUnit(SpeedUnit::KiloBytesPerSecond)
        , autoStart(false)
        , autoStartAsAdmin(false)
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
        , enableDataUsageAlerts(false)
        , dataQuotaGB(0.0)
        , dataAlertThreshold1(80)
        , dataAlertThreshold2(100)
        , showFloatingWindow(false)
        , floatingWindowX(-1)
        , floatingWindowY(-1)
        , floatingWindowOpacity(200)
        , floatingShowNetwork(true)
        , floatingShowCPU(true)
        , floatingShowRAM(true)
    {
    }

    bool operator==(const AppConfig& other) const
    {
        return updateInterval == other.updateInterval &&
               displayUnit == other.displayUnit &&
               autoStart == other.autoStart &&
               autoStartAsAdmin == other.autoStartAsAdmin &&
               enableLogging == other.enableLogging &&
               debugLogging == other.debugLogging &&
               darkTheme == other.darkTheme &&
               themeMode == other.themeMode &&
               historyAutoTrimDays == other.historyAutoTrimDays &&
               language == other.language &&
               selectedInterface == other.selectedInterface &&
               enableConnectionNotification == other.enableConnectionNotification &&
               pingTarget == other.pingTarget &&
               pingIntervalMs == other.pingIntervalMs &&
               hotkeyModifier == other.hotkeyModifier &&
               hotkeyKey == other.hotkeyKey &&
               overlayFontSize == other.overlayFontSize &&
               overlayDownloadColor == other.overlayDownloadColor &&
               overlayUploadColor == other.overlayUploadColor &&
               enableDataUsageAlerts == other.enableDataUsageAlerts &&
               // Use epsilon for float comparison if needed, but direct match is fine for settings
               dataQuotaGB == other.dataQuotaGB && 
               dataAlertThreshold1 == other.dataAlertThreshold1 &&
               dataAlertThreshold2 == other.dataAlertThreshold2 &&
               showFloatingWindow == other.showFloatingWindow &&
               floatingWindowX == other.floatingWindowX &&
               floatingWindowY == other.floatingWindowY &&
               floatingWindowOpacity == other.floatingWindowOpacity &&
               floatingShowNetwork == other.floatingShowNetwork &&
               floatingShowCPU == other.floatingShowCPU &&
               floatingShowRAM == other.floatingShowRAM;
    }

    bool operator!=(const AppConfig& other) const
    {
        return !(*this == other);
    }
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_COMMON_H

