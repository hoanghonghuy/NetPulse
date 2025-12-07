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

Application::Application()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_prevTotalBytesDown(0)
    , m_prevTotalBytesUp(0)
    , m_prevTotalsValid(false)
    , m_wasConnected(true)
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

    // Create and initialize components
    m_pConfigManager = std::make_unique<ConfigManager>();
    if (!LoadConfig())
    {
        // Use default config if load fails
        m_config = AppConfig();
    }

    SetDebugLoggingEnabled(m_config.debugLogging);

    // Apply UI language preference (for STRINGTABLE resources)
    ApplyLanguageFromConfig();

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
    LANGID langId = 0;

    switch (m_config.language)
    {
    case AppLanguage::English:
        langId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        break;

    case AppLanguage::Vietnamese:
        langId = MAKELANGID(LANG_VIETNAMESE, SUBLANG_VIETNAMESE_VIETNAM);
        break;

    case AppLanguage::Japanese:
        langId = MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN);
        break;

    case AppLanguage::Korean:
        langId = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
        break;

    case AppLanguage::ChineseSimplified:
        langId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
        break;

    case AppLanguage::SystemDefault:
    default:
        langId = GetUserDefaultUILanguage();
        break;
    }

    if (langId != 0)
    {
        SetThreadUILanguage(langId);
    }
}

void Application::ShowSettingsDialog()
{
    if (!m_pConfigManager)
    {
        return;
    }

    // Loop to handle dialog reopen (for theme/language changes)
    INT_PTR dialogResult;
    do
    {
        // Keep a copy of current config to detect changes after dialog
        AppConfig oldConfig = m_config;

        SettingsDialog dlg;

        // Set up callback for Apply button - reload and apply settings immediately
        dlg.SetSettingsChangedCallback([this, &oldConfig]()
        {
            // Reload config from registry
            if (LoadConfig())
            {
                SetDebugLoggingEnabled(m_config.debugLogging);
                ThemeHelper::AllowDarkModeForApp(ThemeHelper::IsSystemInDarkMode());

                if (m_pTrayIcon)
                {
                    m_pTrayIcon->SetConfigSource(&m_config);
                }

                if (m_pTaskbarOverlay)
                {
                    m_pTaskbarOverlay->SetDarkTheme(m_config.darkTheme);
                    m_pTaskbarOverlay->SetOverlayStyle(m_config.overlayFontSize,
                                                       m_config.overlayDownloadColor,
                                                       m_config.overlayUploadColor);
                }

                // Apply timer changes
                if (m_config.updateInterval != oldConfig.updateInterval)
                {
                    KillTimer(m_hwnd, TIMER_UPDATE_NETWORK);
                    SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, m_config.updateInterval, nullptr);
                }

                if (m_config.historyAutoTrimDays != oldConfig.historyAutoTrimDays && 
                    m_config.historyAutoTrimDays > 0)
                {
                    HistoryLogger::Instance().TrimToRecentDays(m_config.historyAutoTrimDays);
                }

                if (m_config.language != oldConfig.language)
                {
                    ApplyLanguageFromConfig();
                }

                // Update oldConfig for next comparison
                oldConfig = m_config;

                // Refresh UI
                OnTimer();
            }
        });

        dialogResult = dlg.Show(m_hwnd, m_pConfigManager.get(), m_pNetworkMonitor.get());

        if (dialogResult == IDCANCEL)
        {
            // User cancelled - exit without further processing
            return;
        }

    } while (dialogResult == IDAPPLY_REOPEN);

    // dialogResult == IDOK at this point
    // Settings were already applied via callback, just ensure final state
    LoadConfig();
    SetDebugLoggingEnabled(m_config.debugLogging);
    OnTimer();
}

void Application::ShowDashboardDialog()
{
    DashboardDialog dlg;
    dlg.Show(m_hwnd, m_pNetworkMonitor.get(), &m_config);
}

void Application::ShowHistoryDialog()
{
    HistoryDialog dlg;
    dlg.Show(m_hwnd, &m_config);
}

void Application::ShowAboutDialog()
{
    std::wstring title = LoadStringResource(IDS_ABOUT_TITLE);
    if (title.empty())
    {
        title = L"About NetworkMonitor";
    }

    std::wstring versionLabel = LoadStringResource(IDS_ABOUT_VERSION_LABEL);
    if (versionLabel.empty())
    {
        versionLabel = L"Version: ";
    }

    std::wstring body = LoadStringResource(IDS_ABOUT_BODY);
    if (body.empty())
    {
        body = L"A lightweight network traffic monitor for Windows.\nDisplays real-time download/upload speeds in the system tray and taskbar.";
    }

    std::wstring message = APP_NAME;
    message += L"\n";
    message += versionLabel;
    message += APP_VERSION;
    message += L"\n\n";
    message += body;

    ShowDarkMessageBox(m_hwnd, message, title, MB_OK | MB_ICONINFORMATION, m_config.darkTheme);
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
    switch (menuId)
    {
        case IDM_UPDATE_FAST:
            m_config.updateInterval = UPDATE_INTERVAL_FAST;
            SaveConfig();
            KillTimer(m_hwnd, TIMER_UPDATE_NETWORK);
            SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, m_config.updateInterval, nullptr);
            break;

        case IDM_UPDATE_NORMAL:
            m_config.updateInterval = UPDATE_INTERVAL_NORMAL;
            SaveConfig();
            KillTimer(m_hwnd, TIMER_UPDATE_NETWORK);
            SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, m_config.updateInterval, nullptr);
            break;

        case IDM_UPDATE_SLOW:
            m_config.updateInterval = UPDATE_INTERVAL_SLOW;
            SaveConfig();
            KillTimer(m_hwnd, TIMER_UPDATE_NETWORK);
            SetTimer(m_hwnd, TIMER_UPDATE_NETWORK, m_config.updateInterval, nullptr);
            break;

        case IDM_AUTOSTART:
            m_config.autoStart = !m_config.autoStart;
            SaveConfig();
            break;

        case IDM_SHOW_TASKBAR_OVERLAY:
            if (m_pTaskbarOverlay)
            {
                bool isVisible = m_pTaskbarOverlay->IsVisible();
                m_pTaskbarOverlay->Show(!isVisible);
            }
            break;

        case IDM_SETTINGS:
            ShowSettingsDialog();
            break;

        case IDM_DASHBOARD:
            ShowDashboardDialog();
            break;

        case IDM_ABOUT:
            ShowAboutDialog();
            break;

        case IDM_EXIT:
            DestroyWindow(m_hwnd);
            break;
    }
}

void Application::OnTimer()
{
    if (!m_pNetworkMonitor)
    {
        return;
    }

    // Update network statistics
    m_pNetworkMonitor->Update();

    NetworkStats stats = GetCurrentStatsForConfig();

    if (m_config.enableLogging)
    {
        // History logging: record per-interval usage into SQLite
        LogHistorySample(stats);
    }

    // Update tray icon
    UpdateTrayIcon(stats);

    // Update taskbar overlay
    UpdateTaskbarOverlay(stats);

    // Check connection status for notifications
    CheckConnectionStatus(stats.isActive);
}

NetworkStats Application::GetCurrentStatsForConfig()
{
    NetworkStats stats;
    bool useSpecificInterface = !m_config.selectedInterface.empty();
    if (useSpecificInterface)
    {
        NetworkStats selectedStats;
        if (m_pNetworkMonitor->GetInterfaceStats(m_config.selectedInterface, selectedStats))
        {
            stats = selectedStats;
        }
        else
        {
            stats = m_pNetworkMonitor->GetAggregatedStats();
        }
    }
    else
    {
        stats = m_pNetworkMonitor->GetAggregatedStats();
    }

    return stats;
}

void Application::LogHistorySample(const NetworkStats& stats)
{
    unsigned long long totalDown = static_cast<unsigned long long>(stats.bytesReceived);
    unsigned long long totalUp = static_cast<unsigned long long>(stats.bytesSent);

    if (!m_prevTotalsValid)
    {
        m_prevTotalBytesDown = totalDown;
        m_prevTotalBytesUp = totalUp;
        m_prevTotalsValid = true;
        return;
    }

    unsigned long long deltaDown = 0;
    unsigned long long deltaUp = 0;

    if (totalDown >= m_prevTotalBytesDown)
    {
        deltaDown = totalDown - m_prevTotalBytesDown;
    }
    // else: counters decreased -> treat as reset, don't accumulate

    if (totalUp >= m_prevTotalBytesUp)
    {
        deltaUp = totalUp - m_prevTotalBytesUp;
    }
    // else: counters decreased -> treat as reset

    if (deltaDown > 0 || deltaUp > 0)
    {
        std::wstring ifaceName = stats.interfaceName;
        if (ifaceName.empty())
        {
            ifaceName = LoadStringResource(IDS_ALL_INTERFACES);
            if (ifaceName.empty())
            {
                ifaceName = L"All Interfaces";
            }
        }

        HistoryLogger::Instance().AppendSample(
            ifaceName,
            deltaDown,
            deltaUp
        );
    }

    m_prevTotalBytesDown = totalDown;
    m_prevTotalBytesUp = totalUp;
}

void Application::UpdateTrayIcon(const NetworkStats& stats)
{
    if (m_pTrayIcon)
    {
        m_pTrayIcon->UpdateTooltip(stats, m_config.displayUnit);
        m_pTrayIcon->UpdateIcon(stats.currentDownloadSpeed, stats.currentUploadSpeed);
    }
}

void Application::UpdateTaskbarOverlay(const NetworkStats& stats)
{
    if (m_pTaskbarOverlay && m_pTaskbarOverlay->IsVisible())
    {
        m_pTaskbarOverlay->UpdateSpeed(
            stats.currentDownloadSpeed,
            stats.currentUploadSpeed,
            m_config.displayUnit
        );
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
                OnTimer();
            }
            else if (wParam == TIMER_PING)
            {
                OnPingTimer();
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
            return DefWindowProcW(hwnd, message, wParam, lParam);
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

void Application::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
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

void Application::OnPingTimer()
{
    if (m_pPingMonitor)
    {
        m_pPingMonitor->Update();

        // Update overlay with ping info
        if (m_pTaskbarOverlay && m_pTaskbarOverlay->IsVisible())
        {
            int latency = m_pPingMonitor->GetLatency();
            m_pTaskbarOverlay->SetPingLatency(latency);
        }
    }
}

void Application::CheckConnectionStatus(bool hasActiveInterface)
{
    if (!m_config.enableConnectionNotification)
    {
        m_wasConnected = hasActiveInterface;
        return;
    }

    if (m_wasConnected && !hasActiveInterface)
    {
        // Disconnected
        if (m_pTrayIcon)
        {
            std::wstring title = LoadStringResource(IDS_NOTIFICATION_DISCONNECTED_TITLE);
            std::wstring msg = LoadStringResource(IDS_NOTIFICATION_DISCONNECTED_MSG);
            if (title.empty()) title = L"Network Disconnected";
            if (msg.empty()) msg = L"No active network connection";
            m_pTrayIcon->ShowBalloonNotification(title, msg);
        }
        LogDebug(L"Application::CheckConnectionStatus: Network disconnected");
    }
    else if (!m_wasConnected && hasActiveInterface)
    {
        // Reconnected
        if (m_pTrayIcon)
        {
            std::wstring title = LoadStringResource(IDS_NOTIFICATION_CONNECTED_TITLE);
            std::wstring msg = LoadStringResource(IDS_NOTIFICATION_CONNECTED_MSG);
            if (title.empty()) title = L"Network Connected";
            if (msg.empty()) msg = L"Network connection restored";
            m_pTrayIcon->ShowBalloonNotification(title, msg);
        }
        LogDebug(L"Application::CheckConnectionStatus: Network connected");
    }

    m_wasConnected = hasActiveInterface;
}

} // namespace NetworkMonitor

