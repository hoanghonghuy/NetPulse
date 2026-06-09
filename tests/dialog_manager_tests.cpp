#include "NetPulse/DialogManager.h"
#include "NetPulse/UpdateCoordinator.h"
#include "test_fakes/DialogManagerTestFriend.h"
#include "test_fakes/FakeConfigProvider.h"
#include "test_fakes/FakeNetworkStatsProvider.h"
#include "TestUtils.h"

#include <windows.h>

using namespace NetPulse;

namespace NetPulseTests
{

namespace
{
const wchar_t* kDashboardTestClass = L"NetPulseDashboardTestWindow";
int g_wmUpdateStatsCount = 0;

LRESULT CALLBACK DashboardTestWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_UPDATE_STATS)
    {
        ++g_wmUpdateStatsCount;
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

HWND CreateDashboardTestWindow()
{
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DashboardTestWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kDashboardTestClass;
    RegisterClassExW(&wc);

    return CreateWindowExW(
        0,
        kDashboardTestClass,
        L"Dashboard Test Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        120,
        80,
        nullptr,
        nullptr,
        hInstance,
        nullptr);
}

} // namespace

void RunDialogManagerTests()
{
    LogTestMessage(L"=== DialogManager tests ===");

    DialogManager uninitializedManager;
    uninitializedManager.ShowSettings();
    uninitializedManager.ShowDashboard();
    uninitializedManager.ShowHistory();
    uninitializedManager.ShowPerApp();
    uninitializedManager.ShowSpeedTest();
    uninitializedManager.ShowAbout();
    uninitializedManager.ShowConnectionLog();
    AssertTrue(true, L"DialogManager Show methods without init do not crash");

    HWND parent = CreateDashboardTestWindow();
    if (!parent)
    {
        LogTestMessage(L"[WARN] Failed to create parent window; skipping DialogManager callback tests");
        return;
    }

    AppConfig config;
    FakeConfigProvider configProvider;
    FakeNetworkStatsProvider networkProvider;
    networkProvider.m_aggregated.isActive = true;

    UpdateCoordinator coordinator;
    coordinator.Initialize(&config, &networkProvider, nullptr, nullptr, nullptr);

    DialogManager manager;
    manager.Initialize(parent, &config, &configProvider, &networkProvider, &coordinator, nullptr);

    HWND dashboardWindow = CreateDashboardTestWindow();
    AssertTrue(dashboardWindow != nullptr, L"DialogManager dashboard test window created");
    DialogManagerTestFriend::SetDashboardHandle(manager, dashboardWindow);

    manager.ShowDashboard();
    AssertTrue(IsWindow(dashboardWindow),
               L"DialogManager ShowDashboard reuses tracked dashboard handle");

    int configReloadCount = 0;
    int timerUpdateCount = 0;
    UINT lastTimerInterval = 0;
    int languageApplyCount = 0;
    int applyAllSettingsCount = 0;

    manager.SetConfigReloadCallback([&]() -> bool
    {
        ++configReloadCount;
        return true;
    });
    manager.SetTimerUpdateCallback([&](UINT intervalMs)
    {
        ++timerUpdateCount;
        lastTimerInterval = intervalMs;
    });
    manager.SetLanguageApplyCallback([&]()
    {
        ++languageApplyCount;
    });
    manager.SetApplyAllSettingsCallback([&]()
    {
        ++applyAllSettingsCount;
    });

    networkProvider.m_updateCallCount = 0;

    AppConfig oldConfig = config;
    config.updateInterval = UPDATE_INTERVAL_FAST;
    config.language = AppLanguage::Vietnamese;
    g_wmUpdateStatsCount = 0;

    DialogManagerTestFriend::ApplySettings(manager, oldConfig);

    AssertTrue(configReloadCount == 1, L"DialogManager settings apply invokes config reload");
    AssertTrue(timerUpdateCount == 1 && lastTimerInterval == UPDATE_INTERVAL_FAST,
               L"DialogManager settings apply invokes timer update callback");
    AssertTrue(languageApplyCount == 1, L"DialogManager settings apply invokes language callback");
    AssertTrue(applyAllSettingsCount == 1,
               L"DialogManager settings apply invokes apply-all-settings callback");
    AssertTrue(g_wmUpdateStatsCount == 1,
               L"DialogManager settings apply sends WM_UPDATE_STATS to dashboard");
    AssertTrue(networkProvider.m_updateCallCount == 1,
               L"DialogManager settings apply triggers coordinator network tick");
    AssertTrue(oldConfig.updateInterval == UPDATE_INTERVAL_FAST,
               L"DialogManager settings apply updates oldConfig snapshot");

    configReloadCount = 0;
    manager.SetConfigReloadCallback([&]() -> bool
    {
        ++configReloadCount;
        return false;
    });
    DialogManagerTestFriend::ApplySettings(manager, oldConfig);
    AssertTrue(configReloadCount == 1, L"DialogManager config reload callback still invoked");
    AssertTrue(timerUpdateCount == 1,
               L"DialogManager timer callback not repeated when reload returns false");

    DestroyWindow(dashboardWindow);
    DestroyWindow(parent);
}

} // namespace NetPulseTests
