#include "NetworkMonitor/DialogThemeHelper.h"
#include <uxtheme.h>

#pragma comment(lib, "uxtheme.lib")

namespace NetworkMonitor
{

// Static member initialization
HBRUSH DialogThemeHelper::s_darkBrush = nullptr;

HBRUSH DialogThemeHelper::GetDarkBackgroundBrush()
{
    if (!s_darkBrush)
    {
        s_darkBrush = CreateSolidBrush(DARK_BACKGROUND);
    }
    return s_darkBrush;
}

HBRUSH DialogThemeHelper::HandleControlColor(HDC hdc, bool darkTheme)
{
    if (darkTheme)
    {
        SetTextColor(hdc, DARK_TEXT);
        SetBkColor(hdc, DARK_BACKGROUND);
        return GetDarkBackgroundBrush();
    }
    return nullptr; // Use default system brush
}

void DialogThemeHelper::FillDarkBackground(HDC hdc, const RECT& rect)
{
    HBRUSH hBrush = CreateSolidBrush(DARK_BACKGROUND);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
}

void DialogThemeHelper::DrawButton(DRAWITEMSTRUCT* pDrawItem, bool darkTheme)
{
    if (!pDrawItem || !darkTheme)
    {
        return;
    }

    HDC hdc = pDrawItem->hDC;
    RECT rc = pDrawItem->rcItem;
    bool isPressed = (pDrawItem->itemState & ODS_SELECTED) != 0;
    bool isFocused = (pDrawItem->itemState & ODS_FOCUS) != 0;

    // Draw background
    COLORREF bgColor = isPressed ? DARK_BACKGROUND_SELECTED : DARK_BACKGROUND;
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // Draw border
    HPEN hPen = CreatePen(PS_SOLID, 1, isFocused ? RGB(100, 100, 100) : DARK_BORDER);
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    // Draw text
    wchar_t text[256] = {0};
    GetWindowTextW(pDrawItem->hwndItem, text, 256);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, DARK_TEXT);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DialogThemeHelper::DrawTabItem(DRAWITEMSTRUCT* pDrawItem, bool darkTheme)
{
    if (!pDrawItem || !darkTheme)
    {
        return;
    }

    HDC hdc = pDrawItem->hDC;
    RECT rc = pDrawItem->rcItem;
    bool selected = (pDrawItem->itemState & ODS_SELECTED) != 0;

    // Draw background
    COLORREF backColor = selected ? DARK_BACKGROUND_SELECTED : DARK_BACKGROUND;
    HBRUSH hBrush = CreateSolidBrush(backColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // Draw border for selected tab
    if (selected)
    {
        HPEN hPen = CreatePen(PS_SOLID, 1, DARK_BORDER);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        MoveToEx(hdc, rc.left, rc.top, nullptr);
        LineTo(hdc, rc.right - 1, rc.top);
        LineTo(hdc, rc.right - 1, rc.bottom);
        MoveToEx(hdc, rc.left, rc.top, nullptr);
        LineTo(hdc, rc.left, rc.bottom);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    // Get tab text
    HWND hTab = pDrawItem->hwndItem;
    TCITEMW tci = {0};
    wchar_t text[64] = {0};
    tci.mask = TCIF_TEXT;
    tci.pszText = text;
    tci.cchTextMax = 64;
    TabCtrl_GetItem(hTab, pDrawItem->itemID, &tci);

    // Draw text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, DARK_TEXT);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DialogThemeHelper::ApplyToDialog(HWND hDlg, bool darkTheme)
{
    if (!darkTheme)
    {
        return;
    }

    // Disable visual styles on the dialog for dark theme
    SetWindowTheme(hDlg, L"", L"");
}

void DialogThemeHelper::Cleanup()
{
    if (s_darkBrush)
    {
        DeleteObject(s_darkBrush);
        s_darkBrush = nullptr;
    }
}

} // namespace NetworkMonitor
