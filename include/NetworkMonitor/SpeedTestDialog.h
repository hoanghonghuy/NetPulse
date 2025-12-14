#ifndef NETWORK_MONITOR_SPEED_TEST_DIALOG_H
#define NETWORK_MONITOR_SPEED_TEST_DIALOG_H

#include <Windows.h>
#include <memory>
#include <functional>
#include <string>

namespace NetworkMonitor
{

// Forward declarations
struct AppConfig;
class SpeedTester;
class SpeedTestHistory;
struct SpeedTestResult;

/**
 * @brief Dialog for running bandwidth speed tests
 * 
 * Features:
 * - Start/Cancel speed test
 * - Progress display during test
 * - Results display (Download, Upload, Ping)
 * - History of previous tests
 * - Dark mode support
 */
class SpeedTestDialog
{
public:
    SpeedTestDialog();
    ~SpeedTestDialog();

    /**
     * @brief Show the speed test dialog
     * @param hWndParent Parent window handle
     * @param pConfig Application configuration for theming
     */
    void Show(HWND hWndParent, AppConfig* pConfig);

    /**
     * @brief Check if dialog is currently open
     */
    bool IsOpen() const { return m_hDlg != nullptr; }

    /**
     * @brief Set a pointer to an external HWND variable to store the dialog handle
     * This allows external managers to track the dialog lifecycle
     */
    void SetDialogHandleStorage(HWND* pStorage) { m_pExternalHandle = pStorage; }

private:
    // Dialog procedure
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    // Initialization
    void OnInitDialog(HWND hDlg);
    void ApplyDarkTheme(HWND hDlg);
    void LocalizeControls(HWND hDlg);
    void InitializeHistoryList(HWND hDlg);
    void PopulateHistoryList();

    // Speed test operations
    void StartSpeedTest();
    void CancelSpeedTest();
    void UpdateProgress(int progress, const std::wstring& status);
    void DisplayResult(const SpeedTestResult& result);

    // Message handlers
    void OnCommand(HWND hDlg, WPARAM wParam);
    void OnClose(HWND hDlg);
    bool OnDrawItem(DRAWITEMSTRUCT* pDrawItem);

    // Helpers


private:
    HWND m_hDlg = nullptr;
    AppConfig* m_pConfig = nullptr;
    
    // Speed test components
    std::unique_ptr<SpeedTester> m_speedTester;
    std::unique_ptr<SpeedTestHistory> m_speedTestHistory;
    
    // Control handles
    HWND m_hStartButton = nullptr;
    HWND m_hProgressBar = nullptr;
    HWND m_hStatusLabel = nullptr;
    HWND m_hDownloadValue = nullptr;
    HWND m_hUploadValue = nullptr;
    HWND m_hPingValue = nullptr;
    HWND m_hHistoryList = nullptr;
    
    // Test state
    bool m_isTestRunning = false;

    // External handle storage
    HWND* m_pExternalHandle = nullptr;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_SPEED_TEST_DIALOG_H
