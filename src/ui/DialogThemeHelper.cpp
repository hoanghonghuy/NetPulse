#include "NetworkMonitor/DialogThemeHelper.h"
#include <uxtheme.h>
#include <dwmapi.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

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

    // Draw background
    COLORREF bgColor = isPressed ? DARK_BACKGROUND_SELECTED : DARK_BACKGROUND;
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // NO border drawing - cleaner look
    
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

void DialogThemeHelper::SetThinWindowBorder(HWND hDlg)
{
    // Windows 11 22000+ has better border control
    // DWMWA_WINDOW_CORNER_PREFERENCE = 33 (rounded corners)
    // DWMWA_BORDER_COLOR = 34 (custom border color)
    // DWMWCP_ROUND = 2 (rounded corners)
    
    #ifndef DWMWA_WINDOW_CORNER_PREFERENCE
    #define DWMWA_WINDOW_CORNER_PREFERENCE 33
    #endif
    
    #ifndef DWMWA_BORDER_COLOR  
    #define DWMWA_BORDER_COLOR 34
    #endif
    
    #ifndef DWMWCP_ROUND
    #define DWMWCP_ROUND 2
    #endif
    
    // Try Windows 11 improved border control first
    DWORD cornerPreference = DWMWCP_ROUND;
    HRESULT hr = DwmSetWindowAttribute(hDlg, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
    
    // Set border color to match background for thinner appearance
    // Use very dark gray that blends with dark theme or lighter for light theme
    COLORREF borderColor = RGB(45, 45, 45); // Subtle dark border
    DwmSetWindowAttribute(hDlg, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
    
    // Fallback: Extend frame minimally (works on Win10 but less effective)
    if (FAILED(hr))
    {
        MARGINS margins = {0, 0, 0, 0};
        DwmExtendFrameIntoClientArea(hDlg, &margins);
    }
}

void DialogThemeHelper::ApplyToDialog(HWND hDlg, bool darkTheme)
{
    if (!darkTheme)
    {
        return;
    }

    // Disable visual styles on the dialog for dark theme
    SetWindowTheme(hDlg, L"", L"");
    
    // Apply thin window border
    SetThinWindowBorder(hDlg);
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
