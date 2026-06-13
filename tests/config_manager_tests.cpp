#include "NetPulse/Common.h"
#include "NetPulse/ConfigManager.h"
#include "NetPulse/Utils.h"
#include "TestUtils.h"
#include "test_fakes/FakeAutoStartManager.h"

#include <cstdlib>
#include <string>

using namespace NetPulse;

namespace NetPulseTests
{

namespace
{

static AppConfig MakeDistinctConfig(const AppConfig& baseline)
{
    AppConfig cfg = baseline;

    cfg.updateInterval = UPDATE_INTERVAL_SLOW;
    cfg.displayUnit = SpeedUnit::MegaBitsPerSecond;
    cfg.enableLogging = !baseline.enableLogging;
    cfg.debugLogging = !baseline.debugLogging;
    cfg.themeMode = ThemeMode::Dracula;
    cfg.historyAutoTrimDays = 14;
    cfg.language = AppLanguage::Vietnamese;
    cfg.selectedInterface = L"DistinctAdapter";
    cfg.enableConnectionNotification = !baseline.enableConnectionNotification;
    cfg.pingTarget = L"1.1.1.1";
    cfg.pingIntervalMs = 3000;
    cfg.hotkeyModifier = MOD_CONTROL | MOD_ALT;
    cfg.hotkeyKey = 'P';
    cfg.overlayFontSize = 16;
    cfg.overlayDownloadColor = RGB(255, 128, 0);
    cfg.overlayUploadColor = RGB(128, 0, 255);
    cfg.enableDataUsageAlerts = true;
    cfg.dataQuotaGB = 5.0;
    cfg.dataAlertThreshold1 = 70;
    cfg.dataAlertThreshold2 = 90;
    cfg.showFloatingWindow = true;
    cfg.floatingWindowX = 120;
    cfg.floatingWindowY = 240;
    cfg.floatingWindowOpacity = 180;
    cfg.floatingShowNetwork = false;
    cfg.floatingShowCPU = false;
    cfg.floatingShowRAM = true;
    cfg.floatingShowPing = false;
    cfg.floatingShowDataToday = false;
    cfg.floatingShowSparkline = false;
    cfg.trayAnimationEnabled = false;
    cfg.trayAnimationThresholdKB = 2048;
    cfg.sparklineTimeRange = 2;
    cfg.floatingShowVpnStatus = false;
    cfg.floatingShowPublicIP = false;
    cfg.publicIPUpdateIntervalMs = 60000;
    cfg.autoStartAsAdmin = !baseline.autoStartAsAdmin;
    cfg.darkTheme = IsDarkThemeEnabled(cfg);

    return cfg;
}

static bool WriteTestRegistryDWORD(const wchar_t* valueName, DWORD value)
{
    const wchar_t* registryPath = _wgetenv(L"NETPULSE_TEST_REGISTRY_PATH");
    if (!registryPath || registryPath[0] == L'\0')
    {
        return false;
    }

    HKEY hKey = nullptr;
    DWORD disposition = 0;
    LONG result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        registryPath,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        nullptr,
        &hKey,
        &disposition);

    if (result != ERROR_SUCCESS)
    {
        return false;
    }

    result = RegSetValueExW(
        hKey,
        valueName,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(DWORD));

    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

static bool DeletePortableConfigFile(const std::wstring& path)
{
    if (path.empty())
    {
        return false;
    }
    return DeleteFileW(path.c_str()) != 0;
}

} // namespace

void RunConfigManagerTests()
{
    LogTestMessage(L"=== ConfigManager tests ===");

    const wchar_t* testRegistryPath = _wgetenv(L"NETPULSE_TEST_REGISTRY_PATH");
    AssertTrue(testRegistryPath != nullptr && testRegistryPath[0] != L'\0',
               L"ConfigManager uses NETPULSE_TEST_REGISTRY_PATH sandbox");

    ConfigManager mgr;

    AppConfig original;
    bool loadedOriginal = mgr.LoadConfig(original);
    AssertTrue(loadedOriginal, L"ConfigManager.LoadConfig(original) returns true");

    AppConfig modified = MakeDistinctConfig(original);

    bool saved = mgr.SaveConfig(modified);
    AssertTrue(saved, L"ConfigManager.SaveConfig(modified) returns true");

    AppConfig reloaded;
    bool loaded = mgr.LoadConfig(reloaded);
    AssertTrue(loaded, L"ConfigManager.LoadConfig(reloaded) returns true");

    reloaded.autoStart = modified.autoStart;
    reloaded.floatingSnapToEdge = modified.floatingSnapToEdge;
    reloaded.floatingSnapDistance = modified.floatingSnapDistance;
    reloaded.floatingClickThrough = modified.floatingClickThrough;
    reloaded.floatingMiniMode = modified.floatingMiniMode;
    AssertTrue(reloaded == modified, L"ConfigManager full AppConfig round-trip");

    AppConfig emptyStrings = modified;
    emptyStrings.selectedInterface.clear();
    emptyStrings.pingTarget.clear();
    AssertTrue(mgr.SaveConfig(emptyStrings), L"ConfigManager.SaveConfig empty strings returns true");

    AppConfig emptyReloaded;
    AssertTrue(mgr.LoadConfig(emptyReloaded), L"ConfigManager.LoadConfig empty strings returns true");
    AssertTrue(emptyReloaded.selectedInterface.empty(),
               L"ConfigManager round-trip empty selectedInterface");
    AssertTrue(emptyReloaded.pingTarget.empty(),
               L"ConfigManager round-trip empty pingTarget");

    std::wstring longIface(300, L'X');
    AppConfig longIfaceConfig = modified;
    longIfaceConfig.selectedInterface = longIface;
    AssertTrue(mgr.SaveConfig(longIfaceConfig), L"ConfigManager.SaveConfig long interface returns true");

    AppConfig longIfaceReloaded;
    AssertTrue(mgr.LoadConfig(longIfaceReloaded), L"ConfigManager.LoadConfig long interface returns true");
    AssertTrue(longIfaceReloaded.selectedInterface == longIface,
               L"ConfigManager round-trip interface longer than 256 wchar");

    AssertTrue(WriteTestRegistryDWORD(L"Language", 9999),
               L"ConfigManager test registry write invalid Language");
    AssertTrue(WriteTestRegistryDWORD(L"ThemeMode", 9999),
               L"ConfigManager test registry write invalid ThemeMode");

    AppConfig invalidEnumReloaded;
    AssertTrue(mgr.LoadConfig(invalidEnumReloaded),
               L"ConfigManager.LoadConfig invalid enum values returns true");
    AssertTrue(invalidEnumReloaded.language == AppLanguage::SystemDefault,
               L"ConfigManager invalid Language falls back to SystemDefault");

    AssertTrue(mgr.SaveConfig(modified), L"ConfigManager restore modified config after enum test");

    std::wstring portablePath = mgr.GetPortableFilePath();
    AssertTrue(!portablePath.empty() && portablePath.find(L"netpulse.ini") != std::wstring::npos,
               L"ConfigManager.GetPortableFilePath points to netpulse.ini");

    const bool hadPortableFile = mgr.HasPortableConfigFile();
    if (!hadPortableFile)
    {
        bool portableEnabled = mgr.EnablePortableMode(modified);
        if (portableEnabled)
        {
            AssertTrue(mgr.HasPortableConfigFile(),
                       L"ConfigManager.EnablePortableMode creates portable file");
            AssertTrue(mgr.IsPortableMode(),
                       L"ConfigManager portable mode active after enable");

            AppConfig portableModified = MakeDistinctConfig(modified);
            portableModified.selectedInterface = L"PortableIface";
            AssertTrue(mgr.SaveConfig(portableModified),
                       L"ConfigManager.SaveConfig in portable mode returns true");

            AppConfig portableReloaded;
            AssertTrue(mgr.LoadConfig(portableReloaded),
                       L"ConfigManager.LoadConfig in portable mode returns true");
            portableReloaded.autoStart = portableModified.autoStart;
            portableReloaded.floatingSnapToEdge = portableModified.floatingSnapToEdge;
            portableReloaded.floatingSnapDistance = portableModified.floatingSnapDistance;
            portableReloaded.floatingClickThrough = portableModified.floatingClickThrough;
            portableReloaded.floatingMiniMode = portableModified.floatingMiniMode;
            AssertTrue(portableReloaded == portableModified,
                       L"ConfigManager portable mode round-trip");

            mgr.SetPortableMode(false);
            DeletePortableConfigFile(portablePath);
        }
        else
        {
            LogTestMessage(L"[SKIP] ConfigManager.EnablePortableMode (cannot write next to test binary)");
        }
    }
    else
    {
        LogTestMessage(L"[SKIP] ConfigManager portable create test (netpulse.ini already exists)");
    }

    bool restored = mgr.SaveConfig(original);
    AssertTrue(restored, L"ConfigManager.SaveConfig(original) restore returns true");

    // === SetAutoStart / IsAutoStartEnabled unit tests with FakeAutoStartManager ===
    {
        LogTestMessage(L"  Running SetAutoStart unit tests with FakeAutoStartManager...");
        FakeAutoStartManager fakeAutoStart;
        ConfigManager fakeMgr(&fakeAutoStart);

        // Test 1: Initially disabled
        AssertTrue(!fakeMgr.IsAutoStartEnabled(), L"Initially auto-start should be disabled");

        // Test 2: Enable standard (non-admin)
        AssertTrue(fakeMgr.SetAutoStart(true, false), L"SetAutoStart(true, false) should succeed");
        AssertTrue(fakeAutoStart.m_lastRegistryEnable, L"ConfigureRegistry(true) should be called");
        AssertTrue(!fakeAutoStart.m_lastScheduledTaskEnable, L"ConfigureScheduledTask(false) should be called");
        AssertTrue(fakeMgr.IsAutoStartEnabled(), L"IsAutoStartEnabled should return true when registry is enabled");

        // Test 3: Enable admin (scheduled task)
        AssertTrue(fakeMgr.SetAutoStart(true, true), L"SetAutoStart(true, true) should succeed");
        AssertTrue(!fakeAutoStart.m_lastRegistryEnable, L"ConfigureRegistry(false) should be called to clean up registry autostart");
        AssertTrue(fakeAutoStart.m_lastScheduledTaskEnable, L"ConfigureScheduledTask(true) should be called");
        AssertTrue(fakeMgr.IsAutoStartEnabled(), L"IsAutoStartEnabled should return true when task is enabled");

        // Test 4: Disable
        AssertTrue(fakeMgr.SetAutoStart(false, false), L"SetAutoStart(false, false) should succeed");
        AssertTrue(!fakeAutoStart.m_lastRegistryEnable, L"ConfigureRegistry(false) should be called");
        AssertTrue(!fakeAutoStart.m_lastScheduledTaskEnable, L"ConfigureScheduledTask(false) should be called");
        AssertTrue(!fakeMgr.IsAutoStartEnabled(), L"IsAutoStartEnabled should return false when both disabled");

        // Test 5: Simulated failure on Registry config
        fakeAutoStart.m_configureRegistryResult = false;
        AssertTrue(!fakeMgr.SetAutoStart(true, false), L"SetAutoStart should fail if registry config fails");

        // Test 6: Simulated failure on Task config
        fakeAutoStart.m_configureRegistryResult = true;
        fakeAutoStart.m_configureScheduledTaskResult = false;
        AssertTrue(!fakeMgr.SetAutoStart(true, true), L"SetAutoStart should fail if scheduled task config fails");
    }
}

} // namespace NetPulseTests
