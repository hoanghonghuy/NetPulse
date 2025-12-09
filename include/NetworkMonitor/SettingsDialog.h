#ifndef NETWORK_MONITOR_SETTINGS_DIALOG_H
#define NETWORK_MONITOR_SETTINGS_DIALOG_H

#include "NetworkMonitor/Common.h"
#include <functional>

namespace NetworkMonitor
{

// Forward declarations - use interfaces for DIP
class IConfigProvider;
class INetworkStatsProvider;

class SettingsDialog
{
public:
    SettingsDialog();
    ~SettingsDialog();

    // Show the settings dialog modally. Returns IDOK, IDCANCEL, or IDAPPLY_REOPEN
    INT_PTR Show(HWND parentWindow, IConfigProvider* configProvider, INetworkStatsProvider* statsProvider);

    // Set callback for when settings are applied
    void SetSettingsChangedCallback(std::function<void()> callback);

    // Set external storage for dialog handle (for tracking/bringing to foreground)
    void SetDialogHandleStorage(HWND* pDialogHandle) { m_pExternalHandle = pDialogHandle; }

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    void PopulateDialog(HWND hDlg);
    bool ApplySettingsFromDialog(HWND hDlg);
    void PopulateInterfaceCombo(HWND hDlg);
    void CenterDialogOnScreen(HWND hDlg);
    void InitializeTabControl(HWND hDlg);
    void SwitchTab(HWND hDlg, int tabIndex);

    // Member variables
    HWND m_hDialog;
    HWND* m_pExternalHandle;  // External storage for dialog handle (for tracking)
    IConfigProvider* m_pConfigProvider;
    INetworkStatsProvider* m_pStatsProvider;
    AppConfig m_configCopy;  // Working copy of config
    std::function<void()> m_settingsChangedCallback;
    bool m_isInitializing;   // Prevent recursive updates during initialization
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_SETTINGS_DIALOG_H
