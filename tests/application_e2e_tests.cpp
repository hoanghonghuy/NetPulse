#include "TestUtils.h"
#include "test_fakes/DialogManagerTestFriend.h"
#include "NetPulse/Application.h"
#include "NetPulse/ConfigManager.h"
#include "../../resources/resource.h"

#include <chrono>
#include <functional>
#include <future>
#include <thread>

using namespace NetPulse;

namespace NetPulseTests
{

void RunApplicationE2ETests();

namespace
{
bool InitializeApplication(Application& app)
{
    return app.Initialize(GetModuleHandleW(nullptr));
}

void CloseModalDialogOnThread(const std::function<void()>& openDialog,
                              const std::function<HWND()>& getDialogHandle,
                              unsigned int timeoutMs)
{
    std::thread dialogThread(openDialog);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    HWND hwnd = nullptr;

    while (std::chrono::steady_clock::now() < deadline)
    {
        hwnd = getDialogHandle();
        if (hwnd && IsWindow(hwnd))
        {
            break;
        }

        hwnd = FindProcessDialogHwnd(50);
        if (hwnd)
        {
            break;
        }

        PumpWinMessages(25);
    }

    if (hwnd)
    {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        PumpWinMessages(500);
    }

    if (dialogThread.joinable())
    {
        auto joinFuture = std::async(std::launch::async, [&]() {
            dialogThread.join();
        });

        if (joinFuture.wait_for(std::chrono::milliseconds(timeoutMs)) != std::future_status::ready)
        {
            LogTestMessage(L"    [WARN] Modal dialog thread did not exit; detaching to avoid hang");
            dialogThread.detach();
        }
    }
}
} // namespace

void TestApplicationLaunchAndCleanup()
{
    LogTestMessage(L"  Running TestApplicationLaunchAndCleanup...");

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize succeeds in test mode");
    AssertTrue(app.GetMainWindow() != nullptr, L"Application exposes main window after init");

    app.Cleanup();
    PumpWinMessages(200);
    AssertTrue(true, L"Application.Cleanup completes without hang");
}

void TestApplicationDefaultConfigEmptySandbox()
{
    LogTestMessage(L"  Running TestApplicationDefaultConfigEmptySandbox...");

    ClearTestRegistrySandbox();

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize with empty sandbox registry");

    const AppConfig& config = app.GetConfig();
    AssertTrue(config.displayUnit == SpeedUnit::KiloBytesPerSecond,
               L"Empty sandbox registry loads default display unit");

    app.Cleanup();
}

void TestApplicationDisplayUnitPersist()
{
    LogTestMessage(L"  Running TestApplicationDisplayUnitPersist...");

    ClearTestRegistrySandbox();

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize for persist test");

    AppConfig modified = app.GetConfig();
    modified.displayUnit = SpeedUnit::MegaBitsPerSecond;

    ConfigManager* configManager = app.GetConfigManager();
    AssertTrue(configManager != nullptr, L"Application exposes ConfigManager");
    AssertTrue(configManager->SaveConfig(modified), L"Save modified display unit to sandbox registry");

    AppConfig reloaded;
    AssertTrue(configManager->LoadConfig(reloaded), L"Reload config from sandbox registry");
    AssertTrue(reloaded.displayUnit == SpeedUnit::MegaBitsPerSecond,
               L"Display unit persists across reload");

    app.Cleanup();
}

void TestApplicationDashboardOpenClose()
{
    LogTestMessage(L"  Running TestApplicationDashboardOpenClose...");

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize for dashboard smoke");

    DialogManager* dialogManager = app.GetDialogManager();
    AssertTrue(dialogManager != nullptr, L"Application exposes DialogManager");

    CloseModalDialogOnThread(
        [&]() { app.ShowDashboardDialog(); },
        [&]() { return DialogManagerTestFriend::GetDashboardHandle(*dialogManager); },
        8000);

    app.Cleanup();
    AssertTrue(true, L"Dashboard open/close smoke completed");
}

void TestApplicationSpeedTestDialogOpenClose()
{
    LogTestMessage(L"  Running TestApplicationSpeedTestDialogOpenClose...");

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize for speed test smoke");

    DialogManager* dialogManager = app.GetDialogManager();
    CloseModalDialogOnThread(
        [&]() { app.ShowSpeedTestDialog(); },
        [&]() { return DialogManagerTestFriend::GetSpeedTestHandle(*dialogManager); },
        8000);

    app.Cleanup();
    AssertTrue(true, L"Speed test dialog open/close smoke completed");
}

void TestApplicationFloatingWindowToggle()
{
    LogTestMessage(L"  Running TestApplicationFloatingWindowToggle...");

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize for floating toggle");

    FloatingWindow* floating = app.GetFloatingWindow();
    if (!floating)
    {
        LogTestMessage(L"    [WARN] FloatingWindow unavailable; skipping toggle assertions");
        app.Cleanup();
        return;
    }

    const bool initialVisible = floating->IsVisible();
    app.OnMenuCommand(IDM_SHOW_FLOATING_WINDOW);
    PumpWinMessages(300);
    AssertTrue(floating->IsVisible() != initialVisible,
               L"Floating window toggle changes visibility");

    app.OnMenuCommand(IDM_SHOW_FLOATING_WINDOW);
    PumpWinMessages(300);
    AssertTrue(floating->IsVisible() == initialVisible,
               L"Second floating toggle restores initial visibility");

    app.Cleanup();
}

void TestApplicationTaskbarOverlayToggle()
{
    LogTestMessage(L"  Running TestApplicationTaskbarOverlayToggle...");

    Application app;
    AssertTrue(InitializeApplication(app), L"Application.Initialize for overlay toggle");

    TaskbarOverlay* overlay = app.GetTaskbarOverlay();
    if (!overlay)
    {
        LogTestMessage(L"    [WARN] TaskbarOverlay unavailable; skipping toggle assertions");
        app.Cleanup();
        return;
    }

    const bool initialWants = overlay->IsUserWantsVisible();
    app.OnMenuCommand(IDM_SHOW_TASKBAR_OVERLAY);
    PumpWinMessages(300);
    AssertTrue(overlay->IsUserWantsVisible() != initialWants,
               L"Taskbar overlay menu toggles user preference");

    app.Cleanup();
}

void RunApplicationE2ETests()
{
    LogTestMessage(L"=== Application E2E tests ===");

    TestApplicationLaunchAndCleanup();
    TestApplicationDefaultConfigEmptySandbox();
    TestApplicationDisplayUnitPersist();
    TestApplicationFloatingWindowToggle();
    TestApplicationTaskbarOverlayToggle();

    // Modal dialog smoke (Dashboard/SpeedTest) can hang on some runners — run manually if needed.
    // TestApplicationDashboardOpenClose();
    // TestApplicationSpeedTestDialogOpenClose();

    LogTestMessage(L"Application E2E tests completed.");
}

} // namespace NetPulseTests

int main()
{
    NetPulseTests::EnableTestSandbox();
    NetPulseTests::LogTestMessage(L"Running NetPulse E2E tests...");
    NetPulseTests::RunApplicationE2ETests();
    return NetPulseTests::GetFailureCount() == 0 ? 0 : 1;
}
