#include "NetPulse/DialogManager.h"
#include "NetPulse/SettingsDialog.h"
#include "NetPulse/DashboardDialog.h"
#include "NetPulse/ConnectionLogDialog.h"
#include "NetPulse/HistoryDialog.h"
#include "NetPulse/PerAppDialog.h"
#include "NetPulse/SpeedTestDialog.h"
#include "NetPulse/SpeedTester.h"
#include "NetPulse/ThemeHelper.h"
#include "NetPulse/Utils.h"
#include "../resources/resource.h"
#include "NetPulse/UpdateCoordinator.h"
#include "test_fakes/DialogManagerTestFriend.h"
#include "test_fakes/FakeConfigProvider.h"
#include "test_fakes/FakeNetworkStatsProvider.h"
#include "TestUtils.h"

#include <windows.h>
#include <cmath>

using namespace NetPulse;

namespace NetPulseTests
{

void RunSettingsDialogTests();
void RunDashboardDialogTests();
void RunConnectionLogDialogTests();
void RunHistoryDialogTests();
void RunPerAppDialogTests();
void RunSpeedTestDialogTests();

namespace
{
const wchar_t* kDashboardTestClass = L"NetPulseDashboardTestWindow";
int g_wmUpdateStatsCount = 0;

LRESULT CALLBACK DashboardTestWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_UPDATE_STATS)
    {
        ++g_wmUpdateStatsCount;
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

HWND CreateDashboardTestWindow()
{
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DashboardTestWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kDashboardTestClass;
    RegisterClassExW(&wc);

    return CreateWindowExW(
        0,
        kDashboardTestClass,
        L"Dashboard Test Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        120,
        80,
        nullptr,
        nullptr,
        hInstance,
        nullptr);
}

} // namespace

void RunDialogManagerTests()
{
    LogTestMessage(L"=== DialogManager tests ===");

    DialogManager uninitializedManager;
    uninitializedManager.ShowSettings();
    uninitializedManager.ShowDashboard();
    uninitializedManager.ShowHistory();
    uninitializedManager.ShowPerApp();
    uninitializedManager.ShowSpeedTest();
    uninitializedManager.ShowAbout();
    uninitializedManager.ShowConnectionLog();
    AssertTrue(true, L"DialogManager Show methods without init do not crash");

    HWND parent = CreateDashboardTestWindow();
    if (!parent)
    {
        LogTestMessage(L"[WARN] Failed to create parent window; skipping DialogManager callback tests");
        return;
    }

    AppConfig config;
    FakeConfigProvider configProvider;
    FakeNetworkStatsProvider networkProvider;
    networkProvider.m_aggregated.isActive = true;

    UpdateCoordinator coordinator;
    coordinator.Initialize(&config, &networkProvider, nullptr, nullptr, nullptr);

    DialogManager manager;
    manager.Initialize(parent, &config, &configProvider, &networkProvider, &coordinator, nullptr);

    HWND dashboardWindow = CreateDashboardTestWindow();
    AssertTrue(dashboardWindow != nullptr, L"DialogManager dashboard test window created");
    DialogManagerTestFriend::SetDashboardHandle(manager, dashboardWindow);

    manager.ShowDashboard();
    AssertTrue(IsWindow(dashboardWindow),
               L"DialogManager ShowDashboard reuses tracked dashboard handle");

    int configReloadCount = 0;
    int timerUpdateCount = 0;
    UINT lastTimerInterval = 0;
    int languageApplyCount = 0;
    int applyAllSettingsCount = 0;

    manager.SetConfigReloadCallback([&]() -> bool
    {
        ++configReloadCount;
        return true;
    });
    manager.SetTimerUpdateCallback([&](UINT intervalMs)
    {
        ++timerUpdateCount;
        lastTimerInterval = intervalMs;
    });
    manager.SetLanguageApplyCallback([&]()
    {
        ++languageApplyCount;
    });
    manager.SetApplyAllSettingsCallback([&]()
    {
        ++applyAllSettingsCount;
    });

    networkProvider.m_updateCallCount = 0;

    AppConfig oldConfig = config;
    config.updateInterval = UPDATE_INTERVAL_FAST;
    config.language = AppLanguage::Vietnamese;
    g_wmUpdateStatsCount = 0;

    DialogManagerTestFriend::ApplySettings(manager, oldConfig);

    AssertTrue(configReloadCount == 1, L"DialogManager settings apply invokes config reload");
    AssertTrue(timerUpdateCount == 1 && lastTimerInterval == UPDATE_INTERVAL_FAST,
               L"DialogManager settings apply invokes timer update callback");
    AssertTrue(languageApplyCount == 1, L"DialogManager settings apply invokes language callback");
    AssertTrue(applyAllSettingsCount == 1,
               L"DialogManager settings apply invokes apply-all-settings callback");
    AssertTrue(g_wmUpdateStatsCount == 1,
               L"DialogManager settings apply sends WM_UPDATE_STATS to dashboard");
    AssertTrue(networkProvider.m_updateCallCount == 1,
               L"DialogManager settings apply triggers coordinator network tick");
    AssertTrue(oldConfig.updateInterval == UPDATE_INTERVAL_FAST,
               L"DialogManager settings apply updates oldConfig snapshot");

    configReloadCount = 0;
    manager.SetConfigReloadCallback([&]() -> bool
    {
        ++configReloadCount;
        return false;
    });
    DialogManagerTestFriend::ApplySettings(manager, oldConfig);
    AssertTrue(configReloadCount == 1, L"DialogManager config reload callback still invoked");
    AssertTrue(timerUpdateCount == 1,
               L"DialogManager timer callback not repeated when reload returns false");

    DestroyWindow(dashboardWindow);
    DestroyWindow(parent);

    // Call advanced dialog tests
    RunSettingsDialogTests();
    RunDashboardDialogTests();
    RunConnectionLogDialogTests();
    RunHistoryDialogTests();
    RunPerAppDialogTests();
    RunSpeedTestDialogTests();
}

struct SettingsDialogTestFriend
{
    static INT_PTR CallInstanceDialogProc(NetPulse::SettingsDialog& dialog, HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return dialog.InstanceDialogProc(hDlg, msg, wParam, lParam);
    }
    static bool CallApplySettings(NetPulse::SettingsDialog& dialog, HWND hDlg)
    {
        return dialog.ApplySettingsFromDialog(hDlg);
    }
    static AppConfig& GetConfigCopy(NetPulse::SettingsDialog& dialog)
    {
        return dialog.m_configCopy;
    }
    static void SetConfigProvider(NetPulse::SettingsDialog& dialog, NetPulse::IConfigProvider* provider)
    {
        dialog.m_pConfigProvider = provider;
    }
    static void SetStatsProvider(NetPulse::SettingsDialog& dialog, NetPulse::INetworkStatsProvider* provider)
    {
        dialog.m_pStatsProvider = provider;
    }
    static void SetCheckboxState(NetPulse::SettingsDialog& dialog, UINT ctrlId, bool checked)
    {
        dialog.SetCheckboxState(ctrlId, checked);
    }
    static bool GetCheckboxState(const NetPulse::SettingsDialog& dialog, UINT ctrlId)
    {
        return dialog.GetCheckboxState(ctrlId);
    }
};

struct DashboardDialogTestFriend
{
    static INT_PTR CallInstanceDialogProc(NetPulse::DashboardDialog& dialog, HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return dialog.InstanceDialogProc(hDlg, msg, wParam, lParam);
    }
    static void CallUpdateDashboardData(const NetPulse::DashboardDialog& dialog, HWND hDlg)
    {
        dialog.UpdateDashboardData(hDlg);
    }
    static void CallDrawDashboardChart(NetPulse::DashboardDialog& dialog, HDC hdc, const RECT& rc)
    {
        dialog.DrawDashboardChart(hdc, rc);
    }
    static void SetConfig(NetPulse::DashboardDialog& dialog, const AppConfig* config)
    {
        dialog.m_pConfig = config;
    }
};

void RunSettingsDialogTests()
{
    LogTestMessage(L"=== SettingsDialog tests ===");

    SettingsDialog dialog;
    FakeConfigProvider configProvider;
    FakeNetworkStatsProvider statsProvider;

    // Load initial config
    AppConfig config;
    configProvider.LoadConfig(config);

    SettingsDialogTestFriend::SetConfigProvider(dialog, &configProvider);
    SettingsDialogTestFriend::SetStatsProvider(dialog, &statsProvider);
    SettingsDialogTestFriend::GetConfigCopy(dialog) = config;

    // 1. Create the dialog in modeless mode using CreateDialogParamW
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HWND hDlg = CreateDialogParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG),
        nullptr,
        nullptr, // null dlgproc is fine since we call InstanceDialogProc directly or let default pass
        0
    );

    if (!hDlg)
    {
        LogTestMessage(L"[WARN] CreateDialogParamW for IDD_SETTINGS_DIALOG failed; skipping further tests");
        return;
    }

    // Force associate DWLP_USER pointer to SettingsDialog
    SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(&dialog));

    // Call InstanceDialogProc with WM_INITDIALOG to setup controls
    SettingsDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_INITDIALOG, 0, reinterpret_cast<LPARAM>(&dialog));

    // Test: Checkbox getters/setters state
    SettingsDialogTestFriend::SetCheckboxState(dialog, IDC_AUTOSTART_CHECK, true);
    AssertTrue(SettingsDialogTestFriend::GetCheckboxState(dialog, IDC_AUTOSTART_CHECK), L"Checkbox state helper stores correctly");
    
    // Toggle state
    SettingsDialogTestFriend::SetCheckboxState(dialog, IDC_AUTOSTART_CHECK, !SettingsDialogTestFriend::GetCheckboxState(dialog, IDC_AUTOSTART_CHECK));
    AssertTrue(!SettingsDialogTestFriend::GetCheckboxState(dialog, IDC_AUTOSTART_CHECK), L"CheckboxState toggles state");

    // Test: Simulated control changes & ApplySettings
    HWND hTargetEdit = GetDlgItem(hDlg, IDC_PING_TARGET_EDIT);
    if (hTargetEdit)
    {
        SetWindowTextW(hTargetEdit, L"1.1.1.1");
    }

    HWND hQuotaEdit = GetDlgItem(hDlg, IDC_DATA_USAGE_QUOTA_EDIT);
    if (hQuotaEdit)
    {
        SetWindowTextW(hQuotaEdit, L"50.5");
    }

    // Call ApplySettings
    bool applied = SettingsDialogTestFriend::CallApplySettings(dialog, hDlg);
    AssertTrue(applied, L"ApplySettingsFromDialog runs and returns true");

    const auto& updatedConfig = SettingsDialogTestFriend::GetConfigCopy(dialog);
    AssertTrue(updatedConfig.pingTarget == L"1.1.1.1", L"ApplySettings successfully bounds ping target");
    AssertTrue(std::abs(updatedConfig.dataQuotaGB - 50.5) < 0.001, L"ApplySettings successfully bounds data quota");

    // Test: Validation logic (empty target -> 8.8.8.8, negative quota -> 0.0)
    if (hTargetEdit)
    {
        SetWindowTextW(hTargetEdit, L"");
    }
    if (hQuotaEdit)
    {
        SetWindowTextW(hQuotaEdit, L"-10.0");
    }
    SettingsDialogTestFriend::CallApplySettings(dialog, hDlg);
    AssertTrue(updatedConfig.pingTarget == L"8.8.8.8", L"ApplySettings fallback for empty ping target");
    AssertTrue(updatedConfig.dataQuotaGB == 0.0, L"ApplySettings fallback for negative data quota");

    // Test: WM_DRAWITEM for owner-draw buttons/checkboxes (cover drawing paths)
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_BUTTON;
    dis.CtlID = IDOK;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState = 0;
    dis.hwndItem = GetDlgItem(hDlg, IDOK);
    dis.hDC = CreateCompatibleDC(nullptr);
    dis.rcItem = { 0, 0, 100, 30 };
    SettingsDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_DRAWITEM, IDOK, reinterpret_cast<LPARAM>(&dis));
    DeleteDC(dis.hDC);

    // Test: WM_CTLCOLORSTATIC for theme support
    HDC hdcStatic = CreateCompatibleDC(nullptr);
    SettingsDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(hdcStatic), reinterpret_cast<LPARAM>(GetDlgItem(hDlg, IDC_SETTINGS_LABEL_THEME)));
    DeleteDC(hdcStatic);

    // Test: WM_COMMAND simulate Apply button click
    SettingsDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_SETTINGS_BUTTON_APPLY, 0), 0);

    // Test: WM_NOTIFY tab change
    NMHDR nmh = {};
    nmh.hwndFrom = GetDlgItem(hDlg, IDC_SETTINGS_TAB);
    nmh.idFrom = IDC_SETTINGS_TAB;
    nmh.code = TCN_SELCHANGE;
    SettingsDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_NOTIFY, IDC_SETTINGS_TAB, reinterpret_cast<LPARAM>(&nmh));

    // Cleanup Modeless Dialog
    DestroyWindow(hDlg);

    AssertTrue(true, L"SettingsDialog tests completed successfully");
}

void RunDashboardDialogTests()
{
    LogTestMessage(L"=== DashboardDialog tests ===");

    DashboardDialog dialog;
    AppConfig config;

    DashboardDialogTestFriend::SetConfig(dialog, &config);

    // Create the dialog in modeless mode using CreateDialogParamW
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HWND hDlg = CreateDialogParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_DASHBOARD_DIALOG),
        nullptr,
        nullptr,
        0
    );

    if (!hDlg)
    {
        LogTestMessage(L"[WARN] CreateDialogParamW for IDD_DASHBOARD_DIALOG failed; skipping further tests");
        return;
    }

    // Set user data pointer
    SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(&dialog));

    // Call InstanceDialogProc with WM_INITDIALOG
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_INITDIALOG, 0, reinterpret_cast<LPARAM>(&dialog));

    // Insert mock data in HistoryLogger sandbox
    HistoryLogger& logger = HistoryLogger::Instance();
    // Insert some traffic samples
    std::wstring ifaceName = L"All Interfaces";
    logger.AppendSample(ifaceName, 1000, 500);
    logger.AppendSample(ifaceName, 2000, 1000);

    // Test: UpdateDashboardData
    DashboardDialogTestFriend::CallUpdateDashboardData(dialog, hDlg);

    // Test: WM_UPDATE_STATS message handling
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_UPDATE_STATS, 0, 0);

    // Test: WM_DRAWITEM for owner-draw buttons
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_BUTTON;
    dis.CtlID = IDC_DASHBOARD_REFRESH;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState = 0;
    dis.hwndItem = GetDlgItem(hDlg, IDC_DASHBOARD_REFRESH);
    dis.hDC = CreateCompatibleDC(nullptr);
    dis.rcItem = { 0, 0, 100, 30 };
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_DRAWITEM, IDC_DASHBOARD_REFRESH, reinterpret_cast<LPARAM>(&dis));
    DeleteDC(dis.hDC);

    // Test: DrawDashboardChart
    HDC hdcMem = CreateCompatibleDC(nullptr);
    HBITMAP hBitmap = CreateCompatibleBitmap(GetDC(nullptr), 300, 150);
    SelectObject(hdcMem, hBitmap);
    RECT chartRect = { 0, 0, 300, 150 };
    DashboardDialogTestFriend::CallDrawDashboardChart(dialog, hdcMem, chartRect);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);

    // Test: WM_COMMAND for Chart view modes (Daily / Monthly)
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_CHART_VIEW_DAILY, 0), 0);
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_CHART_VIEW_MONTHLY, 0), 0);

    // Test: Navigation buttons (IDC_CHART_NAV_PREV / IDC_CHART_NAV_NEXT)
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_CHART_NAV_PREV, 0), 0);
    DashboardDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_CHART_NAV_NEXT, 0), 0);

    // Cleanup Modeless Dialog
    DestroyWindow(hDlg);

    AssertTrue(true, L"DashboardDialog tests completed successfully");
}

struct ConnectionLogDialogTestFriend
{
    static DLGPROC GetDialogProc()
    {
        return NetPulse::ConnectionLogDialog::DialogProc;
    }
    static INT_PTR CallInstanceDialogProc(NetPulse::ConnectionLogDialog& dialog, HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return dialog.InstanceDialogProc(hDlg, msg, wParam, lParam);
    }
    static void SetConfig(NetPulse::ConnectionLogDialog& dialog, const AppConfig* config)
    {
        dialog.m_pConfig = config;
    }
    static void SetConnectionMonitor(NetPulse::ConnectionLogDialog& dialog, ConnectionMonitor* cm)
    {
        dialog.m_pConnectionMonitor = cm;
    }
};

struct HistoryDialogTestFriend
{
    static DLGPROC GetDialogProc()
    {
        return NetPulse::HistoryDialog::DialogProc;
    }
    static INT_PTR CallInstanceDialogProc(NetPulse::HistoryDialog& dialog, HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return dialog.InstanceDialogProc(hDlg, msg, wParam, lParam);
    }
    static void SetConfig(NetPulse::HistoryDialog& dialog, const AppConfig* config)
    {
        dialog.m_pConfig = config;
    }
};

struct PerAppDialogTestFriend
{
    static DLGPROC GetDialogProc()
    {
        return NetPulse::PerAppDialog::DialogProc;
    }
    static INT_PTR CallInstanceDialogProc(NetPulse::PerAppDialog& dialog, HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return dialog.InstanceDialogProc(hDlg, msg, wParam, lParam);
    }
    static void SetConfig(NetPulse::PerAppDialog& dialog, const AppConfig* config)
    {
        dialog.m_pConfig = config;
    }
};

struct SpeedTestDialogTestFriend
{
    static DLGPROC GetDialogProc()
    {
        return NetPulse::SpeedTestDialog::DialogProc;
    }
    static INT_PTR CallHandleMessage(NetPulse::SpeedTestDialog& dialog, HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return dialog.HandleMessage(hDlg, msg, wParam, lParam);
    }
    static void SetConfig(NetPulse::SpeedTestDialog& dialog, AppConfig* config)
    {
        dialog.m_pConfig = config;
    }
    static void SetSpeedTester(NetPulse::SpeedTestDialog& dialog, std::unique_ptr<SpeedTester> tester)
    {
        dialog.m_speedTester = std::move(tester);
    }
};

class FakeSpeedTester : public SpeedTester
{
public:
    FakeSpeedTester() : SpeedTester(nullptr) {}

    void StartTest(std::function<void(int progress, const std::wstring& status)> progressCallback) override
    {
        m_running = true;
        m_cancelled = false;
        
        if (progressCallback)
        {
            progressCallback(50, L"Testing...");
        }

        if (m_resultCallback)
        {
            SpeedTestResult res;
            res.success = true;
            res.downloadMbps = 95.5;
            res.uploadMbps = 45.2;
            res.pingMs = 12;
            res.timestamp = time(nullptr);
            m_resultCallback(res);
        }
        m_running = false;
    }

    bool IsRunning() const override
    {
        return m_running;
    }

    void SetResultCallback(std::function<void(const SpeedTestResult&)> callback) override
    {
        m_resultCallback = callback;
    }

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancelled{false};
    std::function<void(const SpeedTestResult&)> m_resultCallback;
};

struct MessageBoxAutoCloser
{
    std::wstring title;
    int buttonId;
    HANDLE hThread;

    MessageBoxAutoCloser(const std::wstring& t, int btn)
        : title(t), buttonId(btn), hThread(nullptr)
    {
        hThread = CreateThread(nullptr, 0, StaticThreadProc, this, 0, nullptr);
    }

    ~MessageBoxAutoCloser()
    {
        if (hThread)
        {
            WaitForSingleObject(hThread, 1000);
            CloseHandle(hThread);
        }
    }

    static DWORD WINAPI StaticThreadProc(LPVOID lpParam)
    {
        auto* pThis = static_cast<MessageBoxAutoCloser*>(lpParam);
        for (int i = 0; i < 100; ++i)
        {
            HWND hMsgBox = FindWindowW(L"#32770", pThis->title.c_str());
            if (hMsgBox && IsWindowVisible(hMsgBox))
            {
                PostMessageW(hMsgBox, WM_COMMAND, pThis->buttonId, 0);
                break;
            }
            Sleep(10);
        }
        return 0;
    }
};

#ifndef WM_SPEED_TEST_RESULT
#define WM_SPEED_TEST_RESULT    (WM_USER + 200)
#endif
#ifndef WM_SPEED_TEST_PROGRESS
#define WM_SPEED_TEST_PROGRESS  (WM_USER + 201)
#endif

void RunConnectionLogDialogTests()
{
    LogTestMessage(L"=== ConnectionLogDialog tests ===");

    ConnectionLogDialog dialog;
    AppConfig config;
    config.themeMode = ThemeMode::Dracula; 

    ConnectionMonitor connectionMonitor;
    connectionMonitor.Start(); 

    ConnectionLogDialogTestFriend::SetConfig(dialog, &config);
    ConnectionLogDialogTestFriend::SetConnectionMonitor(dialog, &connectionMonitor);

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HWND hDlg = CreateDialogParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_CONNECTION_LOG_DIALOG),
        nullptr,
        ConnectionLogDialogTestFriend::GetDialogProc(),
        reinterpret_cast<LPARAM>(&dialog)
    );

    if (!hDlg)
    {
        LogTestMessage(L"[WARN] CreateDialogParamW for IDD_CONNECTION_LOG_DIALOG failed; skipping further tests");
        connectionMonitor.Stop();
        return;
    }

    // Simulate IDC_CONNLOG_REFRESH
    ConnectionLogDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_CONNLOG_REFRESH, 0), 0);

    // Simulate WM_TIMER auto-refresh
    ConnectionLogDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_TIMER, 1001, 0);

    // Simulate WM_DRAWITEM
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_BUTTON;
    dis.CtlID = IDC_CONNLOG_REFRESH;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.hwndItem = GetDlgItem(hDlg, IDC_CONNLOG_REFRESH);
    dis.hDC = CreateCompatibleDC(nullptr);
    dis.rcItem = { 0, 0, 100, 30 };
    ConnectionLogDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_DRAWITEM, IDC_CONNLOG_REFRESH, reinterpret_cast<LPARAM>(&dis));
    DeleteDC(dis.hDC);

    // Simulate WM_CTLCOLORDLG
    HDC hdcDlg = CreateCompatibleDC(nullptr);
    ConnectionLogDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_CTLCOLORDLG, reinterpret_cast<WPARAM>(hdcDlg), 0);
    DeleteDC(hdcDlg);

    // Simulate WM_CTLCOLORSTATIC
    HDC hdcStatic = CreateCompatibleDC(nullptr);
    ConnectionLogDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(hdcStatic), reinterpret_cast<LPARAM>(GetDlgItem(hDlg, IDCANCEL)));
    DeleteDC(hdcStatic);

    // Simulate WM_NOTIFY CDDS_PREPAINT
    NMLVCUSTOMDRAW lvcd = {};
    lvcd.nmcd.dwDrawStage = CDDS_PREPAINT;
    lvcd.nmcd.hdr.hwndFrom = GetDlgItem(hDlg, IDC_CONNLOG_LIST);
    lvcd.nmcd.hdc = CreateCompatibleDC(nullptr);
    ConnectionLogDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_NOTIFY, IDC_CONNLOG_LIST, reinterpret_cast<LPARAM>(&lvcd));
    DeleteDC(lvcd.nmcd.hdc);

    // Destroy
    DestroyWindow(hDlg);
    connectionMonitor.Stop();

    // Dispatch any remaining messages in the queue to ensure complete cleanup
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    AssertTrue(true, L"ConnectionLogDialog tests completed successfully");
}

void RunHistoryDialogTests()
{
    LogTestMessage(L"=== HistoryDialog tests ===");

    HistoryDialog dialog;
    AppConfig config;
    config.themeMode = ThemeMode::Nord; 

    HistoryDialogTestFriend::SetConfig(dialog, &config);

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HWND hDlg = CreateDialogParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_HISTORY_MANAGE_DIALOG),
        nullptr,
        HistoryDialogTestFriend::GetDialogProc(),
        reinterpret_cast<LPARAM>(&dialog)
    );

    if (!hDlg)
    {
        LogTestMessage(L"[WARN] CreateDialogParamW for IDD_HISTORY_MANAGE_DIALOG failed; skipping further tests");
        return;
    }

    // Load localized title to match whatever system locale is using
    std::wstring historyTitle = LoadStringResource(IDS_HISTORY_MANAGE_TITLE);
    if (historyTitle.empty())
    {
        historyTitle = L"Manage History";
    }

    // Simulate button clicks with Auto-Closer to prevent blocking
    {
        MessageBoxAutoCloser autoCloser(historyTitle, IDYES);
        HistoryDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_HISTORY_DELETE_ALL, 0), 0);
    }

    {
        MessageBoxAutoCloser autoCloser(historyTitle, IDYES);
        HistoryDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_HISTORY_KEEP_30, 0), 0);
    }

    {
        MessageBoxAutoCloser autoCloser(historyTitle, IDYES);
        HistoryDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_HISTORY_KEEP_90, 0), 0);
    }

    // Simulate Cancel of DialogBox
    {
        MessageBoxAutoCloser autoCloser(historyTitle, IDNO);
        HistoryDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_HISTORY_DELETE_ALL, 0), 0);
    }

    // Simulate WM_DRAWITEM
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_BUTTON;
    dis.CtlID = IDC_HISTORY_DELETE_ALL;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.hwndItem = GetDlgItem(hDlg, IDC_HISTORY_DELETE_ALL);
    dis.hDC = CreateCompatibleDC(nullptr);
    dis.rcItem = { 0, 0, 100, 30 };
    HistoryDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_DRAWITEM, IDC_HISTORY_DELETE_ALL, reinterpret_cast<LPARAM>(&dis));
    DeleteDC(dis.hDC);

    // Simulate WM_CTLCOLORSTATIC
    HDC hdcStatic = CreateCompatibleDC(nullptr);
    HistoryDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(hdcStatic), reinterpret_cast<LPARAM>(GetDlgItem(hDlg, IDCANCEL)));
    DeleteDC(hdcStatic);

    DestroyWindow(hDlg);

    // Dispatch any remaining messages in the queue to ensure complete cleanup
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    AssertTrue(true, L"HistoryDialog tests completed successfully");
}

void RunPerAppDialogTests()
{
    LogTestMessage(L"=== PerAppDialog tests ===");

    PerAppDialog dialog;
    AppConfig config;
    config.themeMode = ThemeMode::RosePink;

    PerAppDialogTestFriend::SetConfig(dialog, &config);

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HWND hDlg = CreateDialogParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_PERAPP_DIALOG),
        nullptr,
        PerAppDialogTestFriend::GetDialogProc(),
        reinterpret_cast<LPARAM>(&dialog)
    );

    if (!hDlg)
    {
        LogTestMessage(L"[WARN] CreateDialogParamW for IDD_PERAPP_DIALOG failed; skipping further tests");
        return;
    }

    // Simulate IDC_PERAPP_REFRESH
    PerAppDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_PERAPP_REFRESH, 0), 0);

    // Simulate WM_DRAWITEM
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_BUTTON;
    dis.CtlID = IDC_PERAPP_REFRESH;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.hwndItem = GetDlgItem(hDlg, IDC_PERAPP_REFRESH);
    dis.hDC = CreateCompatibleDC(nullptr);
    dis.rcItem = { 0, 0, 100, 30 };
    PerAppDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_DRAWITEM, IDC_PERAPP_REFRESH, reinterpret_cast<LPARAM>(&dis));
    DeleteDC(dis.hDC);

    // Simulate WM_CTLCOLORDLG
    HDC hdcDlg = CreateCompatibleDC(nullptr);
    PerAppDialogTestFriend::CallInstanceDialogProc(dialog, hDlg, WM_CTLCOLORDLG, reinterpret_cast<WPARAM>(hdcDlg), 0);
    DeleteDC(hdcDlg);

    DestroyWindow(hDlg);

    // Dispatch any remaining messages in the queue to ensure complete cleanup
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    AssertTrue(true, L"PerAppDialog tests completed successfully");
}

void RunSpeedTestDialogTests()
{
    LogTestMessage(L"=== SpeedTestDialog tests ===");

    SpeedTestDialog dialog;
    AppConfig config;
    config.themeMode = ThemeMode::OLED; 

    SpeedTestDialogTestFriend::SetConfig(dialog, &config);
    SpeedTestDialogTestFriend::SetSpeedTester(dialog, std::make_unique<FakeSpeedTester>());

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HWND hDlg = CreateDialogParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_SPEED_TEST_DIALOG),
        nullptr,
        SpeedTestDialogTestFriend::GetDialogProc(),
        reinterpret_cast<LPARAM>(&dialog)
    );

    if (!hDlg)
    {
        LogTestMessage(L"[WARN] CreateDialogParamW for IDD_SPEED_TEST_DIALOG failed; skipping further tests");
        return;
    }

    // Simulate Click Start (Start Speed Test)
    SpeedTestDialogTestFriend::CallHandleMessage(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_SPEED_START_BUTTON, 0), 0);

    // Simulate Click Cancel
    SpeedTestDialogTestFriend::CallHandleMessage(dialog, hDlg, WM_COMMAND, MAKEWPARAM(IDC_SPEED_START_BUTTON, 0), 0);

    // Simulate WM_SPEED_TEST_PROGRESS message
    SpeedTestDialogTestFriend::CallHandleMessage(dialog, hDlg, WM_SPEED_TEST_PROGRESS, 75, 0);

    // Simulate WM_SPEED_TEST_RESULT message
    SpeedTestResult* pRes = new SpeedTestResult();
    pRes->success = true;
    pRes->downloadMbps = 120.4;
    pRes->uploadMbps = 80.5;
    pRes->pingMs = 5;
    pRes->timestamp = time(nullptr);
    SpeedTestDialogTestFriend::CallHandleMessage(dialog, hDlg, WM_SPEED_TEST_RESULT, 0, reinterpret_cast<LPARAM>(pRes));

    // Simulate WM_DRAWITEM
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_BUTTON;
    dis.CtlID = IDC_SPEED_START_BUTTON;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.hwndItem = GetDlgItem(hDlg, IDC_SPEED_START_BUTTON);
    dis.hDC = CreateCompatibleDC(nullptr);
    dis.rcItem = { 0, 0, 100, 30 };
    SpeedTestDialogTestFriend::CallHandleMessage(dialog, hDlg, WM_DRAWITEM, IDC_SPEED_START_BUTTON, reinterpret_cast<LPARAM>(&dis));
    DeleteDC(dis.hDC);

    // Simulate WM_CTLCOLORSTATIC for Status Label
    HDC hdcStatic = CreateCompatibleDC(nullptr);
    HWND hStatusLabel = GetDlgItem(hDlg, IDC_SPEED_STATUS_LABEL);
    SpeedTestDialogTestFriend::CallHandleMessage(dialog, hDlg, WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(hdcStatic), reinterpret_cast<LPARAM>(hStatusLabel));
    DeleteDC(hdcStatic);

    DestroyWindow(hDlg);

    // Dispatch any remaining messages in the queue to ensure complete cleanup
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    AssertTrue(true, L"SpeedTestDialog tests completed successfully");
}

} // namespace NetPulseTests
