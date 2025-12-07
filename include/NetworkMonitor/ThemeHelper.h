#ifndef NETWORK_MONITOR_THEMEHELPER_H
#define NETWORK_MONITOR_THEMEHELPER_H

#include <windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace NetworkMonitor
{

class ThemeHelper
{
public:
    /**
     * Enable dark mode support for the entire application process.
     * Should be called during application initialization.
     * Affects context menus and some common controls.
     * @param enable true to enable dark mode support
     */
    static void AllowDarkModeForApp(bool enable);

    /**
     * Apply dark mode to a specific window's title bar.
     * Must be called for each top-level window/dialog.
     * @param hwnd Window handle
     * @param enable true to enable dark title bar
     */
    static void ApplyDarkTitleBar(HWND hwnd, bool enable);

    /**
     * Check if the system is currently using dark theme for apps.
     * @return true if system is in dark mode
     */
    static bool IsSystemInDarkMode();

    /**
     * Initialize necessary function pointers from DLLs.
     * Called automatically by other methods, but can be called explicitly.
     */
    static void Initialize();
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_THEMEHELPER_H
