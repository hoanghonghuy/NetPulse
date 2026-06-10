#ifndef NETWORK_MONITOR_SETTINGS_DIALOG_H
#define NETWORK_MONITOR_SETTINGS_DIALOG_H

#include "NetPulse/Common.h"
#include <functional>
#include <unordered_map>

namespace NetPulseTests
{
struct SettingsDialogTestFriend;
}

namespace NetPulse
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

    friend struct NetPulseTests::SettingsDialogTestFriend;

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    void PopulateDialog(HWND hDlg);
    bool ApplySettingsFromDialog(HWND hDlg);
    void PopulateInterfaceCombo(HWND hDlg);
    static void CenterDialogOnScreen(HWND hDlg);
    static void InitializeTabControl(HWND hDlg);
    static void SwitchTab(HWND hDlg, int tabIndex);
    
    // Checkbox state helpers for dark theme (BS_OWNERDRAW doesn't store state)
    bool GetCheckboxState(UINT ctrlId) const;
    void SetCheckboxState(UINT ctrlId, bool checked);
    void ToggleCheckboxState(UINT ctrlId);

    // Member variables
    HWND m_hDialog;
    HWND* m_pExternalHandle;  // External storage for dialog handle (for tracking)
    IConfigProvider* m_pConfigProvider;
    INetworkStatsProvider* m_pStatsProvider;
    AppConfig m_configCopy;  // Working copy of config
    std::function<void()> m_settingsChangedCallback;
    bool m_isInitializing;   // Prevent recursive updates during initialization
    std::unordered_map<UINT, bool> m_checkboxStates;  // Dark theme checkbox states
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_SETTINGS_DIALOG_H
