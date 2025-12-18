#ifndef NETWORK_MONITOR_DIALOG_THEME_HELPER_H
#define NETWORK_MONITOR_DIALOG_THEME_HELPER_H

#include <windows.h>

namespace NetPulse
{

/**
 * DialogThemeHelper - Provides consistent dark theme styling for dialogs
 * Extracted from individual dialogs for SRP compliance
 */
class DialogThemeHelper
{
public:
    // Dark theme colors
    // Dark theme colors (EVKey-inspired "Professional Dark")
    static constexpr COLORREF DARK_BACKGROUND = RGB(32, 34, 37);      // #202225
    static constexpr COLORREF DARK_PANEL = RGB(43, 45, 49);           // #2b2d31
    static constexpr COLORREF DARK_TEXT = RGB(242, 243, 245);         // #f2f3f5
    static constexpr COLORREF DARK_BORDER = RGB(58, 60, 67);          // #3a3c43
    static constexpr COLORREF DARK_BACKGROUND_SELECTED = RGB(43, 45, 49); // Same as Panel

    // Button specific colors (Matches SettingsDialog legacy style)
    static constexpr COLORREF DARK_BUTTON_BACKGROUND = RGB(40, 40, 40);
    static constexpr COLORREF DARK_BUTTON_PRESSED = RGB(50, 50, 50);
    static constexpr COLORREF DARK_BUTTON_BORDER = RGB(90, 90, 90);
    static constexpr COLORREF DARK_TEXT_DISABLED = RGB(160, 160, 160);

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
     * Set thin window border for modern appearance
     * @param hDlg Dialog handle
     */
    static void SetThinWindowBorder(HWND hDlg);

    /**
     * Apply dark theme to common controls in a dialog
     * @param hDlg Dialog handle
     * @param darkTheme true to apply dark theme
     */
    static void ApplyToDialog(HWND hDlg, bool darkTheme);

    /**
     * Apply dark theme to a header control (Subclassing)
     * @param hHeader Header control handle
     * @param darkTheme true to apply dark theme
     */
    static void ApplyDarkHeader(HWND hHeader, bool darkTheme);

    /**
     * Apply dark theme to a ListView control
     * @param hList ListView handle
     * @param darkTheme true to apply dark theme
     */
    static void ApplyDarkListView(HWND hList, bool darkTheme);
    
    /**
     * Apply owner-drawn style to button for dark theme support
     */
    static void ApplyDarkButton(HWND hButton);

    /**
     * Cleanup cached resources (call on application exit)
     */
    static void Cleanup();

private:
    static HBRUSH s_darkBrush;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_DIALOG_THEME_HELPER_H
