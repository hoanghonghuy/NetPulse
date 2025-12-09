#include "NetworkMonitor/DialogManager.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/SettingsDialog.h"
#include "NetworkMonitor/DashboardDialog.h"
#include "NetworkMonitor/HistoryDialog.h"
#include "NetworkMonitor/PerAppDialog.h"
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
    , m_hSettingsDialog(nullptr)
    , m_hDashboardDialog(nullptr)
    , m_hHistoryDialog(nullptr)
    , m_hPerAppDialog(nullptr)
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

/**
 * Helper function to bring an existing dialog to foreground
 */
static void BringDialogToForeground(HWND hDialog)
{
    if (hDialog && IsWindow(hDialog))
    {
        // Restore if minimized
        if (IsIconic(hDialog))
        {
            ShowWindow(hDialog, SW_RESTORE);
        }
        // Bring to foreground
        SetForegroundWindow(hDialog);
        // Flash the window to get user's attention
        FlashWindow(hDialog, TRUE);
    }
}

void DialogManager::ShowSettings()
{
    if (!m_pConfigProvider || !m_pConfig)
    {
        return;
    }

    // If dialog is already open, bring it to foreground
    if (m_hSettingsDialog && IsWindow(m_hSettingsDialog))
    {
        BringDialogToForeground(m_hSettingsDialog);
        return;
    }

    // Keep a copy of current config to detect changes
    AppConfig oldConfig = *m_pConfig;

    // Loop to handle dialog reopen (for theme/language changes)
    INT_PTR dialogResult;
    do
    {
        SettingsDialog dlg;

        // Set external handle storage for tracking
        dlg.SetDialogHandleStorage(&m_hSettingsDialog);

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
            m_hSettingsDialog = nullptr;
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

    m_hSettingsDialog = nullptr;
}

void DialogManager::ShowDashboard()
{
    if (!m_pNetworkMonitor || !m_pConfig)
    {
        return;
    }

    // If dialog is already open, bring it to foreground
    if (m_hDashboardDialog && IsWindow(m_hDashboardDialog))
    {
        BringDialogToForeground(m_hDashboardDialog);
        return;
    }

    DashboardDialog dlg;
    dlg.SetDialogHandleStorage(&m_hDashboardDialog);
    dlg.Show(m_parentWindow, m_pNetworkMonitor, m_pConfig);

    m_hDashboardDialog = nullptr;
}

void DialogManager::ShowHistory()
{
    if (!m_pConfig)
    {
        return;
    }

    // If dialog is already open, bring it to foreground
    if (m_hHistoryDialog && IsWindow(m_hHistoryDialog))
    {
        BringDialogToForeground(m_hHistoryDialog);
        return;
    }

    HistoryDialog dlg;
    dlg.SetDialogHandleStorage(&m_hHistoryDialog);
    dlg.Show(m_parentWindow, m_pConfig);

    m_hHistoryDialog = nullptr;
}



void DialogManager::ShowPerApp()
{
    if (!m_pConfig)
    {
        return;
    }

    // If dialog is already open, bring it to foreground
    if (m_hPerAppDialog && IsWindow(m_hPerAppDialog))
    {
        BringDialogToForeground(m_hPerAppDialog);
        return;
    }

    PerAppDialog dlg;
    dlg.SetDialogHandleStorage(&m_hPerAppDialog);
    dlg.Show(m_parentWindow, m_pConfig);

    m_hPerAppDialog = nullptr;
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

    // Check if About dialog is already open (using title finding since MessageBox is modal)
    HWND hExisting = FindWindowW(nullptr, title.c_str());
    if (hExisting && IsWindow(hExisting) && IsWindowVisible(hExisting))
    {
        BringDialogToForeground(hExisting);
        return;
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


