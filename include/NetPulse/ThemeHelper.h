#ifndef NETWORK_MONITOR_THEMEHELPER_H
#define NETWORK_MONITOR_THEMEHELPER_H

#include "NetPulse/ThemeColors.h"
#include "NetPulse/Common.h"
#include <windows.h>
#include <dwmapi.h>

#ifdef _MSC_VER
#pragma comment(lib, "dwmapi.lib")
#endif

namespace NetPulse
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
     * Allow dark mode for a specific window/control.
     * Enables dark scrollbars and other dark-themed elements.
     * Must be called BEFORE setting window theme.
     * Requires Windows 10 1809+.
     * @param hwnd Window handle
     * @param enable true to enable dark mode
     */
    static void AllowDarkModeForWindow(HWND hwnd, bool enable);

    /**
     * Apply dark mode to a specific window's title bar.
     * Must be called for each top-level window/dialog.
     * @param hwnd Window handle
     * @param enable true to enable dark title bar
     */
    static void ApplyDarkTitleBar(HWND hwnd, bool enable);

    /**
     * Apply dark theme to a control including dark scrollbars.
     * Combines AllowDarkModeForWindow + SetWindowTheme.
     * @param hwnd Control handle
     * @param enable true to enable dark theme
     */
    static void ApplyDarkThemeToControl(HWND hwnd, bool enable);

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

    /**
     * Get the current color palette based on theme preference.
     * @param dark true for Dark Mode, false for Light Mode
     * @return Reference to the ThemeColors structure
     */
    static const struct ThemeColors& GetColors(ThemeMode mode);
    static const struct ThemeColors& GetColors(bool dark);

    /**
     * Set the current application theme mode.
     * Use this to update the global theme state so GetColors(bool) returns the correct preset.
     */
    static void SetCurrentTheme(ThemeMode mode);
    static ThemeMode GetCurrentTheme();
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_THEMEHELPER_H
