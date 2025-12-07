#include "NetworkMonitor/DialogManager.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/SettingsDialog.h"
#include "NetworkMonitor/DashboardDialog.h"
#include "NetworkMonitor/HistoryDialog.h"
#include "NetworkMonitor/HistoryLogger.h"
#include "NetworkMonitor/ThemeHelper.h"
#include "NetworkMonitor/UpdateCoordinator.h"
#include "NetworkMonitor/Utils.h"
#include "../../resources/resource.h"

namespace NetworkMonitor
{

DialogManager::DialogManager()
    : m_parentWindow(nullptr)
    , m_pConfig(nullptr)
    , m_pConfigProvider(nullptr)
    , m_pNetworkMonitor(nullptr)
    , m_pUpdateCoordinator(nullptr)
{
}

DialogManager::~DialogManager()
{
}

void DialogManager::Initialize(
    HWND parentWindow,
    AppConfig* config,
    IConfigProvider* configProvider,
    NetworkMonitorClass* networkMonitor,
    UpdateCoordinator* updateCoordinator)
{
    m_parentWindow = parentWindow;
    m_pConfig = config;
    m_pConfigProvider = configProvider;
    m_pNetworkMonitor = networkMonitor;
    m_pUpdateCoordinator = updateCoordinator;
}

void DialogManager::SetConfigReloadCallback(ConfigReloadCallback callback)
{
    m_configReloadCallback = callback;
}

void DialogManager::SetLanguageApplyCallback(LanguageApplyCallback callback)
{
    m_languageApplyCallback = callback;
}

void DialogManager::SetTimerUpdateCallback(TimerUpdateCallback callback)
{
    m_timerUpdateCallback = callback;
}

void DialogManager::ShowSettings()
{
    if (!m_pConfigProvider || !m_pConfig)
    {
        return;
    }

    // Keep a copy of current config to detect changes
    AppConfig oldConfig = *m_pConfig;

    // Loop to handle dialog reopen (for theme/language changes)
    INT_PTR dialogResult;
    do
    {
        SettingsDialog dlg;

        // Set up callback for Apply button
        dlg.SetSettingsChangedCallback([this, &oldConfig]()
        {
            // Reload config
            if (m_configReloadCallback && m_configReloadCallback())
            {
                SetDebugLoggingEnabled(m_pConfig->debugLogging);
                ThemeHelper::AllowDarkModeForApp(ThemeHelper::IsSystemInDarkMode());

                // Apply timer changes
                if (m_pConfig->updateInterval != oldConfig.updateInterval)
                {
                    if (m_timerUpdateCallback)
                    {
                        m_timerUpdateCallback(m_pConfig->updateInterval);
                    }
                }

                if (m_pConfig->historyAutoTrimDays != oldConfig.historyAutoTrimDays && 
                    m_pConfig->historyAutoTrimDays > 0)
                {
                    HistoryLogger::Instance().TrimToRecentDays(m_pConfig->historyAutoTrimDays);
                }

                if (m_pConfig->language != oldConfig.language)
                {
                    if (m_languageApplyCallback)
                    {
                        m_languageApplyCallback();
                    }
                }

                // Update oldConfig for next comparison
                oldConfig = *m_pConfig;

                // Refresh UI
                if (m_pUpdateCoordinator)
                {
                    m_pUpdateCoordinator->OnNetworkUpdateTick();
                }
            }
        });

        dialogResult = dlg.Show(m_parentWindow, m_pConfigProvider, m_pNetworkMonitor);

        if (dialogResult == IDCANCEL)
        {
            return;
        }

    } while (dialogResult == IDAPPLY_REOPEN);

    // dialogResult == IDOK - final state
    if (m_configReloadCallback)
    {
        m_configReloadCallback();
    }
    SetDebugLoggingEnabled(m_pConfig->debugLogging);
    if (m_pUpdateCoordinator)
    {
        m_pUpdateCoordinator->OnNetworkUpdateTick();
    }
}

void DialogManager::ShowDashboard()
{
    if (!m_pNetworkMonitor || !m_pConfig)
    {
        return;
    }

    DashboardDialog dlg;
    dlg.Show(m_parentWindow, m_pNetworkMonitor, m_pConfig);
}

void DialogManager::ShowHistory()
{
    if (!m_pConfig)
    {
        return;
    }

    HistoryDialog dlg;
    dlg.Show(m_parentWindow, m_pConfig);
}

void DialogManager::ShowAbout()
{
    if (!m_pConfig)
    {
        return;
    }

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

    ShowDarkMessageBox(m_parentWindow, message, title, MB_OK | MB_ICONINFORMATION, m_pConfig->darkTheme);
}

} // namespace NetworkMonitor
