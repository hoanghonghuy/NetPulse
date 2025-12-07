#ifndef NETWORK_MONITOR_DIALOG_THEME_HELPER_H
#define NETWORK_MONITOR_DIALOG_THEME_HELPER_H

#include <windows.h>

namespace NetworkMonitor
{

/**
 * DialogThemeHelper - Provides consistent dark theme styling for dialogs
 * Extracted from individual dialogs for SRP compliance
 */
class DialogThemeHelper
{
public:
    // Dark theme colors
    static constexpr COLORREF DARK_BACKGROUND = RGB(32, 32, 32);
    static constexpr COLORREF DARK_BACKGROUND_SELECTED = RGB(50, 50, 50);
    static constexpr COLORREF DARK_TEXT = RGB(230, 230, 230);
    static constexpr COLORREF DARK_BORDER = RGB(80, 80, 80);

    // Light theme colors
    static constexpr COLORREF LIGHT_BACKGROUND = RGB(255, 255, 255);
    static constexpr COLORREF LIGHT_TEXT = RGB(0, 0, 0);

    /**
     * Get or create the dark theme background brush (cached)
     * @return Handle to dark background brush
     */
    static HBRUSH GetDarkBackgroundBrush();

    /**
     * Handle WM_CTLCOLOREDIT/WM_CTLCOLORSTATIC for dark theme
     * @param hdc Device context
     * @param darkTheme true if dark theme enabled
     * @return Brush handle for background
     */
    static HBRUSH HandleControlColor(HDC hdc, bool darkTheme);

    /**
     * Fill a rect with dark background
     * @param hdc Device context
     * @param rect Rectangle to fill
     */
    static void FillDarkBackground(HDC hdc, const RECT& rect);

    /**
     * Draw a dark-themed button
     * @param pDrawItem Draw item struct from WM_DRAWITEM
     * @param darkTheme true if dark theme enabled
     */
    static void DrawButton(DRAWITEMSTRUCT* pDrawItem, bool darkTheme);

    /**
     * Draw a dark-themed tab item
     * @param pDrawItem Draw item struct from WM_DRAWITEM
     * @param darkTheme true if dark theme enabled
     */
    static void DrawTabItem(DRAWITEMSTRUCT* pDrawItem, bool darkTheme);

    /**
     * Apply dark theme to common controls in a dialog
     * @param hDlg Dialog handle
     * @param darkTheme true to apply dark theme
     */
    static void ApplyToDialog(HWND hDlg, bool darkTheme);

    /**
     * Cleanup cached resources (call on application exit)
     */
    static void Cleanup();

private:
    static HBRUSH s_darkBrush;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_DIALOG_THEME_HELPER_H
