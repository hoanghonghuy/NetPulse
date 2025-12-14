#ifndef NETWORK_MONITOR_PERAPP_DIALOG_H
#define NETWORK_MONITOR_PERAPP_DIALOG_H

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/PerAppMonitor.h"
#include <windows.h>
#include <commctrl.h>
#include <memory>

namespace NetworkMonitor
{

/**
 * PerAppDialog - Dialog showing per-application network usage
 */
class PerAppDialog
{
public:
    PerAppDialog();
    ~PerAppDialog();

    // Set external storage for dialog handle (for tracking/bringing to foreground)
    void SetDialogHandleStorage(HWND* pDialogHandle) { m_pExternalHandle = pDialogHandle; }

    /**
     * Show the dialog
     * @param parentWindow Parent window handle
     * @param config Application configuration (for theme)
     * @return Dialog result
     */
    INT_PTR Show(HWND parentWindow, const AppConfig* config);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    void InitializeDialog(HWND hDlg);
    void PopulateList(HWND hDlg);
    void RefreshData(HWND hDlg);
    void ApplyTheme(HWND hDlg);
    
    // Header subclass for custom painting


    std::unique_ptr<PerAppMonitor> m_pMonitor;
    HWND m_hList;
    HIMAGELIST m_hImageList;
    const AppConfig* m_pConfig;  // Configuration for theme
    HWND* m_pExternalHandle;     // External storage for dialog handle (for tracking)
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_PERAPP_DIALOG_H
