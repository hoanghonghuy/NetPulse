#ifndef NETWORK_MONITOR_HISTORY_DIALOG_H
#define NETWORK_MONITOR_HISTORY_DIALOG_H

#include "NetPulse/Common.h"

namespace NetPulse
{

class HistoryDialog
{
public:
    HistoryDialog();
    ~HistoryDialog();

    // Set external storage for dialog handle (for tracking/bringing to foreground)
    void SetDialogHandleStorage(HWND* pDialogHandle) { m_pExternalHandle = pDialogHandle; }

    // Show the history management dialog modally
    bool Show(HWND parentWindow, const AppConfig* config);

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    static void CenterDialogOnScreen(HWND hDlg);

    // Member variables
    HWND m_hDialog;
    HWND* m_pExternalHandle;  // External storage for dialog handle (for tracking)
    const AppConfig* m_pConfig;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_HISTORY_DIALOG_H
