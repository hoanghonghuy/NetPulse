#include "NetPulse/Common.h"
#include "NetPulse/TrayIcon.h"
#include "NetPulse/TaskbarOverlay.h"
#include "NetPulse/FloatingWindow.h"
#include "NetPulse/ThemeHelper.h"
#include "TestUtils.h"

#include <windows.h>

using namespace NetPulse;

namespace NetPulseTests
{

namespace
{
    const wchar_t* kTestWindowClass = L"NetworkMonitorTestWindow";

    HWND CreateTestWindow()
    {
        HINSTANCE hInstance = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = hInstance;
        wc.lpszClassName = kTestWindowClass;

        // RegisterClassExW may fail if already registered; that's fine.
        RegisterClassExW(&wc);

        HWND hwnd = CreateWindowExW(
            0,
            kTestWindowClass,
            L"NetworkMonitor Test Window",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            100, 100,
            nullptr,
            nullptr,
            hInstance,
            nullptr);

        return hwnd;
    }
}

void RunTrayIconTests()
{
    LogTestMessage(L"=== TrayIcon tests ===");

    HWND hwnd = CreateTestWindow();
    if (!hwnd)
    {
        LogTestMessage(L"[WARN] Failed to create test window; skipping TrayIcon tests");
        return;
    }

    TrayIcon icon;
    bool init = icon.Initialize(hwnd);
    if (!init)
    {
        LogTestMessage(L"[WARN] TrayIcon.Initialize failed; skipping further TrayIcon tests");
        DestroyWindow(hwnd);
        return;
    }

    AppConfig config;
    icon.SetConfigSource(&config);

    NetworkStats stats;
    stats.currentDownloadSpeed = 1024.0;
    stats.currentUploadSpeed = 512.0;

    icon.UpdateTooltip(stats, SpeedUnit::KiloBytesPerSecond);
    icon.UpdateIcon(stats.currentDownloadSpeed, stats.currentUploadSpeed);

    icon.Cleanup();
    DestroyWindow(hwnd);

    AssertTrue(true, L"TrayIcon Initialize/Update/Cleanup executed without crash");
}

void RunTaskbarOverlayTests()
{
    LogTestMessage(L"=== TaskbarOverlay tests ===");

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    TaskbarOverlay overlay;

    bool init = overlay.Initialize(hInstance);
    if (!init)
    {
        LogTestMessage(L"[WARN] TaskbarOverlay.Initialize failed; skipping further overlay tests");
        return;
    }

    AssertTrue(!overlay.IsVisible(), L"TaskbarOverlay not visible after Initialize");

    overlay.Show(true);
    AssertTrue(overlay.IsUserWantsVisible(), L"TaskbarOverlay user preference enabled after Show(true)");
    AssertTrue(overlay.IsVisible(), L"TaskbarOverlay visible after Show(true)");

    overlay.UpdateSpeed(2048.0, 1024.0, SpeedUnit::KiloBytesPerSecond);

    overlay.Show(false);
    AssertTrue(!overlay.IsUserWantsVisible(), L"TaskbarOverlay user preference disabled after Show(false)");
    AssertTrue(!overlay.IsVisible(), L"TaskbarOverlay not visible after Show(false)");

    overlay.Cleanup();

    AssertTrue(true, L"TaskbarOverlay Initialize/Show/Update/Cleanup executed without crash");
}

// ========== PHASE 1: FLOATING WINDOW TESTS ==========

void RunFloatingWindowTests()
{
    LogTestMessage(L"=== FloatingWindow tests ===");

    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    FloatingWindow floating;

    bool created = floating.Create(hInstance);
    if (!created)
    {
        LogTestMessage(L"[WARN] FloatingWindow.Create failed; skipping further tests");
        return;
    }

    // Test: Initial state
    AssertTrue(!floating.IsVisible(), L"FloatingWindow not visible after Create");
    AssertTrue(floating.IsSnapToEdge(), L"FloatingWindow snap-to-edge enabled by default");
    AssertTrue(!floating.IsClickThrough(), L"FloatingWindow click-through disabled by default");
    AssertTrue(!floating.IsMiniMode(), L"FloatingWindow mini-mode disabled by default");
    AssertTrue(floating.GetSnapDistance() == 20, L"FloatingWindow default snap distance is 20");

    // Test: Show/Hide
    floating.Show(true);
    AssertTrue(floating.IsVisible(), L"FloatingWindow visible after Show(true)");
    
    floating.Show(false);
    AssertTrue(!floating.IsVisible(), L"FloatingWindow not visible after Show(false)");

    // Test: Snap-to-Edge toggle
    floating.SetSnapToEdge(false);
    AssertTrue(!floating.IsSnapToEdge(), L"FloatingWindow snap-to-edge disabled after SetSnapToEdge(false)");
    
    floating.SetSnapToEdge(true);
    AssertTrue(floating.IsSnapToEdge(), L"FloatingWindow snap-to-edge enabled after SetSnapToEdge(true)");

    // Test: Snap distance
    floating.SetSnapDistance(30);
    AssertTrue(floating.GetSnapDistance() == 30, L"FloatingWindow snap distance changed to 30");
    
    floating.SetSnapDistance(-5);  // Invalid value should default to 20
    AssertTrue(floating.GetSnapDistance() == 20, L"FloatingWindow snap distance defaults to 20 for invalid input");

    // Test: Click-through mode
    floating.SetClickThrough(true);
    AssertTrue(floating.IsClickThrough(), L"FloatingWindow click-through enabled after SetClickThrough(true)");
    
    floating.SetClickThrough(false);
    AssertTrue(!floating.IsClickThrough(), L"FloatingWindow click-through disabled after SetClickThrough(false)");

    // Test: Mini-mode
    floating.SetMiniMode(true);
    AssertTrue(floating.IsMiniMode(), L"FloatingWindow mini-mode enabled after SetMiniMode(true)");
    
    floating.SetMiniMode(false);
    AssertTrue(!floating.IsMiniMode(), L"FloatingWindow mini-mode disabled after SetMiniMode(false)");

    // Test: ToggleMiniMode
    floating.ToggleMiniMode();
    AssertTrue(floating.IsMiniMode(), L"FloatingWindow mini-mode enabled after ToggleMiniMode()");
    
    floating.ToggleMiniMode();
    AssertTrue(!floating.IsMiniMode(), L"FloatingWindow mini-mode disabled after second ToggleMiniMode()");

    // Test: Update methods don't crash
    floating.UpdateSpeed(1024.0, 512.0, SpeedUnit::KiloBytesPerSecond);
    floating.UpdateCPU(45.0);
    floating.UpdateRAM(60.0);
    floating.UpdatePing(25);
    floating.UpdateDataToday(1024 * 1024 * 100, 1024 * 1024 * 50);

    // Send standard Win32 messages to test WindowProc / HandleMessage paths
    HWND hfw = floating.GetHWND();
    if (hfw)
    {
        // Add data points to make sure rendering path handles sparklines
        floating.UpdateSpeed(2048.0, 1024.0, SpeedUnit::KiloBytesPerSecond);
        floating.UpdateCPU(50.0);
        floating.UpdateRAM(70.0);
        floating.UpdatePing(30);

        SendMessageW(hfw, WM_PAINT, 0, 0);
        SendMessageW(hfw, WM_ERASEBKGND, 0, 0);
        SendMessageW(hfw, WM_NCHITTEST, 0, 0);
        
        // Test dragging snap-to-edge
        RECT movingRect = { 100, 100, 100 + 190, 100 + 90 };
        SendMessageW(hfw, WM_MOVING, 0, reinterpret_cast<LPARAM>(&movingRect));
        
        // Test double-click to toggle mini-mode
        SendMessageW(hfw, WM_NCLBUTTONDBLCLK, 0, 0);
        AssertTrue(floating.IsMiniMode(), L"FloatingWindow toggled to mini-mode via double-click");
        
        SendMessageW(hfw, WM_NCLBUTTONDBLCLK, 0, 0);
        AssertTrue(!floating.IsMiniMode(), L"FloatingWindow toggled back to normal-mode via double-click");
        
        // Test ExportChartAsBMP
        std::wstring tempBmpPath = L"temp_sparkline_chart.bmp";
        bool exported = floating.ExportChartAsBMP(tempBmpPath);
        AssertTrue(exported, L"FloatingWindow exported sparkline chart as BMP successfully");
        DeleteFileW(tempBmpPath.c_str()); // cleanup
    }

    floating.Destroy();
    AssertTrue(floating.GetHWND() == nullptr, L"FloatingWindow HWND is null after Destroy");

    AssertTrue(true, L"FloatingWindow Phase 1 tests completed successfully");
}

void RunThemeHelperTests()
{
    LogTestMessage(L"=== ThemeHelper tests ===");

    // Test: GetColors for all ThemeMode presets
    const ThemeMode modes[] = {
        ThemeMode::SystemDefault,
        ThemeMode::Light,
        ThemeMode::Dark,
        ThemeMode::Dracula,
        ThemeMode::Cyberpunk,
        ThemeMode::Nord,
        ThemeMode::Forest,
        ThemeMode::OLED,
        ThemeMode::SolarizedLight,
        ThemeMode::MorningMist,
        ThemeMode::SoftPaper,
        ThemeMode::MintFresh,
        ThemeMode::Lavender,
        ThemeMode::RosePink
    };

    for (auto mode : modes)
    {
        const auto& colors = ThemeHelper::GetColors(mode);
        // Basic check that it doesn't crash and returns a valid struct
        AssertTrue(colors.background != 0 || colors.textPrimary != 0, L"ThemeHelper::GetColors returned valid colors");
    }

    // Test: GetColors with boolean
    const auto& darkColors = ThemeHelper::GetColors(true);
    const auto& lightColors = ThemeHelper::GetColors(false);
    AssertTrue(darkColors.background != lightColors.background, L"ThemeHelper::GetColors(bool) distinguishes dark/light");

    // Test: SetCurrentTheme / GetCurrentTheme
    ThemeHelper::SetCurrentTheme(ThemeMode::Nord);
    AssertTrue(ThemeHelper::GetCurrentTheme() == ThemeMode::Nord, L"ThemeHelper::SetCurrentTheme sets correctly");

    ThemeHelper::SetCurrentTheme(ThemeMode::Dark);
    AssertTrue(ThemeHelper::GetCurrentTheme() == ThemeMode::Dark, L"ThemeHelper::SetCurrentTheme sets correctly to Dark");

    // Test: AllowDarkModeForApp
    ThemeHelper::AllowDarkModeForApp(true);
    ThemeHelper::AllowDarkModeForApp(false);

    // Test: IsSystemInDarkMode
    bool isSystemDark = ThemeHelper::IsSystemInDarkMode();
    LogTestMessage((L"  System is in dark mode: " + std::wstring(isSystemDark ? L"Yes" : L"No")).c_str());

    // Test: Apply theme functions with a real window
    HWND hwnd = CreateTestWindow();
    if (hwnd)
    {
        ThemeHelper::AllowDarkModeForWindow(hwnd, true);
        ThemeHelper::ApplyDarkTitleBar(hwnd, true);
        ThemeHelper::ApplyDarkThemeToControl(hwnd, true);
        
        ThemeHelper::AllowDarkModeForWindow(hwnd, false);
        ThemeHelper::ApplyDarkTitleBar(hwnd, false);
        ThemeHelper::ApplyDarkThemeToControl(hwnd, false);

        DestroyWindow(hwnd);
    }

    AssertTrue(true, L"ThemeHelper tests completed successfully");
}

} // namespace NetPulseTests
