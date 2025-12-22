#include "NetPulse/HistoryDialog.h"
#include "NetPulse/HistoryLogger.h"
#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/Utils.h"
#include "NetPulse/ThemeHelper.h"
#include "../../../resources/resource.h"
#include <windowsx.h>
#include <commctrl.h>

namespace NetPulse
{

HistoryDialog::HistoryDialog()
    : m_hDialog(nullptr)
    , m_pExternalHandle(nullptr)
    , m_pConfig(nullptr)
{
}

HistoryDialog::~HistoryDialog()
{
}

bool HistoryDialog::Show(HWND parentWindow, const AppConfig* config)
{
    m_pConfig = config;
    // Create modal dialog
    INT_PTR result = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_HISTORY_MANAGE_DIALOG),
        parentWindow,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return (result == IDOK);
}

INT_PTR CALLBACK HistoryDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HistoryDialog* pThis = nullptr;

    if (message == WM_INITDIALOG)
    {
        pThis = reinterpret_cast<HistoryDialog*>(lParam);
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
        pThis = reinterpret_cast<HistoryDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
    }

    if (pThis)
    {
        return pThis->InstanceDialogProc(hDlg, message, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK HistoryDialog::InstanceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Mirror HistoryManageDialogProc initialization from main.cpp
            CenterDialogOnScreen(hDlg);

            std::wstring title = LoadStringResource(IDS_HISTORY_DIALOG_TITLE);
            if (!title.empty())
            {
                SetWindowTextW(hDlg, title.c_str());
            }

            // Apply dark title bar for dark themes only
            if (m_pConfig)
            {
                ThemeHelper::ApplyDarkTitleBar(hDlg, IsDarkThemeEnabled(*m_pConfig));
            }

            std::wstring opsLabel = LoadStringResource(IDS_HISTORY_LABEL_OPERATIONS);
            if (!opsLabel.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_LABEL_OPERATIONS, opsLabel.c_str());
            }

            // Localize button labels
            std::wstring deleteAllText = LoadStringResource(IDS_HISTORY_BUTTON_DELETE_ALL);
            if (!deleteAllText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_DELETE_ALL, deleteAllText.c_str());
            }

            std::wstring keep30Text = LoadStringResource(IDS_HISTORY_BUTTON_KEEP_30);
            if (!keep30Text.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_KEEP_30, keep30Text.c_str());
            }

            std::wstring keep90Text = LoadStringResource(IDS_HISTORY_BUTTON_KEEP_90);
            if (!keep90Text.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_KEEP_90, keep90Text.c_str());
            }

            std::wstring closeText = LoadStringResource(IDS_BUTTON_CANCEL);
            if (!closeText.empty())
            {
                SetDlgItemTextW(hDlg, IDCANCEL, closeText.c_str());
            }

            // In custom theme, make buttons owner-drawn so we can paint theme
            // backgrounds.
            if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
            {
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_HISTORY_DELETE_ALL));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_HISTORY_KEEP_30));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_HISTORY_KEEP_90));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDCANCEL));

                // Clear default button id so the dialog manager does not draw
                // an initial white default highlight before owner-draw runs.
                SendMessageW(hDlg, DM_SETDEFID, 0, 0);
            }

            std::wstring btnDelete = LoadStringResource(IDS_HISTORY_BUTTON_DELETE_ALL);
            if (!btnDelete.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_DELETE_ALL, btnDelete.c_str());
            }

            std::wstring btn30 = LoadStringResource(IDS_HISTORY_BUTTON_KEEP_30);
            if (!btn30.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_KEEP_30, btn30.c_str());
            }

            std::wstring btn90 = LoadStringResource(IDS_HISTORY_BUTTON_KEEP_90);
            if (!btn90.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HISTORY_KEEP_90, btn90.c_str());
            }

            return TRUE;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                HBRUSH hBrush = DialogThemeHelper::HandleControlColor(hdc, true);
                if (hBrush) return reinterpret_cast<INT_PTR>(hBrush);
            }
            break;
        }
        case WM_DRAWITEM:
        {
            if (m_pConfig && IsCustomThemeEnabled(*m_pConfig))
            {
                DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (pDrawItem->CtlType == ODT_BUTTON)
                {
                    UINT id = pDrawItem->CtlID;
                    if (id == IDC_HISTORY_DELETE_ALL || id == IDC_HISTORY_KEEP_30 ||
                        id == IDC_HISTORY_KEEP_90 || id == IDCANCEL)
                    {
                        DialogThemeHelper::DrawButton(pDrawItem, true);
                        return TRUE;
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_HISTORY_DELETE_ALL:
                case IDC_HISTORY_KEEP_30:
                case IDC_HISTORY_KEEP_90:
                {
                    int days = 0;
                    UINT confirmId = 0;

                    if (LOWORD(wParam) == IDC_HISTORY_DELETE_ALL)
                    {
                        days = 0;
                        confirmId = IDS_HISTORY_CONFIRM_DELETE_ALL;
                    }
                    else if (LOWORD(wParam) == IDC_HISTORY_KEEP_30)
                    {
                        days = 30;
                        confirmId = IDS_HISTORY_CONFIRM_TRIM_30;
                    }
                    else if (LOWORD(wParam) == IDC_HISTORY_KEEP_90)
                    {
                        days = 90;
                        confirmId = IDS_HISTORY_CONFIRM_TRIM_90;
                    }

                    std::wstring confirmText = LoadStringResource(confirmId);
                    if (confirmText.empty())
                    {
                        switch (LOWORD(wParam))
                        {
                        case IDC_HISTORY_DELETE_ALL:
                            confirmText = L"This will delete all logged history. Are you sure?";
                            break;
                        case IDC_HISTORY_KEEP_30:
                            confirmText = L"Delete all records older than 30 days?";
                            break;
                        case IDC_HISTORY_KEEP_90:
                            confirmText = L"Delete all records older than 90 days?";
                            break;
                        }
                    }

                    std::wstring title = LoadStringResource(IDS_HISTORY_MANAGE_TITLE);
                    if (title.empty())
                    {
                        title = L"Manage History";
                    }

                    bool dark = (m_pConfig && IsCustomThemeEnabled(*m_pConfig));
                    int res = ShowDarkMessageBox(hDlg, confirmText, title, MB_YESNO | MB_ICONQUESTION, dark);
                    if (res != IDYES)
                    {
                        return TRUE;
                    }

                    bool ok = false;
                    HistoryLogger& logger = HistoryLogger::Instance();
                    if (days <= 0)
                    {
                        ok = logger.DeleteAll();
                    }
                    else
                    {
                        ok = logger.TrimToRecentDays(days);
                    }

                    if (!ok)
                    {
                        std::wstring err = LoadStringResource(IDS_HISTORY_ERROR_OPERATION);
                        if (err.empty())
                        {
                            err = L"Failed to modify history database.";
                        }
                        ShowDarkMessageBox(hDlg, err, title, MB_OK | MB_ICONERROR, dark);
                    }

                    return TRUE;
                }

                case IDCANCEL:
                case IDOK:
                    // Clear external handle storage before closing
                    if (m_pExternalHandle)
                    {
                        *m_pExternalHandle = nullptr;
                    }
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
            break;
        }
    }

    return FALSE;
}

void HistoryDialog::CenterDialogOnScreen(HWND hDlg)
{
    CenterWindowOnScreen(hDlg);
}

} // namespace NetPulse
