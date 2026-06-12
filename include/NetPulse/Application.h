#ifndef NETWORK_MONITOR_APPLICATION_H
#define NETWORK_MONITOR_APPLICATION_H

#include "NetPulse/Common.h"
#include "NetPulse/ConfigManager.h"
#include "NetPulse/NetworkMonitor.h"
#include "NetPulse/TrayIcon.h"
#include "NetPulse/TaskbarOverlay.h"
#include "NetPulse/PingMonitor.h"
#include "NetPulse/HotkeyManager.h"
#include "NetPulse/MenuHandler.h"
#include "NetPulse/UpdateCoordinator.h"
#include "NetPulse/DialogManager.h"
#include "NetPulse/LanguageManager.h"
#include "NetPulse/FloatingWindow.h"
#include "NetPulse/SystemMonitor.h"
#include "NetPulse/VpnProxyDetector.h"
#include "NetPulse/ConnectionMonitor.h"
#include <windows.h>
#include <memory>

namespace NetPulseTests
{
struct ApplicationTestFriend;
}

namespace NetPulse
{

class UpdateChecker;

class Application
{
public:
    Application();
    ~Application();

    // Application lifecycle
    bool Initialize(HINSTANCE hInstance);
    int Run() const;
    void Cleanup();

    // Component access
    HWND GetMainWindow() const { return m_hwnd; }
    HINSTANCE GetInstance() const { return m_hInstance; }
    ConfigManager* GetConfigManager() { return m_pConfigManager.get(); }
    NetworkMonitorClass* GetNetworkMonitor() { return m_pNetworkMonitor.get(); }
    TrayIcon* GetTrayIcon() { return m_pTrayIcon.get(); }
    TaskbarOverlay* GetTaskbarOverlay() { return m_pTaskbarOverlay.get(); }
    PingMonitor* GetPingMonitor() { return m_pPingMonitor.get(); }
    const AppConfig& GetConfig() const { return m_config; }

    // Configuration operations
    bool LoadConfig();
    bool SaveConfig();
    void ApplyLanguageFromConfig();

    // UI operations (delegated to DialogManager)
    void ShowSettingsDialog();
    void ShowDashboardDialog();
    void ShowHistoryDialog();
    void ShowAboutDialog();
    void ShowPerAppDialog();
    void ShowSpeedTestDialog();
    void ShowConnectionLogDialog();
    void CheckForUpdates();

    void OnTaskbarOverlayRightClick();

    // Menu command handling (delegated to MenuHandler)
    void OnMenuCommand(UINT menuId);

    // Hotkey handling
    void OnHotkey(int hotkeyId);

    FloatingWindow* GetFloatingWindow() { return m_pFloatingWindow.get(); }
    DialogManager* GetDialogManager() { return m_pDialogManager.get(); }

private:
    friend struct NetPulseTests::ApplicationTestFriend;
    // Window procedure (static for Windows API)
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK InstanceWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    // Helper methods
    bool RegisterWindowClass();
    bool CreateMainWindow();
    void SetupHotkeys();

    // Component instances (using smart pointers for automatic cleanup)
    std::unique_ptr<ConfigManager> m_pConfigManager;
    std::unique_ptr<NetworkMonitorClass> m_pNetworkMonitor;
    std::unique_ptr<TrayIcon> m_pTrayIcon;
    std::unique_ptr<TaskbarOverlay> m_pTaskbarOverlay;
    std::unique_ptr<PingMonitor> m_pPingMonitor;
    std::unique_ptr<HotkeyManager> m_pHotkeyManager;
    std::unique_ptr<MenuHandler> m_pMenuHandler;
    std::unique_ptr<UpdateCoordinator> m_pUpdateCoordinator;
    std::unique_ptr<DialogManager> m_pDialogManager;
    std::unique_ptr<LanguageManager> m_pLanguageManager;
    std::unique_ptr<FloatingWindow> m_pFloatingWindow;
    std::unique_ptr<SystemMonitor> m_pSystemMonitor;
    std::unique_ptr<VpnProxyDetector> m_pVpnDetector;  // Phase 3: VPN/Proxy detection
    std::unique_ptr<ConnectionMonitor> m_pConnectionMonitor;  // Phase 4: Connection watchdog
    std::unique_ptr<UpdateChecker> m_pUpdateChecker;

    // Application state
    AppConfig m_config;
    HWND m_hwnd;
    HINSTANCE m_hInstance;

    // Initialization state
    bool m_initialized;
    
    // TaskbarCreated message ID (registered at runtime)
    static UINT s_taskbarCreatedMsg;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_APPLICATION_H

