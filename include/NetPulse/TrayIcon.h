#ifndef NETWORK_MONITOR_TRAYICON_H
#define NETWORK_MONITOR_TRAYICON_H

#include "NetPulse/Common.h"
#include <windows.h>
#include <shellapi.h>
#include <functional>

#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif

namespace NetPulse
{

class TrayIcon
{
public:
    TrayIcon();
    ~TrayIcon();

    /**
     * Initialize and create tray icon
     * @param hwnd Parent window handle
     * @return true if successful, false otherwise
     */
    bool Initialize(HWND hwnd);

    /**
     * Cleanup and remove tray icon
     */
    void Cleanup();

    /**
     * Update tray icon tooltip with network statistics
     * @param stats Network statistics to display
     * @param unit Display unit for speed
     */
    void UpdateTooltip(const NetworkStats& stats, SpeedUnit unit);

    /**
     * Update tray icon based on traffic activity
     * @param downloadSpeed Current download speed in bytes/sec
     * @param uploadSpeed Current upload speed in bytes/sec
     */
    void UpdateIcon(double downloadSpeed, double uploadSpeed);

    /**
     * Handle tray icon messages
     * @param message Message ID
     * @param wParam WPARAM parameter
     * @param lParam LPARAM parameter
     * @return true if message handled, false otherwise
     */
    bool HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    /**
     * Show context menu at cursor position
     */
    void ShowContextMenu();

    /**
     * Set callback for menu item selection
     * @param callback Callback function receiving menu item ID
     */
    void SetMenuCallback(std::function<void(UINT)> callback);

    /**
     * Provide pointer to current configuration (for reflecting menu state)
     */
    void SetConfigSource(const AppConfig* config);

    /**
     * Provide callback to query taskbar overlay visibility state
     */
    void SetOverlayVisibilityProvider(std::function<bool()> provider);

    /**
     * Show balloon notification
     * @param title Notification title
     * @param message Notification message
     */
    void ShowBalloonNotification(const std::wstring& title, const std::wstring& message);

    /**
     * Provide callback to query floating window visibility state
     */
    void SetFloatingWindowVisibilityProvider(std::function<bool()> provider);

    /**
     * Set callback for double-click on tray icon
     */
    void SetDoubleClickCallback(std::function<void()> callback);

    /**
     * Refresh tray icon for theme change
     * @param useDarkTheme true to use dark icon variant
     */
    void RefreshIcon(bool useDarkTheme);

private:
    // Helper to create context menu
    HMENU CreateContextMenu(const AppConfig& config, bool overlayVisible, bool floatingVisible);

    /**
     * Try to restore the icon if it's missing
     */
    void RestoreIcon();

    /**
     * Load application icon
     */
    HICON LoadAppIcon();

    /**
     * Start animation timer for high traffic pulse effect
     */
    void StartAnimation();
    
    /**
     * Stop animation timer and reset to static icon
     */
    void StopAnimation();

    // Make Application class a friend to access menu handlers
    friend class Application;

private:
    HWND m_hwnd;                                    // Parent window handle
    NOTIFYICONDATAW m_notifyIconData;               // Notify icon data structure
    bool m_initialized;                             // Is initialized?
    HICON m_iconIdle;                               // Icon when idle
    HICON m_iconActive;                             // Icon when active
    HICON m_iconHigh;                               // Icon when high traffic
    HICON m_iconIdleDark;                           // Dark theme idle icon
    HICON m_iconActiveDark;                         // Dark theme active icon
    HICON m_iconHighDark;                           // Dark theme high traffic icon
    std::function<void(UINT)> m_menuCallback;       // Menu selection callback
    const AppConfig* m_configRef;                   // Current config reference
    std::function<bool()> m_overlayVisibleProvider; // Overlay visibility provider
    std::function<bool()> m_floatingVisibleProvider; // Floating window visibility provider
    std::function<void()> m_doubleClickCallback;    // Double-click callback
    
    // Animation members
    static const UINT_PTR ANIMATION_TIMER_ID = TIMER_TRAY_ANIMATION;
    bool m_animating;                               // Is animation running
    int m_animationPhase;                           // Current animation frame (0 or 1)
    
public:
    /**
     * Handle animation timer tick (called from parent window)
     */
    void OnAnimationTick();
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_TRAYICON_H
