#include "NetPulse/ThemeHelper.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace NetPulse
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
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND hwnd, bool allow);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();
using fnFlushMenuThemes = void(WINAPI*)();

// DWMWA_USE_IMMERSIVE_DARK_MODE values
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

// Static function pointers (initialized once)
static bool s_initialized = false;
static fnSetPreferredAppMode s_pSetPreferredAppMode = nullptr;
static fnAllowDarkModeForApp s_pAllowDarkModeForApp = nullptr;
static fnAllowDarkModeForWindow s_pAllowDarkModeForWindow = nullptr;
static fnRefreshImmersiveColorPolicyState s_pRefreshImmersiveColorPolicyState = nullptr;
static fnFlushMenuThemes s_pFlushMenuThemes = nullptr;

static void EnsureInitialized()
{
    if (s_initialized)
    {
        return;
    }

    HMODULE hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxTheme)
    {
        // Ordinal 135 is SetPreferredAppMode (Win10 1903+)
        s_pSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135)));
        
        // Ordinal 132 is AllowDarkModeForApp (Win10 1809)
        s_pAllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(132)));

        // Ordinal 133 is AllowDarkModeForWindow (Win10 1809+)
        s_pAllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133)));

        // Ordinal 104 is RefreshImmersiveColorPolicyState
        s_pRefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(104)));

        // Ordinal 136 is FlushMenuThemes
        s_pFlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(136)));
    }
    s_initialized = true;
}

void ThemeHelper::Initialize()
{
    EnsureInitialized();
}

void ThemeHelper::AllowDarkModeForApp(bool enable)
{
    EnsureInitialized();

    if (s_pSetPreferredAppMode)
    {
        s_pSetPreferredAppMode(enable ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
    }
    else if (s_pAllowDarkModeForApp)
    {
        s_pAllowDarkModeForApp(enable);
    }

    if (s_pRefreshImmersiveColorPolicyState)
    {
        s_pRefreshImmersiveColorPolicyState();
    }

    if (s_pFlushMenuThemes)
    {
        s_pFlushMenuThemes();
    }
}

void ThemeHelper::AllowDarkModeForWindow(HWND hwnd, bool enable)
{
    if (!hwnd)
    {
        return;
    }

    EnsureInitialized();

    if (s_pAllowDarkModeForWindow)
    {
        s_pAllowDarkModeForWindow(hwnd, enable);
    }
}

void ThemeHelper::ApplyDarkThemeToControl(HWND hwnd, bool enable)
{
    if (!hwnd)
    {
        return;
    }

    // First allow dark mode for the window
    AllowDarkModeForWindow(hwnd, enable);

    // Check if this is a ComboBox - needs special handling for dropdown list
    wchar_t className[64] = {0};
    GetClassNameW(hwnd, className, 64);
    bool isComboBox = (_wcsicmp(className, L"ComboBox") == 0);

    if (isComboBox && enable)
    {
        // Apply CFD theme to the ComboBox itself for dark appearance
        SetWindowTheme(hwnd, L"CFD", NULL);

        // Get the dropdown list handle and apply dark theme to it
        COMBOBOXINFO cbi = {0};
        cbi.cbSize = sizeof(COMBOBOXINFO);
        if (GetComboBoxInfo(hwnd, &cbi))
        {
            if (cbi.hwndList)
            {
                AllowDarkModeForWindow(cbi.hwndList, true);
                SetWindowTheme(cbi.hwndList, L"DarkMode_Explorer", NULL);
            }
        }
    }
    else if (enable)
    {
        // Use DarkMode_Explorer for other controls (ListView, etc.)
        SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
    }
    else
    {
        // Reset to default theme
        SetWindowTheme(hwnd, L"Explorer", NULL);
    }

    // Force redraw
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ThemeHelper::ApplyDarkTitleBar(HWND hwnd, bool enable)
{
    if (!hwnd) return;

    // IMPORTANT: Must enable dark mode for window BEFORE DwmSetWindowAttribute
    AllowDarkModeForWindow(hwnd, enable);
    
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

} // namespace NetPulse

