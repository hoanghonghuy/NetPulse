// ImGuiTrayIcon.cpp - System Tray Icon Implementation
#include "NetworkMonitor/ImGuiTrayIcon.h"
#include "NetworkMonitor/Common.h"
#include <string>

#pragma comment(lib, "shell32.lib")

#define WM_TRAY_CALLBACK (WM_USER + 100)
#define TRAY_MSG_WINDOW_CLASS L"NetworkMonitorTrayMsgWnd"

namespace NetworkMonitor
{

ImGuiTrayIcon* ImGuiTrayIcon::s_instance = nullptr;

ImGuiTrayIcon::ImGuiTrayIcon()
    : m_hMsgWnd(nullptr)
    , m_nid{}
    , m_initialized(false)
    , m_isMinimized(false)
    , m_hIcon(nullptr)
    , m_hInstance(nullptr)
{
    s_instance = this;
}

ImGuiTrayIcon::~ImGuiTrayIcon()
{
    Shutdown();
    s_instance = nullptr;
}

bool ImGuiTrayIcon::Initialize(HINSTANCE hInstance)
{
    if (m_initialized)
        return true;

    m_hInstance = hInstance;

    // Register hidden message window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MessageWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TRAY_MSG_WINDOW_CLASS;
    RegisterClassExW(&wc);

    // Create hidden message window
    m_hMsgWnd = CreateWindowExW(
        0,
        TRAY_MSG_WINDOW_CLASS,
        L"",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, // Message-only window
        nullptr,
        hInstance,
        nullptr
    );

    if (!m_hMsgWnd)
        return false;

    // Load icon from resources or use default
    m_hIcon = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(1), // IDI_APP_ICON
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR
    ));
    if (!m_hIcon)
    {
        m_hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    // Setup tray icon
    m_nid.cbSize = sizeof(m_nid);
    m_nid.hWnd = m_hMsgWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAY_CALLBACK;
    m_nid.hIcon = m_hIcon;
    wcscpy_s(m_nid.szTip, L"Network Monitor");

    if (!Shell_NotifyIconW(NIM_ADD, &m_nid))
    {
        DestroyWindow(m_hMsgWnd);
        m_hMsgWnd = nullptr;
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiTrayIcon::Shutdown()
{
    if (!m_initialized)
        return;

    Shell_NotifyIconW(NIM_DELETE, &m_nid);

    if (m_hMsgWnd)
    {
        DestroyWindow(m_hMsgWnd);
        m_hMsgWnd = nullptr;
    }

    m_initialized = false;
}

void ImGuiTrayIcon::UpdateTooltip(const wchar_t* tooltip)
{
    if (!m_initialized)
        return;

    wcscpy_s(m_nid.szTip, tooltip);
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void ImGuiTrayIcon::ShowBalloonNotification(const std::wstring& title, const std::wstring& message)
{
    if (!m_initialized)
        return;

    m_nid.uFlags = NIF_INFO;
    m_nid.dwInfoFlags = NIIF_INFO;

    wcsncpy_s(m_nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(m_nid.szInfo, message.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &m_nid);

    // Clear the balloon after showing
    m_nid.szInfoTitle[0] = L'\0';
    m_nid.szInfo[0] = L'\0';
}

void ImGuiTrayIcon::ShowContextMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
        return;

    // Get current config state
    const AppConfig* config = nullptr;
    if (m_configProvider)
    {
        config = m_configProvider();
    }

    UINT currentInterval = config ? config->updateInterval : UPDATE_NORMAL;
    bool autoStart = config ? config->autoStart : false;
    bool overlayVisible = m_overlayVisibleProvider ? m_overlayVisibleProvider() : false;
    bool floatingVisible = m_floatingVisibleProvider ? m_floatingVisibleProvider() : false;

    // Create Update Speed submenu
    HMENU hUpdateMenu = CreatePopupMenu();
    if (hUpdateMenu)
    {
        AppendMenuW(hUpdateMenu, MF_STRING | (currentInterval == UPDATE_FAST ? MF_CHECKED : 0),
                    ID_TRAY_UPDATE_FAST, L"Fast (500ms)");
        AppendMenuW(hUpdateMenu, MF_STRING | (currentInterval == UPDATE_NORMAL ? MF_CHECKED : 0),
                    ID_TRAY_UPDATE_NORMAL, L"Normal (1s)");
        AppendMenuW(hUpdateMenu, MF_STRING | (currentInterval == UPDATE_SLOW ? MF_CHECKED : 0),
                    ID_TRAY_UPDATE_SLOW, L"Slow (2s)");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hUpdateMenu, L"Update Speed");
    }

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // Toggle options
    AppendMenuW(hMenu, MF_STRING | (autoStart ? MF_CHECKED : 0),
                ID_TRAY_AUTOSTART, L"Start with Windows");
    AppendMenuW(hMenu, MF_STRING | (overlayVisible ? MF_CHECKED : 0),
                ID_TRAY_TASKBAR_OVERLAY, L"Show Taskbar Overlay");
    AppendMenuW(hMenu, MF_STRING | (floatingVisible ? MF_CHECKED : 0),
                ID_TRAY_FLOATING_WINDOW, L"Show Floating Window");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // Action items
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Settings");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_DASHBOARD, L"Dashboard");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_PERAPP, L"Per-App Usage");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_ABOUT, L"About");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    // Required for the menu to work correctly
    SetForegroundWindow(m_hMsgWnd);

    UINT cmd = TrackPopupMenu(
        hMenu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        pt.x, pt.y,
        0,
        m_hMsgWnd,
        nullptr
    );

    PostMessage(m_hMsgWnd, WM_NULL, 0, 0); // Dismiss menu

    DestroyMenu(hMenu);

    // Handle menu selection
    switch (cmd)
    {
    case ID_TRAY_UPDATE_FAST:
        if (m_updateIntervalCallback)
            m_updateIntervalCallback(UPDATE_FAST);
        break;
    case ID_TRAY_UPDATE_NORMAL:
        if (m_updateIntervalCallback)
            m_updateIntervalCallback(UPDATE_NORMAL);
        break;
    case ID_TRAY_UPDATE_SLOW:
        if (m_updateIntervalCallback)
            m_updateIntervalCallback(UPDATE_SLOW);
        break;
    case ID_TRAY_AUTOSTART:
        if (m_autoStartCallback)
            m_autoStartCallback(!autoStart);
        break;
    case ID_TRAY_TASKBAR_OVERLAY:
        if (m_overlayCallback)
            m_overlayCallback(!overlayVisible);
        break;
    case ID_TRAY_FLOATING_WINDOW:
        if (m_floatingCallback)
            m_floatingCallback(!floatingVisible);
        break;
    case ID_TRAY_SETTINGS:
        if (m_settingsCallback)
            m_settingsCallback();
        break;
    case ID_TRAY_DASHBOARD:
        if (m_dashboardCallback)
            m_dashboardCallback();
        break;
    case ID_TRAY_PERAPP:
        if (m_perAppCallback)
            m_perAppCallback();
        break;
    case ID_TRAY_ABOUT:
        if (m_aboutCallback)
            m_aboutCallback();
        break;
    case ID_TRAY_EXIT:
        if (m_exitCallback)
            m_exitCallback();
        break;
    }
}

LRESULT CALLBACK ImGuiTrayIcon::MessageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_TRAY_CALLBACK && s_instance)
    {
        switch (LOWORD(lParam))
        {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            s_instance->ShowContextMenu();
            return 0;

        case WM_LBUTTONDBLCLK:
            if (s_instance->m_dashboardCallback)
                s_instance->m_dashboardCallback();
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace NetworkMonitor
