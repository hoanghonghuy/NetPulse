#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/ThemeHelper.h"
#include <uxtheme.h>
#include <dwmapi.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

#include <commctrl.h>

namespace NetPulse
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
        SetBkMode(hdc, TRANSPARENT); // Use TRANSPARENT instead of SetBkColor for better UI
        return GetDarkBackgroundBrush();
    }
    return nullptr; // Use default system brush
}

HBRUSH DialogThemeHelper::HandleEditControlColor(HDC hdc, bool darkTheme)
{
    if (darkTheme)
    {
        SetTextColor(hdc, DARK_TEXT);
        SetBkColor(hdc, DARK_INPUT_BACKGROUND);
        
        static HBRUSH s_inputBrush = nullptr;
        if (!s_inputBrush)
        {
            s_inputBrush = CreateSolidBrush(DARK_INPUT_BACKGROUND);
        }
        return s_inputBrush;
    }
    return nullptr; // Use default system brush
}

void DialogThemeHelper::ApplyDarkEditControl(HWND hEdit)
{
    if (!hEdit) return;

    // Remove client edge (thick border)
    LONG_PTR exStyle = GetWindowLongPtrW(hEdit, GWL_EXSTYLE);
    if (exStyle & WS_EX_CLIENTEDGE)
    {
        SetWindowLongPtrW(hEdit, GWL_EXSTYLE, exStyle & ~WS_EX_CLIENTEDGE);
    }

    // Add simple border
    LONG_PTR style = GetWindowLongPtrW(hEdit, GWL_STYLE);
    if ((style & WS_BORDER) == 0)
    {
        SetWindowLongPtrW(hEdit, GWL_STYLE, style | WS_BORDER);
    }
    
    // Force redraw
    SetWindowPos(hEdit, nullptr, 0, 0, 0, 0, 
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void DialogThemeHelper::ApplyDarkButton(HWND hButton)
{
    if (!hButton) return;
    
    // Check if already owner-drawn
    LONG_PTR style = GetWindowLongPtrW(hButton, GWL_STYLE);
    if ((style & BS_OWNERDRAW) == 0)
    {
        style &= ~BS_TYPEMASK;
        style |= BS_OWNERDRAW;
        SetWindowLongPtrW(hButton, GWL_STYLE, style);
        
        // Remove visual styles to prevent conflict
        SetWindowTheme(hButton, L"", L"");
        InvalidateRect(hButton, nullptr, TRUE);
    }
}

void DialogThemeHelper::ApplyDarkCheckbox(HWND hCheckbox)
{
    if (!hCheckbox) return;
    LONG_PTR style = GetWindowLongPtrW(hCheckbox, GWL_STYLE);
    style |= BS_OWNERDRAW;
    SetWindowLongPtrW(hCheckbox, GWL_STYLE, style);
}

void DialogThemeHelper::FillDarkBackground(HDC hdc, const RECT& rect)
{
    FillRect(hdc, &rect, GetDarkBackgroundBrush());
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
    bool isDisabled = (pDrawItem->itemState & ODS_DISABLED) != 0;
    bool isFocused = (pDrawItem->itemState & ODS_FOCUS) != 0;

    // Draw background
    COLORREF bgColor = isPressed ? DARK_BUTTON_PRESSED : DARK_BUTTON_BACKGROUND;
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // Draw border
    HBRUSH hBorder = CreateSolidBrush(DARK_BUTTON_BORDER);
    FrameRect(hdc, &rc, hBorder);
    DeleteObject(hBorder);

    // Draw text
    wchar_t text[256] = {0};
    GetWindowTextW(pDrawItem->hwndItem, text, 256);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isDisabled ? DARK_TEXT_DISABLED : DARK_TEXT);
    
    // Adjust text rect if needed (e.g. slight offset when pressed?) 
    // SettingsDialog didn't do this, simply centered.
    // SettingsDialog used InflateRect(&textRc, -4, -2) which we can keep for safety/padding.
    RECT textRc = rc;
    InflateRect(&textRc, -4, -2);
    DrawTextW(hdc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    // Draw focus rect if needed
    if (isFocused)
    {
        RECT focusRc = rc;
        InflateRect(&focusRc, -3, -3);
        DrawFocusRect(hdc, &focusRc);
    }
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


// Header subclass procedure for dark theme
static LRESULT CALLBACK HeaderSubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam, UINT_PTR /*uIdSubclass*/,
    DWORD_PTR /*dwRefData*/)
{
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Get client rect
        RECT rc;
        GetClientRect(hwnd, &rc);

        // Fill dark background
        DialogThemeHelper::FillDarkBackground(hdc, rc);

        // Draw bottom border only (NO column separators!)
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
        LineTo(hdc, rc.right, rc.bottom - 1);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Set text colors
        SetTextColor(hdc, DialogThemeHelper::DARK_TEXT);
        SetBkMode(hdc, TRANSPARENT);

        // Draw header items WITHOUT separators
        int itemCount = Header_GetItemCount(hwnd);
        for (int i = 0; i < itemCount; i++)
        {
            RECT itemRect;
            Header_GetItemRect(hwnd, i, &itemRect);

            // Get item text
            HDITEMW hdi = { 0 };
            hdi.mask = HDI_TEXT | HDI_FORMAT; // Added HDI_FORMAT to get alignment
            wchar_t text[256];
            hdi.pszText = text;
            hdi.cchTextMax = 256;
            Header_GetItem(hwnd, i, &hdi);

            // Draw text centered/aligned WITHOUT any borders
            RECT textRect = itemRect;
            textRect.left += 5;   // Small padding
            textRect.right -= 5;

            // Determine alignment from item format
            UINT format = DT_LEFT;
            if (hdi.fmt & HDF_CENTER) format = DT_CENTER;
            else if (hdi.fmt & HDF_RIGHT) format = DT_RIGHT;

            DrawTextW(hdc, text, -1, &textRect, format | DT_VCENTER | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    
    // Allow clean removal
    if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hwnd, HeaderSubclassProc, 0);
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void DialogThemeHelper::ApplyDarkHeader(HWND hHeader, bool darkTheme)
{
    if (!hHeader) return;

    if (darkTheme)
    {
        // Set subclass
        SetWindowSubclass(hHeader, HeaderSubclassProc, 0, 0);
    }
    else
    {
        // Remove subclass
        RemoveWindowSubclass(hHeader, HeaderSubclassProc, 0);
    }
}


void DialogThemeHelper::DrawListViewBorder(HDC hdc, HWND hList, HWND hDlg)
{
    if (!hList || !hdc || !hDlg)
        return;

    RECT rcList;
    GetWindowRect(hList, &rcList);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rcList, 2);
    InflateRect(&rcList, 1, 1);
    
    // Use lighter border for better visibility in dark mode (consistent with previous PerApp/ConnectionLog implementation)
    HBRUSH borderBrush = CreateSolidBrush(RGB(100, 100, 100));
    FrameRect(hdc, &rcList, borderBrush);
    DeleteObject(borderBrush);
}

void DialogThemeHelper::ApplyDarkListView(HWND hList, bool darkTheme)
{
    if (!hList) return;

    if (darkTheme)
    {
        // Colors
        ListView_SetBkColor(hList, DARK_PANEL);
        ListView_SetTextBkColor(hList, DARK_PANEL);
        ListView_SetTextColor(hList, DARK_TEXT);

        // Scrollbars and Theme
        ThemeHelper::ApplyDarkThemeToControl(hList, true);
        SetWindowTheme(hList, L"DarkMode_Explorer", nullptr);

        // Remove borders for cleaner look
        LONG_PTR style = GetWindowLongPtrW(hList, GWL_STYLE);
        style &= ~WS_BORDER;
        SetWindowLongPtrW(hList, GWL_STYLE, style);

        LONG_PTR exStyle = GetWindowLongPtrW(hList, GWL_EXSTYLE);
        exStyle &= ~WS_EX_CLIENTEDGE;
        SetWindowLongPtrW(hList, GWL_EXSTYLE, exStyle);

        // Force frame update
        SetWindowPos(hList, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        // Header
        HWND hHeader = ListView_GetHeader(hList);
        if (hHeader)
        {
            ApplyDarkHeader(hHeader, true);
        }
    }
    else
    {
        // Colors
        ListView_SetBkColor(hList, LIGHT_BACKGROUND);
        ListView_SetTextBkColor(hList, LIGHT_BACKGROUND);
        ListView_SetTextColor(hList, LIGHT_TEXT);

        // Scrollbars and Theme
        ThemeHelper::ApplyDarkThemeToControl(hList, false);
        SetWindowTheme(hList, L"Explorer", nullptr);

        // Restore borders
        LONG_PTR style = GetWindowLongPtrW(hList, GWL_STYLE);
        style |= WS_BORDER;
        SetWindowLongPtrW(hList, GWL_STYLE, style);

        // Force frame update
        SetWindowPos(hList, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        // Header
        HWND hHeader = ListView_GetHeader(hList);
        if (hHeader)
        {
            ApplyDarkHeader(hHeader, false);
            SetWindowTheme(hHeader, L"ItemsView", nullptr);
        }
    }
}

} // namespace NetPulse
