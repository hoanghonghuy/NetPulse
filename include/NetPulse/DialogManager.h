#ifndef NETWORK_MONITOR_DIALOGMANAGER_H
#define NETWORK_MONITOR_DIALOGMANAGER_H

#include "NetPulse/Common.h"
#include "NetPulse/Interfaces/IConfigProvider.h"
#include "NetPulse/Interfaces/INetworkStatsProvider.h"
#include <functional>

namespace NetPulseTests
{
struct DialogManagerTestFriend;
}

namespace NetPulse
{

class UpdateCoordinator;
class NetworkMonitorClass;
class ConnectionMonitor;

/**
 * DialogManager - Manages dialog display and coordination
 * 
 * This class is responsible for showing various dialogs (Settings, Dashboard,
 * History, About) and handling their interactions with the application,
 * following the Single Responsibility Principle.
 */
class DialogManager
{
    friend struct NetPulseTests::DialogManagerTestFriend;

public:
    // Callbacks for application-level actions
    using ConfigReloadCallback = std::function<bool()>;
    using LanguageApplyCallback = std::function<void()>;
    using TimerUpdateCallback = std::function<void(UINT intervalMs)>;
    using ApplyAllSettingsCallback = std::function<void()>;  // Apply FloatingWindow/Hotkey/Ping/TrayIcon settings

    DialogManager();
    ~DialogManager();

    /**
     * Initialize the dialog manager with required dependencies
     */
    void Initialize(
        HWND parentWindow,
        AppConfig* config,
        IConfigProvider* configProvider,
        INetworkStatsProvider* networkMonitor,
        UpdateCoordinator* updateCoordinator,
        ConnectionMonitor* connectionMonitor = nullptr
    );

    /**
     * Set callbacks for various actions
     */
    void SetConfigReloadCallback(ConfigReloadCallback callback);
    void SetLanguageApplyCallback(LanguageApplyCallback callback);
    void SetTimerUpdateCallback(TimerUpdateCallback callback);
    void SetApplyAllSettingsCallback(ApplyAllSettingsCallback callback);

    /**
     * Show Settings dialog
     */
    void ShowSettings();

    /**
     * Show Dashboard dialog
     */
    void ShowDashboard();

    /**
     * Show History dialog
     */
    void ShowHistory();

    /**
     * Show Per-App dialog
     */
    void ShowPerApp();

    /**
     * Show Speed Test dialog
     */
    void ShowSpeedTest();

    /**
     * Show About dialog
     */
    void ShowAbout();

    /**
     * Show Connection Log dialog
     */
    void ShowConnectionLog();

private:
    void HandleSettingsApplied(AppConfig& oldConfig);

    HWND m_parentWindow;
    AppConfig* m_pConfig;
    IConfigProvider* m_pConfigProvider;
    INetworkStatsProvider* m_pNetworkMonitor;
    UpdateCoordinator* m_pUpdateCoordinator;
    ConnectionMonitor* m_pConnectionMonitor;

    // Callbacks
    ConfigReloadCallback m_configReloadCallback;
    LanguageApplyCallback m_languageApplyCallback;
    TimerUpdateCallback m_timerUpdateCallback;
    ApplyAllSettingsCallback m_applyAllSettingsCallback;

    // Dialog handles for tracking open dialogs (to prevent multiple instances and bring to foreground)
    HWND m_hSettingsDialog;
    HWND m_hDashboardDialog;
    HWND m_hHistoryDialog;
    HWND m_hPerAppDialog;
    HWND m_hSpeedTestDialog;
    HWND m_hConnectionLogDialog;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_DIALOGMANAGER_H
