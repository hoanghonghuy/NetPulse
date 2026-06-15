#ifndef NETWORK_MONITOR_UTILS_H
#define NETWORK_MONITOR_UTILS_H

#include "NetPulse/Common.h"
#include <string>
#include <commdlg.h>

namespace NetPulse
{

// ============================================================================
// STRING UTILITIES
// ============================================================================

/**
 * Convert bytes to human-readable string with appropriate unit
 * @param bytes Number of bytes
 * @param unit Target unit for conversion
 * @return Formatted string (e.g., "1.23 MB/s")
 */
std::wstring FormatSpeed(double bytesPerSecond, SpeedUnit unit);

/**
 * Convert bytes to human-readable size string
 * @param bytes Number of bytes
 * @return Formatted string (e.g., "1.23 GB")
 */
std::wstring FormatBytes(ULONG64 bytes);

std::wstring LoadStringResource(UINT resourceId);



// ============================================================================
// TIME UTILITIES
// ============================================================================

/**
 * Get elapsed time in seconds between two GetTickCount values
 * @param start Start time from GetTickCount
 * @param end End time from GetTickCount
 * @return Elapsed time in seconds
 */
double GetElapsedSeconds(DWORD start, DWORD end);

// ============================================================================
// ERROR HANDLING UTILITIES
// ============================================================================

/**
 * Get last Windows error message as string
 * @return Error message string
 */
std::wstring GetLastErrorString();
void LogDebug(const std::wstring& message);
void LogError(const std::wstring& message);
void SetDebugLoggingEnabled(bool enabled);
void ShowErrorMessage(const std::wstring& message, const std::wstring& title = L"Error");

typedef int (*MessageBoxHook)(HWND owner, const wchar_t* message, const wchar_t* title, UINT flags, bool darkTheme);
extern MessageBoxHook g_messageBoxHook;

int ShowDarkMessageBox(HWND owner,
                       const std::wstring& message,
                       const std::wstring& title,
                       UINT flags,
                       bool darkTheme);

typedef BOOL (WINAPI *GetSaveFileNameHook)(LPOPENFILENAMEW lpofn);
extern GetSaveFileNameHook g_getSaveFileNameHook;

BOOL ShowSaveFileDialog(LPOPENFILENAMEW lpofn);

// ============================================================================
// UI UTILITIES
// ============================================================================

/**
 * Check if foreground window is fullscreen (game, video, etc.)
 */
bool IsForegroundWindowFullscreen();

/**
 * Center a window/dialog on the screen
 * @param hWnd Handle to the window/dialog
 */
void CenterWindowOnScreen(HWND hWnd);

// Open the application log file (or its folder) in the default handler
void OpenLogFileInExplorer();

bool IsDarkThemeEnabled(const AppConfig& config);
bool IsCustomThemeEnabled(const AppConfig& config);

} // namespace NetPulse

#endif // NETWORK_MONITOR_UTILS_H
