#include "NetworkMonitor/ConfigManager.h"
#include "NetworkMonitor/Utils.h"
#include "NetworkMonitor/ThemeHelper.h"
#include <shellapi.h>

namespace NetworkMonitor
{

ConfigManager::ConfigManager()
{
}

ConfigManager::~ConfigManager()
{
}

bool ConfigManager::LoadConfig(AppConfig& config)
{
    HKEY hKey = nullptr;
    if (!OpenSettingsKey(hKey))
    {
        // Use default config if cannot open registry
        config = AppConfig();
        // Override default dark theme based on system preference
        bool systemDark = ThemeHelper::IsSystemInDarkMode();
        config.darkTheme = systemDark;
        config.themeMode = ThemeMode::SystemDefault;
        return true;
    }

    // Load settings from registry
    config.updateInterval = ReadDWORD(hKey, L"UpdateInterval", DEFAULT_UPDATE_INTERVAL);
    config.displayUnit = static_cast<SpeedUnit>(ReadDWORD(hKey, L"DisplayUnit", static_cast<DWORD>(SpeedUnit::KiloBytesPerSecond)));
    config.enableLogging = ReadDWORD(hKey, L"EnableLogging", 1) != 0;
    config.debugLogging = ReadDWORD(hKey, L"DebugLogging", 0) != 0;
    
    // Default to system theme if not found in registry
    bool defaultDark = ThemeHelper::IsSystemInDarkMode();
    config.darkTheme = ReadDWORD(hKey, L"DarkTheme", defaultDark ? 1 : 0) != 0;

    // Load theme mode if present; otherwise infer from legacy DarkTheme
    DWORD rawThemeMode = ReadDWORD(hKey, L"ThemeMode", static_cast<DWORD>(ThemeMode::SystemDefault));
    if (rawThemeMode > static_cast<DWORD>(ThemeMode::Dark))
    {
        // Registry does not contain a valid ThemeMode value yet.
        // Infer mode from legacy DarkTheme and current system theme so
        // that existing configurations migrate smoothly.
        bool systemDark = ThemeHelper::IsSystemInDarkMode();
        if (config.darkTheme == systemDark)
        {
            config.themeMode = ThemeMode::SystemDefault;
        }
        else
        {
            config.themeMode = config.darkTheme ? ThemeMode::Dark : ThemeMode::Light;
        }
    }
    else
    {
        config.themeMode = static_cast<ThemeMode>(rawThemeMode);
    }

    // Keep legacy darkTheme flag synchronized with the effective theme so
    // existing code paths that still read darkTheme behave consistently with
    // the selected ThemeMode.
    config.darkTheme = IsDarkThemeEnabled(config);
    config.historyAutoTrimDays = static_cast<int>(ReadDWORD(hKey, L"HistoryAutoTrimDays", DEFAULT_HISTORY_AUTO_TRIM_DAYS));
    if (config.historyAutoTrimDays > MAX_HISTORY_AUTO_TRIM_DAYS)
    {
        config.historyAutoTrimDays = MAX_HISTORY_AUTO_TRIM_DAYS;
    }
    DWORD langValue = ReadDWORD(hKey, L"Language", static_cast<DWORD>(AppLanguage::SystemDefault));
    if (langValue > static_cast<DWORD>(AppLanguage::ChineseSimplified))
    {
        langValue = static_cast<DWORD>(AppLanguage::SystemDefault);
    }
    config.language = static_cast<AppLanguage>(langValue);
    config.selectedInterface = ReadString(hKey, L"SelectedInterface", L"");
    config.autoStart = IsAutoStartEnabled();
    config.autoStartAsAdmin = ReadDWORD(hKey, L"AutoStartAsAdmin", 0) != 0;
    config.enableConnectionNotification = ReadDWORD(hKey, L"EnableConnectionNotify", 1) != 0;
    config.pingTarget = ReadString(hKey, L"PingTarget", L"8.8.8.8");
    config.pingIntervalMs = ReadDWORD(hKey, L"PingIntervalMs", 5000);
    config.hotkeyModifier = ReadDWORD(hKey, L"HotkeyModifier", MOD_WIN | MOD_SHIFT);
    config.hotkeyKey = ReadDWORD(hKey, L"HotkeyKey", 'N');
    config.overlayFontSize = static_cast<int>(ReadDWORD(hKey, L"OverlayFontSize", 13));
    config.overlayDownloadColor = ReadDWORD(hKey, L"OverlayDownloadColor", RGB(0, 255, 255));
    config.overlayUploadColor = ReadDWORD(hKey, L"OverlayUploadColor", RGB(0, 255, 0));
    
    // Data Usage Alerts
    config.enableDataUsageAlerts = ReadDWORD(hKey, L"EnableDataUsageAlerts", 0) != 0;
    DWORD quotaMB = ReadDWORD(hKey, L"DataQuotaMB", 0);  // Store in MB for registry precision
    config.dataQuotaGB = static_cast<double>(quotaMB) / 1024.0;
    config.dataAlertThreshold1 = static_cast<int>(ReadDWORD(hKey, L"DataAlertThreshold1", 80));
    config.dataAlertThreshold2 = static_cast<int>(ReadDWORD(hKey, L"DataAlertThreshold2", 100));

    // Floating Window
    config.showFloatingWindow = ReadDWORD(hKey, L"ShowFloatingWindow", 0) != 0;
    config.floatingWindowX = static_cast<int>(ReadDWORD(hKey, L"FloatingWindowX", static_cast<DWORD>(-1)));
    config.floatingWindowY = static_cast<int>(ReadDWORD(hKey, L"FloatingWindowY", static_cast<DWORD>(-1)));
    config.floatingWindowOpacity = static_cast<BYTE>(ReadDWORD(hKey, L"FloatingWindowOpacity", 200));
    config.floatingShowNetwork = ReadDWORD(hKey, L"FloatingShowNetwork", 1) != 0;
    config.floatingShowCPU = ReadDWORD(hKey, L"FloatingShowCPU", 1) != 0;
    config.floatingShowRAM = ReadDWORD(hKey, L"FloatingShowRAM", 1) != 0;
    config.floatingShowPing = ReadDWORD(hKey, L"FloatingShowPing", 1) != 0;
    config.floatingShowDataToday = ReadDWORD(hKey, L"FloatingShowDataToday", 1) != 0;

    RegCloseKey(hKey);
    return true;
}

bool ConfigManager::SaveConfig(const AppConfig& config)
{
    HKEY hKey = nullptr;
    if (!OpenSettingsKey(hKey))
    {
        return false;
    }

    // Save settings to registry
    bool success = true;
    success &= WriteDWORD(hKey, L"UpdateInterval", config.updateInterval);
    success &= WriteDWORD(hKey, L"DisplayUnit", static_cast<DWORD>(config.displayUnit));
    success &= WriteDWORD(hKey, L"EnableLogging", config.enableLogging ? 1 : 0);
    success &= WriteDWORD(hKey, L"DebugLogging", config.debugLogging ? 1 : 0);
    success &= WriteDWORD(hKey, L"DarkTheme", config.darkTheme ? 1 : 0);

    // If ThemeMode was never explicitly set (still SystemDefault), derive
    // a stable value from DarkTheme vs current system theme so that a
    // simple toggle of DarkTheme round-trips correctly through the
    // registry and tests observing darkTheme continue to pass.
    ThemeMode modeToSave = config.themeMode;
    if (modeToSave == ThemeMode::SystemDefault)
    {
        bool systemDark = ThemeHelper::IsSystemInDarkMode();
        if (config.darkTheme == systemDark)
        {
            modeToSave = ThemeMode::SystemDefault;
        }
        else
        {
            modeToSave = config.darkTheme ? ThemeMode::Dark : ThemeMode::Light;
        }
    }

    success &= WriteDWORD(hKey, L"ThemeMode", static_cast<DWORD>(modeToSave));
    int trimDays = config.historyAutoTrimDays;
    if (trimDays < 0)
    {
        trimDays = 0;
    }
    else if (trimDays > MAX_HISTORY_AUTO_TRIM_DAYS)
    {
        trimDays = MAX_HISTORY_AUTO_TRIM_DAYS;
    }
    success &= WriteDWORD(hKey, L"HistoryAutoTrimDays", static_cast<DWORD>(trimDays));
    success &= WriteDWORD(hKey, L"Language", static_cast<DWORD>(config.language));
    success &= WriteString(hKey, L"SelectedInterface", config.selectedInterface);
    success &= WriteDWORD(hKey, L"EnableConnectionNotify", config.enableConnectionNotification ? 1 : 0);
    success &= WriteString(hKey, L"PingTarget", config.pingTarget);
    success &= WriteDWORD(hKey, L"PingIntervalMs", config.pingIntervalMs);
    success &= WriteDWORD(hKey, L"HotkeyModifier", config.hotkeyModifier);
    success &= WriteDWORD(hKey, L"HotkeyKey", config.hotkeyKey);
    success &= WriteDWORD(hKey, L"OverlayFontSize", static_cast<DWORD>(config.overlayFontSize));
    success &= WriteDWORD(hKey, L"OverlayDownloadColor", config.overlayDownloadColor);
    success &= WriteDWORD(hKey, L"OverlayUploadColor", config.overlayUploadColor);
    
    // Data Usage Alerts
    success &= WriteDWORD(hKey, L"EnableDataUsageAlerts", config.enableDataUsageAlerts ? 1 : 0);
    DWORD quotaMB = static_cast<DWORD>(config.dataQuotaGB * 1024.0);  // Store in MB
    success &= WriteDWORD(hKey, L"DataQuotaMB", quotaMB);
    success &= WriteDWORD(hKey, L"DataAlertThreshold1", static_cast<DWORD>(config.dataAlertThreshold1));
    success &= WriteDWORD(hKey, L"DataAlertThreshold2", static_cast<DWORD>(config.dataAlertThreshold2));

    // Floating Window
    success &= WriteDWORD(hKey, L"ShowFloatingWindow", config.showFloatingWindow ? 1 : 0);
    success &= WriteDWORD(hKey, L"FloatingWindowX", static_cast<DWORD>(config.floatingWindowX));
    success &= WriteDWORD(hKey, L"FloatingWindowY", static_cast<DWORD>(config.floatingWindowY));
    success &= WriteDWORD(hKey, L"FloatingWindowOpacity", static_cast<DWORD>(config.floatingWindowOpacity));
    success &= WriteDWORD(hKey, L"FloatingShowNetwork", config.floatingShowNetwork ? 1 : 0);
    success &= WriteDWORD(hKey, L"FloatingShowCPU", config.floatingShowCPU ? 1 : 0);
    success &= WriteDWORD(hKey, L"FloatingShowRAM", config.floatingShowRAM ? 1 : 0);
    success &= WriteDWORD(hKey, L"FloatingShowPing", config.floatingShowPing ? 1 : 0);
    success &= WriteDWORD(hKey, L"FloatingShowDataToday", config.floatingShowDataToday ? 1 : 0);

    // Save auto-start setting (registry only)
    success &= WriteDWORD(hKey, L"AutoStartAsAdmin", config.autoStartAsAdmin ? 1 : 0);
    
    // Note: SetAutoStart is NOT called here anymore to avoid unnecessary UAC prompts.
    // Callers (like SettingsDialog) must call SetAutoStart explicitly if those settings changed.

    RegCloseKey(hKey);
    return success;
}

bool ConfigManager::SetAutoStart(bool enable, bool asAdmin)
{
    LogDebug(L"SetAutoStart called: enable=" + std::to_wstring(enable) + L", asAdmin=" + std::to_wstring(asAdmin));
    
    static const wchar_t* TASK_NAME = L"NetworkMonitorAutoStart";
    bool regSuccess = true;
    bool taskSuccess = true;

    // === REGISTRY: Standard auto-start (non-admin) ===
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTOSTART_PATH, 0, KEY_WRITE, &hKey);
    if (result == ERROR_SUCCESS)
    {
        if (enable && !asAdmin)
        {
            // Enable via Registry
            wchar_t exePath[MAX_PATH] = {0};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            result = RegSetValueExW(hKey, APP_NAME, 0, REG_SZ,
                                    reinterpret_cast<const BYTE*>(exePath),
                                    static_cast<DWORD>((wcslen(exePath) + 1) * sizeof(wchar_t)));
            regSuccess = (result == ERROR_SUCCESS);
        }
        else
        {
            // Remove from Registry (either disabled or using Admin mode)
            result = RegDeleteValueW(hKey, APP_NAME);
            regSuccess = (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
        }
        RegCloseKey(hKey);
    }
    else
    {
        regSuccess = false;
    }

    // === TASK SCHEDULER: Admin auto-start ===
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t cmdParams[1024] = {0};

    if (enable && asAdmin)
    {
        // Create scheduled task with highest privileges
        swprintf_s(cmdParams, L"/Create /TN \"%ls\" /TR \"\\\"%ls\\\"\" /SC ONLOGON /RL HIGHEST /F",
                   TASK_NAME, exePath);

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // Request UAC elevation
        sei.lpFile = L"schtasks.exe";
        sei.lpParameters = cmdParams;
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            WaitForSingleObject(sei.hProcess, 10000);  // Wait up to 10s
            DWORD exitCode = 0;
            GetExitCodeProcess(sei.hProcess, &exitCode);
            CloseHandle(sei.hProcess);
            taskSuccess = (exitCode == 0);
        }
        else
        {
            taskSuccess = false;
        }
    }
    else
    {
        // Delete scheduled task (if exists) - need admin to delete admin task
        swprintf_s(cmdParams, L"/Delete /TN \"%ls\" /F", TASK_NAME);

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // Need admin to delete elevated task
        sei.lpFile = L"schtasks.exe";
        sei.lpParameters = cmdParams;
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            WaitForSingleObject(sei.hProcess, 5000);
            DWORD exitCode = 0;
            GetExitCodeProcess(sei.hProcess, &exitCode);
            CloseHandle(sei.hProcess);
            
            wchar_t dbgDelete[128];
            swprintf_s(dbgDelete, L"[DEBUG] Task delete exitCode=%lu\n", exitCode);
            LogDebug(L"Task delete exit code: " + std::to_wstring(exitCode));
            
            // Exit code 1 means task not found - that's OK
            taskSuccess = (exitCode == 0 || exitCode == 1);
        }
        else
        {
            // ShellExecuteEx failed - try without elevation (task might not exist)
            DWORD err = GetLastError();
            LogError(L"Task delete ShellExecuteEx failed, error=" + std::to_wstring(err));
            
            // Error 1223 means user cancelled UAC - not a success
            taskSuccess = (err != ERROR_CANCELLED);
        }
    }

    if (regSuccess && taskSuccess)
    {
        LogDebug(L"SetAutoStart completed successfully.");
    }
    else
    {
        LogError(L"SetAutoStart failed: Reg=" + std::to_wstring(regSuccess) + L", Task=" + std::to_wstring(taskSuccess));
    }

    return regSuccess && taskSuccess;
}

bool ConfigManager::IsAutoStartEnabled()
{
    // First check Registry (standard auto-start)
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, AUTOSTART_PATH, 0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS)
    {
        wchar_t value[MAX_PATH] = {0};
        DWORD valueSize = sizeof(value);
        DWORD type = REG_SZ;

        result = RegQueryValueExW(hKey, APP_NAME, nullptr, &type, 
                                  reinterpret_cast<BYTE*>(value), &valueSize);
        RegCloseKey(hKey);
        
        if (result == ERROR_SUCCESS)
        {
            LogDebug(L"IsAutoStartEnabled: TRUE (found in registry)");
            return true;  // Found in registry
        }
    }

    // Check Task Scheduler for admin auto-start
    static const wchar_t* TASK_NAME = L"NetworkMonitorAutoStart";
    wchar_t cmdParams[256] = {0};
    swprintf_s(cmdParams, L"/Query /TN \"%ls\"", TASK_NAME);

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = nullptr;
    sei.lpFile = L"schtasks.exe";
    sei.lpParameters = cmdParams;
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExW(&sei))
    {
        WaitForSingleObject(sei.hProcess, 3000);
        DWORD exitCode = 0;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        
        if (exitCode == 0)
        {
            LogDebug(L"IsAutoStartEnabled: TRUE (found in task scheduler)");
            return true;  // Found in Task Scheduler
        }
    }

    OutputDebugStringW(L"[DEBUG] IsAutoStartEnabled: FALSE (not found)\n");
    return false;
}

bool ConfigManager::OpenSettingsKey(HKEY& hKey)
{
    DWORD disposition = 0;
    LONG result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        REGISTRY_PATH,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        nullptr,
        &hKey,
        &disposition
    );

    return (result == ERROR_SUCCESS);
}

DWORD ConfigManager::ReadDWORD(HKEY hKey, const wchar_t* valueName, DWORD defaultValue)
{
    DWORD value = defaultValue;
    DWORD valueSize = sizeof(DWORD);
    DWORD type = REG_DWORD;

    LONG result = RegQueryValueExW(hKey, valueName, nullptr, &type, 
                                    reinterpret_cast<BYTE*>(&value), &valueSize);

    if (result != ERROR_SUCCESS || type != REG_DWORD)
    {
        return defaultValue;
    }

    return value;
}

bool ConfigManager::WriteDWORD(HKEY hKey, const wchar_t* valueName, DWORD value)
{
    LONG result = RegSetValueExW(hKey, valueName, 0, REG_DWORD, 
                                  reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));

    return (result == ERROR_SUCCESS);
}

std::wstring ConfigManager::ReadString(HKEY hKey, const wchar_t* valueName, const std::wstring& defaultValue)
{
    wchar_t buffer[256] = {0};
    DWORD bufferSize = sizeof(buffer);
    DWORD type = REG_SZ;

    LONG result = RegQueryValueExW(hKey, valueName, nullptr, &type, 
                                    reinterpret_cast<BYTE*>(buffer), &bufferSize);

    if (result != ERROR_SUCCESS || type != REG_SZ)
    {
        return defaultValue;
    }

    return std::wstring(buffer);
}

bool ConfigManager::WriteString(HKEY hKey, const wchar_t* valueName, const std::wstring& value)
{
    LONG result = RegSetValueExW(hKey, valueName, 0, REG_SZ, 
                                  reinterpret_cast<const BYTE*>(value.c_str()), 
                                  static_cast<DWORD>((value.length() + 1) * sizeof(wchar_t)));

    return (result == ERROR_SUCCESS);
}

} // namespace NetworkMonitor
