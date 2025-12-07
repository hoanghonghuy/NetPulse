#ifndef NETWORK_MONITOR_DIALOGMANAGER_H
#define NETWORK_MONITOR_DIALOGMANAGER_H

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/Interfaces/IConfigProvider.h"
#include "NetworkMonitor/Interfaces/INetworkStatsProvider.h"
#include <functional>

namespace NetworkMonitor
{

class UpdateCoordinator;
class NetworkMonitorClass;

/**
 * DialogManager - Manages dialog display and coordination
 * 
 * This class is responsible for showing various dialogs (Settings, Dashboard,
 * History, About) and handling their interactions with the application,
 * following the Single Responsibility Principle.
 */
class DialogManager
{
public:
    // Callbacks for application-level actions
    using ConfigReloadCallback = std::function<bool()>;
    using LanguageApplyCallback = std::function<void()>;
    using TimerUpdateCallback = std::function<void(UINT intervalMs)>;

    DialogManager();
    ~DialogManager();

    /**
     * Initialize the dialog manager with required dependencies
     */
    void Initialize(
        HWND parentWindow,
        AppConfig* config,
        IConfigProvider* configProvider,
        NetworkMonitorClass* networkMonitor,
        UpdateCoordinator* updateCoordinator
    );

    /**
     * Set callbacks for various actions
     */
    void SetConfigReloadCallback(ConfigReloadCallback callback);
    void SetLanguageApplyCallback(LanguageApplyCallback callback);
    void SetTimerUpdateCallback(TimerUpdateCallback callback);

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
     * Show About dialog
     */
    void ShowAbout();

private:
    HWND m_parentWindow;
    AppConfig* m_pConfig;
    IConfigProvider* m_pConfigProvider;
    NetworkMonitorClass* m_pNetworkMonitor;
    UpdateCoordinator* m_pUpdateCoordinator;

    // Callbacks
    ConfigReloadCallback m_configReloadCallback;
    LanguageApplyCallback m_languageApplyCallback;
    TimerUpdateCallback m_timerUpdateCallback;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_DIALOGMANAGER_H
