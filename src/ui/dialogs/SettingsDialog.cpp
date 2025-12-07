#include "NetworkMonitor/SettingsDialog.h"
#include "NetworkMonitor/Interfaces/IConfigProvider.h"
#include "NetworkMonitor/Interfaces/INetworkStatsProvider.h"
#include "NetworkMonitor/DialogThemeHelper.h"
#include "NetworkMonitor/Utils.h"
#include "NetworkMonitor/ThemeHelper.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <vector>

namespace NetworkMonitor
{

// Tab Control subclass procedure for dark theme
static WNDPROC s_originalTabProc = nullptr;
static LRESULT CALLBACK DarkTabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_ERASEBKGND)
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(DialogThemeHelper::DARK_BACKGROUND);
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        return TRUE;
    }
    return CallWindowProc(s_originalTabProc, hwnd, msg, wParam, lParam);
}

// ComboBox subclass procedure for dark dropdown button
static const wchar_t* COMBOBOX_OLDPROC_PROP = L"DarkComboOldProc";
static LRESULT CALLBACK DarkComboBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = reinterpret_cast<WNDPROC>(GetPropW(hwnd, COMBOBOX_OLDPROC_PROP));

    if (msg == WM_PAINT)
    {
        // Let the default paint happen first
        LRESULT result = CallWindowProcW(oldProc, hwnd, msg, wParam, lParam);

        // Now draw over the dropdown button with dark theme
        COMBOBOXINFO cbi = {0};
        cbi.cbSize = sizeof(COMBOBOXINFO);
        if (GetComboBoxInfo(hwnd, &cbi))
        {
            HDC hdc = GetDC(hwnd);
            if (hdc)
            {
                RECT btnRect = cbi.rcButton;

                // Draw dark button background
                COLORREF btnBg = RGB(50, 50, 50);
                COLORREF btnBorder = RGB(80, 80, 80);
                COLORREF arrowColor = RGB(200, 200, 200);

                HBRUSH hBrush = CreateSolidBrush(btnBg);
                FillRect(hdc, &btnRect, hBrush);
                DeleteObject(hBrush);

                // Draw border
                HBRUSH hBorder = CreateSolidBrush(btnBorder);
                FrameRect(hdc, &btnRect, hBorder);
                DeleteObject(hBorder);

                // Draw dropdown arrow (triangle pointing down)
                int cx = (btnRect.left + btnRect.right) / 2;
                int cy = (btnRect.top + btnRect.bottom) / 2;

                POINT arrow[3] = {
                    {cx - 4, cy - 2},
                    {cx + 4, cy - 2},
                    {cx, cy + 3}
                };

                HBRUSH arrowBrush = CreateSolidBrush(arrowColor);
                HPEN arrowPen = CreatePen(PS_SOLID, 1, arrowColor);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, arrowBrush);
                HPEN oldPen = (HPEN)SelectObject(hdc, arrowPen);

                Polygon(hdc, arrow, 3);

                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(arrowBrush);
                DeleteObject(arrowPen);

                ReleaseDC(hwnd, hdc);
            }
        }
        return result;
    }

    if (msg == WM_NCDESTROY)
    {
        if (oldProc)
        {
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oldProc));
        }
        RemovePropW(hwnd, COMBOBOX_OLDPROC_PROP);
    }

    return oldProc ? CallWindowProcW(oldProc, hwnd, msg, wParam, lParam)
                   : DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Shared color presets for overlay colors
struct OverlayColorPreset { COLORREF down; COLORREF up; const wchar_t* label; };
static const OverlayColorPreset s_overlayColorPresets[] = {
    {RGB(0, 255, 255), RGB(0, 255, 0), L"Cyan/Green (Default)"},
    {RGB(50, 255, 100), RGB(255, 180, 50), L"Green/Orange"},
    {RGB(100, 200, 255), RGB(255, 100, 100), L"Blue/Red"},
    {RGB(255, 255, 255), RGB(200, 200, 200), L"White/Gray"},
};
static const int s_overlayColorPresetCount = 4;

SettingsDialog::SettingsDialog()
    : m_hDialog(nullptr)
    , m_pConfigProvider(nullptr)
    , m_pStatsProvider(nullptr)
    , m_isInitializing(false)
{
}

SettingsDialog::~SettingsDialog()
{
}

INT_PTR SettingsDialog::Show(HWND parentWindow, IConfigProvider* configProvider, INetworkStatsProvider* statsProvider)
{
    if (!configProvider)
    {
        return IDCANCEL;
    }

    m_pConfigProvider = configProvider;
    m_pStatsProvider = statsProvider;
    m_isInitializing = true;

    // Load current config into working copy
    if (!m_pConfigProvider->LoadConfig(m_configCopy))
    {
        m_configCopy = AppConfig(); // Use defaults if load fails
    }

    // Create modal dialog
    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    m_isInitializing = false;
    return result;
}

void SettingsDialog::SetSettingsChangedCallback(std::function<void()> callback)
{
    m_settingsChangedCallback = callback;
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SettingsDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<SettingsDialog*>(lParam);
        SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hDialog = hDlg;
    }
    else
    {
        pThis = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK SettingsDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Localize dialog caption and static controls using current resources
            std::wstring title = LoadStringResource(IDS_SETTINGS_TITLE);
            if (!title.empty())
            {
                SetWindowTextW(hDlg, title.c_str());
            }

            // Apply dark title bar if enabled
            ThemeHelper::ApplyDarkTitleBar(hDlg, m_configCopy.darkTheme);

            std::wstring generalText = LoadStringResource(IDS_SETTINGS_GROUP_GENERAL);
            if (!generalText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_GENERAL, generalText.c_str());
            }

            std::wstring updateText = LoadStringResource(IDS_SETTINGS_GROUP_UPDATE);
            if (!updateText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_UPDATE, updateText.c_str());
            }

            std::wstring networkText = LoadStringResource(IDS_SETTINGS_GROUP_NETWORK);
            if (!networkText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_NETWORK, networkText.c_str());
            }

            std::wstring langText = LoadStringResource(IDS_SETTINGS_LABEL_LANGUAGE);
            if (!langText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_LANGUAGE, langText.c_str());
            }

            std::wstring intervalText = LoadStringResource(IDS_SETTINGS_LABEL_INTERVAL);
            if (!intervalText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_INTERVAL, intervalText.c_str());
            }

            std::wstring monitorText = LoadStringResource(IDS_SETTINGS_LABEL_MONITOR);
            if (!monitorText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_MONITOR, monitorText.c_str());
            }

            std::wstring autostartText = LoadStringResource(IDS_SETTINGS_LABEL_AUTOSTART);
            if (!autostartText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_AUTOSTART_CHECK, autostartText.c_str());
            }

            std::wstring loggingText = LoadStringResource(IDS_SETTINGS_LABEL_LOGGING);
            if (!loggingText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_ENABLE_LOGGING_CHECK, loggingText.c_str());
            }

            std::wstring debugLogText = LoadStringResource(IDS_SETTINGS_LABEL_DEBUGLOGGING);
            if (!debugLogText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DEBUG_LOGGING_CHECK, debugLogText.c_str());
            }

            std::wstring connectionNotifyText = LoadStringResource(IDS_SETTINGS_LABEL_CONNECTION_NOTIFY);
            if (!connectionNotifyText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_CONNECTION_NOTIFY_CHECK, connectionNotifyText.c_str());
            }

            std::wstring darkThemeText = LoadStringResource(IDS_SETTINGS_LABEL_DARK_THEME);
            if (!darkThemeText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_THEME, darkThemeText.c_str());
            }

            std::wstring unitLabelText = LoadStringResource(IDS_SETTINGS_LABEL_SPEED_UNIT);
            if (!unitLabelText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_SPEED_UNIT, unitLabelText.c_str());
            }

            std::wstring trimLabelText = LoadStringResource(IDS_SETTINGS_LABEL_AUTOTRIM);
            if (!trimLabelText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_AUTOTRIM, trimLabelText.c_str());
            }

            std::wstring openLogText = LoadStringResource(IDS_SETTINGS_BUTTON_OPEN_LOG);
            if (!openLogText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_BUTTON_OPEN_LOG, openLogText.c_str());
            }

            // Localize button labels
            std::wstring okText = LoadStringResource(IDS_BUTTON_OK);
            if (!okText.empty())
            {
                SetDlgItemTextW(hDlg, IDOK, okText.c_str());
            }

            std::wstring cancelText = LoadStringResource(IDS_BUTTON_CANCEL);
            if (!cancelText.empty())
            {
                SetDlgItemTextW(hDlg, IDCANCEL, cancelText.c_str());
            }

            std::wstring applyText = LoadStringResource(IDS_BUTTON_APPLY);
            if (!applyText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_BUTTON_APPLY, applyText.c_str());
            }

            // Populate dialog controls
            PopulateDialog(hDlg);
            CenterDialogOnScreen(hDlg);

            // Initialize Tab Control
            InitializeTabControl(hDlg);
            SwitchTab(hDlg, 0); // Show General tab by default

            // In dark theme, disable visual styles on tab control
            if (m_configCopy.darkTheme)
            {
                HWND hTab = GetDlgItem(hDlg, IDC_SETTINGS_TAB);
                if (hTab)
                {
                    SetWindowTheme(hTab, L"", L"");
                    // Make tab control owner-draw
                    LONG_PTR style = GetWindowLongPtrW(hTab, GWL_STYLE);
                    style |= TCS_OWNERDRAWFIXED;
                    SetWindowLongPtrW(hTab, GWL_STYLE, style);
                    // Subclass to handle WM_ERASEBKGND for dark background
                    s_originalTabProc = reinterpret_cast<WNDPROC>(
                        SetWindowLongPtrW(hTab, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DarkTabProc)));
                }
            }

            // In dark theme, make bottom buttons owner-drawn so we can paint
            // dark backgrounds consistently.
            if (m_configCopy.darkTheme)
            {
                auto makeOwnerDraw = [](HWND hButton)
                {
                    if (!hButton) return;
                    LONG_PTR style = GetWindowLongPtrW(hButton, GWL_STYLE);
                    if ((style & BS_OWNERDRAW) == 0)
                    {
                        style &= ~BS_TYPEMASK;  // clear existing button type (e.g., BS_DEFPUSHBUTTON)
                        style |= BS_OWNERDRAW;
                        SetWindowLongPtrW(hButton, GWL_STYLE, style);

                        // Force immediate redraw using owner-draw logic so
                        // the initial frame/background is also dark.
                        InvalidateRect(hButton, nullptr, TRUE);
                        UpdateWindow(hButton);
                    }
                };

                makeOwnerDraw(GetDlgItem(hDlg, IDC_SETTINGS_BUTTON_OPEN_LOG));
                makeOwnerDraw(GetDlgItem(hDlg, IDOK));
                makeOwnerDraw(GetDlgItem(hDlg, IDC_SETTINGS_BUTTON_APPLY));
                makeOwnerDraw(GetDlgItem(hDlg, IDCANCEL));

                // Make checkboxes owner-draw for dark theme
                auto makeOwnerDrawCheckbox = [](HWND hCheck)
                {
                    if (!hCheck) return;
                    LONG_PTR style = GetWindowLongPtrW(hCheck, GWL_STYLE);
                    style |= BS_OWNERDRAW;
                    SetWindowLongPtrW(hCheck, GWL_STYLE, style);
                    InvalidateRect(hCheck, nullptr, TRUE);
                };

                makeOwnerDrawCheckbox(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK));
                makeOwnerDrawCheckbox(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK));
                makeOwnerDrawCheckbox(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK));
                makeOwnerDrawCheckbox(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK));

                // Clear default button to prevent the system from drawing an
                // initial white default highlight before owner-draw kicks in.
                SendMessageW(hDlg, DM_SETDEFID, 0, 0);
            }
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_SETTINGS_BUTTON_OPEN_LOG:
                {
                    OpenLogFileInExplorer();
                    return TRUE;
                }

                case IDOK:
                {
                    if (ApplySettingsFromDialog(hDlg))
                    {
                        if (m_settingsChangedCallback)
                        {
                            m_settingsChangedCallback();
                        }
                        EndDialog(hDlg, IDOK);
                    }
                    return TRUE;
                }

                case IDC_SETTINGS_BUTTON_APPLY:
                {
                    // Remember current theme before applying
                    bool oldDarkTheme = m_configCopy.darkTheme;
                    AppLanguage oldLanguage = m_configCopy.language;

                    // Apply settings without closing dialog
                    if (ApplySettingsFromDialog(hDlg))
                    {
                        if (m_settingsChangedCallback)
                        {
                            m_settingsChangedCallback();
                        }

                        // If theme or language changed, close and signal reopen
                        if (m_configCopy.darkTheme != oldDarkTheme || 
                            m_configCopy.language != oldLanguage)
                        {
                            EndDialog(hDlg, IDAPPLY_REOPEN);
                        }
                    }
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;

                // Handle owner-draw checkbox clicks
                case IDC_AUTOSTART_CHECK:
                case IDC_ENABLE_LOGGING_CHECK:
                case IDC_DEBUG_LOGGING_CHECK:
                case IDC_CONNECTION_NOTIFY_CHECK:
                {
                    if (m_configCopy.darkTheme && HIWORD(wParam) == BN_CLICKED)
                    {
                        HWND hCheck = reinterpret_cast<HWND>(lParam);
                        // Toggle check state
                        LRESULT state = SendMessageW(hCheck, BM_GETCHECK, 0, 0);
                        SendMessageW(hCheck, BM_SETCHECK, 
                            (state == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED, 0);
                        // Force redraw
                        InvalidateRect(hCheck, nullptr, TRUE);
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
            if (pnmh->idFrom == IDC_SETTINGS_TAB && pnmh->code == TCN_SELCHANGE)
            {
                int tabIndex = TabCtrl_GetCurSel(pnmh->hwndFrom);
                SwitchTab(hDlg, tabIndex);
                return TRUE;
            }
            // Handle Tab Control custom draw for dark background
            if (pnmh->idFrom == IDC_SETTINGS_TAB && pnmh->code == NM_CUSTOMDRAW && m_configCopy.darkTheme)
            {
                LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                switch (pNMCD->dwDrawStage)
                {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                    case CDDS_PREERASE:
                    {
                        // Fill entire tab control area with dark background
                        HBRUSH hBrush = CreateSolidBrush(DialogThemeHelper::DARK_BACKGROUND);
                        FillRect(pNMCD->hdc, &pNMCD->rc, hBrush);
                        DeleteObject(hBrush);
                        return CDRF_SKIPDEFAULT;
                    }
                }
            }
            break;
        }

        case WM_ERASEBKGND:
        {
            if (m_configCopy.darkTheme)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                RECT rc;
                GetClientRect(hDlg, &rc);
                HBRUSH hBrush = CreateSolidBrush(DialogThemeHelper::DARK_BACKGROUND);
                FillRect(hdc, &rc, hBrush);
                DeleteObject(hBrush);
                return TRUE;
            }
            break;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT:
        {
            if (m_configCopy.darkTheme)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                HBRUSH darkBrush = DialogThemeHelper::GetDarkBackgroundBrush();

                HWND hwndCtl = reinterpret_cast<HWND>(lParam);
                int ctrlId = GetDlgCtrlID(hwndCtl);

                bool isComboArea =
                    (ctrlId == IDC_LANGUAGE_COMBO) ||
                    (ctrlId == IDC_UPDATE_INTERVAL_COMBO) ||
                    (ctrlId == IDC_DISPLAY_UNIT_COMBO) ||
                    (ctrlId == IDC_INTERFACE_COMBO) ||
                    (ctrlId == IDC_HISTORY_AUTO_TRIM_COMBO) ||
                    (ctrlId == IDC_THEME_MODE_COMBO);

                if (message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLOREDIT || isComboArea)
                {
                    // For combobox edit/dropdown areas: fill opaque dark background
                    SetTextColor(hdc, DialogThemeHelper::DARK_TEXT);
                    SetBkColor(hdc, DialogThemeHelper::DARK_BACKGROUND);
                    SetBkMode(hdc, OPAQUE);
                }
                else
                {
                    // For labels, group boxes, buttons: transparent over dark dialog
                    SetTextColor(hdc, DialogThemeHelper::DARK_TEXT);
                    SetBkMode(hdc, TRANSPARENT);
                }

                return reinterpret_cast<INT_PTR>(darkBrush);
            }
            break;
        }
        case WM_DRAWITEM:
        {
            if (m_configCopy.darkTheme)
            {
                DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (pDrawItem->CtlType == ODT_BUTTON)
                {
                    UINT id = pDrawItem->CtlID;
                    if (id == IDC_SETTINGS_BUTTON_OPEN_LOG || id == IDOK || 
                        id == IDC_SETTINGS_BUTTON_APPLY || id == IDCANCEL)
                    {
                        HDC hdc = pDrawItem->hDC;
                        RECT rc = pDrawItem->rcItem;

                        bool pressed = (pDrawItem->itemState & ODS_SELECTED) != 0;
                        bool focused = (pDrawItem->itemState & ODS_FOCUS) != 0;
                        bool disabled = (pDrawItem->itemState & ODS_DISABLED) != 0;

                        COLORREF backColor = pressed ? RGB(50, 50, 50) : RGB(40, 40, 40);
                        COLORREF borderColor = RGB(90, 90, 90);
                        COLORREF textColor = disabled ? RGB(160, 160, 160) : DialogThemeHelper::DARK_TEXT;

                        HBRUSH hBrush = CreateSolidBrush(backColor);
                        FillRect(hdc, &rc, hBrush);
                        DeleteObject(hBrush);

                        // Draw only the frame so we don't overwrite the
                        // dark background we just filled.
                        HBRUSH hBorder = CreateSolidBrush(borderColor);
                        FrameRect(hdc, &rc, hBorder);
                        DeleteObject(hBorder);

                        wchar_t text[128] = {0};
                        GetWindowTextW(pDrawItem->hwndItem, text, static_cast<int>(std::size(text)));

                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, textColor);

                        RECT textRc = rc;
                        InflateRect(&textRc, -4, -2);
                        DrawTextW(hdc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                        if (focused)
                        {
                            RECT focusRc = rc;
                            InflateRect(&focusRc, -3, -3);
                            DrawFocusRect(hdc, &focusRc);
                        }

                        return TRUE;
                    }
                }
                // Handle Tab Control owner-draw
                else if (pDrawItem->CtlType == ODT_TAB)
                {
                    HDC hdc = pDrawItem->hDC;
                    RECT rc = pDrawItem->rcItem;
                    bool selected = (pDrawItem->itemState & ODS_SELECTED) != 0;

                    // Dark background for tab
                    COLORREF backColor = selected ? DialogThemeHelper::DARK_BACKGROUND_SELECTED : DialogThemeHelper::DARK_BACKGROUND;
                    COLORREF textColor = DialogThemeHelper::DARK_TEXT;

                    HBRUSH hBrush = CreateSolidBrush(backColor);
                    FillRect(hdc, &rc, hBrush);
                    DeleteObject(hBrush);

                    // Draw border
                    HBRUSH hBorder = CreateSolidBrush(RGB(70, 70, 70));
                    FrameRect(hdc, &rc, hBorder);
                    DeleteObject(hBorder);

                    // Get tab text
                    TCITEM tci = {};
                    wchar_t text[64] = {};
                    tci.mask = TCIF_TEXT;
                    tci.pszText = text;
                    tci.cchTextMax = static_cast<int>(std::size(text));
                    TabCtrl_GetItem(pDrawItem->hwndItem, pDrawItem->itemID, &tci);

                    // Draw text
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, textColor);
                    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    return TRUE;
                }
                // Handle Checkbox owner-draw
                UINT ctlId = pDrawItem->CtlID;
                if (ctlId == IDC_AUTOSTART_CHECK || ctlId == IDC_ENABLE_LOGGING_CHECK ||
                    ctlId == IDC_DEBUG_LOGGING_CHECK || ctlId == IDC_CONNECTION_NOTIFY_CHECK)
                {
                    HDC hdc = pDrawItem->hDC;
                    RECT rc = pDrawItem->rcItem;
                    bool checked = (pDrawItem->itemState & ODS_SELECTED) != 0;
                    bool focused = (pDrawItem->itemState & ODS_FOCUS) != 0;
                    bool disabled = (pDrawItem->itemState & ODS_DISABLED) != 0;

                    // Get actual check state from button
                    checked = (SendMessageW(pDrawItem->hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED);

                    // Colors
                    COLORREF bgColor = DialogThemeHelper::DARK_BACKGROUND;
                    COLORREF boxBgColor = RGB(45, 45, 45);
                    COLORREF boxBorderColor = RGB(100, 100, 100);
                    COLORREF checkColor = disabled ? RGB(120, 120, 120) : RGB(0, 200, 255);
                    COLORREF textColor = disabled ? RGB(120, 120, 120) : DialogThemeHelper::DARK_TEXT;

                    // Fill background
                    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
                    FillRect(hdc, &rc, hBgBrush);
                    DeleteObject(hBgBrush);

                    // Draw checkbox box (13x13 pixels)
                    int boxSize = 13;
                    int boxY = rc.top + (rc.bottom - rc.top - boxSize) / 2;
                    RECT boxRect = {rc.left, boxY, rc.left + boxSize, boxY + boxSize};

                    // Box background
                    HBRUSH hBoxBrush = CreateSolidBrush(boxBgColor);
                    FillRect(hdc, &boxRect, hBoxBrush);
                    DeleteObject(hBoxBrush);

                    // Box border
                    HBRUSH hBorderBrush = CreateSolidBrush(boxBorderColor);
                    FrameRect(hdc, &boxRect, hBorderBrush);
                    DeleteObject(hBorderBrush);

                    // Draw checkmark if checked
                    if (checked)
                    {
                        HPEN hPen = CreatePen(PS_SOLID, 2, checkColor);
                        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

                        // Draw checkmark (V shape)
                        int cx = boxRect.left + boxSize / 2;
                        int cy = boxRect.top + boxSize / 2;
                        MoveToEx(hdc, cx - 3, cy, NULL);
                        LineTo(hdc, cx - 1, cy + 3);
                        LineTo(hdc, cx + 4, cy - 3);

                        SelectObject(hdc, hOldPen);
                        DeleteObject(hPen);
                    }

                    // Draw text
                    wchar_t text[128] = {0};
                    GetWindowTextW(pDrawItem->hwndItem, text, 128);

                    RECT textRect = rc;
                    textRect.left = boxRect.right + 6;

                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, textColor);
                    DrawTextW(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    // Focus rectangle
                    if (focused)
                    {
                        RECT focusRect = textRect;
                        focusRect.right = focusRect.left + 4;
                        SIZE textSize;
                        GetTextExtentPoint32W(hdc, text, (int)wcslen(text), &textSize);
                        focusRect.right = focusRect.left + textSize.cx + 4;
                        focusRect.top = (rc.top + rc.bottom - textSize.cy) / 2 - 1;
                        focusRect.bottom = focusRect.top + textSize.cy + 2;
                        DrawFocusRect(hdc, &focusRect);
                    }

                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

void SettingsDialog::PopulateDialog(HWND hDlg)
{
    // Populate language combo (mirrors main.cpp logic, using m_configCopy)
    HWND hLanguage = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
    if (hLanguage)
    {
        struct LanguageOption
        {
            AppLanguage language;
            UINT resourceId;
        };

        const LanguageOption langs[] = {
            {AppLanguage::SystemDefault,     IDS_LANGUAGE_SYSTEM},
            {AppLanguage::English,           IDS_LANGUAGE_ENGLISH},
            {AppLanguage::Vietnamese,        IDS_LANGUAGE_VIETNAMESE},
            {AppLanguage::Japanese,          IDS_LANGUAGE_JAPANESE},
            {AppLanguage::Korean,            IDS_LANGUAGE_KOREAN},
            {AppLanguage::ChineseSimplified, IDS_LANGUAGE_CHINESE_SIMPLIFIED},
        };

        int selectedIndex = -1;
        for (const auto& option : langs)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.language)
                {
                case AppLanguage::SystemDefault:
                    label = L"System (Windows default)";
                    break;
                case AppLanguage::English:
                    label = L"English";
                    break;
                case AppLanguage::Vietnamese:
                    label = L"Tiếng Việt";
                    break;
                case AppLanguage::Japanese:
                    label = L"日本語";
                    break;
                case AppLanguage::Korean:
                    label = L"한국어";
                    break;
                case AppLanguage::ChineseSimplified:
                    label = L"简体中文";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hLanguage, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hLanguage, CB_SETITEMDATA, index, static_cast<LPARAM>(static_cast<int>(option.language)));

            if (m_configCopy.language == option.language)
            {
                selectedIndex = index;
            }
        }

        if (selectedIndex >= 0)
        {
            SendMessageW(hLanguage, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hLanguage, CB_SETCURSEL, 0, 0);
        }
    }

    // Populate update interval combo
    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    if (hInterval)
    {
        struct IntervalOption
        {
            UINT resourceId;
            UINT interval;
        };

        const IntervalOption intervals[] = {
            {IDS_INTERVAL_FAST,   UPDATE_INTERVAL_FAST},
            {IDS_INTERVAL_NORMAL, UPDATE_INTERVAL_NORMAL},
            {IDS_INTERVAL_SLOW,   UPDATE_INTERVAL_SLOW},
        };

        for (const auto& option : intervals)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.interval)
                {
                case UPDATE_INTERVAL_FAST:
                    label = L"Fast (1s)";
                    break;
                case UPDATE_INTERVAL_NORMAL:
                    label = L"Normal (2s)";
                    break;
                case UPDATE_INTERVAL_SLOW:
                    label = L"Slow (5s)";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hInterval, CB_SETITEMDATA, index, option.interval);
            if (m_configCopy.updateInterval == option.interval)
            {
                SendMessageW(hInterval, CB_SETCURSEL, index, 0);
            }
        }
    }

    // Populate display unit combo
    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    if (hUnit)
    {
        struct UnitOption
        {
            UINT resourceId;
            SpeedUnit unit;
        };

        const UnitOption units[] = {
            {IDS_UNIT_BYTES_PER_SECOND,     SpeedUnit::BytesPerSecond},
            {IDS_UNIT_KILOBYTES_PER_SECOND, SpeedUnit::KiloBytesPerSecond},
            {IDS_UNIT_MEGABYTES_PER_SECOND, SpeedUnit::MegaBytesPerSecond},
            {IDS_UNIT_MEGABITS_PER_SECOND,  SpeedUnit::MegaBitsPerSecond},
        };

        for (const auto& option : units)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.unit)
                {
                case SpeedUnit::BytesPerSecond:
                    label = L"Bytes per second";
                    break;
                case SpeedUnit::KiloBytesPerSecond:
                    label = L"Kilobytes per second";
                    break;
                case SpeedUnit::MegaBytesPerSecond:
                    label = L"Megabytes per second";
                    break;
                case SpeedUnit::MegaBitsPerSecond:
                    label = L"Megabits per second";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hUnit, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hUnit, CB_SETITEMDATA, index, static_cast<WPARAM>(option.unit));
            if (m_configCopy.displayUnit == option.unit)
            {
                SendMessageW(hUnit, CB_SETCURSEL, index, 0);
            }
        }
    }

    // Populate theme mode combo
    HWND hThemeMode = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
    if (hThemeMode)
    {
        struct ThemeOption
        {
            ThemeMode mode;
            UINT resourceId;
        };

        const ThemeOption themes[] = {
            {ThemeMode::SystemDefault, IDS_SETTINGS_THEME_SYSTEM},
            {ThemeMode::Light,         IDS_SETTINGS_THEME_LIGHT},
            {ThemeMode::Dark,          IDS_SETTINGS_THEME_DARK},
        };

        int selectedIndex = -1;
        for (const auto& option : themes)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                switch (option.mode)
                {
                case ThemeMode::SystemDefault:
                    label = L"System (Windows default)";
                    break;
                case ThemeMode::Light:
                    label = L"Light";
                    break;
                case ThemeMode::Dark:
                    label = L"Dark";
                    break;
                default:
                    label = L"Unknown";
                    break;
                }
            }

            int index = static_cast<int>(SendMessageW(hThemeMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hThemeMode, CB_SETITEMDATA, index, static_cast<WPARAM>(static_cast<int>(option.mode)));

            if (m_configCopy.themeMode == option.mode)
            {
                selectedIndex = index;
            }
        }

        if (selectedIndex >= 0)
        {
            SendMessageW(hThemeMode, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hThemeMode, CB_SETCURSEL, 0, 0);
        }
    }

    // Populate interface combo
    PopulateInterfaceCombo(hDlg);

    // Set checkbox states (only controls that exist in current dialog)
    Button_SetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK), m_configCopy.autoStart ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK), m_configCopy.enableLogging ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK), m_configCopy.debugLogging ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK), m_configCopy.enableConnectionNotification ? BST_CHECKED : BST_UNCHECKED);

    // Populate history auto-trim combo
    HWND hTrim = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
    if (hTrim)
    {
        struct TrimOption
        {
            UINT days;
            UINT resourceId;
        };

        const TrimOption options[] = {
            {0,   IDS_HISTORY_AUTO_TRIM_NONE},
            {7,   IDS_HISTORY_AUTO_TRIM_7D},
            {30,  IDS_HISTORY_AUTO_TRIM_30D},
            {90,  IDS_HISTORY_AUTO_TRIM_90D},
            {365, IDS_HISTORY_AUTO_TRIM_365D},
        };

        int selectedIndex = -1;
        for (const auto& option : options)
        {
            std::wstring label = LoadStringResource(option.resourceId);
            if (label.empty())
            {
                if (option.days == 0)
                {
                    label = L"Do not auto delete";
                }
                else
                {
                    wchar_t buffer[64] = {0};
                    swprintf_s(buffer, L"Keep last %u days", option.days);
                    label = buffer;
                }
            }

            int index = static_cast<int>(SendMessageW(hTrim, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(hTrim, CB_SETITEMDATA, index, option.days);

            if (static_cast<UINT>(m_configCopy.historyAutoTrimDays) == option.days)
            {
                selectedIndex = index;
            }
        }

        if (selectedIndex >= 0)
        {
            SendMessageW(hTrim, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hTrim, CB_SETCURSEL, 0, 0);
        }
    }

    // Populate ping target edit
    HWND hPingTarget = GetDlgItem(hDlg, IDC_PING_TARGET_EDIT);
    if (hPingTarget)
    {
        SetWindowTextW(hPingTarget, m_configCopy.pingTarget.c_str());
    }

    // Populate ping interval combo
    HWND hPingInterval = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
    if (hPingInterval)
    {
        struct IntervalOption { UINT ms; const wchar_t* label; };
        const IntervalOption intervals[] = {
            {3000,  L"3s"},
            {5000,  L"5s"},
            {10000, L"10s"},
            {30000, L"30s"},
        };

        int selectedIndex = -1;
        for (const auto& option : intervals)
        {
            int index = static_cast<int>(SendMessageW(hPingInterval, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.label)));
            SendMessageW(hPingInterval, CB_SETITEMDATA, index, option.ms);
            if (m_configCopy.pingIntervalMs == option.ms)
            {
                selectedIndex = index;
            }
        }
        if (selectedIndex >= 0)
        {
            SendMessageW(hPingInterval, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hPingInterval, CB_SETCURSEL, 1, 0); // Default to 5s
        }
    }

    // Populate hotkey combo
    HWND hHotkey = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
    if (hHotkey)
    {
        struct HotkeyOption { UINT modifier; UINT key; const wchar_t* label; };
        const HotkeyOption hotkeys[] = {
            {MOD_WIN | MOD_SHIFT, 'N', L"Win+Shift+N"},
            {MOD_WIN | MOD_SHIFT, 'M', L"Win+Shift+M"},
            {MOD_CONTROL | MOD_SHIFT, 'N', L"Ctrl+Shift+N"},
            {MOD_CONTROL | MOD_ALT, 'N', L"Ctrl+Alt+N"},
        };

        int selectedIndex = -1;
        for (const auto& option : hotkeys)
        {
            int index = static_cast<int>(SendMessageW(hHotkey, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.label)));
            // Store both modifier and key in item data (modifier in high word, key in low word)
            LPARAM data = MAKELPARAM(option.key, option.modifier);
            SendMessageW(hHotkey, CB_SETITEMDATA, index, data);
            if (m_configCopy.hotkeyModifier == option.modifier && m_configCopy.hotkeyKey == option.key)
            {
                selectedIndex = index;
            }
        }
        if (selectedIndex >= 0)
        {
            SendMessageW(hHotkey, CB_SETCURSEL, selectedIndex, 0);
        }
        else
        {
            SendMessageW(hHotkey, CB_SETCURSEL, 0, 0); // Default to Win+Shift+N
        }
    }

    // Populate font size combo
    HWND hFontSize = GetDlgItem(hDlg, IDC_OVERLAY_FONT_SIZE_COMBO);
    if (hFontSize)
    {
        const int fontSizes[] = {10, 11, 12, 13, 14, 15, 16};
        int selectedIndex = -1;
        for (int size : fontSizes)
        {
            wchar_t buf[8];
            swprintf_s(buf, L"%d", size);
            int index = static_cast<int>(SendMessageW(hFontSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buf)));
            SendMessageW(hFontSize, CB_SETITEMDATA, index, size);
            if (m_configCopy.overlayFontSize == size)
            {
                selectedIndex = index;
            }
        }
        SendMessageW(hFontSize, CB_SETCURSEL, selectedIndex >= 0 ? selectedIndex : 3, 0); // Default 13
    }

    // Populate overlay color combo
    HWND hOverlayColor = GetDlgItem(hDlg, IDC_OVERLAY_COLOR_COMBO);
    if (hOverlayColor)
    {
        int selectedIndex = 0;
        for (int i = 0; i < s_overlayColorPresetCount; i++)
        {
            int index = static_cast<int>(SendMessageW(hOverlayColor, CB_ADDSTRING, 0, 
                reinterpret_cast<LPARAM>(s_overlayColorPresets[i].label)));
            SendMessageW(hOverlayColor, CB_SETITEMDATA, index, i);
            if (m_configCopy.overlayDownloadColor == s_overlayColorPresets[i].down && 
                m_configCopy.overlayUploadColor == s_overlayColorPresets[i].up)
            {
                selectedIndex = index;
            }
        }
        SendMessageW(hOverlayColor, CB_SETCURSEL, selectedIndex, 0);
    }

    // Apply dark theme with dark dropdown lists to all comboboxes
    if (m_configCopy.darkTheme)
    {
        HWND hLangTheme   = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
        HWND hIntTheme    = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
        HWND hUnitTheme   = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
        HWND hIfaceTheme  = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
        HWND hTrimTheme   = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
        HWND hThemeModeCB = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
        HWND hPingIntTheme = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
        HWND hHotkeyTheme = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
        HWND hFontSizeTheme = GetDlgItem(hDlg, IDC_OVERLAY_FONT_SIZE_COMBO);
        HWND hOverlayColorTheme = GetDlgItem(hDlg, IDC_OVERLAY_COLOR_COMBO);

        if (hLangTheme)    ThemeHelper::ApplyDarkThemeToControl(hLangTheme, true);
        if (hIntTheme)     ThemeHelper::ApplyDarkThemeToControl(hIntTheme, true);
        if (hUnitTheme)    ThemeHelper::ApplyDarkThemeToControl(hUnitTheme, true);
        if (hIfaceTheme)   ThemeHelper::ApplyDarkThemeToControl(hIfaceTheme, true);
        if (hTrimTheme)    ThemeHelper::ApplyDarkThemeToControl(hTrimTheme, true);
        if (hThemeModeCB)  ThemeHelper::ApplyDarkThemeToControl(hThemeModeCB, true);
        if (hPingIntTheme) ThemeHelper::ApplyDarkThemeToControl(hPingIntTheme, true);
        if (hHotkeyTheme)  ThemeHelper::ApplyDarkThemeToControl(hHotkeyTheme, true);
        if (hFontSizeTheme) ThemeHelper::ApplyDarkThemeToControl(hFontSizeTheme, true);
        if (hOverlayColorTheme) ThemeHelper::ApplyDarkThemeToControl(hOverlayColorTheme, true);

        // Subclass comboboxes to draw dark dropdown button
        auto subclassComboBox = [](HWND hCombo)
        {
            if (!hCombo) return;
            WNDPROC oldProc = reinterpret_cast<WNDPROC>(
                GetWindowLongPtrW(hCombo, GWLP_WNDPROC));
            SetPropW(hCombo, COMBOBOX_OLDPROC_PROP, reinterpret_cast<HANDLE>(oldProc));
            SetWindowLongPtrW(hCombo, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DarkComboBoxProc));
            InvalidateRect(hCombo, nullptr, TRUE);
        };

        subclassComboBox(hLangTheme);
        subclassComboBox(hIntTheme);
        subclassComboBox(hUnitTheme);
        subclassComboBox(hIfaceTheme);
        subclassComboBox(hTrimTheme);
        subclassComboBox(hThemeModeCB);
        subclassComboBox(hPingIntTheme);
        subclassComboBox(hHotkeyTheme);
        subclassComboBox(hFontSizeTheme);
        subclassComboBox(hOverlayColorTheme);
    }
}

bool SettingsDialog::ApplySettingsFromDialog(HWND hDlg)
{
    // Get update interval selection
    UINT newInterval = m_configCopy.updateInterval;
    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    if (hInterval)
    {
        int sel = static_cast<int>(SendMessageW(hInterval, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            newInterval = static_cast<UINT>(SendMessageW(hInterval, CB_GETITEMDATA, sel, 0));
        }
    }

    // Get display unit selection
    SpeedUnit newUnit = m_configCopy.displayUnit;
    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    if (hUnit)
    {
        int sel = static_cast<int>(SendMessageW(hUnit, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            newUnit = static_cast<SpeedUnit>(SendMessageW(hUnit, CB_GETITEMDATA, sel, 0));
        }
    }

    // Get checkbox states
    bool newAutoStart = (Button_GetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK)) == BST_CHECKED);
    bool newEnableLogging = (Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK)) == BST_CHECKED);
    bool newDebugLogging = (Button_GetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK)) == BST_CHECKED);
    bool newConnectionNotify = (Button_GetCheck(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK)) == BST_CHECKED);
    bool newDarkTheme = m_configCopy.darkTheme;

    // Keep ThemeMode roughly in sync with the dark theme checkbox so that
    // future logic based on ThemeMode can distinguish between explicit
    // Light/Dark overrides (while SystemDefault remains the migrated value
    // when the user has not changed the theme setting yet).
    ThemeMode newThemeMode = m_configCopy.themeMode;
    HWND hThemeModeDlg = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
    if (hThemeModeDlg)
    {
        int selTheme = static_cast<int>(SendMessageW(hThemeModeDlg, CB_GETCURSEL, 0, 0));
        if (selTheme != CB_ERR)
        {
            LRESULT data = SendMessageW(hThemeModeDlg, CB_GETITEMDATA, selTheme, 0);
            if (data != CB_ERR)
            {
                newThemeMode = static_cast<ThemeMode>(static_cast<int>(data));
            }
        }
    }

    AppConfig tempConfig = m_configCopy;
    tempConfig.themeMode = newThemeMode;
    newDarkTheme = IsDarkThemeEnabled(tempConfig);

    // Get interface selection
    std::wstring newInterface = m_configCopy.selectedInterface;
    HWND hInterface = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
    if (hInterface)
    {
        int sel = static_cast<int>(SendMessageW(hInterface, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            wchar_t buffer[256] = {0};
            SendMessageW(hInterface, CB_GETLBTEXT, sel, reinterpret_cast<LPARAM>(buffer));
            if (sel == 0)
            {
                newInterface.clear();
            }
            else
            {
                newInterface = buffer;
            }
        }
    }

    // Get history auto-trim selection
    int newTrimDays = m_configCopy.historyAutoTrimDays;
    HWND hTrim = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
    if (hTrim)
    {
        int selTrim = static_cast<int>(SendMessageW(hTrim, CB_GETCURSEL, 0, 0));
        if (selTrim != CB_ERR)
        {
            LRESULT days = SendMessageW(hTrim, CB_GETITEMDATA, selTrim, 0);
            if (days != CB_ERR)
            {
                newTrimDays = static_cast<int>(days);
            }
        }
    }

    // Get language selection
    AppLanguage newLanguage = m_configCopy.language;
    HWND hLanguage = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
    if (hLanguage)
    {
        int selLang = static_cast<int>(SendMessageW(hLanguage, CB_GETCURSEL, 0, 0));
        if (selLang != CB_ERR)
        {
            LRESULT langVal = SendMessageW(hLanguage, CB_GETITEMDATA, selLang, 0);
            if (langVal != CB_ERR)
            {
                newLanguage = static_cast<AppLanguage>(langVal);
            }
        }
    }

    // Get ping target
    std::wstring newPingTarget = m_configCopy.pingTarget;
    HWND hPingTarget = GetDlgItem(hDlg, IDC_PING_TARGET_EDIT);
    if (hPingTarget)
    {
        wchar_t buffer[256] = {0};
        GetWindowTextW(hPingTarget, buffer, 256);
        newPingTarget = buffer;
        if (newPingTarget.empty())
        {
            newPingTarget = L"8.8.8.8";
        }
    }

    // Get ping interval
    UINT newPingInterval = m_configCopy.pingIntervalMs;
    HWND hPingInterval = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
    if (hPingInterval)
    {
        int sel = static_cast<int>(SendMessageW(hPingInterval, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hPingInterval, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                newPingInterval = static_cast<UINT>(data);
            }
        }
    }

    // Get hotkey
    UINT newHotkeyModifier = m_configCopy.hotkeyModifier;
    UINT newHotkeyKey = m_configCopy.hotkeyKey;
    HWND hHotkeyCombo = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
    if (hHotkeyCombo)
    {
        int sel = static_cast<int>(SendMessageW(hHotkeyCombo, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hHotkeyCombo, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                newHotkeyKey = LOWORD(data);
                newHotkeyModifier = HIWORD(data);
            }
        }
    }

    // Update working copy
    m_configCopy.updateInterval = newInterval;
    m_configCopy.displayUnit = newUnit;
    m_configCopy.autoStart = newAutoStart;
    m_configCopy.enableLogging = newEnableLogging;
    m_configCopy.debugLogging = newDebugLogging;
    m_configCopy.enableConnectionNotification = newConnectionNotify;
    m_configCopy.darkTheme = newDarkTheme;
    m_configCopy.themeMode = newThemeMode;
    m_configCopy.selectedInterface = newInterface;
    m_configCopy.historyAutoTrimDays = newTrimDays;
    m_configCopy.language = newLanguage;
    m_configCopy.pingTarget = newPingTarget;
    m_configCopy.pingIntervalMs = newPingInterval;
    m_configCopy.hotkeyModifier = newHotkeyModifier;
    m_configCopy.hotkeyKey = newHotkeyKey;

    // Get font size
    HWND hFontSize = GetDlgItem(hDlg, IDC_OVERLAY_FONT_SIZE_COMBO);
    if (hFontSize)
    {
        int sel = static_cast<int>(SendMessageW(hFontSize, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hFontSize, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                m_configCopy.overlayFontSize = static_cast<int>(data);
            }
        }
    }

    // Get overlay colors
    HWND hOverlayColor = GetDlgItem(hDlg, IDC_OVERLAY_COLOR_COMBO);
    if (hOverlayColor)
    {
        int sel = static_cast<int>(SendMessageW(hOverlayColor, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR && sel < s_overlayColorPresetCount)
        {
            m_configCopy.overlayDownloadColor = s_overlayColorPresets[sel].down;
            m_configCopy.overlayUploadColor = s_overlayColorPresets[sel].up;
        }
    }

    // Save to registry via config provider (ignore errors for now)
    if (m_pConfigProvider)
    {
        m_pConfigProvider->SaveConfig(m_configCopy);
    }

    return true;
}

void SettingsDialog::PopulateInterfaceCombo(HWND hDlg)
{
    HWND hInterface = GetDlgItem(hDlg, IDC_INTERFACE_COMBO);
    if (!hInterface)
    {
        return;
    }

    ComboBox_ResetContent(hInterface);

    // Add "All Interfaces" option
    std::wstring allLabel = LoadStringResource(IDS_ALL_INTERFACES);
    if (allLabel.empty())
    {
        allLabel = L"All Interfaces";
    }

    int indexAll = ComboBox_AddString(hInterface, allLabel.c_str());
    if (m_configCopy.selectedInterface.empty())
    {
        ComboBox_SetCurSel(hInterface, indexAll);
    }

    if (m_pStatsProvider)
    {
        std::vector<NetworkStats> statsList = m_pStatsProvider->GetAllStats();
        int selectedIdx = -1;

        for (const auto& stats : statsList)
        {
            if (stats.interfaceName.empty())
            {
                continue;
            }

            int idx = ComboBox_AddString(hInterface, stats.interfaceName.c_str());
            if (stats.interfaceName == m_configCopy.selectedInterface)
            {
                selectedIdx = idx;
            }
        }

        if (selectedIdx >= 0)
        {
            ComboBox_SetCurSel(hInterface, selectedIdx);
        }
        else if (!m_configCopy.selectedInterface.empty())
        {
            int idx = ComboBox_AddString(hInterface, m_configCopy.selectedInterface.c_str());
            ComboBox_SetCurSel(hInterface, idx);
        }
    }
    else if (!m_configCopy.selectedInterface.empty())
    {
        int idx = ComboBox_AddString(hInterface, m_configCopy.selectedInterface.c_str());
        ComboBox_SetCurSel(hInterface, idx);
    }
}

void SettingsDialog::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
}

void SettingsDialog::InitializeTabControl(HWND hDlg)
{
    HWND hTab = GetDlgItem(hDlg, IDC_SETTINGS_TAB);
    if (!hTab)
    {
        return;
    }

    TCITEM tie = {};
    tie.mask = TCIF_TEXT;

    std::wstring tabGeneral = LoadStringResource(IDS_SETTINGS_TAB_GENERAL);
    if (tabGeneral.empty()) tabGeneral = L"General";
    tie.pszText = const_cast<LPWSTR>(tabGeneral.c_str());
    TabCtrl_InsertItem(hTab, 0, &tie);

    std::wstring tabDisplay = LoadStringResource(IDS_SETTINGS_TAB_DISPLAY);
    if (tabDisplay.empty()) tabDisplay = L"Display";
    tie.pszText = const_cast<LPWSTR>(tabDisplay.c_str());
    TabCtrl_InsertItem(hTab, 1, &tie);

    std::wstring tabAdvanced = LoadStringResource(IDS_SETTINGS_TAB_ADVANCED);
    if (tabAdvanced.empty()) tabAdvanced = L"Advanced";
    tie.pszText = const_cast<LPWSTR>(tabAdvanced.c_str());
    TabCtrl_InsertItem(hTab, 2, &tie);
}

void SettingsDialog::SwitchTab(HWND hDlg, int tabIndex)
{
    // General tab controls
    const int generalControls[] = {
        IDC_SETTINGS_LABEL_LANGUAGE, IDC_LANGUAGE_COMBO,
        IDC_AUTOSTART_CHECK, IDC_ENABLE_LOGGING_CHECK,
        IDC_DEBUG_LOGGING_CHECK, IDC_CONNECTION_NOTIFY_CHECK
    };

    // Display tab controls
    const int displayControls[] = {
        IDC_SETTINGS_LABEL_THEME, IDC_THEME_MODE_COMBO,
        IDC_SETTINGS_LABEL_INTERVAL, IDC_UPDATE_INTERVAL_COMBO,
        IDC_DISPLAY_UNIT_LABEL, IDC_DISPLAY_UNIT_COMBO,
        IDC_FONT_SIZE_LABEL, IDC_OVERLAY_FONT_SIZE_COMBO,
        IDC_OVERLAY_COLOR_LABEL, IDC_OVERLAY_COLOR_COMBO
    };

    // Advanced tab controls
    const int advancedControls[] = {
        IDC_SETTINGS_LABEL_MONITOR, IDC_INTERFACE_COMBO,
        IDC_HISTORY_TRIM_LABEL, IDC_HISTORY_AUTO_TRIM_COMBO,
        IDC_PING_TARGET_LABEL, IDC_PING_TARGET_EDIT,
        IDC_PING_INTERVAL_LABEL, IDC_PING_INTERVAL_COMBO,
        IDC_HOTKEY_LABEL, IDC_HOTKEY_COMBO
    };

    auto showControls = [hDlg](const int* ids, int count, bool show)
    {
        int cmdShow = show ? SW_SHOW : SW_HIDE;
        for (int i = 0; i < count; i++)
        {
            HWND hCtrl = GetDlgItem(hDlg, ids[i]);
            if (hCtrl)
            {
                ShowWindow(hCtrl, cmdShow);
            }
        }
    };

    // Hide all, then show selected tab controls
    showControls(generalControls, _countof(generalControls), false);
    showControls(displayControls, _countof(displayControls), false);
    showControls(advancedControls, _countof(advancedControls), false);

    switch (tabIndex)
    {
        case 0:
            showControls(generalControls, _countof(generalControls), true);
            break;
        case 1:
            showControls(displayControls, _countof(displayControls), true);
            break;
        case 2:
            showControls(advancedControls, _countof(advancedControls), true);
            break;
    }
}

} // namespace NetworkMonitor
