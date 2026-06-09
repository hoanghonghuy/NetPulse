#include "NetPulse/Common.h"
#include "NetPulse/DialogManager.h"
#include "NetPulse/NetworkMonitor.h"
#include "NetPulse/SettingsDialog.h"
#include "NetPulse/DashboardDialog.h"
#include "NetPulse/HistoryDialog.h"
#include "NetPulse/PerAppDialog.h"
#include "NetPulse/SpeedTestDialog.h"
#include "NetPulse/ConnectionLogDialog.h"
#include "NetPulse/HistoryLogger.h"
#include "NetPulse/ThemeHelper.h"
#include "NetPulse/UpdateCoordinator.h"
#include "NetPulse/Utils.h"
#include "../../resources/resource.h"
#include "NetPulse/DialogThemeHelper.h"
#include <shellapi.h>

namespace NetPulse
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
    , m_hSpeedTestDialog(nullptr)
    , m_pConnectionMonitor(nullptr)
    , m_hConnectionLogDialog(nullptr)
{
}

DialogManager::~DialogManager()
{
}

void DialogManager::Initialize(
    HWND parentWindow,
    AppConfig* config,
    IConfigProvider* configProvider,
    INetworkStatsProvider* networkMonitor,
    UpdateCoordinator* updateCoordinator,
    ConnectionMonitor* connectionMonitor)
{
    m_parentWindow = parentWindow;
    m_pConfig = config;
    m_pConfigProvider = configProvider;
    m_pNetworkMonitor = networkMonitor;
    m_pUpdateCoordinator = updateCoordinator;
    m_pConnectionMonitor = connectionMonitor;
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

void DialogManager::SetApplyAllSettingsCallback(ApplyAllSettingsCallback callback)
{
    m_applyAllSettingsCallback = callback;
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

        dlg.SetSettingsChangedCallback([this, &oldConfig]()
        {
            HandleSettingsApplied(oldConfig);
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
    
    // Apply overlay style changes (same as Apply button path)
    if (m_pUpdateCoordinator)
    {
        m_pUpdateCoordinator->ApplyOverlayStyleFromConfig();
    }
    
    // Apply all other settings (FloatingWindow, Hotkey, Ping, TrayIcon)
    if (m_applyAllSettingsCallback)
    {
        m_applyAllSettingsCallback();
    }
    
    if (m_pUpdateCoordinator)
    {
        m_pUpdateCoordinator->OnNetworkUpdateTick();
    }

    m_hSettingsDialog = nullptr;
}

void DialogManager::HandleSettingsApplied(AppConfig& oldConfig)
{
    if (!m_configReloadCallback || !m_configReloadCallback())
    {
        return;
    }

    SetDebugLoggingEnabled(m_pConfig->debugLogging);
    ThemeHelper::AllowDarkModeForApp(m_pConfig->darkTheme);

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

    oldConfig = *m_pConfig;

    if (m_pUpdateCoordinator)
    {
        m_pUpdateCoordinator->ApplyOverlayStyleFromConfig();
    }

    if (m_applyAllSettingsCallback)
    {
        m_applyAllSettingsCallback();
    }

    bool useDarkTitleBar = IsDarkThemeEnabled(*m_pConfig);
    bool useCustomTheme = IsCustomThemeEnabled(*m_pConfig);

    if (m_hDashboardDialog && IsWindow(m_hDashboardDialog))
    {
        ThemeHelper::ApplyDarkTitleBar(m_hDashboardDialog, useDarkTitleBar);
        DialogThemeHelper::ApplyToDialog(m_hDashboardDialog, useCustomTheme);
        InvalidateRect(m_hDashboardDialog, nullptr, TRUE);
        SendMessage(m_hDashboardDialog, WM_UPDATE_STATS, 0, 0);
    }
    if (m_hHistoryDialog && IsWindow(m_hHistoryDialog))
    {
        ThemeHelper::ApplyDarkTitleBar(m_hHistoryDialog, useDarkTitleBar);
        DialogThemeHelper::ApplyToDialog(m_hHistoryDialog, useCustomTheme);
        InvalidateRect(m_hHistoryDialog, nullptr, TRUE);
    }
    if (m_hConnectionLogDialog && IsWindow(m_hConnectionLogDialog))
    {
        ThemeHelper::ApplyDarkTitleBar(m_hConnectionLogDialog, useDarkTitleBar);
        DialogThemeHelper::ApplyToDialog(m_hConnectionLogDialog, useCustomTheme);
        InvalidateRect(m_hConnectionLogDialog, nullptr, TRUE);
    }

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

    // If dialog is already open, bring it to foreground
    if (m_hDashboardDialog && IsWindow(m_hDashboardDialog))
    {
        BringDialogToForeground(m_hDashboardDialog);
        return;
    }

    DashboardDialog dlg;
    dlg.SetDialogHandleStorage(&m_hDashboardDialog);
    dlg.Show(m_parentWindow, m_pConfig);

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

void DialogManager::ShowSpeedTest()
{
    if (!m_pConfig)
    {
        return;
    }

    // If dialog is already open, bring it to foreground
    if (m_hSpeedTestDialog && IsWindow(m_hSpeedTestDialog))
    {
        BringDialogToForeground(m_hSpeedTestDialog);
        return;
    }

    SpeedTestDialog dlg;
    dlg.SetDialogHandleStorage(&m_hSpeedTestDialog);
    dlg.Show(m_parentWindow, m_pConfig);

    m_hSpeedTestDialog = nullptr;
}

// Helper for About Dialog
static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static AppConfig* s_pConfig = nullptr;
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Store config pointer passed via lParam
            s_pConfig = reinterpret_cast<AppConfig*>(lParam);
            
            // Center the dialog relative to parent (or desktop if no parent/hidden)
            HWND hParent = GetParent(hDlg);
            RECT rcOwner, rcDlg, rc;
            bool useParent = false;
            
            if (hParent && IsWindowVisible(hParent))
            {
                GetWindowRect(hParent, &rcOwner);
                useParent = true;
            }

            if (!useParent)
            {
                rcOwner.left = 0; rcOwner.top = 0;
                rcOwner.right = GetSystemMetrics(SM_CXSCREEN);
                rcOwner.bottom = GetSystemMetrics(SM_CYSCREEN);
            }
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);

            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

            // Set version text dynamically from Common.h
            std::wstring versionText = std::wstring(L"Version ") + APP_VERSION;
            SetDlgItemTextW(hDlg, IDC_VERSION_TEXT, versionText.c_str());

            SetWindowPos(hDlg,
                HWND_TOP,
                rcOwner.left + (rc.right / 2),
                rcOwner.top + (rc.bottom / 2),
                0, 0,          // Ignores size arguments.
                SWP_NOSIZE);

            // Apply theme LAST, matching SpeedTestDialog logic
            if (s_pConfig && IsCustomThemeEnabled(*s_pConfig))
            {
                bool useDarkTitleBar = IsDarkThemeEnabled(*s_pConfig);
                ThemeHelper::ApplyDarkTitleBar(hDlg, useDarkTitleBar);
                
                // Make OK button owner-draw for dark theme
                HWND hButton = GetDlgItem(hDlg, IDOK);
                if (hButton)
                {
                    LONG_PTR style = GetWindowLongPtrW(hButton, GWL_STYLE);
                    
                    // Always re-apply to ensure it sticks
                    style &= ~BS_TYPEMASK;
                    style |= BS_OWNERDRAW;
                    
                    SetWindowLongPtrW(hButton, GWL_STYLE, style);
                    SetWindowTheme(hButton, L"", L""); // Strip theme
                    
                    // Force frame update for the button
                    SetWindowPos(hButton, nullptr, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
            }
                
            return (INT_PTR)TRUE;
        }

    case WM_DRAWITEM:
        if (s_pConfig && IsCustomThemeEnabled(*s_pConfig))
        {
            DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pDrawItem->CtlType == ODT_BUTTON && pDrawItem->CtlID == IDOK)
            {
                DialogThemeHelper::DrawButton(pDrawItem, true);
                return (INT_PTR)TRUE;
            }
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case NM_CLICK:
        case NM_RETURN:
            {
                PNMLINK pNMLink = (PNMLINK)lParam;
                LITEM item = pNMLink->item;
                ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
                return (INT_PTR)TRUE;
            }
        }
        break;
        
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        if (s_pConfig && IsCustomThemeEnabled(*s_pConfig))
        {
            HDC hdc = (HDC)wParam;
            const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());
            SetTextColor(hdc, colors.dialogText);
            SetBkColor(hdc, colors.dialogBackground);
            return (INT_PTR)DialogThemeHelper::GetDarkBackgroundBrush();
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void DialogManager::ShowAbout()
{
    if (!m_pConfig)
    {
        return;
    }

    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT_DIALOG), m_parentWindow, AboutDialogProc, reinterpret_cast<LPARAM>(m_pConfig));
}

void DialogManager::ShowConnectionLog()
{
    if (!m_pConnectionMonitor || !m_pConfig)
    {
        return;
    }

    // If dialog is already open, bring it to foreground
    if (m_hConnectionLogDialog && IsWindow(m_hConnectionLogDialog))
    {
        BringDialogToForeground(m_hConnectionLogDialog);
        return;
    }

    ConnectionLogDialog dlg;
    dlg.SetDialogHandleStorage(&m_hConnectionLogDialog);
    dlg.Show(m_parentWindow, m_pConfig, m_pConnectionMonitor);

    m_hConnectionLogDialog = nullptr;
}

} // namespace NetPulse


