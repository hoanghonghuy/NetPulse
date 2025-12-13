#include "NetworkMonitor/Application.h"
#include "NetworkMonitor/Utils.h"
#include "NetworkMonitor/HistoryLogger.h"
#include "NetworkMonitor/SettingsDialog.h"
#include "NetworkMonitor/DashboardDialog.h"
#include "NetworkMonitor/HistoryDialog.h"

#include "NetworkMonitor/ThemeHelper.h"
#include "../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>

namespace NetworkMonitor
{

// Static member initialization
UINT Application::s_taskbarCreatedMsg = 0;

Application::Application()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_initialized(false)
{
}

Application::~Application()
{
    Cleanup();
}

bool Application::Initialize(HINSTANCE hInstance)
{
    if (m_initialized)
    {
        return true;
    }

    m_hInstance = hInstance;

    LogDebug(L"Application::Initialize: starting");

    // Initialize common controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&icc))
    {
        // Fallback to a generic initialization error message
        ShowErrorMessage(LoadStringResource(IDS_ERROR_INIT));
        return false;
    }

    // Register window class
    if (!RegisterWindowClass())
    {
        return false;
    }

    // Create main window (hidden)
    if (!CreateMainWindow())
    {
        return false;
    }

    // Register for TaskbarCreated message (sent when explorer.exe restarts)
    s_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");

    // Create and initialize components
    m_pConfigManager = std::make_unique<ConfigManager>();
    if (!LoadConfig())
    {
        // Use default config if load fails
        m_config = AppConfig();
    }

    // CRITICAL: Create and apply language IMMEDIATELY after loading config
    // BEFORE any string resources are loaded
    m_pLanguageManager = std::make_unique<LanguageManager>();
    ApplyLanguageFromConfig();

    SetDebugLoggingEnabled(m_config.debugLogging);

    // Initialize dark mode support for process-level elements (context
    // menus, some common controls) based on the current system app theme
    // so that shell-provided UI (like the tray menu) matches Windows.
    bool systemDark = ThemeHelper::IsSystemInDarkMode();
    ThemeHelper::AllowDarkModeForApp(systemDark);

    // Initialize history logger with auto-trim settings
    if (m_config.historyAutoTrimDays > 0)
    {
        HistoryLogger::Instance().TrimToRecentDays(m_config.historyAutoTrimDays);
    }

    // Create and initialize network monitor
    m_pNetworkMonitor = std::make_unique<NetworkMonitorClass>();
    if (!m_pNetworkMonitor->Start())
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_START_NETWORK_MONITOR));
        return false;
    }

    // Create and initialize tray icon
    m_pTrayIcon = std::make_unique<TrayIcon>();
    if (!m_pTrayIcon->Initialize(m_hwnd))
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_INIT_TRAY_ICON));
        return false;
    }

    // Set tray icon callbacks and configuration source
    m_pTrayIcon->SetMenuCallback([this](UINT menuId) { OnMenuCommand(menuId); });
    m_pTrayIcon->SetConfigSource(&m_config);
    m_pTrayIcon->SetOverlayVisibilityProvider([this]() -> bool {
        return m_pTaskbarOverlay != nullptr && m_pTaskbarOverlay->IsVisible();
    });
    m_pTrayIcon->SetFloatingWindowVisibilityProvider([this]() -> bool {
        return m_pFloatingWindow != nullptr && m_pFloatingWindow->IsVisible();
    });
    m_pTrayIcon->SetDoubleClickCallback([this]() {
        // Double-click opens Dashboard
        OnMenuCommand(IDM_DASHBOARD);
    });

    // Create and initialize taskbar overlay (enabled, same behavior as legacy main.cpp)
    m_pTaskbarOverlay = std::make_unique<TaskbarOverlay>();
    if (!m_pTaskbarOverlay->Initialize(m_hInstance))
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_INIT_TASKBAR_OVERLAY));
        // Don't fail completely, just log warning and continue without overlay
        m_pTaskbarOverlay.reset();
    }
    else
    {
        // Set right-click callback for overlay
        m_pTaskbarOverlay->SetRightClickCallback([this]() { OnTaskbarOverlayRightClick(); });

        // Show overlay by default
        m_pTaskbarOverlay->Show(true);

        m_pTaskbarOverlay->SetDarkTheme(m_config.darkTheme);
        m_pTaskbarOverlay->SetOverlayStyle(m_config.overlayFontSize, 
                                           m_config.overlayDownloadColor, 
                                           m_config.overlayUploadColor);
    }

    // Create and initialize ping monitor
    m_pPingMonitor = std::make_unique<PingMonitor>();
    if (!m_pPingMonitor->Initialize(m_config.pingTarget))
    {
        LogDebug(L"Application::Initialize: PingMonitor init failed, continuing without ping");
        m_pPingMonitor.reset();
    }

    // Start timer for network monitoring updates
    SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, m_config.updateInterval, nullptr);

    // Start timer for ping (use configured interval)
    if (m_pPingMonitor)
    {
        SetTimer(m_hwnd, TIMER_PING, m_config.pingIntervalMs, nullptr);
    }

    // Create and initialize hotkey manager
    m_pHotkeyManager = std::make_unique<HotkeyManager>();
    m_pHotkeyManager->Initialize(m_hwnd);
    m_pHotkeyManager->SetCallback([this](int id) { OnHotkey(id); });
    SetupHotkeys();

    // Create and initialize MenuHandler
    m_pMenuHandler = std::make_unique<MenuHandler>();
    m_pMenuHandler->Initialize(&m_config, m_pConfigManager.get(), m_pTaskbarOverlay.get());
    m_pMenuHandler->SetSaveConfigCallback([this]() { SaveConfig(); });
    m_pMenuHandler->SetShowSettingsCallback([this]() { ShowSettingsDialog(); });
    m_pMenuHandler->SetShowDashboardCallback([this]() { ShowDashboardDialog(); });
    m_pMenuHandler->SetShowAboutCallback([this]() { ShowAboutDialog(); });
    m_pMenuHandler->SetExitCallback([this]() { DestroyWindow(m_hwnd); });
    m_pMenuHandler->SetUpdateTimerCallback([this](UINT intervalMs) {
        KillTimer(m_hwnd, TIMER_UPDATE_NETWORK);
        SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, intervalMs, nullptr);
    });
    m_pMenuHandler->SetToggleFloatingWindowCallback([this]() {
        if (m_pFloatingWindow)
        {
            bool isVisible = m_pFloatingWindow->IsVisible();
            m_pFloatingWindow->Show(!isVisible);
            m_config.showFloatingWindow = !isVisible;
            SaveConfig();
        }
    });
    m_pMenuHandler->SetShowPerAppCallback([this]() { ShowPerAppDialog(); });
    m_pMenuHandler->SetShowSpeedTestCallback([this]() { ShowSpeedTestDialog(); });

    // Create and initialize UpdateCoordinator
    m_pUpdateCoordinator = std::make_unique<UpdateCoordinator>();
    m_pUpdateCoordinator->Initialize(
        &m_config,
        m_pNetworkMonitor.get(),
        m_pTrayIcon.get(),
        m_pTaskbarOverlay.get(),
        m_pPingMonitor.get()
    );
    m_pUpdateCoordinator->SetLogHistoryCallback([this](unsigned long long bytesDown, unsigned long long bytesUp) {
        // Get interface name for logging
        std::wstring ifaceName = m_config.selectedInterface;
        if (ifaceName.empty())
        {
            ifaceName = LoadStringResource(IDS_ALL_INTERFACES);
            if (ifaceName.empty())
            {
                ifaceName = L"All Interfaces";
            }
        }
        HistoryLogger::Instance().AppendSample(ifaceName, bytesDown, bytesUp);
    });


    // Create and initialize DialogManager
    m_pDialogManager = std::make_unique<DialogManager>();
    m_pDialogManager->Initialize(
        m_hwnd,
        &m_config,
        m_pConfigManager.get(),
        m_pNetworkMonitor.get(),
        m_pUpdateCoordinator.get()
    );
    m_pDialogManager->SetConfigReloadCallback([this]() { return LoadConfig(); });
    m_pDialogManager->SetLanguageApplyCallback([this]() { ApplyLanguageFromConfig(); });
    m_pDialogManager->SetTimerUpdateCallback([this](UINT intervalMs) {
        KillTimer(m_hwnd, TIMER_UPDATE_NETWORK);
        SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, intervalMs, nullptr);
    });
    m_pDialogManager->SetApplyAllSettingsCallback([this]() {
        // Apply FloatingWindow settings
        if (m_pFloatingWindow)
        {
            m_pFloatingWindow->SetDarkTheme(m_config.darkTheme);
            m_pFloatingWindow->SetOpacity(m_config.floatingWindowOpacity);
            m_pFloatingWindow->SetShowNetwork(m_config.floatingShowNetwork);
            m_pFloatingWindow->SetShowCPU(m_config.floatingShowCPU);
            m_pFloatingWindow->SetShowRAM(m_config.floatingShowRAM);
            m_pFloatingWindow->SetShowPing(m_config.floatingShowPing);
            m_pFloatingWindow->SetShowDataToday(m_config.floatingShowDataToday);
            m_pFloatingWindow->SetShowSparkline(m_config.floatingShowSparkline);
            m_pFloatingWindow->SetSparklineTimeRange(m_config.sparklineTimeRange);
        }
        
        // Apply Ping Target change
        if (m_pPingMonitor)
        {
            m_pPingMonitor->SetTarget(m_config.pingTarget);
        }
        
        // Re-register Hotkeys if changed
        if (m_pHotkeyManager)
        {
            m_pHotkeyManager->UnregisterAll();
            SetupHotkeys();
        }
        
        // Apply TrayIcon theme
        if (m_pTrayIcon)
        {
            m_pTrayIcon->RefreshIcon(m_config.darkTheme);
        }
        
        // Apply VPN/Proxy settings (Phase 3)
        if (m_pFloatingWindow)
        {
            m_pFloatingWindow->SetShowVpnStatus(m_config.floatingShowVpnStatus);
            m_pFloatingWindow->SetShowPublicIP(m_config.floatingShowPublicIP);
        }
        if (m_pVpnDetector)
        {
            m_pVpnDetector->SetPublicIPUpdateInterval(m_config.publicIPUpdateIntervalMs);
        }
    });

    // Create and initialize SystemMonitor for CPU/RAM
    m_pSystemMonitor = std::make_unique<SystemMonitor>();
    m_pSystemMonitor->Initialize();

    // Create and initialize FloatingWindow
    m_pFloatingWindow = std::make_unique<FloatingWindow>();
    if (m_pFloatingWindow->Create(m_hInstance))
    {
        // Apply config settings
        m_pFloatingWindow->SetDarkTheme(m_config.darkTheme);
        m_pFloatingWindow->SetOpacity(m_config.floatingWindowOpacity);
        m_pFloatingWindow->SetShowNetwork(m_config.floatingShowNetwork);
        m_pFloatingWindow->SetShowCPU(m_config.floatingShowCPU);
        m_pFloatingWindow->SetShowRAM(m_config.floatingShowRAM);
        m_pFloatingWindow->SetShowPing(m_config.floatingShowPing);
        m_pFloatingWindow->SetShowDataToday(m_config.floatingShowDataToday);
        m_pFloatingWindow->SetShowSparkline(m_config.floatingShowSparkline);
        m_pFloatingWindow->SetSparklineTimeRange(m_config.sparklineTimeRange);
        
        // Set callback to save sparkline time range when changed via context menu
        m_pFloatingWindow->SetConfigChangeCallback([this](int timeRange) {
            m_config.sparklineTimeRange = timeRange;
            m_pConfigManager->SaveConfig(m_config);
        });
        
        // Set position if saved
        if (m_config.floatingWindowX >= 0 && m_config.floatingWindowY >= 0)
        {
            m_pFloatingWindow->SetPosition(m_config.floatingWindowX, m_config.floatingWindowY);
        }
        
        // Show if enabled in config
        m_pFloatingWindow->Show(m_config.showFloatingWindow);
    }
    else
    {
        LogDebug(L"Application::Initialize: FloatingWindow create failed, continuing without it");
        m_pFloatingWindow.reset();
    }

    // Create and initialize VPN/Proxy detector (Phase 3)
    m_pVpnDetector = std::make_unique<VpnProxyDetector>();
    if (m_pVpnDetector->Initialize())
    {
        // Set update interval from config
        m_pVpnDetector->SetPublicIPUpdateInterval(m_config.publicIPUpdateIntervalMs);
        
        // Apply VPN display settings to floating window
        if (m_pFloatingWindow)
        {
            m_pFloatingWindow->SetShowVpnStatus(m_config.floatingShowVpnStatus);
            m_pFloatingWindow->SetShowPublicIP(m_config.floatingShowPublicIP);
        }
        
        // Start VPN update timer (30 seconds for VPN detection, IP uses its own rate limiting)
        SetTimer(m_hwnd, TIMER_VPN_UPDATE, 30000, nullptr);
    }
    else
    {
        LogDebug(L"Application::Initialize: VpnProxyDetector init failed, continuing without VPN detection");
        m_pVpnDetector.reset();
    }

    m_initialized = true;
    LogDebug(L"Application::Initialize: succeeded");
    return true;
}

int Application::Run()
{
    if (!m_initialized)
    {
        return -1;
    }

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

void Application::Cleanup()
{
    if (!m_initialized)
    {
        return;
    }

    LogDebug(L"Application::Cleanup: starting");

    // Cleanup hotkey manager (auto-unregisters all hotkeys)
    m_pHotkeyManager.reset();

    // Stop ping monitor
    if (m_pPingMonitor)
    {
        KillTimer(m_hwnd, TIMER_PING);
        m_pPingMonitor->Cleanup();
        m_pPingMonitor.reset();
    }

    // Stop network monitoring
    if (m_pNetworkMonitor)
    {
        m_pNetworkMonitor->Stop();
        m_pNetworkMonitor.reset();
    }

    // Cleanup taskbar overlay
    if (m_pTaskbarOverlay)
    {
        m_pTaskbarOverlay->Cleanup();
        m_pTaskbarOverlay.reset();
    }

    // Cleanup tray icon
    if (m_pTrayIcon)
    {
        m_pTrayIcon->Cleanup();
        m_pTrayIcon.reset();
    }

    // Cleanup floating window (save position before destroying)
    if (m_pFloatingWindow)
    {
        if (m_pFloatingWindow->IsVisible())
        {
            int x, y;
            m_pFloatingWindow->GetPosition(x, y);
            m_config.floatingWindowX = x;
            m_config.floatingWindowY = y;
            SaveConfig();
        }
        m_pFloatingWindow->Destroy();
        m_pFloatingWindow.reset();
    }

    // Cleanup system monitor
    if (m_pSystemMonitor)
    {
        m_pSystemMonitor->Shutdown();
        m_pSystemMonitor.reset();
    }

    // Cleanup VPN detector (Phase 3)
    if (m_pVpnDetector)
    {
        KillTimer(m_hwnd, TIMER_VPN_UPDATE);
        m_pVpnDetector->Cleanup();
        m_pVpnDetector.reset();
    }

    // Cleanup config manager
    m_pConfigManager.reset();

    // Destroy main window
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    m_initialized = false;
    LogDebug(L"Application::Cleanup: completed");
}

bool Application::LoadConfig()
{
    if (!m_pConfigManager)
    {
        return false;
    }

    return m_pConfigManager->LoadConfig(m_config);
}

bool Application::SaveConfig()
{
    if (!m_pConfigManager)
    {
        return false;
    }

    return m_pConfigManager->SaveConfig(m_config);
}

void Application::ApplyLanguageFromConfig()
{
    if (m_pLanguageManager)
    {
        m_pLanguageManager->ApplyLanguage(m_config.language);
    }
}

void Application::ShowSettingsDialog()
{
    if (m_pDialogManager)
    {
        m_pDialogManager->ShowSettings();
    }
}

void Application::ShowDashboardDialog()
{
    if (m_pDialogManager)
    {
        m_pDialogManager->ShowDashboard();
    }
}

void Application::ShowHistoryDialog()
{
    if (m_pDialogManager)
    {
        m_pDialogManager->ShowHistory();
    }
}

void Application::ShowAboutDialog()
{
    if (m_pDialogManager)
    {
        m_pDialogManager->ShowAbout();
    }
}

void Application::ShowPerAppDialog()
{
    if (m_pDialogManager)
    {
        m_pDialogManager->ShowPerApp();
    }
}

void Application::ShowSpeedTestDialog()
{
    if (m_pDialogManager)
    {
        m_pDialogManager->ShowSpeedTest();
    }
}

void Application::OnTaskbarOverlayRightClick()
{
    // When user right-clicks on taskbar overlay, show the tray icon context menu
    if (m_pTrayIcon)
    {
        m_pTrayIcon->ShowContextMenu();
    }
}

void Application::OnMenuCommand(UINT menuId)
{
    if (m_pMenuHandler)
    {
        m_pMenuHandler->HandleCommand(menuId);
    }
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Application* pThis = nullptr;

    if (message == WM_CREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<Application*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->InstanceWindowProc(hwnd, message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK Application::InstanceWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == TIMER_UPDATE_NETWORK)
            {
                if (m_pUpdateCoordinator)
                {
                    m_pUpdateCoordinator->OnNetworkUpdateTick();
                }
                
                // Update floating window with network speed and system info
                if (m_pFloatingWindow && m_pFloatingWindow->IsVisible())
                {
                    // Update system monitor
                    if (m_pSystemMonitor)
                    {
                        m_pSystemMonitor->Update();
                        m_pFloatingWindow->UpdateCPU(m_pSystemMonitor->GetCPUPercent());
                        m_pFloatingWindow->UpdateRAM(m_pSystemMonitor->GetRAMPercent());
                    }
                    
                    // Update network speed from network monitor
                    if (m_pNetworkMonitor)
                    {
                        NetworkStats stats = m_pNetworkMonitor->GetAggregatedStats();
                        m_pFloatingWindow->UpdateSpeed(
                            stats.currentDownloadSpeed,
                            stats.currentUploadSpeed,
                            m_config.displayUnit
                        );
                    }
                    
                    // Update ping latency from ping monitor
                    if (m_pPingMonitor)
                    {
                        m_pFloatingWindow->UpdatePing(m_pPingMonitor->GetLatency());
                    }
                    
                    // Update Data Today (from HistoryLogger)
                    // HistoryLogger is a singleton
                    unsigned long long todayDown = 0;
                    unsigned long long todayUp = 0;
                    if (HistoryLogger::Instance().GetTotalsToday(todayDown, todayUp))
                    {
                        m_pFloatingWindow->UpdateDataToday(todayDown, todayUp);
                    }
                }
            }
            else if (wParam == TIMER_PING)
            {
                if (m_pUpdateCoordinator)
                {
                    m_pUpdateCoordinator->OnPingTick();
                }
            }
            else if (wParam == TIMER_VPN_UPDATE)  // Phase 3: VPN update
            {
                if (m_pVpnDetector)
                {
                    m_pVpnDetector->Update();
                    
                    // Update floating window with VPN status
                    if (m_pFloatingWindow && m_pFloatingWindow->IsVisible())
                    {
                        m_pFloatingWindow->UpdateVpnStatus(
                            m_pVpnDetector->IsVpnActive(),
                            m_pVpnDetector->IsProxyActive()
                        );
                        m_pFloatingWindow->UpdatePublicIP(m_pVpnDetector->GetPublicIP());
                    }
                }
            }
            else if (wParam == 9001) // TrayIcon ANIMATION_TIMER_ID
            {
                if (m_pTrayIcon)
                {
                    m_pTrayIcon->OnAnimationTick();
                }
            }
            return 0;
        }

        case WM_HOTKEY:
        {
            if (m_pHotkeyManager)
            {
                m_pHotkeyManager->OnHotkey(static_cast<int>(wParam));
            }
            return 0;
        }

        case WM_TRAYICON:
        {
            // Handle tray icon messages
            if (m_pTrayIcon)
            {
                m_pTrayIcon->HandleMessage(message, wParam, lParam);
            }
            return 0;
        }

        case WM_COMMAND:
        {
            // Handle menu commands
            OnMenuCommand(LOWORD(wParam));
            return 0;
        }

        case WM_MEASUREITEM:
        {
            if (((LPMEASUREITEMSTRUCT)lParam)->CtlType == ODT_MENU)
            {
                m_pTrayIcon->HandleMenuMeasureItem((LPMEASUREITEMSTRUCT)lParam);
                return TRUE;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        case WM_DRAWITEM:
        {
            if (((LPDRAWITEMSTRUCT)lParam)->CtlType == ODT_MENU)
            {
                m_pTrayIcon->HandleMenuDrawItem((LPDRAWITEMSTRUCT)lParam);
                return TRUE;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        case WM_DESTROY:
        {
            // Kill timers
            KillTimer(hwnd, TIMER_UPDATE_NETWORK);
            KillTimer(hwnd, TIMER_PING);
            
            // Post quit message
            PostQuitMessage(0);
            return 0;
        }

        default:
        {
            // Handle TaskbarCreated message to restore tray icon
            if (message == s_taskbarCreatedMsg && s_taskbarCreatedMsg != 0)
            {
                LogDebug(L"Application::InstanceWindowProc: TaskbarCreated received, restoring tray icon");
                
                // Recreate tray icon (explorer.exe has been restarted)
                if (m_pTrayIcon)
                {
                    // First cleanup the old (now invalid) icon
                    m_pTrayIcon->Cleanup();
                    
                    // Re-initialize the tray icon
                    if (!m_pTrayIcon->Initialize(m_hwnd))
                    {
                        LogError(L"Application::InstanceWindowProc: Failed to restore tray icon");
                    }
                    // Icon state will be updated automatically on next network update tick
                }
                return 0;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
}

bool Application::RegisterWindowClass()
{
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = APP_WINDOW_CLASS;
    wc.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&wc))
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_REGISTER_WINDOW_CLASS));
        return false;
    }

    return true;
}

bool Application::CreateMainWindow()
{
    // Create a message-only window (completely invisible)
    m_hwnd = CreateWindowExW(
        0,
        APP_WINDOW_CLASS,
        APP_NAME,
        0,  // No styles
        0, 0, 0, 0,  // No position or size
        HWND_MESSAGE,  // Message-only window parent
        nullptr,
        m_hInstance,
        this
    );

    if (!m_hwnd)
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_CREATE_WINDOW));
        return false;
    }
    
    return true;
}

void Application::SetupHotkeys()
{
    if (!m_pHotkeyManager)
    {
        return;
    }

    // Register configurable hotkey to toggle overlay
    m_pHotkeyManager->RegisterHotkey(HOTKEY_TOGGLE_OVERLAY, 
                                     m_config.hotkeyModifier, 
                                     m_config.hotkeyKey);
}

void Application::OnHotkey(int hotkeyId)
{
    if (hotkeyId == HOTKEY_TOGGLE_OVERLAY)
    {
        if (m_pTaskbarOverlay)
        {
            bool isVisible = m_pTaskbarOverlay->IsVisible();
            m_pTaskbarOverlay->Show(!isVisible);
            LogDebug(L"Application::OnHotkey: Toggled overlay visibility");
        }
    }
}

} // namespace NetworkMonitor

