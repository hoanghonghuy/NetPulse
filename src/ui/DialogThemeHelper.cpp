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
HBRUSH DialogThemeHelper::s_inputBrush = nullptr;
static ThemeMode s_cachedThemeMode = ThemeMode::SystemDefault; // Track which theme brushes were created for

HBRUSH DialogThemeHelper::GetDarkBackgroundBrush()
{
    ThemeMode currentTheme = ThemeHelper::GetCurrentTheme();
    
    // If theme changed since brush was created, invalidate old brush
    if (s_darkBrush && s_cachedThemeMode != currentTheme)
    {
        DeleteObject(s_darkBrush);
        s_darkBrush = nullptr;
        if (s_inputBrush)
        {
            DeleteObject(s_inputBrush);
            s_inputBrush = nullptr;
        }
    }
    
    if (!s_darkBrush)
    {
        s_darkBrush = CreateSolidBrush(ThemeHelper::GetColors(currentTheme).dialogBackground);
        s_cachedThemeMode = currentTheme;
    }
    return s_darkBrush;
}

HBRUSH DialogThemeHelper::HandleControlColor(HDC hdc, bool darkTheme)
{
    if (darkTheme)
    {
        SetTextColor(hdc, ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).dialogText);
        SetBkMode(hdc, TRANSPARENT); // Use TRANSPARENT instead of SetBkColor for better UI
        return GetDarkBackgroundBrush();
    }
    return nullptr; // Use default system brush
}

HBRUSH DialogThemeHelper::HandleEditControlColor(HDC hdc, bool darkTheme)
{
    if (darkTheme)
    {
        ThemeMode currentTheme = ThemeHelper::GetCurrentTheme();
        const auto& colors = ThemeHelper::GetColors(currentTheme);
        
        SetTextColor(hdc, colors.dialogText);
        SetBkColor(hdc, colors.inputBackground);
        
        // Invalidate s_inputBrush if theme changed (same logic as GetDarkBackgroundBrush)
        // GetDarkBackgroundBrush handles the invalidation, but we need to recreate inputBrush if null
        if (!s_inputBrush)
        {
            s_inputBrush = CreateSolidBrush(colors.inputBackground);
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
    COLORREF bgColor = isPressed ? ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).buttonPressed 
                               : ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).buttonBackground;
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // Draw border
    HBRUSH hBorder = CreateSolidBrush(ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).buttonBorder);
    FrameRect(hdc, &rc, hBorder);
    DeleteObject(hBorder);

    // Draw text
    wchar_t text[256] = {0};
    GetWindowTextW(pDrawItem->hwndItem, text, 256);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isDisabled ? ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).dialogTextDisabled 
                               : ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).dialogText);
    
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
    COLORREF backColor = selected ? ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).tabSelectedBackground 
                               : ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).tabBackground;
    HBRUSH hBrush = CreateSolidBrush(backColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // Draw border for selected tab
    if (selected)
    {
        HPEN hPen = CreatePen(PS_SOLID, 1, ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).dialogBorder);
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
    SetTextColor(hdc, selected ? ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).tabSelectedText
                               : ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).tabText);
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
    if (s_inputBrush)
    {
        DeleteObject(s_inputBrush);
        s_inputBrush = nullptr;
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

        // Fill background with list header color
        HBRUSH hBrush = CreateSolidBrush(ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).listHeaderBackground);
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        // Draw bottom border only (NO column separators!)
        HPEN hPen = CreatePen(PS_SOLID, 1, ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).dialogBorder);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
        LineTo(hdc, rc.right, rc.bottom - 1);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // Set text colors
        // Set text colors
        SetTextColor(hdc, ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).listHeaderText);
        SetBkMode(hdc, TRANSPARENT);

        // Draw header items WITHOUT separators
        int itemCount = Header_GetItemCount(hwnd);
        for (int i = 0; i < itemCount; i++)
        {
            RECT itemRect;
            Header_GetItemRect(hwnd, i, &itemRect);

            // Get item text
            HDITEMW hdi = { 0 };
            hdi.mask = HDI_TEXT | HDI_FORMAT;
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
    
    HBRUSH borderBrush = CreateSolidBrush(ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme()).dialogBorder);
    FrameRect(hdc, &rcList, borderBrush);
    DeleteObject(borderBrush);
}

void DialogThemeHelper::ApplyDarkListView(HWND hList, bool darkTheme)
{
    if (!hList) return;

    // Always use current theme colors - this works for both dark and light presets
    const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());
    ListView_SetBkColor(hList, colors.listBackground);
    ListView_SetTextBkColor(hList, colors.listBackground);
    ListView_SetTextColor(hList, colors.listText);

    if (darkTheme)
    {
        // Dark-specific styling: dark scrollbars, dark window theme
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
        // Light-specific styling: light scrollbars, normal window theme
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
