#ifndef NETPULSE_CONNECTION_LOG_DIALOG_H
#define NETPULSE_CONNECTION_LOG_DIALOG_H

#include "NetPulse/Common.h"
#include "NetPulse/ConnectionMonitor.h"
#include <windows.h>
#include <vector>

namespace NetPulseTests
{
struct ConnectionLogDialogTestFriend;
}

namespace NetPulse
{

class ConnectionLogDialog
{
public:
    ConnectionLogDialog();
    ~ConnectionLogDialog();

    // Set external storage for dialog handle (for tracking/bringing to foreground)
    void SetDialogHandleStorage(HWND* pDialogHandle) { m_pExternalHandle = pDialogHandle; }

    // Show the connection log dialog modally
    INT_PTR Show(HWND parentWindow, const AppConfig* config, ConnectionMonitor* connectionMonitor);

    friend struct NetPulseTests::ConnectionLogDialogTestFriend;

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    void InitializeDialog(HWND hDlg);
    void InitializeListView();
    void PopulateList();
    void RefreshData();
    void ApplyTheme(HWND hDlg) const;

    // Member variables
    HWND m_hDialog;
    HWND m_hList;
    HWND* m_pExternalHandle;
    const AppConfig* m_pConfig;
    ConnectionMonitor* m_pConnectionMonitor;
    std::vector<NetConnectionInfo> m_connections;
    
    // Timer for auto-refresh
    static const UINT_PTR REFRESH_TIMER_ID = 1001;
    static const UINT REFRESH_INTERVAL_MS = 2000;
};

} // namespace NetPulse

#endif // NETPULSE_CONNECTION_LOG_DIALOG_H
