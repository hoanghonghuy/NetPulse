// Prevent winsock.h vs winsock2.h conflict
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "NetPulse/SpeedTestDialog.h"
#include "NetPulse/SpeedTester.h"
#include "NetPulse/SpeedTestHistory.h"
#include "NetPulse/Common.h"
#include "NetPulse/ThemeHelper.h"
#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/Utils.h"
#include "../resources/resource.h"

#include <CommCtrl.h>
#include <windowsx.h>
#include <cstdio>
#include <Uxtheme.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace NetPulse
{

// Custom messages for async updates
#define WM_SPEED_TEST_RESULT    (WM_USER + 200)
#define WM_SPEED_TEST_PROGRESS  (WM_USER + 201)

// Subclass ID for header control


// Header subclass procedure for dark theme


SpeedTestDialog::SpeedTestDialog()
    : m_speedTester(std::make_unique<SpeedTester>())
    , m_speedTestHistory(std::make_unique<SpeedTestHistory>())
    , m_isTestRunning(false)
    , m_isTestComplete(false)
{
}

SpeedTestDialog::~SpeedTestDialog()
{
}

void SpeedTestDialog::Show(HWND hWndParent, AppConfig* pConfig)
{
    m_pConfig = pConfig;
    
    DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_SPEED_TEST_DIALOG),
        hWndParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );
}

INT_PTR CALLBACK SpeedTestDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SpeedTestDialog* pThis = nullptr;
    
    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<SpeedTestDialog*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDlg = hDlg;
        
        // Update external handle storage if provided
        if (pThis->m_pExternalHandle)
        {
            *pThis->m_pExternalHandle = hDlg;
        }
        
    }
    else
    {
        pThis = reinterpret_cast<SpeedTestDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }
    
    if (pThis)
    {
        return pThis->HandleMessage(hDlg, message, wParam, lParam);
    }
    
    return FALSE;
}

INT_PTR SpeedTestDialog::HandleMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            OnInitDialog(hDlg);
            return TRUE;
            
        case WM_COMMAND:
            OnCommand(hDlg, wParam);
            return TRUE;
            
        case WM_CLOSE:
            OnClose(hDlg);
            return TRUE;
            
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            if (m_pConfig && m_pConfig->darkTheme)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                int id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

                // Status Label - Green if complete, Blue if Running
                if (id == IDC_SPEED_STATUS_LABEL)
                {
                    SetBkMode(hdc, TRANSPARENT);
                    if (m_isTestComplete)
                         SetTextColor(hdc, RGB(0, 255, 128)); // Bright Green
                    else if (m_isTestRunning)
                         SetTextColor(hdc, RGB(80, 160, 240)); // Blue (matches progress bar)
                    else
                         SetTextColor(hdc, DialogThemeHelper::DARK_TEXT);
                    
                    return reinterpret_cast<INT_PTR>(DialogThemeHelper::GetDarkBackgroundBrush());
                }

                HBRUSH hBrush = DialogThemeHelper::HandleControlColor(reinterpret_cast<HDC>(wParam), true);
                if (hBrush)
                {
                    return reinterpret_cast<INT_PTR>(hBrush);
                }
            }
            break;
            
        case WM_DRAWITEM:
            if (m_pConfig && m_pConfig->darkTheme)
            {
                if (OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)))
                {
                    return TRUE;
                }
            }
            break;
            
        case WM_SPEED_TEST_RESULT:
        {
            SpeedTestResult* pResult = reinterpret_cast<SpeedTestResult*>(lParam);
            if (pResult)
            {
                DisplayResult(*pResult);
                delete pResult;
            }
            return TRUE;
        }
        
        case WM_SPEED_TEST_PROGRESS:
        {
            int progress = static_cast<int>(wParam);
            UpdateProgress(progress, L"");
            return TRUE;
        }
    }
    
    return FALSE;
}

void SpeedTestDialog::OnInitDialog(HWND hDlg)
{
    // Get control handles
    m_hStartButton = GetDlgItem(hDlg, IDC_SPEED_START_BUTTON);
    m_hProgressBar = GetDlgItem(hDlg, IDC_SPEED_TEST_PROGRESS);
    m_hStatusLabel = GetDlgItem(hDlg, IDC_SPEED_STATUS_LABEL);
    m_hDownloadValue = GetDlgItem(hDlg, IDC_SPEED_DOWNLOAD_VALUE);
    m_hUploadValue = GetDlgItem(hDlg, IDC_SPEED_UPLOAD_VALUE);
    m_hPingValue = GetDlgItem(hDlg, IDC_SPEED_PING_VALUE);
    m_hHistoryList = GetDlgItem(hDlg, IDC_SPEED_HISTORY_LIST);
    
    // Initialize progress bar
    if (m_hProgressBar)
    {
        SendMessageW(m_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(m_hProgressBar, PBM_SETPOS, 0, 0);
    }
    
    // Center dialog
    // Center dialog on screen
    RECT rcScreen;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcScreen, 0);
    RECT rcDlg;
    GetWindowRect(hDlg, &rcDlg);
    int x = rcScreen.left + (rcScreen.right - rcScreen.left - (rcDlg.right - rcDlg.left)) / 2;
    int y = rcScreen.top + (rcScreen.bottom - rcScreen.top - (rcDlg.bottom - rcDlg.top)) / 2;
    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    // Localize controls
    LocalizeControls(hDlg);
    
    // Initialize history list
    InitializeHistoryList(hDlg);
    PopulateHistoryList();
    
    // Apply dark theme if enabled
    if (m_pConfig && m_pConfig->darkTheme)
    {
        ApplyDarkTheme(hDlg);
    }
}

void SpeedTestDialog::ApplyDarkTheme(HWND hDlg)
{
    // Apply dark title bar
    ThemeHelper::ApplyDarkTitleBar(hDlg, true);
    
    // Make buttons owner-draw
    // Make buttons owner-draw
    DialogThemeHelper::ApplyDarkButton(m_hStartButton);
    DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDCANCEL));
    
    // Progress bar dark colors - strip visual styles first for PBM_SETBKCOLOR to work
    if (m_hProgressBar)
    {
        SetWindowTheme(m_hProgressBar, L"", L"");
        SendMessageW(m_hProgressBar, PBM_SETBARCOLOR, 0, RGB(80, 160, 240));
        SendMessageW(m_hProgressBar, PBM_SETBKCOLOR, 0, DialogThemeHelper::DARK_PANEL);
    }
    
    // History list dark theme
    if (m_hHistoryList)
    {
        DialogThemeHelper::ApplyDarkListView(m_hHistoryList, true);
    }
    
    // Strip visual styles from GroupBox for proper dark text rendering
    HWND hResultGroup = GetDlgItem(hDlg, IDC_SPEED_RESULT_GROUP);
    if (hResultGroup)
    {
        SetWindowTheme(hResultGroup, L"", L"");
    }

    InvalidateRect(hDlg, nullptr, TRUE);
}

void SpeedTestDialog::LocalizeControls(HWND hDlg)
{
    // Dialog title
    std::wstring title = LoadStringResource(IDS_SPEED_TEST_DIALOG_TITLE);
    if (title.empty()) title = L"Speed Test";
    SetWindowTextW(hDlg, title.c_str());
    
    // Start button
    std::wstring startText = LoadStringResource(IDS_SPEED_TEST_BUTTON);
    if (startText.empty()) startText = L"Start Test";
    if (m_hStartButton) SetWindowTextW(m_hStartButton, startText.c_str());
    
    // Labels
    std::wstring dlText = LoadStringResource(IDS_SPEED_DOWNLOAD);
    if (dlText.empty()) dlText = L"Download:";
    HWND hDlLabel = GetDlgItem(hDlg, IDC_SPEED_DOWNLOAD_LABEL);
    if (hDlLabel) SetWindowTextW(hDlLabel, dlText.c_str());
    
    std::wstring ulText = LoadStringResource(IDS_SPEED_UPLOAD);
    if (ulText.empty()) ulText = L"Upload:";
    HWND hUlLabel = GetDlgItem(hDlg, IDC_SPEED_UPLOAD_LABEL);
    if (hUlLabel) SetWindowTextW(hUlLabel, ulText.c_str());
    
    std::wstring pingText = LoadStringResource(IDS_SPEED_PING);
    if (pingText.empty()) pingText = L"Ping:";
    HWND hPingLabel = GetDlgItem(hDlg, IDC_SPEED_PING_LABEL);
    if (hPingLabel) SetWindowTextW(hPingLabel, pingText.c_str());
    
    // GroupBox Title - Change to "Results"
    std::wstring groupText = LoadStringResource(IDS_SPEED_RESULTS);
    if (groupText.empty()) groupText = L"Results";
    HWND hGroup = GetDlgItem(hDlg, IDC_SPEED_RESULT_GROUP);
    if (hGroup) SetWindowTextW(hGroup, groupText.c_str());

    // History Label
    std::wstring historyText = LoadStringResource(IDS_SPEED_HISTORY);
    if (historyText.empty()) historyText = L"History:";
    HWND hHistoryLabel = GetDlgItem(hDlg, IDC_SPEED_HISTORY_LABEL);
    if (hHistoryLabel) SetWindowTextW(hHistoryLabel, historyText.c_str());
    
    // Cancel button
    std::wstring cancelText = LoadStringResource(IDS_SPEED_CANCEL);
    if (cancelText.empty()) cancelText = L"Cancel";
    HWND hCancel = GetDlgItem(hDlg, IDCANCEL);
    if (hCancel) SetWindowTextW(hCancel, cancelText.c_str());
}

void SpeedTestDialog::InitializeHistoryList(HWND /*hDlg*/)
{
    if (!m_hHistoryList) return;
    
    // Set extended styles
    ListView_SetExtendedListViewStyle(m_hHistoryList, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    
    // Add columns
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    
    std::wstring timeHeader = LoadStringResource(IDS_DASHBOARD_COL_TIME);
    if (timeHeader.empty()) timeHeader = L"Time";
    col.pszText = const_cast<LPWSTR>(timeHeader.c_str());
    col.cx = 120;
    ListView_InsertColumn(m_hHistoryList, 0, &col);
    
    std::wstring downHeader = LoadStringResource(IDS_DASHBOARD_COL_DOWN);
    if (downHeader.empty()) downHeader = L"Download";
    col.pszText = const_cast<LPWSTR>(downHeader.c_str());
    col.cx = 80;
    col.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hHistoryList, 1, &col);
    
    std::wstring upHeader = LoadStringResource(IDS_DASHBOARD_COL_UP);
    if (upHeader.empty()) upHeader = L"Upload";
    col.pszText = const_cast<LPWSTR>(upHeader.c_str());
    col.cx = 80;
    ListView_InsertColumn(m_hHistoryList, 2, &col);
    
    std::wstring pingHeader = LoadStringResource(IDS_FLOATING_SHOW_PING);
    if (pingHeader.empty()) pingHeader = L"Ping";
    col.pszText = const_cast<LPWSTR>(pingHeader.c_str());
    col.cx = 60;
    ListView_InsertColumn(m_hHistoryList, 3, &col);
}

void SpeedTestDialog::PopulateHistoryList()
{
    if (!m_hHistoryList || !m_speedTestHistory) return;
    
    ListView_DeleteAllItems(m_hHistoryList);
    
    auto history = m_speedTestHistory->GetHistory();
    
    // History is stored newest first (AddResult inserts at begin)
    // So we iterate with begin(), not rbegin()
    int count = 0;
    for (auto it = history.begin(); it != history.end() && count < 10; ++it, ++count)
    {
        const auto& result = *it;
        
        // Format time
        wchar_t timeStr[64];
        struct tm tm = {};
        localtime_s(&tm, &result.timestamp);
        wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M", &tm);
        
        // Insert item
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = count;
        item.pszText = timeStr;
        int idx = ListView_InsertItem(m_hHistoryList, &item);
        
        // Download
        wchar_t valStr[32];
        swprintf_s(valStr, L"%.1f Mbps", result.downloadMbps);
        ListView_SetItemText(m_hHistoryList, idx, 1, valStr);
        
        // Upload
        swprintf_s(valStr, L"%.1f Mbps", result.uploadMbps);
        ListView_SetItemText(m_hHistoryList, idx, 2, valStr);
        
        // Ping
        swprintf_s(valStr, L"%d ms", result.pingMs);
        ListView_SetItemText(m_hHistoryList, idx, 3, valStr);
    }
}

void SpeedTestDialog::StartSpeedTest()
{
    if (!m_speedTester || m_isTestRunning) return;
    
    m_isTestRunning = true;
    m_isTestComplete = false; // Reset complete flag
    
    // Update UI
    std::wstring testingText = LoadStringResource(IDS_SPEED_TESTING);
    if (testingText.empty()) testingText = L"Testing...";
    SetWindowTextW(m_hStartButton, testingText.c_str());
    EnableWindow(m_hStartButton, FALSE);
    
    // Show progress bar
    if (m_hProgressBar)
    {
        SendMessageW(m_hProgressBar, PBM_SETPOS, 0, 0);
        ShowWindow(m_hProgressBar, SW_SHOW);
    }
    
    // Clear results
    if (m_hDownloadValue) SetWindowTextW(m_hDownloadValue, L"...");
    if (m_hUploadValue) SetWindowTextW(m_hUploadValue, L"...");
    if (m_hPingValue) SetWindowTextW(m_hPingValue, L"...");
    if (m_hStatusLabel) SetWindowTextW(m_hStatusLabel, L""); // Hide text in Status Label
    
    // Set callbacks
    HWND hDlg = m_hDlg;
    
    m_speedTester->SetResultCallback([hDlg](const SpeedTestResult& result) {
        PostMessageW(hDlg, WM_SPEED_TEST_RESULT, 0, 
            reinterpret_cast<LPARAM>(new SpeedTestResult(result)));
    });
    
    m_speedTester->StartTest([hDlg](int progress, const std::wstring& /*status*/) {
        PostMessageW(hDlg, WM_SPEED_TEST_PROGRESS, static_cast<WPARAM>(progress), 0);
    });
}

void SpeedTestDialog::CancelSpeedTest()
{
    if (m_speedTester && m_isTestRunning)
    {
        m_speedTester->CancelTest();
        m_isTestRunning = false;
        
        // Reset UI
        std::wstring startText = LoadStringResource(IDS_SPEED_TEST_BUTTON);
        if (startText.empty()) startText = L"Start Test";
        SetWindowTextW(m_hStartButton, startText.c_str());
        EnableWindow(m_hStartButton, TRUE);
        
        if (m_hStatusLabel)
        {
            std::wstring cancelText = LoadStringResource(IDS_SPEED_CANCEL);
            if (cancelText.empty()) cancelText = L"Cancelled";
            SetWindowTextW(m_hStatusLabel, cancelText.c_str());
        }
    }
}

void SpeedTestDialog::UpdateProgress(int progress, const std::wstring& status)
{
    if (m_hProgressBar)
    {
        SendMessageW(m_hProgressBar, PBM_SETPOS, progress, 0);
    }
    
    if (!status.empty() && m_hStatusLabel)
    {
        SetWindowTextW(m_hStatusLabel, status.c_str());
    }
}

void SpeedTestDialog::DisplayResult(const SpeedTestResult& result)
{
    m_isTestRunning = false;
    m_isTestComplete = true; // Mark as complete for green text
    
    // Re-enable button
    std::wstring startText = LoadStringResource(IDS_SPEED_TEST_BUTTON);
    if (startText.empty()) startText = L"Start Test";
    SetWindowTextW(m_hStartButton, startText.c_str());
    EnableWindow(m_hStartButton, TRUE);
    
    if (result.success)
    {
        // Display results
        wchar_t valStr[32];
        
        swprintf_s(valStr, L"%.1f Mbps", result.downloadMbps);
        if (m_hDownloadValue) SetWindowTextW(m_hDownloadValue, valStr);
        
        swprintf_s(valStr, L"%.1f Mbps", result.uploadMbps);
        if (m_hUploadValue) SetWindowTextW(m_hUploadValue, valStr);
        
        swprintf_s(valStr, L"%d ms", result.pingMs);
        if (m_hPingValue) SetWindowTextW(m_hPingValue, valStr);
        
        std::wstring completeText = LoadStringResource(IDS_SPEED_TEST_COMPLETE);
        if (completeText.empty()) completeText = L"Test complete";
        if (m_hStatusLabel)
        {
            SetWindowTextW(m_hStatusLabel, completeText.c_str());
            // Force repaint to show green text
            InvalidateRect(m_hStatusLabel, nullptr, TRUE);
        }
        
        // Save to history
        if (m_speedTestHistory)
        {
            m_speedTestHistory->AddResult(result);
            PopulateHistoryList();
        }
    }
    else
    {
        // Show error
        std::wstring failedText = LoadStringResource(IDS_SPEED_TEST_FAILED);
        if (failedText.empty()) failedText = L"Test failed";
        if (m_hStatusLabel) SetWindowTextW(m_hStatusLabel, failedText.c_str());
        
        if (m_hDownloadValue) SetWindowTextW(m_hDownloadValue, L"--");
        if (m_hUploadValue) SetWindowTextW(m_hUploadValue, L"--");
        if (m_hPingValue) SetWindowTextW(m_hPingValue, L"--");
    }
}

void SpeedTestDialog::OnCommand(HWND hDlg, WPARAM wParam)
{
    WORD cmd = LOWORD(wParam);
    
    switch (cmd)
    {
        case IDC_SPEED_START_BUTTON:
            if (m_isTestRunning)
            {
                CancelSpeedTest();
            }
            else
            {
                StartSpeedTest();
            }
            break;
            
        case IDCANCEL:
            OnClose(hDlg);
            break;
    }
}

void SpeedTestDialog::OnClose(HWND hDlg)
{
    // Cancel any running test
    if (m_speedTester && m_isTestRunning)
    {
        m_speedTester->CancelTest();
    }
    
    // Clear external handle storage
    if (m_pExternalHandle)
    {
        *m_pExternalHandle = nullptr;
    }

    m_hDlg = nullptr;
    EndDialog(hDlg, IDCANCEL);
}

bool SpeedTestDialog::OnDrawItem(DRAWITEMSTRUCT* pDrawItem) const
{
    if (pDrawItem->CtlType != ODT_BUTTON) return false;
    
    UINT id = pDrawItem->CtlID;
    if (id != IDC_SPEED_START_BUTTON && id != IDCANCEL) return false;
    
    // Custom draw for Start Button when running to show Blue Text
    if (id == IDC_SPEED_START_BUTTON && m_isTestRunning)
    {
        HDC hdc = pDrawItem->hDC;
        RECT rc = pDrawItem->rcItem;
        
        // Background (Disabled Dark Button)
        HBRUSH hBg = CreateSolidBrush(RGB(40, 40, 40)); // Standard dark bg
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);
        
        // Border
        HBRUSH hBorder = CreateSolidBrush(RGB(90, 90, 90));
        FrameRect(hdc, &rc, hBorder);
        DeleteObject(hBorder);
        
        // Text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(80, 160, 240)); // Blue Text
        
        wchar_t text[256];
        GetWindowTextW(pDrawItem->hwndItem, text, 256);
        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        return true;
    }

    DialogThemeHelper::DrawButton(pDrawItem, m_pConfig && m_pConfig->darkTheme);
    return true;
}

} // namespace NetPulse




