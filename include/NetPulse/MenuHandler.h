#ifndef NETWORK_MONITOR_MENUHANDLER_H
#define NETWORK_MONITOR_MENUHANDLER_H

#include "NetPulse/Common.h"
#include "NetPulse/Interfaces/IConfigProvider.h"
#include <functional>

namespace NetPulse
{

class TaskbarOverlay;

/**
 * MenuHandler - Handles menu command processing
 * 
 * This class is responsible for processing menu commands from the tray icon
 * context menu, following the Single Responsibility Principle.
 */
class MenuHandler
{
public:
    // Callback types for application-level actions
    using SaveConfigCallback = std::function<void()>;
    using ShowDialogCallback = std::function<void()>;
    using ExitCallback = std::function<void()>;
    using UpdateTimerCallback = std::function<void(UINT intervalMs)>;
    using ToggleFloatingWindowCallback = std::function<void()>;

    MenuHandler();
    ~MenuHandler();

    /**
     * Initialize the menu handler with required dependencies
     */
    void Initialize(
        AppConfig* config,
        IConfigProvider* configProvider,
        TaskbarOverlay* overlay
    );

    /**
     * Set callbacks for various menu actions
     */
    void SetSaveConfigCallback(SaveConfigCallback callback);
    void SetShowSettingsCallback(ShowDialogCallback callback);
    void SetShowDashboardCallback(ShowDialogCallback callback);
    void SetShowAboutCallback(ShowDialogCallback callback);
    void SetShowSpeedTestCallback(ShowDialogCallback callback);
    void SetExitCallback(ExitCallback callback);
    void SetUpdateTimerCallback(UpdateTimerCallback callback);
    void SetToggleFloatingWindowCallback(ToggleFloatingWindowCallback callback);
    void SetShowPerAppCallback(ShowDialogCallback callback);
    void SetShowConnectionLogCallback(ShowDialogCallback callback);

    /**
     * Handle a menu command
     * @param menuId The menu command ID
     */
    void HandleCommand(UINT menuId);

private:
    AppConfig* m_pConfig;
    IConfigProvider* m_pConfigProvider;
    TaskbarOverlay* m_pOverlay;

    // Callbacks
    SaveConfigCallback m_saveConfigCallback;
    ShowDialogCallback m_showSettingsCallback;
    ShowDialogCallback m_showDashboardCallback;
    ShowDialogCallback m_showAboutCallback;
    ShowDialogCallback m_showSpeedTestCallback;
    ExitCallback m_exitCallback;
    UpdateTimerCallback m_updateTimerCallback;
    ToggleFloatingWindowCallback m_toggleFloatingWindowCallback;
    ShowDialogCallback m_showPerAppCallback;
    ShowDialogCallback m_showConnectionLogCallback;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_MENUHANDLER_H
