#include "NetPulse/PerAppDialog.h"
#include "NetPulse/Utils.h"
#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/ThemeHelper.h"
#include "../../../resources/resource.h"
#include <commctrl.h>
#include <windowsx.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace NetPulse
{

PerAppDialog::PerAppDialog()
    : m_hList(nullptr)
    , m_hImageList(nullptr)
    , m_pConfig(nullptr)
    , m_pExternalHandle(nullptr)
    , m_pMonitor(std::make_unique<PerAppMonitor>())
{
}

PerAppDialog::~PerAppDialog()
{
    if (m_hImageList)
    {
        ImageList_Destroy(m_hImageList);
        m_hImageList = nullptr;
    }
}

INT_PTR PerAppDialog::Show(HWND parentWindow, const AppConfig* config)
{
    m_pConfig = config;
    return DialogBoxParamW(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_PERAPP_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );
}

INT_PTR CALLBACK PerAppDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PerAppDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<PerAppDialog*>(lParam);
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        
        // Update external handle storage for tracking
        if (pThis && pThis->m_pExternalHandle)
        {
            *pThis->m_pExternalHandle = hDlg;
        }
    }
    else
    {
        pThis = reinterpret_cast<PerAppDialog*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR PerAppDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    (void)lParam; // Unused in this dialog
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
            // Clear external handle storage before closing
            if (m_pExternalHandle)
            {
                *m_pExternalHandle = nullptr;
            }
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;

        case IDC_PERAPP_REFRESH:
            RefreshData(hDlg);
            return TRUE;
        }
        break;

    case WM_DRAWITEM:
        if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
        {
            DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pDrawItem->CtlType == ODT_BUTTON)
            {
                UINT id = pDrawItem->CtlID;
                if (id == IDC_PERAPP_REFRESH || id == IDOK || id == IDCANCEL)
                {
                    DialogThemeHelper::DrawButton(pDrawItem, true);

                    return TRUE;
                }
            }
        }
        break;

    case WM_DESTROY:
        // Stop ETW monitoring when dialog closes to save RAM
        if (m_pMonitor)
        {
            m_pMonitor->EnableEtw(false);
        }
        return TRUE;

    case WM_CTLCOLORDLG:
        if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HBRUSH hBrush = DialogThemeHelper::HandleControlColor(hdc, true);
            
            // Draw border around ListView (replaced system border)
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
    }

    return FALSE;
}

void PerAppDialog::InitializeDialog(HWND hDlg)
{
    // Center dialog on PRIMARY MONITOR
    CenterWindowOnScreen(hDlg);

    // Determine theme from config (not system theme!)
    bool customTheme = (m_pConfig && IsCustomThemeEnabled(*m_pConfig));
    bool darkTheme = (m_pConfig && IsDarkThemeEnabled(*m_pConfig));
    
    // Enable dark mode for the window FIRST (required before DwmSetWindowAttribute)
    // Only strictly needed for actual Dark Mode - light themes use light title bar
    if (darkTheme)
    {
        ThemeHelper::AllowDarkModeForWindow(hDlg, true);
    }
    
    // Apply dark title bar only for dark themes
    ThemeHelper::ApplyDarkTitleBar(hDlg, darkTheme);
    
    // Apply DialogThemeHelper for consistent theming with Settings/Dashboard
    if (customTheme) {
        DialogThemeHelper::SetThinWindowBorder(hDlg);
    }
    
    // Get list view handle
    m_hList = GetDlgItem(hDlg, IDC_PERAPP_LIST);
    if (!m_hList)
    {
        return;
    }

    // Set extended list view styles - Remove GRIDLINES for cleaner modern look
    ListView_SetExtendedListViewStyle(m_hList, 
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);

    // Create image list for icons
    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 10, 10);
    ListView_SetImageList(m_hList, m_hImageList, LVSIL_SMALL);




    // Add columns
    LVCOLUMNW lvc = {};
    lvc.fmt = LVCFMT_LEFT;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT; // Added LVCF_FMT

    lvc.iSubItem = 0;
    std::wstring colApp = LoadStringResource(IDS_PERAPP_COL_APPLICATION);
    lvc.pszText = const_cast<LPWSTR>(colApp.c_str());
    lvc.cx = 160;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(m_hList, 0, &lvc);

    lvc.iSubItem = 1;
    std::wstring colPid = LoadStringResource(IDS_PERAPP_COL_PID);
    lvc.pszText = const_cast<LPWSTR>(colPid.c_str());
    lvc.cx = 55;
    lvc.fmt = LVCFMT_RIGHT; // Numeric -> Right
    ListView_InsertColumn(m_hList, 1, &lvc);

    lvc.iSubItem = 2;
    std::wstring colTcp = LoadStringResource(IDS_PERAPP_COL_TCP);
    lvc.pszText = const_cast<LPWSTR>(colTcp.c_str());
    lvc.cx = 45;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 2, &lvc);

    lvc.iSubItem = 3;
    std::wstring colUdp = LoadStringResource(IDS_PERAPP_COL_UDP);
    lvc.pszText = const_cast<LPWSTR>(colUdp.c_str());
    lvc.cx = 45;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 3, &lvc);

    lvc.iSubItem = 4;
    std::wstring colSent = LoadStringResource(IDS_PERAPP_COL_SENT);
    lvc.pszText = const_cast<LPWSTR>(colSent.c_str());
    lvc.cx = 80;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 4, &lvc);

    lvc.iSubItem = 5;
    std::wstring colReceived = LoadStringResource(IDS_PERAPP_COL_RECEIVED);
    lvc.pszText = const_cast<LPWSTR>(colReceived.c_str());
    lvc.cx = 80;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hList, 5, &lvc);

    // NOW apply theme (after ListView is created and columns added)
    ApplyTheme(hDlg);
    
    // Subclass header for custom dark mode painting


    // Initialize monitor
    m_pMonitor->Initialize();
    
    // Start ETW monitoring (will auto-stop when dialog closes via WM_DESTROY)
    m_pMonitor->EnableEtw(true);
    
    RefreshData(hDlg);
}

void PerAppDialog::PopulateList(HWND hDlg)
{
    (void)hDlg; // Only use m_hList member
    if (!m_hList)
    {
        return;
    }

    // Clear existing items
    ListView_DeleteAllItems(m_hList);
    ImageList_RemoveAll(m_hImageList);

    const auto& appUsage = m_pMonitor->GetAppUsage();
    
    for (size_t i = 0; i < appUsage.size(); i++)
    {
        const auto& app = appUsage[i];

        // Add icon to image list
        int imageIndex = -1;
        if (app.processIcon)
        {
            imageIndex = ImageList_AddIcon(m_hImageList, app.processIcon);
        }

        // Insert item
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem = static_cast<int>(i);
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(app.processName.c_str());
        lvi.iImage = imageIndex;
        int itemIndex = ListView_InsertItem(m_hList, &lvi);

        // Set subitems
        wchar_t buf[32];

        // PID
        swprintf_s(buf, L"%u", app.processId);
        ListView_SetItemText(m_hList, itemIndex, 1, buf);

        // TCP connections
        swprintf_s(buf, L"%d", app.tcpConnections);
        ListView_SetItemText(m_hList, itemIndex, 2, buf);

        // UDP connections
        swprintf_s(buf, L"%d", app.udpConnections);
        ListView_SetItemText(m_hList, itemIndex, 3, buf);

        // Bytes Sent
        if (app.bytesSent > 0)
        {
            if (app.bytesSent >= 1024 * 1024 * 1024)
                swprintf_s(buf, L"%.1f GB", app.bytesSent / (1024.0 * 1024.0 * 1024.0));
            else if (app.bytesSent >= 1024 * 1024)
                swprintf_s(buf, L"%.1f MB", app.bytesSent / (1024.0 * 1024.0));
            else if (app.bytesSent >= 1024)
                swprintf_s(buf, L"%.1f KB", app.bytesSent / 1024.0);
            else
                swprintf_s(buf, L"%llu B", app.bytesSent);
        }
        else
        {
            wcscpy_s(buf, L"0 B");
        }
        ListView_SetItemText(m_hList, itemIndex, 4, buf);

        // Bytes Received
        if (app.bytesReceived > 0)
        {
            if (app.bytesReceived >= 1024 * 1024 * 1024)
                swprintf_s(buf, L"%.1f GB", app.bytesReceived / (1024.0 * 1024.0 * 1024.0));
            else if (app.bytesReceived >= 1024 * 1024)
                swprintf_s(buf, L"%.1f MB", app.bytesReceived / (1024.0 * 1024.0));
            else if (app.bytesReceived >= 1024)
                swprintf_s(buf, L"%.1f KB", app.bytesReceived / 1024.0);
            else
                swprintf_s(buf, L"%llu B", app.bytesReceived);
        }
        else
        {
            wcscpy_s(buf, L"0 B");
        }
        ListView_SetItemText(m_hList, itemIndex, 5, buf);
    }
}

void PerAppDialog::RefreshData(HWND hDlg)
{
    m_pMonitor->Refresh();
    PopulateList(hDlg);
}

void PerAppDialog::ApplyTheme(HWND hDlg) const
{
    bool customTheme = (m_pConfig && IsCustomThemeEnabled(*m_pConfig));
    bool useDarkScrollbar = (m_pConfig && IsDarkThemeEnabled(*m_pConfig));
    
    if (customTheme)
    {
        // Apply thin window border (fixes white frame issue)
        DialogThemeHelper::SetThinWindowBorder(hDlg);
        
        // Apply theme to list view (scrollbar dark/light based on IsDarkThemeEnabled)
        if (m_hList)
        {
            DialogThemeHelper::ApplyDarkListView(m_hList, useDarkScrollbar);
        }
        
        // Toggle BS_OWNERDRAW on buttons for custom theme
        DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDOK));
        DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_PERAPP_REFRESH));
        DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDCANCEL));
    }
    // For SystemDefault/Light theme: use default Windows controls - no custom styling needed
}


} // namespace NetPulse
