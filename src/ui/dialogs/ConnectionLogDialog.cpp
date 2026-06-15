#include "NetPulse/ConnectionLogDialog.h"
#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/ThemeHelper.h"
#include "NetPulse/Utils.h"
#include "../../../resources/resource.h"
#include <commctrl.h>
#include <windowsx.h>

namespace NetPulse
{

ConnectionLogDialog::ConnectionLogDialog()
    : m_hDialog(nullptr)
    , m_hList(nullptr)
    , m_pExternalHandle(nullptr)
    , m_pConfig(nullptr)
    , m_pConnectionMonitor(nullptr)
{
}

ConnectionLogDialog::~ConnectionLogDialog()
{
}

INT_PTR ConnectionLogDialog::Show(HWND parentWindow, const AppConfig* config, ConnectionMonitor* connectionMonitor)
{
    m_pConfig = config;
    m_pConnectionMonitor = connectionMonitor;
    
    return DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_CONNECTION_LOG_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );
}

INT_PTR CALLBACK ConnectionLogDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    ConnectionLogDialog* pThis = nullptr;
    
    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<ConnectionLogDialog*>(lParam);
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        
        // Update external handle storage for tracking
        if (pThis && pThis->m_pExternalHandle)
        {
            *pThis->m_pExternalHandle = hDlg;
        }
    }
    else
    {
        pThis = reinterpret_cast<ConnectionLogDialog*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    }
    
    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }
    
    return FALSE;
}

INT_PTR ConnectionLogDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        InitializeDialog(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            KillTimer(hDlg, REFRESH_TIMER_ID);
            if (m_pExternalHandle)
            {
                *m_pExternalHandle = nullptr;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;

        case IDC_CONNLOG_REFRESH:
            RefreshData();
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (wParam == REFRESH_TIMER_ID)
        {
            RefreshData();
        }
        return TRUE;

    case WM_DRAWITEM:
        if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
        {
            DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pDrawItem->CtlType == ODT_BUTTON)
            {
                UINT id = pDrawItem->CtlID;
                if (id == IDC_CONNLOG_REFRESH || id == IDOK || id == IDCANCEL)
                {
                    DialogThemeHelper::DrawButton(pDrawItem, true);
                    return TRUE;
                }
            }
        }
        break;

    case WM_CTLCOLORDLG:
        if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HBRUSH hBrush = DialogThemeHelper::HandleControlColor(hdc, true);
            
            // Draw border around ListView
            DialogThemeHelper::DrawListViewBorder(hdc, m_hList, hDlg);
            
            return reinterpret_cast<INT_PTR>(hBrush);
        }
        break;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            return reinterpret_cast<INT_PTR>(DialogThemeHelper::HandleControlColor(hdc, true));
        }
        break;

    case WM_NOTIFY:
    {
        NMHDR* pnmh = reinterpret_cast<NMHDR*>(lParam);
        if (pnmh && pnmh->hwndFrom == m_hList && pnmh->code == NM_CUSTOMDRAW)
        {
            if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
            {
                NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
                switch (pLVCD->nmcd.dwDrawStage)
                {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                    
                case CDDS_ITEMPREPAINT:
                    return CDRF_NOTIFYPOSTPAINT;
                    
                case CDDS_ITEMPOSTPAINT:
                {
                    // Draw grid lines after each item
                    RECT rcItem;
                    ListView_GetItemRect(m_hList, static_cast<int>(pLVCD->nmcd.dwItemSpec), &rcItem, LVIR_BOUNDS);
                    
                    const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());
                    HDC hdc = pLVCD->nmcd.hdc;
                    HPEN hPen = CreatePen(PS_SOLID, 1, colors.chartGrid); // Subtle dark grid
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    // Draw horizontal line at bottom of item
                    MoveToEx(hdc, rcItem.left, rcItem.bottom - 1, nullptr);
                    LineTo(hdc, rcItem.right, rcItem.bottom - 1);
                    
                    // Draw vertical lines for each column
                    int numCols = Header_GetItemCount(ListView_GetHeader(m_hList));
                    int x = 0;
                    for (int i = 0; i < numCols; i++)
                    {
                        int width = ListView_GetColumnWidth(m_hList, i);
                        x += width;
                        MoveToEx(hdc, x - 1, rcItem.top, nullptr);
                        LineTo(hdc, x - 1, rcItem.bottom);
                    }
                    
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                    return CDRF_DODEFAULT;
                }
                }
            }
        }
        break;
    }

    case WM_CLOSE:
        KillTimer(hDlg, REFRESH_TIMER_ID);
        if (m_pExternalHandle)
        {
            *m_pExternalHandle = nullptr;
        }
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

void ConnectionLogDialog::InitializeDialog(HWND hDlg)
{
    m_hDialog = hDlg;
    
    // Center dialog on PRIMARY MONITOR
    CenterWindowOnScreen(hDlg);

    // Set dialog title
    std::wstring title = LoadStringResource(IDS_APP_TITLE);
    if (title.empty()) title = L"NetPulse";
    std::wstring titleSuffix = LoadStringResource(IDS_CONNLOG_TITLE_SUFFIX);
    if (titleSuffix.empty()) titleSuffix = L" - Connection Log";
    title += titleSuffix;
    SetWindowTextW(hDlg, title.c_str());

    // Determine theme from config (not system theme!)
    bool customTheme = (m_pConfig && IsCustomThemeEnabled(*m_pConfig));
    bool darkTheme = (m_pConfig && IsDarkThemeEnabled(*m_pConfig));
    
    // Enable dark mode for the window FIRST
    if (darkTheme)
    {
        ThemeHelper::AllowDarkModeForWindow(hDlg, true);
    }
    
    // Apply dark title bar
    ThemeHelper::ApplyDarkTitleBar(hDlg, darkTheme);
    
    // Apply thin window border in custom theme
    if (customTheme)
    {
        DialogThemeHelper::SetThinWindowBorder(hDlg);
    }
    
    // Get list view handle
    m_hList = GetDlgItem(hDlg, IDC_CONNLOG_LIST);
    if (!m_hList)
    {
        return;
    }
    
    // Initialize ListView
    InitializeListView();
    
    // Apply theme (after ListView is created and columns added)
    ApplyTheme(hDlg);
    
    // Populate initial data
    PopulateList();
    
    // Start auto-refresh timer
    SetTimer(hDlg, REFRESH_TIMER_ID, REFRESH_INTERVAL_MS, nullptr);
}

void ConnectionLogDialog::InitializeListView()
{
    if (!m_hList) return;
    
    // Set extended list view styles - Double buffering reduces flicker, gridlines show column borders
    ListView_SetExtendedListViewStyle(m_hList, 
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES);

    // Add columns
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;

    // Process Name
    lvc.iSubItem = 0;
    std::wstring colProcess = LoadStringResource(IDS_CONNLOG_COL_PROCESS);
    lvc.pszText = const_cast<LPWSTR>(colProcess.c_str());
    lvc.cx = 120;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(m_hList, 0, &lvc);

    // PID
    lvc.iSubItem = 1;
    std::wstring colPid = LoadStringResource(IDS_CONNLOG_COL_PID);
    lvc.pszText = const_cast<LPWSTR>(colPid.c_str());
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 1, &lvc);

    // Protocol
    lvc.iSubItem = 2;
    std::wstring colProtocol = LoadStringResource(IDS_CONNLOG_COL_PROTOCOL);
    lvc.pszText = const_cast<LPWSTR>(colProtocol.c_str());
    lvc.cx = 65;
    lvc.fmt = LVCFMT_CENTER;
    ListView_InsertColumn(m_hList, 2, &lvc);

    // Local Address
    lvc.iSubItem = 3;
    std::wstring colLocalAddr = LoadStringResource(IDS_CONNLOG_COL_LOCAL_ADDR);
    lvc.pszText = const_cast<LPWSTR>(colLocalAddr.c_str());
    lvc.cx = 100;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(m_hList, 3, &lvc);

    // Local Port
    lvc.iSubItem = 4;
    std::wstring colLocalPort = LoadStringResource(IDS_CONNLOG_COL_LOCAL_PORT);
    lvc.pszText = const_cast<LPWSTR>(colLocalPort.c_str());
    lvc.cx = 55;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 4, &lvc);

    // Remote Address
    lvc.iSubItem = 5;
    std::wstring colRemoteAddr = LoadStringResource(IDS_CONNLOG_COL_REMOTE_ADDR);
    lvc.pszText = const_cast<LPWSTR>(colRemoteAddr.c_str());
    lvc.cx = 100;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(m_hList, 5, &lvc);

    // Remote Port
    lvc.iSubItem = 6;
    std::wstring colRemotePort = LoadStringResource(IDS_CONNLOG_COL_REMOTE_PORT);
    lvc.pszText = const_cast<LPWSTR>(colRemotePort.c_str());
    lvc.cx = 55;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 6, &lvc);

    // State
    lvc.iSubItem = 7;
    std::wstring colState = LoadStringResource(IDS_CONNLOG_COL_STATE);
    lvc.pszText = const_cast<LPWSTR>(colState.c_str());
    lvc.cx = 85;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(m_hList, 7, &lvc);
}

void ConnectionLogDialog::PopulateList()
{
    if (!m_hList || !m_pConnectionMonitor)
    {
        return;
    }
    
    // Get active connections
    m_connections = m_pConnectionMonitor->GetActiveConnections();
    
    // Disable redraw while updating to prevent flicker
    SendMessage(m_hList, WM_SETREDRAW, FALSE, 0);
    
    // Clear existing items
    ListView_DeleteAllItems(m_hList);
    
    // Add items
    for (size_t i = 0; i < m_connections.size(); i++)
    {
        const auto& conn = m_connections[i];
        
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(i);
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(conn.processName.c_str());
        int itemIndex = ListView_InsertItem(m_hList, &lvi);
        
        // PID
        std::wstring pidStr = std::to_wstring(conn.processId);
        ListView_SetItemText(m_hList, itemIndex, 1, const_cast<LPWSTR>(pidStr.c_str()));
        
        // Protocol
        ListView_SetItemText(m_hList, itemIndex, 2, const_cast<LPWSTR>(conn.protocol.c_str()));
        
        // Local Address
        ListView_SetItemText(m_hList, itemIndex, 3, const_cast<LPWSTR>(conn.localAddress.c_str()));
        
        // Local Port
        std::wstring localPortStr = std::to_wstring(conn.localPort);
        ListView_SetItemText(m_hList, itemIndex, 4, const_cast<LPWSTR>(localPortStr.c_str()));
        
        // Remote Address
        ListView_SetItemText(m_hList, itemIndex, 5, const_cast<LPWSTR>(conn.remoteAddress.c_str()));
        
        // Remote Port
        std::wstring remotePortStr = std::to_wstring(conn.remotePort);
        ListView_SetItemText(m_hList, itemIndex, 6, const_cast<LPWSTR>(remotePortStr.c_str()));
        
        // State
        ListView_SetItemText(m_hList, itemIndex, 7, const_cast<LPWSTR>(conn.state.c_str()));
    }
    
    // Re-enable redraw and invalidate
    SendMessage(m_hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hList, nullptr, TRUE);
    
    // Update window title with count
    if (m_hDialog)
    {
        std::wstring title = LoadStringResource(IDS_APP_TITLE);
        if (title.empty()) title = L"NetPulse";
        std::wstring titleSuffix = LoadStringResource(IDS_CONNLOG_TITLE_SUFFIX);
        if (titleSuffix.empty()) titleSuffix = L" - Connection Log";
        std::wstring connCountSuffix = LoadStringResource(IDS_CONNLOG_CONNECTIONS_COUNT);
        if (connCountSuffix.empty()) connCountSuffix = L" connections";
        title += titleSuffix + L" (" + std::to_wstring(m_connections.size()) + connCountSuffix + L")";
        SetWindowTextW(m_hDialog, title.c_str());
    }
}

void ConnectionLogDialog::RefreshData()
{
    PopulateList();
}

void ConnectionLogDialog::ApplyTheme(HWND hDlg) const
{
    bool customTheme = (m_pConfig && IsCustomThemeEnabled(*m_pConfig));
    bool useDarkScrollbar = (m_pConfig && IsDarkThemeEnabled(*m_pConfig));
    
    if (customTheme)
    {
        // Apply thin window border
        DialogThemeHelper::SetThinWindowBorder(hDlg);
        
        // Apply theme to list view (scrollbar dark/light based on IsDarkThemeEnabled)
        if (m_hList)
        {
            DialogThemeHelper::ApplyDarkListView(m_hList, useDarkScrollbar);
        }
        
        // Toggle BS_OWNERDRAW on buttons for custom theme
        DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_CONNLOG_REFRESH));
        DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDCANCEL));
    }
    // For SystemDefault/Light theme: use default Windows controls - no custom styling needed
}

} // namespace NetPulse
