#include "NetworkMonitor/ThemeHelper.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace NetworkMonitor
{

// Undocumented definitions for dark mode support
enum class PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnAllowDarkModeForApp = bool(WINAPI*)(bool allow);

// DWMWA_USE_IMMERSIVE_DARK_MODE values
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

void ThemeHelper::Initialize()
{
    // No-op, initialization happens on demand
}

void ThemeHelper::AllowDarkModeForApp(bool enable)
{
    static bool s_initialized = false;
    static fnSetPreferredAppMode s_pSetPreferredAppMode = nullptr;
    static fnAllowDarkModeForApp s_pAllowDarkModeForApp = nullptr;

    if (!s_initialized)
    {
        HMODULE hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (hUxTheme)
        {
            // Ordinal 135 is SetPreferredAppMode (Win10 1903+)
            s_pSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135)));
            
            // Ordinal 132 is AllowDarkModeForApp (Win10 1809)
            s_pAllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(132)));
        }
        s_initialized = true;
    }

    if (s_pSetPreferredAppMode)
    {
        s_pSetPreferredAppMode(enable ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
    }
    else if (s_pAllowDarkModeForApp)
    {
        s_pAllowDarkModeForApp(enable);
    }
}

void ThemeHelper::ApplyDarkTitleBar(HWND hwnd, bool enable)
{
    if (!hwnd) return;

    BOOL value = enable ? TRUE : FALSE;
    
    // Try the modern attribute first (Windows 11, Win 10 20H1+)
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    
    if (FAILED(hr))
    {
        // Fallback to the older undocumented attribute (Windows 10 1809-1909)
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &value, sizeof(value));
    }

    // Force a repaint of the non-client area (title bar)
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool ThemeHelper::IsSystemInDarkMode()
{
    // Check registry for AppsUseLightTheme
    // 0 = Dark, 1 = Light
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS)
    {
        return false; // Default to light if key missing
    }

    DWORD useLightTheme = 1;
    DWORD dataSize = sizeof(useLightTheme);
    DWORD type = REG_DWORD;

    result = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, &type, reinterpret_cast<LPBYTE>(&useLightTheme), &dataSize);
    
    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS)
    {
        return (useLightTheme == 0);
    }

    return false;
}

} // namespace NetworkMonitor
