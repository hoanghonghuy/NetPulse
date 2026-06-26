#include "NetPulse/Utils.h"
#include "NetPulse/ApplicationRuntime.h"
#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/ThemeHelper.h"
#include "../../resources/resource.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <shellapi.h>

namespace NetPulse
{

static bool g_debugLoggingEnabled = false;

MessageBoxHook g_messageBoxHook = nullptr;
GetSaveFileNameHook g_getSaveFileNameHook = nullptr;

// ============================================================================
// STRING UTILITIES IMPLEMENTATION
// ============================================================================

std::wstring FormatSpeed(double bytesPerSecond, SpeedUnit unit)
{
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;

    double convertedValue = 0.0;
    std::wstring unitStr;

    switch (unit)
    {
    case SpeedUnit::BytesPerSecond:
    {
        convertedValue = bytesPerSecond;
        unitStr = L"B/s";
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"KB/s";
        }
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"MB/s";
        }
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"GB/s";
        }
        break;
    }

    case SpeedUnit::KiloBytesPerSecond:
    {
        convertedValue = bytesPerSecond / KB;
        unitStr = L"KB/s";
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"MB/s";
        }
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"GB/s";
        }
        break;
    }

    case SpeedUnit::MegaBytesPerSecond:
    {
        convertedValue = bytesPerSecond / MB;
        unitStr = L"MB/s";
        if (convertedValue >= KB)
        {
            convertedValue /= KB;
            unitStr = L"GB/s";
        }
        break;
    }

    case SpeedUnit::MegaBitsPerSecond:
    default:
        convertedValue = (bytesPerSecond * 8.0) / 1000000.0; // decimal Mbps
        unitStr = L"Mbps";
        break;
    }

    std::wostringstream oss;
    oss << std::fixed << std::setprecision(2) << convertedValue << L" " << unitStr;
    return oss.str();
}

std::wstring FormatBytes(ULONG64 bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    std::wostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (bytes >= TB)
    {
        oss << (bytes / TB) << L" TB";
    }
    else if (bytes >= GB)
    {
        oss << (bytes / GB) << L" GB";
    }
    else if (bytes >= MB)
    {
        oss << (bytes / MB) << L" MB";
    }
    else if (bytes >= KB)
    {
        oss << (bytes / KB) << L" KB";
    }
    else
    {
        oss << bytes << L" B";
    }

    return oss.str();
}

std::wstring LoadStringResource(UINT resourceId)
{
    wchar_t buffer[256] = {};

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    int length = 0;
    if (hInstance)
    {
        length = LoadStringW(hInstance, resourceId, buffer, static_cast<int>(sizeof(buffer) / sizeof(wchar_t)));
    }

    if (length <= 0)
    {
        return std::wstring();
    }

    return std::wstring(buffer, length);
}



// ============================================================================
// TIME UTILITIES IMPLEMENTATION
// ============================================================================

double GetElapsedSeconds(DWORD start, DWORD end)
{
    // Handle GetTickCount wraparound (occurs every ~49.7 days)
    DWORD elapsed;
    if (end >= start)
    {
        elapsed = end - start;
    }
    else
    {
        // Wraparound occurred
        elapsed = (MAXDWORD - start) + end + 1;
    }

    return elapsed / 1000.0; // Convert milliseconds to seconds
}

// ============================================================================
// ERROR HANDLING UTILITIES IMPLEMENTATION
// ============================================================================

std::wstring GetLastErrorString()
{
    DWORD errorCode = GetLastError();
    if (errorCode == 0)
    {
        return L"No error";
    }

    LPWSTR buffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );

    std::wstring message;
    if (size > 0 && buffer != nullptr)
    {
        message = buffer;
        LocalFree(buffer);
    }
    else
    {
        message = L"Unknown error";
    }

    return message;
}

namespace
{
    struct DarkMessageBoxData
    {
        const std::wstring* message;
        const std::wstring* title;
        UINT flags;
        bool darkTheme;
    };

    INT_PTR CALLBACK DarkMessageDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        DarkMessageBoxData* data = nullptr;

        if (message == WM_INITDIALOG)
        {
            data = reinterpret_cast<DarkMessageBoxData*>(lParam);
            SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(data));

            if (data)
            {
                if (data->title)
                {
                    SetWindowTextW(hDlg, data->title->c_str());
                }

                if (data->message)
                {
                    SetDlgItemTextW(hDlg, IDC_MESSAGE_TEXT, data->message->c_str());
                }

                if (data->darkTheme)
                {
                    // Use actual current theme mode for title bar
                    ThemeMode currentTheme = ThemeHelper::GetCurrentTheme();
                    bool useDarkTitleBar = (currentTheme != ThemeMode::SystemDefault && 
                                            currentTheme != ThemeMode::Light &&
                                            currentTheme != ThemeMode::SolarizedLight &&
                                            currentTheme != ThemeMode::MorningMist &&
                                            currentTheme != ThemeMode::SoftPaper &&
                                            currentTheme != ThemeMode::MintFresh &&
                                            currentTheme != ThemeMode::Lavender &&
                                            currentTheme != ThemeMode::RosePink);
                    ThemeHelper::ApplyDarkTitleBar(hDlg, useDarkTitleBar);

                    DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDOK));
                    DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDCANCEL));
                    DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDYES));
                    DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDNO));
                }

                UINT type = (data->flags & MB_TYPEMASK);
                if (type == MB_OK)
                {
                    ShowWindow(GetDlgItem(hDlg, IDYES), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDNO), SW_HIDE);
                }
                else if (type == MB_YESNO)
                {
                    ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
                }
            }

            return TRUE;
        }

        data = reinterpret_cast<DarkMessageBoxData*>(GetWindowLongPtrW(hDlg, DWLP_USER));

        switch (message)
        {
        case WM_DRAWITEM:
        {
             if (data && data->darkTheme)
             {
                 DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                 if (pDrawItem->CtlType == ODT_BUTTON)
                 {
                     DialogThemeHelper::DrawButton(pDrawItem, true);
                     return TRUE;
                 }
             }
        }
        break;

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            if (data && data->darkTheme)
            {
                HBRUSH hBrush = DialogThemeHelper::HandleControlColor(reinterpret_cast<HDC>(wParam), true);
                if (hBrush) return reinterpret_cast<INT_PTR>(hBrush);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK:
            case IDYES:
            case IDNO:
            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            default:
                break;
            }
            break;
        default:
            break;
        }

        return FALSE;
    }
}

int ShowDarkMessageBox(HWND owner,
                       const std::wstring& message,
                       const std::wstring& title,
                       UINT flags,
                       bool darkTheme)
{
    if (g_messageBoxHook)
    {
        return g_messageBoxHook(owner, message.c_str(), title.c_str(), flags, darkTheme);
    }

    if (!darkTheme)
    {
        return static_cast<int>(MessageBoxW(owner, message.c_str(), title.c_str(), flags));
    }

    DarkMessageBoxData data;
    data.message = &message;
    data.title = &title;
    data.flags = flags;
    data.darkTheme = true;

    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_MESSAGE_DIALOG),
        owner,
        DarkMessageDialogProc,
        reinterpret_cast<LPARAM>(&data));

    if (result == -1)
    {
        return static_cast<int>(MessageBoxW(owner, message.c_str(), title.c_str(), flags));
    }

    return static_cast<int>(result);
}

void ShowErrorMessage(const std::wstring& message, const std::wstring& title)
{
    LogError(title + L": " + message);
    bool dark = ThemeHelper::IsSystemInDarkMode();
    ShowDarkMessageBox(nullptr, message, title, MB_OK | MB_ICONERROR, dark);
}

BOOL ShowSaveFileDialog(LPOPENFILENAMEW lpofn)
{
    if (g_getSaveFileNameHook)
    {
        return g_getSaveFileNameHook(lpofn);
    }
    return GetSaveFileNameW(lpofn);
}

namespace
{
    std::wstring GetLogFilePath()
    {
        wchar_t buffer[MAX_PATH] = {0};
        DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, static_cast<DWORD>(MAX_PATH));

        std::wstring basePath;
        if (length == 0 || length >= MAX_PATH)
        {
            basePath = L".";
        }
        else
        {
            basePath.assign(buffer, length);
        }

        std::wstring dirPath = basePath + L"\\NetPulse";
        CreateDirectoryW(dirPath.c_str(), nullptr);

        std::wstring filePath = dirPath + L"\\NetPulse.log";
        return filePath;
    }

    void AppendLogLine(const wchar_t* level, const std::wstring& message)
    {
        if (!level)
        {
            return;
        }

        SYSTEMTIME st = {};
        GetLocalTime(&st);

        wchar_t timeBuffer[32] = {0};
        swprintf_s(timeBuffer, L"%04u-%02u-%02u %02u:%02u:%02u",
                   st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        std::wstring filePath = GetLogFilePath();

        try
        {
            std::wofstream file(std::filesystem::path(filePath), std::ios::app);
            if (!file.is_open())
            {
                return;
            }

            file << timeBuffer << L" [" << level << L"] " << message << L"\n";
        }
        catch (...)
        {
        }
    }
}

void LogDebug(const std::wstring& message)
{
    if (!g_debugLoggingEnabled)
    {
        return;
    }

    AppendLogLine(L"DEBUG", message);
}

void LogError(const std::wstring& message)
{
    AppendLogLine(L"ERROR", message);
}

void SetDebugLoggingEnabled(bool enabled)
{
    g_debugLoggingEnabled = enabled;
}

void OpenLogFileInExplorer()
{
    std::wstring logPath = GetLogFilePath();

    if (logPath.empty())
    {
        return;
    }

    if (GetFileAttributesW(logPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        ShellExecuteW(nullptr, L"open", logPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    size_t pos = logPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
    {
        std::wstring folder = logPath.substr(0, pos);
        ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

// ============================================================================
// UI UTILITIES IMPLEMENTATION
// ============================================================================

void CenterWindowOnScreen(HWND hWnd)
{
    if (!hWnd)
    {
        return;
    }

    RECT rc = {};
    if (!GetWindowRect(hWnd, &rc))
    {
        return;
    }

    int dlgWidth = rc.right - rc.left;
    int dlgHeight = rc.bottom - rc.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int posX = (screenWidth - dlgWidth) / 2;
    int posY = (screenHeight - dlgHeight) / 2;

    SetWindowPos(hWnd, nullptr, posX, posY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

bool IsDarkThemeEnabled(const AppConfig& config)
{
    switch (config.themeMode)
    {
    case ThemeMode::Light:
    case ThemeMode::SolarizedLight:
    case ThemeMode::MorningMist:
    case ThemeMode::SoftPaper:
    case ThemeMode::MintFresh:
    case ThemeMode::Lavender:
    case ThemeMode::RosePink:
        return false;

    case ThemeMode::Dark:
    case ThemeMode::Dracula:
    case ThemeMode::Cyberpunk:
    case ThemeMode::Nord:
    case ThemeMode::Forest:
    case ThemeMode::OLED:
        return true;

    case ThemeMode::SystemDefault:
    default:
        return ThemeHelper::IsSystemInDarkMode();
    }
}

bool IsCustomThemeEnabled(const AppConfig& config)
{
    // Basic Light uses Windows default styling (no custom painting)
    if (config.themeMode == ThemeMode::Light)
    {
        return false;
    }
    
    // SystemDefault: use custom dark styling when Windows is in dark mode
    // Otherwise use Windows default light styling
    if (config.themeMode == ThemeMode::SystemDefault)
    {
        return ThemeHelper::IsSystemInDarkMode();
    }
    
    // All other themes (dark presets AND light presets) use custom painting
    return true;
}

bool IsForegroundWindowFullscreen()
{
    if (ApplicationRuntime::IsTestMode())
    {
        return false;
    }

    HWND hForeground = GetForegroundWindow();
    if (!hForeground)
    {
        return false;
    }

    // Exclude desktop and shell windows - they cover the entire screen
    // but are not actual fullscreen applications
    wchar_t className[64] = {};
    GetClassNameW(hForeground, className, _countof(className));
    if (_wcsicmp(className, L"Progman") == 0 ||        // Desktop (Program Manager)
        _wcsicmp(className, L"WorkerW") == 0 ||        // Desktop worker window
        _wcsicmp(className, L"Shell_TrayWnd") == 0 ||  // Taskbar
        _wcsicmp(className, L"Shell_SecondaryTrayWnd") == 0) // Secondary taskbar
    {
        return false;
    }

    // Get foreground window rect
    RECT windowRect;
    if (!GetWindowRect(hForeground, &windowRect))
    {
        return false;
    }

    // Get monitor info for the foreground window
    HMONITOR hMonitor = MonitorFromWindow(hForeground, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor)
    {
        return false;
    }

    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(hMonitor, &monitorInfo))
    {
        return false;
    }

    // Check if window covers the entire monitor
    return (windowRect.left <= monitorInfo.rcMonitor.left &&
            windowRect.top <= monitorInfo.rcMonitor.top &&
            windowRect.right >= monitorInfo.rcMonitor.right &&
            windowRect.bottom >= monitorInfo.rcMonitor.bottom);
}

} // namespace NetPulse
