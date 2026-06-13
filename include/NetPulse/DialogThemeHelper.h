#ifndef NETWORK_MONITOR_DIALOG_THEME_HELPER_H
#define NETWORK_MONITOR_DIALOG_THEME_HELPER_H

#include "NetPulse/Common.h"

namespace NetPulse
{

/**
 * DialogThemeHelper - Provides consistent dark theme styling for dialogs
 * Extracted from individual dialogs for SRP compliance
 */
class DialogThemeHelper
{
public:
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
     * Handle WM_CTLCOLOREDIT for dark theme
     * @param hdc Device context
     * @param darkTheme true if dark theme enabled
     * @return Brush handle for background
     */
    static HBRUSH HandleEditControlColor(HDC hdc, bool darkTheme);

    /**
     * Apply dark theme style to an Edit control (remove ClientEdge, add Border)
     * @param hEdit Handle to edit control
     */
    static void ApplyDarkEditControl(HWND hEdit);

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
     * Apply owner-drawn style to checkbox for dark theme support
     */
    static void ApplyDarkCheckbox(HWND hCheckbox);

    /**
     * Draw border around ListView
     * @param hdc Device context
     * @param hList ListView handle
     * @param hDlg Dialog handle
     */
    static void DrawListViewBorder(HDC hdc, HWND hList, HWND hDlg);

    /**
     * Cleanup cached resources (call on application exit)
     */
    static void Cleanup();

private:
    static HBRUSH s_darkBrush;
    static HBRUSH s_inputBrush;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_DIALOG_THEME_HELPER_H
