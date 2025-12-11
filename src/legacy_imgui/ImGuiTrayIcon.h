// ImGuiTrayIcon.h - System Tray Icon for ImGui Application
// Uses a hidden HWND to receive tray messages
#ifndef NETWORK_MONITOR_IMGUI_TRAY_ICON_H
#define NETWORK_MONITOR_IMGUI_TRAY_ICON_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NetworkMonitor/Common.h"
#include <windows.h>
#include <shellapi.h>
#include <functional>

namespace NetworkMonitor
{

// Menu item IDs (matching original TrayIcon)
enum TrayMenuID : UINT
{
    ID_TRAY_UPDATE_FAST = 1001,
    ID_TRAY_UPDATE_NORMAL,
    ID_TRAY_UPDATE_SLOW,
    ID_TRAY_AUTOSTART,
    ID_TRAY_TASKBAR_OVERLAY,
    ID_TRAY_FLOATING_WINDOW,
    ID_TRAY_SETTINGS,
    ID_TRAY_DASHBOARD,
    ID_TRAY_PERAPP,
    ID_TRAY_ABOUT,
    ID_TRAY_EXIT
};

// Update interval values (matching Common.h)
constexpr UINT UPDATE_FAST = 500;
constexpr UINT UPDATE_NORMAL = 1000;
constexpr UINT UPDATE_SLOW = 2000;

class ImGuiTrayIcon
{
public:
    ImGuiTrayIcon();
    ~ImGuiTrayIcon();

    // Initialize tray icon
    bool Initialize(HINSTANCE hInstance);

    // Cleanup tray icon
    void Shutdown();

    // Update tooltip
    void UpdateTooltip(const wchar_t* tooltip);

    // Show balloon notification
    void ShowBalloonNotification(const std::wstring& title, const std::wstring& message);

    // Set menu action callbacks
    void SetDashboardCallback(std::function<void()> callback) { m_dashboardCallback = callback; }
    void SetSettingsCallback(std::function<void()> callback) { m_settingsCallback = callback; }
    void SetPerAppCallback(std::function<void()> callback) { m_perAppCallback = callback; }
    void SetAboutCallback(std::function<void()> callback) { m_aboutCallback = callback; }
    void SetExitCallback(std::function<void()> callback) { m_exitCallback = callback; }

    // Set toggle callbacks (return new state)
    void SetAutoStartCallback(std::function<void(bool)> callback) { m_autoStartCallback = callback; }
    void SetTaskbarOverlayCallback(std::function<void(bool)> callback) { m_overlayCallback = callback; }
    void SetFloatingWindowCallback(std::function<void(bool)> callback) { m_floatingCallback = callback; }
    void SetUpdateIntervalCallback(std::function<void(UINT)> callback) { m_updateIntervalCallback = callback; }

    // Set state providers for checkable items
    void SetConfigProvider(std::function<const AppConfig*()> provider) { m_configProvider = provider; }
    void SetFloatingVisibleProvider(std::function<bool()> provider) { m_floatingVisibleProvider = provider; }
    void SetOverlayVisibleProvider(std::function<bool()> provider) { m_overlayVisibleProvider = provider; }

    // Check if minimized to tray
    bool IsMinimizedToTray() const { return m_isMinimized; }
    void SetMinimizedToTray(bool minimized) { m_isMinimized = minimized; }

private:
    // Window procedure for hidden message window
    static LRESULT CALLBACK MessageWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Show context menu
    void ShowContextMenu();

    HWND m_hMsgWnd;
    NOTIFYICONDATAW m_nid;
    bool m_initialized;
    bool m_isMinimized;
    HICON m_hIcon;
    HINSTANCE m_hInstance;

    // Action callbacks
    std::function<void()> m_dashboardCallback;
    std::function<void()> m_settingsCallback;
    std::function<void()> m_perAppCallback;
    std::function<void()> m_aboutCallback;
    std::function<void()> m_exitCallback;

    // Toggle callbacks
    std::function<void(bool)> m_autoStartCallback;
    std::function<void(bool)> m_overlayCallback;
    std::function<void(bool)> m_floatingCallback;
    std::function<void(UINT)> m_updateIntervalCallback;

    // State providers
    std::function<const AppConfig*()> m_configProvider;
    std::function<bool()> m_floatingVisibleProvider;
    std::function<bool()> m_overlayVisibleProvider;

    static ImGuiTrayIcon* s_instance;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IMGUI_TRAY_ICON_H
