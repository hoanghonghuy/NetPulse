#ifndef NETWORK_MONITOR_DASHBOARD_DIALOG_H
#define NETWORK_MONITOR_DASHBOARD_DIALOG_H

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/HistoryLogger.h"
#include <vector>

namespace NetworkMonitor
{

class NetworkMonitorClass;
class HistoryLogger;

class DashboardDialog
{
public:
    DashboardDialog();
    ~DashboardDialog();

    // Show the dashboard dialog modally
    bool Show(HWND parentWindow, NetworkMonitorClass* networkMonitor, const AppConfig* config);

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    void UpdateDashboardData(HWND hDlg);
    void DrawDashboardChart(HDC hdc, const RECT& rc);
    void CenterDialogOnScreen(HWND hDlg);

    // Custom draw handler for the list header when dark theme is enabled
    static LRESULT CALLBACK HeaderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Member variables
    HWND m_hDialog;
    NetworkMonitorClass* m_pNetworkMonitor;
    const AppConfig* m_pConfig;
    std::vector<NetworkMonitor::HistorySample> m_chartSamples;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_DASHBOARD_DIALOG_H
