// main_imgui.cpp - Modern ImGui Application Entry Point
// Complete UI replacement for Network Monitor
// Build: cmake -S . -B build -DBUILD_IMGUI_APP=ON && cmake --build build --config Release --target NetworkMonitorImGui

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "NetPulse/ImGuiApp.h"
#include "NetPulse/ImGuiDashboard.h"
#include "NetPulse/ImGuiSettings.h"
#include "NetPulse/ImGuiPerApp.h"
#include "NetPulse/ImGuiHistory.h"
#include "NetPulse/ImGuiMainMenu.h"
#include "NetPulse/ImGuiFloatingWindow.h"
#include "NetPulse/ImGuiTrayIcon.h"
#include "NetPulse/NetworkMonitor.h"
#include "NetPulse/SystemMonitor.h"
#include "NetPulse/ConfigManager.h"
#include "NetPulse/PerAppMonitor.h"
#include "NetPulse/Utils.h"
#include "imgui.h"
#include <memory>
#include <sstream>

// Global instances
std::unique_ptr<NetPulse::NetworkMonitorClass> g_networkMonitor;
std::unique_ptr<NetPulse::SystemMonitor> g_systemMonitor;
std::unique_ptr<NetPulse::ConfigManager> g_configManager;
std::unique_ptr<NetPulse::PerAppMonitor> g_perAppMonitor;
std::unique_ptr<NetPulse::ImGuiDashboard> g_dashboard;
std::unique_ptr<NetPulse::ImGuiSettings> g_settings;
std::unique_ptr<NetPulse::ImGuiPerApp> g_perApp;
std::unique_ptr<NetPulse::ImGuiHistory> g_history;
std::unique_ptr<NetPulse::ImGuiMainMenu> g_mainMenu;
std::unique_ptr<NetPulse::ImGuiFloatingWindow> g_floatingWindow;
std::unique_ptr<NetPulse::ImGuiTrayIcon> g_trayIcon;

// Panel visibility
NetPulse::PanelVisibility g_visibility;

// Current config (for theme switching)
NetPulse::AppConfig g_currentConfig;

// Exit flag
bool g_shouldExit = false;

// Apply common style settings (shared between themes)
void ApplyCommonStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
}

// Apply professional Dark Theme
void ApplyDarkTheme()
{
    ApplyCommonStyle();
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_WindowBg]           = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.12f, 0.12f, 0.15f, 0.95f);
    colors[ImGuiCol_Border]             = ImVec4(0.25f, 0.25f, 0.30f, 0.50f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.30f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.30f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.40f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.25f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.30f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.40f, 0.60f, 0.70f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.25f, 0.50f, 0.75f, 0.80f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.30f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_Tab]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.30f, 0.50f, 0.70f, 0.80f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]      = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]  = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_TableBorderLight]   = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]      = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    colors[ImGuiCol_Text]               = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    colors[ImGuiCol_PlotLines]          = ImVec4(0.30f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
}

// Apply professional Light Theme
void ApplyLightTheme()
{
    ApplyCommonStyle();
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_WindowBg]           = ImVec4(0.96f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.98f, 0.98f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]             = ImVec4(0.70f, 0.70f, 0.75f, 0.50f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.80f, 0.80f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.82f, 0.82f, 0.88f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.70f, 0.70f, 0.75f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.65f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.20f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.20f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.15f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.25f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.30f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.20f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.25f, 0.55f, 0.85f, 0.50f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.30f, 0.60f, 0.90f, 0.60f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.20f, 0.50f, 0.80f, 0.70f);
    colors[ImGuiCol_Tab]                = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.30f, 0.60f, 0.90f, 0.60f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.25f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]      = ImVec4(0.88f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]  = ImVec4(0.70f, 0.70f, 0.75f, 1.00f);
    colors[ImGuiCol_TableBorderLight]   = ImVec4(0.80f, 0.80f, 0.85f, 1.00f);
    colors[ImGuiCol_TableRowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]      = ImVec4(0.00f, 0.00f, 0.00f, 0.03f);
    colors[ImGuiCol_Text]               = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_PlotLines]          = ImVec4(0.20f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogram]      = ImVec4(0.90f, 0.55f, 0.10f, 1.00f);
}

// Apply theme based on config
void ApplyTheme(NetPulse::ThemeMode mode)
{
    switch (mode)
    {
    case NetPulse::ThemeMode::Light:
        ApplyLightTheme();
        break;
    case NetPulse::ThemeMode::Dark:
    case NetPulse::ThemeMode::SystemDefault:
    default:
        ApplyDarkTheme();
        break;
    }
}

void RenderMainUI()
{
    // Main menu bar
    if (!g_mainMenu->Render(g_visibility))
    {
        g_shouldExit = true;
    }

    // Update stats (throttled to once per second)
    static DWORD lastUpdate = 0;
    DWORD now = GetTickCount();
    if (now - lastUpdate > 1000)
    {
        g_networkMonitor->Update();
        g_dashboard->Update(g_networkMonitor.get(), g_systemMonitor.get());
        g_perApp->Update(g_perAppMonitor.get());
        g_history->Update();
        g_floatingWindow->Update(g_networkMonitor.get(), g_systemMonitor.get());

        // Update tray tooltip with speeds
        auto stats = g_networkMonitor->GetAggregatedStats();
        std::wstringstream ss;
        ss << L"Down: " << NetPulse::FormatSpeed(stats.currentDownloadSpeed, NetPulse::SpeedUnit::BytesPerSecond)
           << L" Up: " << NetPulse::FormatSpeed(stats.currentUploadSpeed, NetPulse::SpeedUnit::BytesPerSecond);
        g_trayIcon->UpdateTooltip(ss.str().c_str());

        lastUpdate = now;
    }

    // Render panels based on visibility
    if (g_visibility.showDashboard)
    {
        g_dashboard->Render();
    }

    if (g_visibility.showSettings)
    {
        if (g_settings->Render()) // Returns true if Save clicked
        {
            // Save settings to registry
            NetPulse::AppConfig newConfig = g_settings->GetConfig();
            g_configManager->SaveConfig(newConfig);
            g_settings->ResetModified();
            
            // Apply theme if changed
            if (newConfig.themeMode != g_currentConfig.themeMode)
            {
                ApplyTheme(newConfig.themeMode);
            }
            g_currentConfig = newConfig;
        }
    }

    if (g_visibility.showHistory)
    {
        g_history->Render();
    }

    if (g_visibility.showPerApp)
    {
        g_perApp->Render();
    }

    // Floating window (always rendered if config enabled)
    g_floatingWindow->Render(g_currentConfig);
}

#ifdef IMGUI_APP_STANDALONE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Initialize config manager
    g_configManager = std::make_unique<NetPulse::ConfigManager>();

    // Initialize monitors
    g_networkMonitor = std::make_unique<NetPulse::NetworkMonitorClass>();
    g_systemMonitor = std::make_unique<NetPulse::SystemMonitor>();
    g_perAppMonitor = std::make_unique<NetPulse::PerAppMonitor>();

    // Initialize UI panels
    g_dashboard = std::make_unique<NetPulse::ImGuiDashboard>();
    g_settings = std::make_unique<NetPulse::ImGuiSettings>();
    g_perApp = std::make_unique<NetPulse::ImGuiPerApp>();
    g_history = std::make_unique<NetPulse::ImGuiHistory>();
    g_mainMenu = std::make_unique<NetPulse::ImGuiMainMenu>();
    g_floatingWindow = std::make_unique<NetPulse::ImGuiFloatingWindow>();
    g_trayIcon = std::make_unique<NetPulse::ImGuiTrayIcon>();

    // Setup exit callback
    g_mainMenu->SetExitCallback([]() { g_shouldExit = true; });

    // Load and apply config
    g_configManager->LoadConfig(g_currentConfig);
    g_settings->SetConfig(g_currentConfig);

    if (!g_networkMonitor->Start())
    {
        MessageBoxW(nullptr, L"Failed to start network monitor", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!g_systemMonitor->Initialize())
    {
        MessageBoxW(nullptr, L"Failed to start system monitor", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!g_perAppMonitor->Initialize())
    {
        MessageBoxW(nullptr, L"Failed to start per-app monitor", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create and run ImGui app
    NetPulse::ImGuiApp app;
    if (!app.Initialize(hInstance, nCmdShow))
    {
        MessageBoxW(nullptr, L"Failed to initialize ImGui app", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Apply theme from config after ImGui context is created
    ApplyTheme(g_currentConfig.themeMode);

    // Store app pointer for callbacks
    NetPulse::ImGuiApp* pApp = &app;

    // Setup minimize to tray callback
    app.SetMinimizeCallback([]() {
        // Window will be hidden by WndProc, nothing else to do
    });

    // Initialize tray icon with full menu support
    g_trayIcon->Initialize(hInstance);

    // Config provider for menu state
    g_trayIcon->SetConfigProvider([]() -> const NetPulse::AppConfig* {
        return &g_currentConfig;
    });

    // Floating window visibility provider
    g_trayIcon->SetFloatingVisibleProvider([]() -> bool {
        return g_currentConfig.showFloatingWindow;
    });

    // Action callbacks
    g_trayIcon->SetDashboardCallback([pApp]() {
        pApp->RestoreWindow();
        g_visibility.showDashboard = true;
    });

    g_trayIcon->SetSettingsCallback([pApp]() {
        pApp->RestoreWindow();
        g_visibility.showSettings = true;
    });

    g_trayIcon->SetPerAppCallback([pApp]() {
        pApp->RestoreWindow();
        g_visibility.showPerApp = true;
    });

    g_trayIcon->SetAboutCallback([pApp]() {
        pApp->RestoreWindow();
        // About dialog could be shown via ImGui popup
    });

    g_trayIcon->SetExitCallback([]() {
        g_shouldExit = true;
    });

    // Toggle callbacks
    g_trayIcon->SetAutoStartCallback([](bool enabled) {
        g_currentConfig.autoStart = enabled;
        g_configManager->SaveConfig(g_currentConfig);
        g_settings->SetConfig(g_currentConfig);
    });

    g_trayIcon->SetFloatingWindowCallback([](bool enabled) {
        g_currentConfig.showFloatingWindow = enabled;
        g_configManager->SaveConfig(g_currentConfig);
        g_settings->SetConfig(g_currentConfig);
    });

    g_trayIcon->SetUpdateIntervalCallback([](UINT interval) {
        g_currentConfig.updateInterval = interval;
        g_configManager->SaveConfig(g_currentConfig);
        g_settings->SetConfig(g_currentConfig);
    });

    // Register global hotkey (Win+Shift+N to toggle window)
    app.SetHotkeyCallback([pApp]() {
        if (pApp->IsWindowVisible())
        {
            pApp->HideWindow();
        }
        else
        {
            pApp->RestoreWindow();
        }
    });
    app.RegisterGlobalHotkey(MOD_WIN | MOD_SHIFT, 'N');

    // Initial history load
    g_history->Update();

    // Main loop with exit check
    std::function<bool()> renderCallback = [&]() -> bool {
        RenderMainUI();
        return !g_shouldExit;
    };
    app.Run(renderCallback);

    // Cleanup
    app.UnregisterGlobalHotkey();
    app.Shutdown();

    // Cleanup
    g_trayIcon->Shutdown();
    g_perAppMonitor->Shutdown();
    g_networkMonitor->Stop();
    g_systemMonitor->Shutdown();

    return 0;
}
#endif


