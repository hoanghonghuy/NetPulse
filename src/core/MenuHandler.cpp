#include "NetworkMonitor/MenuHandler.h"
#include "NetworkMonitor/TaskbarOverlay.h"
#include "../../resources/resource.h"

namespace NetworkMonitor
{

MenuHandler::MenuHandler()
    : m_pConfig(nullptr)
    , m_pConfigProvider(nullptr)
    , m_pOverlay(nullptr)
{
}

MenuHandler::~MenuHandler()
{
}

void MenuHandler::Initialize(
    AppConfig* config,
    IConfigProvider* configProvider,
    TaskbarOverlay* overlay)
{
    m_pConfig = config;
    m_pConfigProvider = configProvider;
    m_pOverlay = overlay;
}

void MenuHandler::SetSaveConfigCallback(SaveConfigCallback callback)
{
    m_saveConfigCallback = callback;
}

void MenuHandler::SetShowSettingsCallback(ShowDialogCallback callback)
{
    m_showSettingsCallback = callback;
}

void MenuHandler::SetShowDashboardCallback(ShowDialogCallback callback)
{
    m_showDashboardCallback = callback;
}

void MenuHandler::SetShowAboutCallback(ShowDialogCallback callback)
{
    m_showAboutCallback = callback;
}

void MenuHandler::SetExitCallback(ExitCallback callback)
{
    m_exitCallback = callback;
}

void MenuHandler::SetUpdateTimerCallback(UpdateTimerCallback callback)
{
    m_updateTimerCallback = callback;
}

void MenuHandler::SetToggleFloatingWindowCallback(ToggleFloatingWindowCallback callback)
{
    m_toggleFloatingWindowCallback = callback;
}

void MenuHandler::SetShowPerAppCallback(ShowDialogCallback callback)
{
    m_showPerAppCallback = callback;
}

void MenuHandler::HandleCommand(UINT menuId)
{
    if (!m_pConfig)
    {
        return;
    }

    switch (menuId)
    {
        case IDM_UPDATE_FAST:
            m_pConfig->updateInterval = UPDATE_INTERVAL_FAST;
            if (m_saveConfigCallback) m_saveConfigCallback();
            if (m_updateTimerCallback) m_updateTimerCallback(m_pConfig->updateInterval);
            break;

        case IDM_UPDATE_NORMAL:
            m_pConfig->updateInterval = UPDATE_INTERVAL_NORMAL;
            if (m_saveConfigCallback) m_saveConfigCallback();
            if (m_updateTimerCallback) m_updateTimerCallback(m_pConfig->updateInterval);
            break;

        case IDM_UPDATE_SLOW:
            m_pConfig->updateInterval = UPDATE_INTERVAL_SLOW;
            if (m_saveConfigCallback) m_saveConfigCallback();
            if (m_updateTimerCallback) m_updateTimerCallback(m_pConfig->updateInterval);
            break;

        case IDM_AUTOSTART:
            m_pConfig->autoStart = !m_pConfig->autoStart;
            if (m_saveConfigCallback) m_saveConfigCallback();
            break;

        case IDM_SHOW_TASKBAR_OVERLAY:
            if (m_pOverlay)
            {
                bool isVisible = m_pOverlay->IsVisible();
                m_pOverlay->Show(!isVisible);
            }
            break;

        case IDM_SHOW_FLOATING_WINDOW:
            if (m_toggleFloatingWindowCallback) m_toggleFloatingWindowCallback();
            break;

        case IDM_SETTINGS:
            if (m_showSettingsCallback) m_showSettingsCallback();
            break;

        case IDM_DASHBOARD:
            if (m_showDashboardCallback) m_showDashboardCallback();
            break;

        case IDM_PERAPP:
            if (m_showPerAppCallback) m_showPerAppCallback();
            break;

        case IDM_ABOUT:
            if (m_showAboutCallback) m_showAboutCallback();
            break;

        case IDM_EXIT:
            if (m_exitCallback) m_exitCallback();
            break;
    }
}

} // namespace NetworkMonitor
