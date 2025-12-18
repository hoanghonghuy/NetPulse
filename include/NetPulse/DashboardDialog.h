#ifndef NETWORK_MONITOR_DASHBOARD_DIALOG_H
#define NETWORK_MONITOR_DASHBOARD_DIALOG_H

#include "NetPulse/Common.h"
#include "NetPulse/HistoryLogger.h"
#include "NetPulse/ChartRenderer.h"
#include <vector>
#include <memory>

namespace NetPulse
{

class DashboardDialog
{
public:
    // Chart view modes
    enum class ChartViewMode
    {
        DailyThisMonth,    // Show daily usage for current/selected month
        MonthlyThisYear    // Show monthly usage for current year
    };

    DashboardDialog();
    ~DashboardDialog();

    // Set external storage for dialog handle (for tracking/bringing to foreground)
    void SetDialogHandleStorage(HWND* pDialogHandle) { m_pExternalHandle = pDialogHandle; }

    // Show the dashboard dialog modally
    bool Show(HWND parentWindow, const AppConfig* config);

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Dialog helper methods
    void UpdateDashboardData(HWND hDlg);
    void DrawDashboardChart(HDC hdc, const RECT& rc);
    void CenterDialogOnScreen(HWND hDlg);
    void UpdateChartTitle(HWND hDlg);
    void CreateChartControls(HWND hDlg);

    // Member variables
    HWND m_hDialog;
    HWND* m_pExternalHandle;  // External storage for dialog handle (for tracking)
    // Network monitor dependency removed (unused)
    const AppConfig* m_pConfig;
    std::vector<NetPulse::HistorySample> m_chartSamples;

    // Chart navigation state
    ChartViewMode m_chartViewMode;
    int m_chartYear;
    int m_chartMonth;

    // Chart interaction state
    HWND m_hChartTooltip;                           // Tooltip control
    int m_hoveredBarIndex;                          // Currently hovered bar (-1 = none)
    std::vector<ChartDataPoint> m_currentChartData; // Current chart data for hit testing

    // Chart subclass
    static LRESULT CALLBACK ChartSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    void OnChartMouseMove(HWND hChart, int x, int y);
    void OnChartLButtonDown(HWND hChart, int x, int y);
    void UpdateTooltip(HWND hChart, int barIndex);
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_DASHBOARD_DIALOG_H

