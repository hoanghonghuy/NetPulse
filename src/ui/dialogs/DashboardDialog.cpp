#include "NetworkMonitor/DashboardDialog.h"
#include "NetworkMonitor/NetworkMonitor.h"
#include "NetworkMonitor/HistoryLogger.h"
#include "NetworkMonitor/HistoryDialog.h"
#include "NetworkMonitor/DialogThemeHelper.h"
#include "NetworkMonitor/Utils.h"
#include "NetworkMonitor/ThemeHelper.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <algorithm>
#include <sstream>
#include <ctime>

// Fix Windows macro conflicts
#undef max
#undef min

namespace NetworkMonitor
{

namespace
{
    // Property names for associating original WndProc and DashboardDialog
    // instance with the header control.
    const wchar_t* HEADER_OLDPROC_PROP = L"NM_DASHBOARD_HEADER_OLDPROC";
    const wchar_t* HEADER_THIS_PROP    = L"NM_DASHBOARD_HEADER_THIS";
}

DashboardDialog::DashboardDialog()
    : m_hDialog(nullptr)
    , m_pExternalHandle(nullptr)
    , m_pNetworkMonitor(nullptr)
    , m_pConfig(nullptr)
{
}

DashboardDialog::~DashboardDialog()
{
}

bool DashboardDialog::Show(HWND parentWindow, NetworkMonitorClass* networkMonitor, const AppConfig* config)
{
    if (!networkMonitor || !config)
    {
        return false;
    }

    m_pNetworkMonitor = networkMonitor;
    m_pConfig = config;

    // Create modal dialog
    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_DASHBOARD_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return (result == IDOK);
}

INT_PTR CALLBACK DashboardDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DashboardDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<DashboardDialog*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDialog = hDlg;
        // Update external handle storage for tracking
        if (pThis->m_pExternalHandle)
        {
            *pThis->m_pExternalHandle = hDlg;
        }
    }
    else
    {
        pThis = reinterpret_cast<DashboardDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK DashboardDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Mirror DashboardDialogProc initialization from main.cpp
            CenterDialogOnScreen(hDlg);

            std::wstring dashTitle = LoadStringResource(IDS_DASHBOARD_TITLE);
            if (!dashTitle.empty())
            {
                SetWindowTextW(hDlg, dashTitle.c_str());
            }

            // Apply dark title bar if enabled
            if (m_pConfig)
            {
                ThemeHelper::ApplyDarkTitleBar(hDlg, m_pConfig->darkTheme);
            }

            std::wstring todayLabel = LoadStringResource(IDS_DASHBOARD_LABEL_TODAY);
            if (!todayLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_TODAY, todayLabel.c_str());
            }

            std::wstring monthLabel = LoadStringResource(IDS_DASHBOARD_LABEL_THIS_MONTH);
            if (!monthLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_MONTH, monthLabel.c_str());
            }

            std::wstring dlLabel = LoadStringResource(IDS_DASHBOARD_LABEL_DOWNLOAD);
            if (!dlLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_DOWNLOAD_T, dlLabel.c_str());
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_DOWNLOAD_M, dlLabel.c_str());
            }

            std::wstring ulLabel = LoadStringResource(IDS_DASHBOARD_LABEL_UPLOAD);
            if (!ulLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_UPLOAD_T, ulLabel.c_str());
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_LABEL_UPLOAD_M, ulLabel.c_str());
            }

            // Localize button labels
            std::wstring manageText = LoadStringResource(IDS_DASHBOARD_BUTTON_MANAGE);
            if (!manageText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_MANAGE, manageText.c_str());
            }

            std::wstring refreshText = LoadStringResource(IDS_DASHBOARD_BUTTON_REFRESH);
            if (!refreshText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DASHBOARD_REFRESH, refreshText.c_str());
            }

            std::wstring closeText = LoadStringResource(IDS_DASHBOARD_BUTTON_CLOSE);
            if (!closeText.empty())
            {
                SetDlgItemTextW(hDlg, IDOK, closeText.c_str());
            }

            // Initialize list columns once
            HWND hList = GetDlgItem(hDlg, IDC_RECENT_LIST);
            if (hList)
            {
                ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

                LVCOLUMNW col = {};
                col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;

                std::wstring timeHeader = LoadStringResource(IDS_DASHBOARD_COL_TIME);
                if (timeHeader.empty())
                {
                    timeHeader = L"Time";
                }
                col.pszText = const_cast<wchar_t*>(timeHeader.c_str());
                col.cx = 135;
                col.iSubItem = 0;
                col.fmt = LVCFMT_LEFT;
                ListView_InsertColumn(hList, 0, &col);

                std::wstring ifaceHeader = LoadStringResource(IDS_DASHBOARD_COL_INTERFACE);
                if (ifaceHeader.empty())
                {
                    ifaceHeader = L"Interface";
                }
                col.pszText = const_cast<wchar_t*>(ifaceHeader.c_str());
                col.cx = 110;
                col.iSubItem = 1;
                col.fmt = LVCFMT_LEFT;
                ListView_InsertColumn(hList, 1, &col);

                std::wstring downHeader = LoadStringResource(IDS_DASHBOARD_COL_DOWN);
                if (downHeader.empty())
                {
                    downHeader = L"Down";
                }
                col.pszText = const_cast<wchar_t*>(downHeader.c_str());
                col.cx = 85;
                col.iSubItem = 2;
                col.fmt = LVCFMT_RIGHT;
                ListView_InsertColumn(hList, 2, &col);

                std::wstring upHeader = LoadStringResource(IDS_DASHBOARD_COL_UP);
                if (upHeader.empty())
                {
                    upHeader = L"Up";
                }
                col.pszText = const_cast<wchar_t*>(upHeader.c_str());
                col.cx = 85;
                col.iSubItem = 3;
                col.fmt = LVCFMT_RIGHT;
                ListView_InsertColumn(hList, 3, &col);

                if (m_pConfig && m_pConfig->darkTheme)
                {
                    ListView_SetBkColor(hList, RGB(24, 24, 24));
                    ListView_SetTextBkColor(hList, RGB(24, 24, 24));
                    ListView_SetTextColor(hList, DialogThemeHelper::DARK_TEXT);

                    // Apply dark theme with dark scrollbars
                    ThemeHelper::ApplyDarkThemeToControl(hList, true);

                    // Subclass the list header so we can paint it fully dark.
                    HWND hHeader = ListView_GetHeader(hList);
                    if (hHeader)
                    {
                        WNDPROC oldProc = reinterpret_cast<WNDPROC>(
                            GetWindowLongPtrW(hHeader, GWLP_WNDPROC));
                        SetPropW(hHeader, HEADER_OLDPROC_PROP,
                                 reinterpret_cast<HANDLE>(oldProc));
                        SetPropW(hHeader, HEADER_THIS_PROP,
                                 reinterpret_cast<HANDLE>(this));

                        SetWindowLongPtrW(hHeader, GWLP_WNDPROC,
                                          reinterpret_cast<LONG_PTR>(HeaderWndProc));

                        // Apply dark theme to header as well
                        ThemeHelper::ApplyDarkThemeToControl(hHeader, true);
                    }

                }
            }

            // For dark theme, make bottom buttons owner-drawn so we can
            // render dark backgrounds consistently.
            if (m_pConfig && m_pConfig->darkTheme)
            {
                auto makeOwnerDraw = [](HWND hButton)
                {
                    if (!hButton) return;
                    LONG_PTR style = GetWindowLongPtrW(hButton, GWL_STYLE);
                    if ((style & BS_OWNERDRAW) == 0)
                    {
                        style &= ~BS_TYPEMASK;
                        style |= BS_OWNERDRAW;
                        SetWindowLongPtrW(hButton, GWL_STYLE, style);

                        // Disable visual styles so themed drawing does not
                        // override our owner-draw dark appearance.
                        SetWindowTheme(hButton, L"", L"");

                        InvalidateRect(hButton, nullptr, TRUE);
                        UpdateWindow(hButton);
                    }
                };

                makeOwnerDraw(GetDlgItem(hDlg, IDC_HISTORY_MANAGE));
                makeOwnerDraw(GetDlgItem(hDlg, IDC_DASHBOARD_REFRESH));
                makeOwnerDraw(GetDlgItem(hDlg, IDOK));

                // Clear default button so the system does not try to paint
                // a default highlight using the classic white style before
                // our owner-draw logic runs.
                SendMessageW(hDlg, DM_SETDEFID, 0, 0);
            }

            // Fill data
            PostMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DASHBOARD_REFRESH, 0), 0);
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_DASHBOARD_REFRESH:
                {
                    UpdateDashboardData(hDlg);

                    HWND hChart = GetDlgItem(hDlg, IDC_DASHBOARD_CHART);
                    if (hChart)
                    {
                        InvalidateRect(hChart, nullptr, TRUE);
                    }
                    return TRUE;
                }

                case IDC_HISTORY_MANAGE:
                {
                    // Use HistoryDialog class instead of legacy main.cpp dialog
                    HistoryDialog dlg;
                    dlg.Show(hDlg, m_pConfig);
                    // After user modifies history, refresh dashboard view
                    PostMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DASHBOARD_REFRESH, 0), 0);
                    return TRUE;
                }

                case IDOK:
                case IDCANCEL:
                {
                    // Clear external handle storage before closing
                    if (m_pExternalHandle)
                    {
                        *m_pExternalHandle = nullptr;
                    }
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }
            }
            break;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            if (m_pConfig && m_pConfig->darkTheme)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                HBRUSH darkBrush = DialogThemeHelper::GetDarkBackgroundBrush();

                SetTextColor(hdc, DialogThemeHelper::DARK_TEXT);
                SetBkMode(hdc, TRANSPARENT);

                return reinterpret_cast<INT_PTR>(darkBrush);
            }
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);

            // Owner-draw chart (existing behavior)
            if (wParam == IDC_DASHBOARD_CHART && pDrawItem->CtlType == ODT_STATIC)
            {
                HDC hdc = pDrawItem->hDC;
                RECT rc = pDrawItem->rcItem;
                DrawDashboardChart(hdc, rc);
                return TRUE;
            }

            // Owner-draw bottom buttons in dark theme
            if (m_pConfig && m_pConfig->darkTheme && pDrawItem->CtlType == ODT_BUTTON)
            {
                UINT id = pDrawItem->CtlID;
                if (id == IDC_HISTORY_MANAGE || id == IDC_DASHBOARD_REFRESH || id == IDOK)
                {
                    HDC hdc = pDrawItem->hDC;
                    RECT rc = pDrawItem->rcItem;

                    bool pressed = (pDrawItem->itemState & ODS_SELECTED) != 0;
                    bool focused = (pDrawItem->itemState & ODS_FOCUS) != 0;
                    bool disabled = (pDrawItem->itemState & ODS_DISABLED) != 0;

                    COLORREF backColor = pressed ? RGB(50, 50, 50) : RGB(40, 40, 40);
                    COLORREF borderColor = RGB(90, 90, 90);
                    COLORREF textColor = disabled ? RGB(160, 160, 160) : DialogThemeHelper::DARK_TEXT;

                    // Fill background
                    HBRUSH hBrush = CreateSolidBrush(backColor);
                    FillRect(hdc, &rc, hBrush);
                    DeleteObject(hBrush);

                    // Draw border only, keep dark background
                    HBRUSH hBorder = CreateSolidBrush(borderColor);
                    FrameRect(hdc, &rc, hBorder);
                    DeleteObject(hBorder);

                    // Draw button text
                    wchar_t text[128] = {0};
                    GetWindowTextW(pDrawItem->hwndItem, text, static_cast<int>(std::size(text)));

                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, textColor);

                    RECT textRc = rc;
                    InflateRect(&textRc, -4, -2);
                    DrawTextW(hdc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    // Focus rectangle
                    if (focused)
                    {
                        RECT focusRc = rc;
                        InflateRect(&focusRc, -3, -3);
                        DrawFocusRect(hdc, &focusRc);
                    }

                    return TRUE;
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            // Currently unused; header is subclassed directly in dark theme.
            break;
        }
    }

    return FALSE;
}

void DashboardDialog::UpdateDashboardData(HWND hDlg)
{
    unsigned long long todayDown = 0;
    unsigned long long todayUp = 0;
    unsigned long long monthDown = 0;
    unsigned long long monthUp = 0;

    HistoryLogger& logger = HistoryLogger::Instance();
    const std::wstring* ifaceFilter = nullptr;

    if (m_pConfig && !m_pConfig->selectedInterface.empty())
    {
        ifaceFilter = &m_pConfig->selectedInterface;
    }

    logger.GetTotalsToday(todayDown, todayUp, ifaceFilter);
    logger.GetTotalsThisMonth(monthDown, monthUp, ifaceFilter);

    std::wstring todayDownStr = FormatBytes(static_cast<ULONG64>(todayDown));
    std::wstring todayUpStr = FormatBytes(static_cast<ULONG64>(todayUp));
    std::wstring monthDownStr = FormatBytes(static_cast<ULONG64>(monthDown));
    std::wstring monthUpStr = FormatBytes(static_cast<ULONG64>(monthUp));

    SetDlgItemTextW(hDlg, IDC_TODAY_DOWN, todayDownStr.c_str());
    SetDlgItemTextW(hDlg, IDC_TODAY_UP, todayUpStr.c_str());
    SetDlgItemTextW(hDlg, IDC_MONTH_DOWN, monthDownStr.c_str());
    SetDlgItemTextW(hDlg, IDC_MONTH_UP, monthUpStr.c_str());

    // Populate recent samples list
    HWND hList = GetDlgItem(hDlg, IDC_RECENT_LIST);
    if (hList)
    {
        ListView_DeleteAllItems(hList);

        std::vector<HistorySample> samples;
        logger.GetRecentSamples(100, samples, ifaceFilter, true /*onlyToday*/);

        int index = 0;
        for (const auto& sample : samples)
        {
            wchar_t timeBuffer[64] = {};
            std::tm localTime = {};
            std::time_t ts = sample.timestamp;
            if (localtime_s(&localTime, &ts) == 0)
            {
                wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &localTime);
            }

            LVITEMW item = {};
            item.mask = LVIF_TEXT;
            item.iItem = index;
            item.iSubItem = 0;
            item.pszText = timeBuffer;
            int rowIndex = ListView_InsertItem(hList, &item);

            std::wstring iface = sample.interfaceName;
            if (iface.empty())
            {
                iface = LoadStringResource(IDS_ALL_INTERFACES);
                if (iface.empty())
                {
                    iface = L"All Interfaces";
                }
            }
            ListView_SetItemText(hList, rowIndex, 1, const_cast<wchar_t*>(iface.c_str()));

            std::wstring downStr = FormatBytes(static_cast<ULONG64>(sample.bytesDown));
            std::wstring upStr = FormatBytes(static_cast<ULONG64>(sample.bytesUp));

            ListView_SetItemText(hList, rowIndex, 2, const_cast<wchar_t*>(downStr.c_str()));
            ListView_SetItemText(hList, rowIndex, 3, const_cast<wchar_t*>(upStr.c_str()));

            ++index;
        }

        // Auto-size columns to contents
        ListView_SetColumnWidth(hList, 0, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(hList, 1, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(hList, 2, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(hList, 3, LVSCW_AUTOSIZE_USEHEADER);
    }
}

void DashboardDialog::DrawDashboardChart(HDC hdc, const RECT& rc)
{
    if (!hdc)
    {
        return;
    }

    bool darkTheme = (m_pConfig && m_pConfig->darkTheme);

    // Slightly lighter background for dark theme so the chart does not
    // appear too black compared to the rest of the dialog.
    COLORREF backColor = darkTheme ? RGB(28, 28, 28) : GetSysColor(COLOR_WINDOW);
    COLORREF borderColor = darkTheme ? RGB(80, 80, 80) : RGB(200, 200, 200);
    COLORREF downColor = darkTheme ? RGB(80, 200, 120) : RGB(0, 128, 0);
    COLORREF upColor = darkTheme ? RGB(80, 160, 240) : RGB(0, 0, 200);

    HBRUSH backBrush = CreateSolidBrush(backColor);
    FillRect(hdc, &rc, backBrush);
    DeleteObject(backBrush);

    HistoryLogger& logger = HistoryLogger::Instance();
    const std::wstring* ifaceFilter = nullptr;
    if (m_pConfig && !m_pConfig->selectedInterface.empty())
    {
        ifaceFilter = &m_pConfig->selectedInterface;
    }

    std::vector<HistorySample> samples;
    logger.GetRecentSamples(100, samples, ifaceFilter, true);

    if (samples.empty())
    {
        HBRUSH frameBrush = CreateSolidBrush(borderColor);
        FrameRect(hdc, &rc, frameBrush);
        DeleteObject(frameBrush);
        return;
    }

    unsigned long long maxValue = 0;
    for (const auto& s : samples)
    {
        maxValue = (std::max)(maxValue, s.bytesDown);
        maxValue = (std::max)(maxValue, s.bytesUp);
    }

    if (maxValue == 0)
    {
        HBRUSH frameBrush = CreateSolidBrush(borderColor);
        FrameRect(hdc, &rc, frameBrush);
        DeleteObject(frameBrush);
        return;
    }

    RECT inner = rc;
    inner.left += 4;
    inner.right -= 4;
    inner.top += 4;
    inner.bottom -= 4;

    // Draw only the border of the chart area, keeping the dark
    // background that we already filled above.
    HBRUSH frameBrush = CreateSolidBrush(borderColor);
    FrameRect(hdc, &inner, frameBrush);
    DeleteObject(frameBrush);

    int width = inner.right - inner.left;
    int height = inner.bottom - inner.top;
    size_t count = samples.size();

    if (count == 0 || width <= 0 || height <= 0)
    {
        return;
    }

    auto getX = [&](size_t index) -> int {
        if (count <= 1)
        {
            return inner.left + width / 2;
        }
        return inner.left + static_cast<int>((static_cast<double>(index) * static_cast<double>(width - 1)) / static_cast<double>(count - 1));
    };

    auto getY = [&](unsigned long long value) -> int {
        double ratio = static_cast<double>(value) / static_cast<double>(maxValue);
        if (ratio < 0.0)
        {
            ratio = 0.0;
        }
        if (ratio > 1.0)
        {
            ratio = 1.0;
        }
        return inner.bottom - static_cast<int>(ratio * static_cast<double>(height));
    };

    std::vector<POINT> downPoints;
    std::vector<POINT> upPoints;
    downPoints.reserve(count);
    upPoints.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        const auto& s = samples[count - 1 - i];
        POINT pd;
        pd.x = getX(i);
        pd.y = getY(s.bytesDown);
        downPoints.push_back(pd);

        POINT pu;
        pu.x = pd.x;
        pu.y = getY(s.bytesUp);
        upPoints.push_back(pu);
    }

    HPEN downPen = CreatePen(PS_SOLID, 1, downColor);
    HPEN upPen = CreatePen(PS_SOLID, 1, upColor);

    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, downPen));
    if (!downPoints.empty())
    {
        MoveToEx(hdc, downPoints[0].x, downPoints[0].y, nullptr);
        for (size_t i = 1; i < downPoints.size(); ++i)
        {
            LineTo(hdc, downPoints[i].x, downPoints[i].y);
        }
    }

    SelectObject(hdc, upPen);
    if (!upPoints.empty())
    {
        MoveToEx(hdc, upPoints[0].x, upPoints[0].y, nullptr);
        for (size_t i = 1; i < upPoints.size(); ++i)
        {
            LineTo(hdc, upPoints[i].x, upPoints[i].y);
        }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(downPen);
    DeleteObject(upPen);
}

void DashboardDialog::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
}

LRESULT CALLBACK DashboardDialog::HeaderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = reinterpret_cast<WNDPROC>(GetPropW(hwnd, HEADER_OLDPROC_PROP));
    DashboardDialog* pThis = reinterpret_cast<DashboardDialog*>(
        GetPropW(hwnd, HEADER_THIS_PROP));

    if (msg == WM_NCDESTROY)
    {
        if (oldProc)
        {
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oldProc));
        }
        RemovePropW(hwnd, HEADER_OLDPROC_PROP);
        RemovePropW(hwnd, HEADER_THIS_PROP);

        return oldProc ? CallWindowProcW(oldProc, hwnd, msg, wParam, lParam)
                       : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    // If we don't have a dialog instance or config, fall back to original proc
    if (!(pThis && pThis->m_pConfig && pThis->m_pConfig->darkTheme))
    {
        return oldProc ? CallWindowProcW(oldProc, hwnd, msg, wParam, lParam)
                       : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_ERASEBKGND:
        // We'll handle background in WM_PAINT
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (!hdc)
        {
            break;
        }

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Fill entire header background dark
        HBRUSH hBack = CreateSolidBrush(DialogThemeHelper::DARK_BACKGROUND);
        FillRect(hdc, &rcClient, hBack);
        DeleteObject(hBack);

        int count = static_cast<int>(SendMessageW(hwnd, HDM_GETITEMCOUNT, 0, 0));
        for (int i = 0; i < count; ++i)
        {
            RECT rcItem;
            if (!SendMessageW(hwnd, HDM_GETITEMRECT, static_cast<WPARAM>(i),
                              reinterpret_cast<LPARAM>(&rcItem)))
            {
                continue;
            }

            WCHAR text[128] = {0};
            HDITEMW item = {};
            item.mask = HDI_TEXT | HDI_FORMAT;
            item.pszText = text;
            item.cchTextMax = static_cast<int>(sizeof(text) / sizeof(text[0]));

            if (!SendMessageW(hwnd, HDM_GETITEMW, static_cast<WPARAM>(i),
                              reinterpret_cast<LPARAM>(&item)))
            {
                continue;
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, DialogThemeHelper::DARK_TEXT);

            RECT rcText = rcItem;
            rcText.left += 4;

            UINT align = DT_SINGLELINE | DT_VCENTER;
            if (item.fmt & HDF_CENTER)
            {
                align |= DT_CENTER;
            }
            else if (item.fmt & HDF_RIGHT)
            {
                align |= DT_RIGHT;
            }
            else
            {
                align |= DT_LEFT;
            }

            DrawTextW(hdc, text, -1, &rcText, align);
        }

        // Bottom border line
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(90, 90, 90));
        HPEN hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, hPen));
        MoveToEx(hdc, rcClient.left, rcClient.bottom - 1, nullptr);
        LineTo(hdc, rcClient.right, rcClient.bottom - 1);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }

    return oldProc ? CallWindowProcW(oldProc, hwnd, msg, wParam, lParam)
                   : DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace NetworkMonitor
