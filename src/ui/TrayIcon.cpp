#include "NetPulse/TrayIcon.h"
#include "NetPulse/Utils.h"
#include "NetPulse/ThemeHelper.h"
#include "../../resources/resource.h"

namespace NetPulse
{

TrayIcon::TrayIcon()
    : m_hwnd(nullptr)
    , m_initialized(false)
    , m_iconIdle(nullptr)
    , m_iconActive(nullptr)
    , m_iconHigh(nullptr)
    , m_iconIdleDark(nullptr)
    , m_iconActiveDark(nullptr)
    , m_iconHighDark(nullptr)
    , m_configRef(nullptr)
    , m_overlayVisibleProvider(nullptr)
    , m_floatingVisibleProvider(nullptr)
    , m_animating(false)
    , m_animationPhase(0)
{
    ZeroMemory(&m_notifyIconData, sizeof(NOTIFYICONDATAW));
}

TrayIcon::~TrayIcon()
{
    Cleanup();
}

bool TrayIcon::Initialize(HWND hwnd)
{
    if (m_initialized)
    {
        return true;
    }

    m_hwnd = hwnd;

    // Load icons from application resources
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    // Idle icon (default tray icon)
    m_iconIdle = LoadAppIcon();

    // Active icon
    m_iconActive = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_TRAY_ACTIVE),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    if (!m_iconActive)
    {
        m_iconActive = m_iconIdle;
    }

    // High traffic icon
    m_iconHigh = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_TRAY_HIGH),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    if (!m_iconHigh)
    {
        m_iconHigh = m_iconIdle;
    }

    // Optional dark-theme icons (fallback to normal icons if not present)
    m_iconIdleDark = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_TRAY_IDLE_DARK),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    if (!m_iconIdleDark)
    {
        m_iconIdleDark = m_iconIdle;
    }

    m_iconActiveDark = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_TRAY_ACTIVE_DARK),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    if (!m_iconActiveDark)
    {
        m_iconActiveDark = m_iconActive;
    }

    m_iconHighDark = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_TRAY_HIGH_DARK),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    if (!m_iconHighDark)
    {
        m_iconHighDark = m_iconHigh;
    }

    if (m_iconIdle == nullptr)
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_LOAD_APP_ICON));
        return false;
    }

    // Initialize notify icon data
    m_notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
    m_notifyIconData.hWnd = m_hwnd;
    m_notifyIconData.uID = ID_TRAY_ICON;
    m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_notifyIconData.uCallbackMessage = WM_TRAYICON;

    bool useDark = false;
    if (m_configRef)
    {
        useDark = m_configRef->darkTheme;
    }
    else
    {
        useDark = ThemeHelper::IsSystemInDarkMode();
    }

    m_notifyIconData.hIcon = useDark ? m_iconIdleDark : m_iconIdle;
    wcscpy_s(m_notifyIconData.szTip, APP_NAME);

    // Add tray icon
    if (!Shell_NotifyIconW(NIM_ADD, &m_notifyIconData))
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_CREATE_TRAY_ICON));
        return false;
    }

    // Set version for modern behavior (Windows Vista+)
    m_notifyIconData.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &m_notifyIconData);

    m_initialized = true;
    return true;
}

void TrayIcon::Cleanup()
{
    // Stop animation timer if running
    StopAnimation();
    
    if (m_initialized)
    {
        // Remove tray icon
        Shell_NotifyIconW(NIM_DELETE, &m_notifyIconData);
        m_initialized = false;
    }

    // Cleanup light theme icons
    if (m_iconIdle)
    {
        DestroyIcon(m_iconIdle);
        m_iconIdle = nullptr;
    }
    if (m_iconActive && m_iconActive != m_iconIdle)
    {
        DestroyIcon(m_iconActive);
        m_iconActive = nullptr;
    }
    if (m_iconHigh && m_iconHigh != m_iconIdle)
    {
        DestroyIcon(m_iconHigh);
        m_iconHigh = nullptr;
    }

    // Cleanup dark theme icons (only if they are distinct from light icons)
    if (m_iconIdleDark && m_iconIdleDark != m_iconIdle)
    {
        DestroyIcon(m_iconIdleDark);
        m_iconIdleDark = nullptr;
    }
    if (m_iconActiveDark && m_iconActiveDark != m_iconActive && m_iconActiveDark != m_iconIdle)
    {
        DestroyIcon(m_iconActiveDark);
        m_iconActiveDark = nullptr;
    }
    if (m_iconHighDark && m_iconHighDark != m_iconHigh && m_iconHighDark != m_iconIdle)
    {
        DestroyIcon(m_iconHighDark);
        m_iconHighDark = nullptr;
    }
}

void TrayIcon::UpdateTooltip(const NetworkStats& stats, SpeedUnit unit)
{
    if (!m_initialized)
    {
        return;
    }

    // Format tooltip text with current and peak speeds
    std::wstring downloadStr = FormatSpeed(stats.currentDownloadSpeed, unit);
    std::wstring uploadStr = FormatSpeed(stats.currentUploadSpeed, unit);
    std::wstring peakDownStr = FormatSpeed(stats.peakDownloadSpeed, unit);
    std::wstring peakUpStr = FormatSpeed(stats.peakUploadSpeed, unit);

    std::wstring tooltip = APP_NAME;
    tooltip += L"\n";
    tooltip += L"↓ " + downloadStr + L" (peak: " + peakDownStr + L")";
    tooltip += L"\n";
    tooltip += L"↑ " + uploadStr + L" (peak: " + peakUpStr + L")";

    // Update tooltip
    wcscpy_s(m_notifyIconData.szTip, tooltip.c_str());
    m_notifyIconData.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &m_notifyIconData); // We ignore failure here as UpdateIcon will catch it soon, or we can add RestoreIcon here too
    if (!Shell_NotifyIconW(NIM_MODIFY, &m_notifyIconData))
    {
        RestoreIcon();
    }
}

void TrayIcon::UpdateIcon(double downloadSpeed, double uploadSpeed)
{
    if (!m_initialized)
    {
        return;
    }

    // Get animation settings from config
    bool animationEnabled = true;
    double animationThreshold = 1024.0 * 1024.0;  // Default 1 MB/s
    if (m_configRef)
    {
        animationEnabled = m_configRef->trayAnimationEnabled;
        animationThreshold = m_configRef->trayAnimationThresholdKB * 1024.0;  // Convert KB to bytes
    }
    
    // Determine traffic level
    const double STOP_ANIMATION_THRESHOLD = animationThreshold * 0.5;  // 50% hysteresis
    const double ACTIVE_THRESHOLD = 10.0 * 1024.0;  // 10 KB/s

    double totalSpeed = downloadSpeed + uploadSpeed;

    // Handle animation state (only if enabled)
    if (animationEnabled && totalSpeed > animationThreshold)
    {
        // Start animation when high traffic
        if (!m_animating)
        {
            StartAnimation();
        }
        return; // OnAnimationTick will handle icon updates
    }
    else if (totalSpeed < STOP_ANIMATION_THRESHOLD && m_animating)
    {
        // Stop animation when traffic drops below threshold
        StopAnimation();
    }

    // If animating, let OnAnimationTick handle icons
    if (m_animating)
    {
        return;
    }

    // Normal icon update (not animating)
    bool useDark = false;
    if (m_configRef)
    {
        useDark = m_configRef->darkTheme;
    }
    else
    {
        useDark = ThemeHelper::IsSystemInDarkMode();
    }

    HICON newIcon = useDark ? m_iconIdleDark : m_iconIdle;

    if (totalSpeed > ACTIVE_THRESHOLD)
    {
        newIcon = useDark ? m_iconActiveDark : m_iconActive;
    }

    // Always try to update/modify the icon. 
    // This allows us to detect if the icon has been removed (e.g. explorer restart or sleep/wake issue)
    // and restore it automatically if NIM_MODIFY fails.
    m_notifyIconData.hIcon = newIcon;
    m_notifyIconData.uFlags = NIF_ICON;
    if (!Shell_NotifyIconW(NIM_MODIFY, &m_notifyIconData))
    {
        RestoreIcon();
    }
}

bool TrayIcon::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    if (message != WM_TRAYICON)
    {
        return false;
    }

    // Handle tray icon messages
    switch (LOWORD(lParam))
    {
        case WM_LBUTTONUP:
        {
            // Left click - show main window or do nothing
            // Can be customized based on needs
            return true;
        }

        case WM_LBUTTONDBLCLK:
        {
            // Double-click - open Dashboard
            if (m_doubleClickCallback)
            {
                m_doubleClickCallback();
            }
            return true;
        }

        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
        {
            // Right click - show context menu
            // Note: Only handle WM_RBUTTONUP to avoid showing menu twice
            if (LOWORD(lParam) == WM_RBUTTONUP)
            {
                ShowContextMenu();
            }
            return true;
        }

        case NIN_SELECT:
        case NIN_KEYSELECT:
        {
            // Tray icon selected (for NOTIFYICON_VERSION_4)
            return true;
        }
    }

    return false;
}

void TrayIcon::ShowContextMenu()
{
    if (!m_initialized)
    {
        return;
    }

    const AppConfig* configPtr = m_configRef;
    AppConfig tempConfig;
    if (!configPtr)
    {
        configPtr = &tempConfig;
    }

    // Ensure the process-level dark mode preference matches the APP configuration
    // so that the tray context menu follows the users choice immediately.
    bool useDark = configPtr->darkTheme;
    ThemeHelper::AllowDarkModeForApp(useDark);
    ThemeHelper::AllowDarkModeForWindow(m_hwnd, useDark);

    bool overlayVisible = false;
    if (m_overlayVisibleProvider)
    {
        overlayVisible = m_overlayVisibleProvider();
    }

    bool floatingVisible = false;
    if (m_floatingVisibleProvider)
    {
        floatingVisible = m_floatingVisibleProvider();
    }

    // Get cursor position
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    // Create context menu
    HMENU hMenu = CreateContextMenu(*configPtr, overlayVisible, floatingVisible);
    if (hMenu == nullptr)
    {
        return;
    }

    // Required for proper menu behavior in system tray
    SetForegroundWindow(m_hwnd);

    // Show menu with owner-drawn items for dark theme support
    UINT menuItemId = TrackPopupMenuEx(
        hMenu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        cursorPos.x,
        cursorPos.y,
        m_hwnd,
        nullptr
    );

    // Required to make menu disappear properly
    PostMessage(m_hwnd, WM_NULL, 0, 0);

    // Cleanup
    DestroyMenu(hMenu);

    // Invoke callback if menu item selected
    if (menuItemId != 0 && m_menuCallback)
    {
        m_menuCallback(menuItemId);
    }
}

void TrayIcon::SetMenuCallback(std::function<void(UINT)> callback)
{
    m_menuCallback = callback;
}

void TrayIcon::SetConfigSource(const AppConfig* config)
{
    m_configRef = config;
}

void TrayIcon::SetOverlayVisibilityProvider(std::function<bool()> provider)
{
    m_overlayVisibleProvider = std::move(provider);
}

void TrayIcon::SetFloatingWindowVisibilityProvider(std::function<bool()> provider)
{
    m_floatingVisibleProvider = std::move(provider);
}

HMENU TrayIcon::CreateContextMenu(const AppConfig& config, bool overlayVisible, bool floatingVisible)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
    {
        return nullptr;
    }

    // Helper lambda to add native menu item
    auto AddNativeMenuItem = [](HMENU hMenu, UINT id, const std::wstring& text, bool checked = false, bool separator = false) {
        if (separator)
        {
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        }
        else
        {
            UINT flags = MF_STRING;
            if (checked) flags |= MF_CHECKED;
            AppendMenuW(hMenu, flags, id, text.c_str());
        }
    };

    // Update Speed submenu
    HMENU hUpdateMenu = CreatePopupMenu();
    AddNativeMenuItem(hUpdateMenu, IDM_UPDATE_FAST, LoadStringResource(IDS_MENU_UPDATE_FAST), 
                config.updateInterval == UPDATE_INTERVAL_FAST);
    AddNativeMenuItem(hUpdateMenu, IDM_UPDATE_NORMAL, LoadStringResource(IDS_MENU_UPDATE_NORMAL),
                config.updateInterval == UPDATE_INTERVAL_NORMAL);
    AddNativeMenuItem(hUpdateMenu, IDM_UPDATE_SLOW, LoadStringResource(IDS_MENU_UPDATE_SLOW),
                config.updateInterval == UPDATE_INTERVAL_SLOW);

    // Add submenu to main menu
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hUpdateMenu, LoadStringResource(IDS_MENU_UPDATE_INTERVAL).c_str());
    
    AddNativeMenuItem(hMenu, 0, L"", false, true); // Separator

    // Main menu items
    AddNativeMenuItem(hMenu, IDM_AUTOSTART, LoadStringResource(IDS_MENU_AUTOSTART), config.autoStart);
    AddNativeMenuItem(hMenu, IDM_SHOW_TASKBAR_OVERLAY, LoadStringResource(IDS_MENU_TASKBAR_OVERLAY), overlayVisible);
    AddNativeMenuItem(hMenu, IDM_SHOW_FLOATING_WINDOW, LoadStringResource(IDS_MENU_FLOATING_WINDOW), floatingVisible);
    
    AddNativeMenuItem(hMenu, 0, L"", false, true); // Separator

    AddNativeMenuItem(hMenu, IDM_SETTINGS, LoadStringResource(IDS_MENU_SETTINGS));
    AddNativeMenuItem(hMenu, IDM_DASHBOARD, LoadStringResource(IDS_MENU_DASHBOARD));
    AddNativeMenuItem(hMenu, IDM_PERAPP, LoadStringResource(IDS_MENU_PERAPP));
    AddNativeMenuItem(hMenu, IDM_SPEED_TEST, LoadStringResource(IDS_MENU_SPEED_TEST));
    AddNativeMenuItem(hMenu, IDM_CONNECTION_LOG, LoadStringResource(IDS_MENU_CONNECTION_LOG));
    AddNativeMenuItem(hMenu, IDM_ABOUT, LoadStringResource(IDS_MENU_ABOUT));
    
    AddNativeMenuItem(hMenu, 0, L"", false, true); // Separator

    AddNativeMenuItem(hMenu, IDM_EXIT, LoadStringResource(IDS_MENU_EXIT));

    return hMenu;
}

HICON TrayIcon::LoadAppIcon()
{
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    // Try to load the application's tray idle icon first
    HICON hIcon = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_TRAY_IDLE),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));

    if (!hIcon)
    {
        // Fallback to main app icon
        hIcon = static_cast<HICON>(LoadImageW(
            hInstance,
            MAKEINTRESOURCEW(IDI_APP_ICON),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR));
    }

    if (!hIcon)
    {
        // Final fallback to default system application icon
        hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    return hIcon;
}

void TrayIcon::ShowBalloonNotification(const std::wstring& title, const std::wstring& message)
{
    if (!m_initialized)
    {
        return;
    }

    m_notifyIconData.uFlags = NIF_INFO;
    m_notifyIconData.dwInfoFlags = NIIF_INFO;

    wcsncpy_s(m_notifyIconData.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(m_notifyIconData.szInfo, message.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &m_notifyIconData);

    // Clear the balloon after showing
    m_notifyIconData.szInfoTitle[0] = L'\0';
    m_notifyIconData.szInfo[0] = L'\0';
}

void TrayIcon::SetDoubleClickCallback(std::function<void()> callback)
{
    m_doubleClickCallback = std::move(callback);
}

void TrayIcon::RefreshIcon(bool useDarkTheme)
{
    if (!m_initialized)
    {
        return;
    }
    
    // Select appropriate icon based on theme
    m_notifyIconData.hIcon = useDarkTheme ? m_iconIdleDark : m_iconIdle;
    m_notifyIconData.uFlags = NIF_ICON;
    if (!Shell_NotifyIconW(NIM_MODIFY, &m_notifyIconData))
    {
        RestoreIcon();
    }
}


void TrayIcon::StartAnimation()
{
    if (m_animating || !m_initialized || !m_hwnd)
    {
        return;
    }
    
    m_animating = true;
    m_animationPhase = 0;
    
    // Start 250ms timer for pulsing effect
    SetTimer(m_hwnd, ANIMATION_TIMER_ID, 250, nullptr);
}

void TrayIcon::StopAnimation()
{
    if (!m_animating)
    {
        return;
    }
    
    m_animating = false;
    m_animationPhase = 0;
    
    // Kill animation timer
    if (m_hwnd)
    {
        KillTimer(m_hwnd, ANIMATION_TIMER_ID);
    }
}

void TrayIcon::RestoreIcon()
{
    if (!m_initialized || !m_hwnd)
    {
        return;
    }

    // Save current flags
    UINT oldFlags = m_notifyIconData.uFlags;

    // Set required flags for adding a new icon
    m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    // Note: hIcon is already set to the current intended icon
    // tpTip is already set to the current tooltip
    // uCallbackMessage is set in Initialize

    if (Shell_NotifyIconW(NIM_ADD, &m_notifyIconData))
    {
        // Restore version behavior for modern tooltip/behavior
        m_notifyIconData.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &m_notifyIconData);
    }

    // Restore original flags for future partial updates
    m_notifyIconData.uFlags = oldFlags;
}

void TrayIcon::OnAnimationTick()
{
    if (!m_animating || !m_initialized)
    {
        return;
    }
    
    // Toggle animation phase
    m_animationPhase = (m_animationPhase + 1) % 2;
    
    // Determine theme
    bool useDark = false;
    if (m_configRef)
    {
        useDark = m_configRef->darkTheme;
    }
    else
    {
        useDark = ThemeHelper::IsSystemInDarkMode();
    }
    
    // Alternate between active and high icons for pulse effect
    HICON newIcon;
    if (m_animationPhase == 0)
    {
        newIcon = useDark ? m_iconActiveDark : m_iconActive;
    }
    else
    {
        newIcon = useDark ? m_iconHighDark : m_iconHigh;
    }
    
    m_notifyIconData.hIcon = newIcon;
    m_notifyIconData.uFlags = NIF_ICON;
    if (!Shell_NotifyIconW(NIM_MODIFY, &m_notifyIconData))
    {
        RestoreIcon();
    }
}

} // namespace NetPulse
