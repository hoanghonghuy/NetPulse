#include "NetPulse/MenuHandler.h"
#include "NetPulse/TaskbarOverlay.h"
#include "test_fakes/FakeConfigProvider.h"
#include "TestUtils.h"
#include "../../resources/resource.h"

#include <windows.h>

using namespace NetPulse;

namespace NetPulseTests
{

void RunMenuHandlerTests()
{
    LogTestMessage(L"=== MenuHandler tests ===");

    AppConfig config;
    config.updateInterval = UPDATE_INTERVAL_NORMAL;
    config.autoStart = false;
    config.autoStartAsAdmin = true;

    FakeConfigProvider configProvider;
    MenuHandler handler;
    handler.Initialize(&config, &configProvider, nullptr);

    bool saveCalled = false;
    UINT lastTimerInterval = 0;
    handler.SetSaveConfigCallback([&]() { saveCalled = true; });
    handler.SetUpdateTimerCallback([&](UINT intervalMs) { lastTimerInterval = intervalMs; });

    handler.HandleCommand(IDM_UPDATE_FAST);
    AssertTrue(config.updateInterval == UPDATE_INTERVAL_FAST,
               L"MenuHandler sets fast update interval");
    AssertTrue(saveCalled && lastTimerInterval == UPDATE_INTERVAL_FAST,
               L"MenuHandler save and timer callbacks for fast interval");

    saveCalled = false;
    handler.HandleCommand(IDM_UPDATE_NORMAL);
    AssertTrue(config.updateInterval == UPDATE_INTERVAL_NORMAL,
               L"MenuHandler sets normal update interval");
    AssertTrue(saveCalled && lastTimerInterval == UPDATE_INTERVAL_NORMAL,
               L"MenuHandler save and timer callbacks for normal interval");

    saveCalled = false;
    handler.HandleCommand(IDM_UPDATE_SLOW);
    AssertTrue(config.updateInterval == UPDATE_INTERVAL_SLOW,
               L"MenuHandler sets slow update interval");
    AssertTrue(saveCalled && lastTimerInterval == UPDATE_INTERVAL_SLOW,
               L"MenuHandler save and timer callbacks for slow interval");

    handler.HandleCommand(IDM_AUTOSTART);
    AssertTrue(config.autoStart, L"MenuHandler toggles autoStart on");
    AssertTrue(configProvider.m_setAutoStartCallCount == 1,
               L"MenuHandler calls SetAutoStart once");
    AssertTrue(configProvider.m_lastSetAutoStartValue && configProvider.m_lastSetAutoStartAsAdmin,
               L"MenuHandler passes autoStart and asAdmin flags to config provider");

    bool settingsCalled = false;
    bool dashboardCalled = false;
    bool perAppCalled = false;
    bool speedTestCalled = false;
    bool connectionLogCalled = false;
    bool updatesCalled = false;
    bool floatingCalled = false;
    bool exitCalled = false;

    handler.SetShowSettingsCallback([&]() { settingsCalled = true; });
    handler.SetShowDashboardCallback([&]() { dashboardCalled = true; });
    handler.SetShowPerAppCallback([&]() { perAppCalled = true; });
    handler.SetShowSpeedTestCallback([&]() { speedTestCalled = true; });
    handler.SetShowConnectionLogCallback([&]() { connectionLogCalled = true; });
    handler.SetCheckForUpdatesCallback([&]() { updatesCalled = true; });
    handler.SetToggleFloatingWindowCallback([&]() { floatingCalled = true; });
    handler.SetExitCallback([&]() { exitCalled = true; });

    handler.HandleCommand(IDM_SETTINGS);
    handler.HandleCommand(IDM_DASHBOARD);
    handler.HandleCommand(IDM_PERAPP);
    handler.HandleCommand(IDM_SPEED_TEST);
    handler.HandleCommand(IDM_CONNECTION_LOG);
    handler.HandleCommand(IDM_CHECK_FOR_UPDATES);
    handler.HandleCommand(IDM_SHOW_FLOATING_WINDOW);
    handler.HandleCommand(IDM_EXIT);

    AssertTrue(settingsCalled && dashboardCalled && perAppCalled && speedTestCalled &&
               connectionLogCalled && updatesCalled && floatingCalled && exitCalled,
               L"MenuHandler invokes dialog and action callbacks");

    TaskbarOverlay overlay;
    if (overlay.Initialize(GetModuleHandleW(nullptr)))
    {
        MenuHandler overlayHandler;
        overlayHandler.Initialize(&config, &configProvider, &overlay);

        AssertTrue(!overlay.IsUserWantsVisible(),
                   L"MenuHandler overlay starts hidden");

        overlayHandler.HandleCommand(IDM_SHOW_TASKBAR_OVERLAY);
        AssertTrue(overlay.IsUserWantsVisible(),
                   L"MenuHandler toggles overlay visible");

        overlayHandler.HandleCommand(IDM_SHOW_TASKBAR_OVERLAY);
        AssertTrue(!overlay.IsUserWantsVisible(),
                   L"MenuHandler toggles overlay hidden");

        overlay.Cleanup();
    }

    MenuHandler handlerWithoutConfig;
    handlerWithoutConfig.HandleCommand(IDM_UPDATE_FAST);
    AssertTrue(true, L"MenuHandler without config pointer does not crash");
}

} // namespace NetPulseTests
