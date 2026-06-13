#include "NetPulse/SettingsDialog.h"
#include "NetPulse/Interfaces/IConfigProvider.h"
#include "NetPulse/Interfaces/INetworkStatsProvider.h"
#include "NetPulse/DialogThemeHelper.h"
#include "NetPulse/Utils.h"
#include "NetPulse/ThemeHelper.h"
#include "../../../resources/resource.h"
#include <cmath>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <ShlObj.h>
#include <vector>


namespace NetPulse
{

// Tab Control subclass procedure for dark theme
static WNDPROC s_originalTabProc = nullptr;
static LRESULT CALLBACK DarkTabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Handle WM_PAINT to completely replace system drawing
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // 1. Fill entire background with dark color
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        DialogThemeHelper::FillDarkBackground(hdc, rcClient);

        // 2. Calculate drawing areas
        RECT displayRect = rcClient;
        TabCtrl_AdjustRect(hwnd, FALSE, &displayRect);
        
        int tabCount = TabCtrl_GetItemCount(hwnd);
        int sel = TabCtrl_GetCurSel(hwnd);

        const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());
        COLORREF borderColor = colors.dialogBorder; 
        HBRUSH borderBrush = CreateSolidBrush(borderColor);
        
        // 3. Draw Page Border (Content Area) - STRICT 1px THIN
        // We draw 1px line *around* the displayRect, not filling the whole margin.
        
        // Left (1px line at displayRect.left - 1)
        RECT leftBorder = { displayRect.left - 1, displayRect.top, displayRect.left, displayRect.bottom };
        FillRect(hdc, &leftBorder, borderBrush);
        
        // Right (1px line at displayRect.right)
        RECT rightBorder = { displayRect.right, displayRect.top, displayRect.right + 1, displayRect.bottom };
        FillRect(hdc, &rightBorder, borderBrush);
        
        // Bottom (1px line at displayRect.bottom)
        RECT bottomBorder = { displayRect.left - 1, displayRect.bottom, displayRect.right + 1, displayRect.bottom + 1 };
        FillRect(hdc, &bottomBorder, borderBrush);
        
        // Top - segments skipping active tab
        RECT selRect = {};
        if (sel >= 0 && sel < tabCount)
        {
            TabCtrl_GetItemRect(hwnd, sel, &selRect);
        }
        
        // Top Left Segment
        if (selRect.left > displayRect.left)
        {
            // Draw from content left edge to selected tab left edge
            RECT topLeftBorder = { displayRect.left - 1, displayRect.top - 1, selRect.left, displayRect.top };
            FillRect(hdc, &topLeftBorder, borderBrush);
        }
        else if (selRect.left <= displayRect.left)
        {
             // If selected tab starts before or at content edge, essentially no top-left border 
             // (or just the tiny corner pixel if needed, but usually fine to skip)
        }
        
        // Top Right Segment
        if (selRect.right < displayRect.right)
        {
            // Draw from selected tab right to content right edge
            RECT topRightBorder = { selRect.right, displayRect.top - 1, displayRect.right + 1, displayRect.top };
            FillRect(hdc, &topRightBorder, borderBrush);
        }

        DeleteObject(borderBrush);
        
        // 4. Draw Tabs
        HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        HGDIOBJ oldFont = SelectObject(hdc, hFont);
        
        SetBkMode(hdc, TRANSPARENT);
        
        for (int i = 0; i < tabCount; ++i)
        {
            RECT rcItem;
            TabCtrl_GetItemRect(hwnd, i, &rcItem);
            
            bool isSel = (i == sel);
            // Match Unikey: Selected tab matches content background
            // Unselected tab is slightly lighter/different to stand out as "behind"
            COLORREF itemBg = isSel ? colors.dialogBackground : colors.dialogPanel; 
            COLORREF textColor = colors.dialogText;
            
            // Draw Tab Background
            HBRUSH itemBrush = CreateSolidBrush(itemBg);
            FillRect(hdc, &rcItem, itemBrush);
            DeleteObject(itemBrush);
            
            // Draw Tab Border
            if (isSel)
            {
                 HBRUSH tabBorderBrush = CreateSolidBrush(borderColor);
                 // Left
                 RECT l = { rcItem.left, rcItem.top, rcItem.left + 1, rcItem.bottom + 1 }; 
                 FillRect(hdc, &l, tabBorderBrush);
                 // Top
                 RECT t = { rcItem.left, rcItem.top, rcItem.right, rcItem.top + 1 };
                 FillRect(hdc, &t, tabBorderBrush);
                 // Right
                 RECT r = { rcItem.right - 1, rcItem.top, rcItem.right, rcItem.bottom + 1 };
                 FillRect(hdc, &r, tabBorderBrush);
                 DeleteObject(tabBorderBrush);
                 
            }
            else
            {
                 // Unselected tabs also need a border to look defined
                 HBRUSH tabBorderBrush = CreateSolidBrush(borderColor);
                 FrameRect(hdc, &rcItem, tabBorderBrush);
                 DeleteObject(tabBorderBrush);
            }
            
            // Draw Text
            wchar_t text[64] = {0};
            TCITEMW tci = { TCIF_TEXT };
            tci.pszText = text;
            tci.cchTextMax = 64;
            TabCtrl_GetItem(hwnd, i, &tci);
            
            SetTextColor(hdc, textColor);
            DrawTextW(hdc, text, -1, &rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        
        SelectObject(hdc, oldFont);
        EndPaint(hwnd, &ps);
        return 0;
    }

    // For other messages, use default
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

        // Now draw over with theme colors
        COMBOBOXINFO cbi = {};
        cbi.cbSize = sizeof(COMBOBOXINFO);
        if (GetComboBoxInfo(hwnd, &cbi))
        {
            HDC hdc = GetDC(hwnd);
            if (hdc)
            {
                const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());
                
                // Draw combobox text area background (cbi.rcItem is the text display area)
                RECT itemRect = cbi.rcItem;
                HBRUSH itemBrush = CreateSolidBrush(colors.inputBackground);
                FillRect(hdc, &itemRect, itemBrush);
                DeleteObject(itemBrush);
                
                // Get and draw the selected text
                wchar_t text[256] = {0};
                GetWindowTextW(hwnd, text, 256);
                if (text[0])
                {
                    SetTextColor(hdc, colors.dialogText);
                    SetBkMode(hdc, TRANSPARENT);
                    HFONT hFont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
                    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
                    
                    // Add some padding
                    itemRect.left += 4;
                    DrawTextW(hdc, text, -1, &itemRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
                    
                    SelectObject(hdc, oldFont);
                }
                
                // Draw dropdown button background
                RECT btnRect = cbi.rcButton;
                COLORREF btnBg = colors.buttonBackground;
                COLORREF btnBorder = colors.buttonBorder;
                COLORREF arrowColor = colors.dialogText;

                HBRUSH hBrush = CreateSolidBrush(btnBg);
                FillRect(hdc, &btnRect, hBrush);
                DeleteObject(hBrush);

                // Draw border around dropdown button
                HBRUSH hBorder = CreateSolidBrush(btnBorder);
                FrameRect(hdc, &btnRect, hBorder);
                DeleteObject(hBorder);

                // Draw dropdown arrow (chevron pointing down)
                int cx = (btnRect.left + btnRect.right) / 2;
                int cy = (btnRect.top + btnRect.bottom) / 2;

                POINT arrow[3] = {
                    {cx - 3, cy - 1},
                    {cx, cy + 2},
                    {cx + 3, cy - 1}
                };

                // Use 2px pen for better visibility/boldness
                HBRUSH arrowBrush = CreateSolidBrush(arrowColor);
                HPEN arrowPen = CreatePen(PS_SOLID, 2, arrowColor);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, arrowBrush);
                HPEN oldPen = (HPEN)SelectObject(hdc, arrowPen);

                Polyline(hdc, arrow, 3);

                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(arrowBrush);
                DeleteObject(arrowPen);

                // Draw entire combobox border with theme color
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                HBRUSH comboBorder = CreateSolidBrush(colors.dialogBorder);
                FrameRect(hdc, &rcClient, comboBorder);
                DeleteObject(comboBorder);

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
    // Classic presets
    {RGB(0, 255, 255), RGB(0, 255, 0), L"Cyan/Green (Default)"},
    {RGB(50, 255, 100), RGB(255, 180, 50), L"Green/Orange"},
    {RGB(100, 200, 255), RGB(255, 100, 100), L"Blue/Red"},
    {RGB(255, 255, 255), RGB(200, 200, 200), L"White/Gray"},
    // New presets
    {RGB(180, 100, 255), RGB(255, 100, 180), L"Purple/Pink"},
    {RGB(0, 200, 255), RGB(255, 200, 0), L"Ocean/Gold"},
    {RGB(0, 255, 128), RGB(128, 0, 255), L"Mint/Violet"},
    {RGB(255, 100, 0), RGB(0, 200, 100), L"Orange/Teal"},
    {RGB(0, 255, 0), RGB(255, 0, 128), L"Neon Green/Magenta"},
    {RGB(100, 150, 255), RGB(255, 150, 100), L"Soft Blue/Peach"},
};
static const int s_overlayColorPresetCount = 10;

SettingsDialog::SettingsDialog()
    : m_hDialog(nullptr)
    , m_pExternalHandle(nullptr)
    , m_pConfigProvider(nullptr)
    , m_pStatsProvider(nullptr)
    , m_isInitializing(false)
{
}

SettingsDialog::~SettingsDialog()
{
}

bool SettingsDialog::GetCheckboxState(UINT ctrlId) const
{
    auto it = m_checkboxStates.find(ctrlId);
    return (it != m_checkboxStates.end()) ? it->second : false;
}

void SettingsDialog::SetCheckboxState(UINT ctrlId, bool checked)
{
    m_checkboxStates[ctrlId] = checked;
}

void SettingsDialog::ToggleCheckboxState(UINT ctrlId)
{
    m_checkboxStates[ctrlId] = !GetCheckboxState(ctrlId);
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
        // Update external handle storage for tracking
        if (pThis->m_pExternalHandle)
        {
            *pThis->m_pExternalHandle = hDlg;
        }
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

            // Apply dark title bar for dark themes only
            ThemeHelper::ApplyDarkTitleBar(hDlg, IsDarkThemeEnabled(m_configCopy));

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

            // Localize Floating Window group
            std::wstring floatingText = LoadStringResource(IDS_SETTINGS_GROUP_FLOATING);
            if (!floatingText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_FLOATING, floatingText.c_str());
            }

            // Localize Tray Icon group
            std::wstring trayGroup = LoadStringResource(IDS_SETTINGS_GROUP_TRAY);
            if (!trayGroup.empty()) SetDlgItemTextW(hDlg, IDC_SETTINGS_GROUP_TRAY, trayGroup.c_str());
            
            std::wstring trayAnim = LoadStringResource(IDS_TRAY_ANIMATION_ENABLE);
            if (!trayAnim.empty()) SetDlgItemTextW(hDlg, IDC_TRAY_ANIMATION_CHECK, trayAnim.c_str());

            std::wstring trayThres = LoadStringResource(IDS_TRAY_ANIMATION_THRESHOLD_LABEL);
            if (!trayThres.empty()) SetDlgItemTextW(hDlg, IDC_TRAY_ANIMATION_THRESHOLD_LABEL, trayThres.c_str());

            // Localize Sparkline Time Range settings
            std::wstring sparklineLabel = LoadStringResource(IDS_SPARKLINE_TIME_RANGE_LABEL);
            if (!sparklineLabel.empty()) SetDlgItemTextW(hDlg, IDC_SPARKLINE_TIME_RANGE_LABEL, sparklineLabel.c_str());

            std::wstring floatNet = LoadStringResource(IDS_FLOATING_SHOW_NETWORK);
            if (!floatNet.empty()) SetDlgItemTextW(hDlg, IDC_FLOATING_SHOW_NETWORK_CHECK, floatNet.c_str());

            std::wstring floatCpu = LoadStringResource(IDS_FLOATING_SHOW_CPU);
            if (!floatCpu.empty()) SetDlgItemTextW(hDlg, IDC_FLOATING_SHOW_CPU_CHECK, floatCpu.c_str());

            std::wstring floatRam = LoadStringResource(IDS_FLOATING_SHOW_RAM);
            if (!floatRam.empty()) SetDlgItemTextW(hDlg, IDC_FLOATING_SHOW_RAM_CHECK, floatRam.c_str());

            std::wstring floatPing = LoadStringResource(IDS_FLOATING_SHOW_PING);
            if (!floatPing.empty()) SetDlgItemTextW(hDlg, IDC_FLOATING_SHOW_PING_CHECK, floatPing.c_str());

            std::wstring floatDataToday = LoadStringResource(IDS_FLOATING_SHOW_DATA_TODAY);
            if (!floatDataToday.empty()) SetDlgItemTextW(hDlg, IDC_FLOATING_SHOW_DATA_TODAY_CHECK, floatDataToday.c_str());

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

            std::wstring autostartAdminText = LoadStringResource(IDS_SETTINGS_LABEL_AUTOSTART_ADMIN);
            if (!autostartAdminText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_AUTOSTART_ADMIN_CHECK, autostartAdminText.c_str());
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
                SetDlgItemTextW(hDlg, IDC_HISTORY_TRIM_LABEL, trimLabelText.c_str());
            }

            // Localize new labels (previously missing i18n)
            std::wstring overlayFontSizeText = LoadStringResource(IDS_SETTINGS_LABEL_OVERLAY_FONT_SIZE);
            if (!overlayFontSizeText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_FONT_SIZE_LABEL, overlayFontSizeText.c_str());
            }

            std::wstring overlayColorsText = LoadStringResource(IDS_SETTINGS_LABEL_OVERLAY_COLORS);
            if (!overlayColorsText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_OVERLAY_COLOR_LABEL, overlayColorsText.c_str());
            }

            std::wstring pingTargetText = LoadStringResource(IDS_SETTINGS_LABEL_PING_TARGET);
            if (!pingTargetText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_PING_TARGET_LABEL, pingTargetText.c_str());
            }

            std::wstring pingIntervalText = LoadStringResource(IDS_SETTINGS_LABEL_PING_INTERVAL);
            if (!pingIntervalText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_PING_INTERVAL_LABEL, pingIntervalText.c_str());
            }

            std::wstring hotkeyText = LoadStringResource(IDS_SETTINGS_LABEL_HOTKEY);
            if (!hotkeyText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_HOTKEY_LABEL, hotkeyText.c_str());
            }

            std::wstring quotaLabelText = LoadStringResource(IDS_DATA_USAGE_QUOTA_GB);
            if (!quotaLabelText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DATA_USAGE_QUOTA_LABEL, quotaLabelText.c_str());
            }

            std::wstring dataUsageEnableText = LoadStringResource(IDS_SETTINGS_LABEL_DATA_USAGE_ENABLE);
            if (!dataUsageEnableText.empty())
            {
                SetDlgItemTextW(hDlg, IDC_DATA_USAGE_ENABLE_CHECK, dataUsageEnableText.c_str());
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

            // Create Portable Mode controls dynamically (not in resource file)
            {
                // Position them below Connection Notify checkbox in General tab
                HWND hConnNotify = GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK);
                RECT rcConnNotify = {};
                if (hConnNotify)
                {
                    GetWindowRect(hConnNotify, &rcConnNotify);
                    MapWindowPoints(nullptr, hDlg, reinterpret_cast<LPPOINT>(&rcConnNotify), 2);
                }
                
                HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(hDlg, WM_GETFONT, 0, 0));
                int baseY = rcConnNotify.bottom + 15;
                
                // Detect if running from Program Files
                bool inProgramFiles = false;
                wchar_t exePath[MAX_PATH] = {0};
                if (GetModuleFileNameW(nullptr, exePath, MAX_PATH))
                {
                    wchar_t progFiles[MAX_PATH] = {0};
                    wchar_t progFilesX86[MAX_PATH] = {0};
                    
                    SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, progFiles);
                    SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, progFilesX86);
                    
                    auto isUnderFolder = [](const std::wstring& path, const std::wstring& folder) -> bool {
                        if (folder.empty() || path.empty())
                            return false;
                        if (path.size() < folder.size())
                            return false;
                        if (_wcsnicmp(path.c_str(), folder.c_str(), folder.size()) != 0)
                            return false;
                        if (path.size() == folder.size())
                            return true;
                        wchar_t nextChar = path[folder.size()];
                        return (nextChar == L'\\' || nextChar == L'/');
                    };
                    
                    if (isUnderFolder(exePath, progFiles) || isUnderFolder(exePath, progFilesX86))
                    {
                        inProgramFiles = true;
                    }
                }

                // Create checkbox for "Use Portable Mode"
                std::wstring checkLabel = LoadStringResource(IDS_PORTABLE_MODE_CHECK);
                if (checkLabel.empty()) checkLabel = L"Use Portable Mode";
                
                if (inProgramFiles)
                {
                    std::wstring suffix = L" (N/A in Program Files)";
                    if (m_configCopy.language == AppLanguage::Vietnamese)
                    {
                        suffix = L" (Không khả dụng trong Program Files)";
                    }
                    else if (m_configCopy.language == AppLanguage::Japanese)
                    {
                        // L" (Program Files内では無効)" có nghĩa là "(Không khả dụng trong Program Files)" trong tiếng Nhật.
                        suffix = L" (Program Files内では無効)";
                    }
                    else if (m_configCopy.language == AppLanguage::Korean)
                    {
                        suffix = L" (Program Files에서 사용 불가)";
                    }
                    else if (m_configCopy.language == AppLanguage::ChineseSimplified)
                    {
                        suffix = L" (在 Program Files 中不可用)";
                    }
                    checkLabel += suffix;
                }
                
                HWND hPortableCheck = CreateWindowExW(
                    0, L"BUTTON", checkLabel.c_str(),
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    rcConnNotify.left, baseY, inProgramFiles ? 320 : 160, 20,
                    hDlg, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PORTABLE_MODE_CHECK)),
                    GetModuleHandleW(nullptr), nullptr
                );
                
                if (hPortableCheck && hFont)
                {
                    SendMessageW(hPortableCheck, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
                }
                
                // Create button for "Create Portable Config"
                std::wstring btnLabel = LoadStringResource(IDS_PORTABLE_MODE_BUTTON);
                if (btnLabel.empty()) btnLabel = L"Create Portable Config";
                
                HWND hPortableBtn = CreateWindowExW(
                    0, L"BUTTON", btnLabel.c_str(),
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    rcConnNotify.left + 170, baseY - 2, 150, 24,
                    hDlg, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PORTABLE_MODE_BUTTON)),
                    GetModuleHandleW(nullptr), nullptr
                );
                
                if (hPortableBtn && hFont)
                {
                    SendMessageW(hPortableBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
                }
                
                // Set initial states based on config provider
                if (m_pConfigProvider)
                {
                    bool fileExists = m_pConfigProvider->HasPortableConfigFile();
                    bool isPortable = m_pConfigProvider->IsPortableMode();
                    
                    // Checkbox: enabled only if file exists, checked if portable mode active
                    if (hPortableCheck)
                    {
                        if (inProgramFiles)
                        {
                            EnableWindow(hPortableCheck, FALSE);
                            Button_SetCheck(hPortableCheck, BST_UNCHECKED);
                            SetCheckboxState(IDC_PORTABLE_MODE_CHECK, false);
                        }
                        else
                        {
                            EnableWindow(hPortableCheck, fileExists);
                            Button_SetCheck(hPortableCheck, isPortable ? BST_CHECKED : BST_UNCHECKED);
                            SetCheckboxState(IDC_PORTABLE_MODE_CHECK, isPortable);
                        }
                    }
                    
                    // Button: disabled if file already exists or running from Program Files
                    if (hPortableBtn)
                    {
                        if (inProgramFiles)
                        {
                            EnableWindow(hPortableBtn, FALSE);
                            ShowWindow(hPortableBtn, SW_HIDE);
                        }
                        else
                        {
                            EnableWindow(hPortableBtn, !fileExists);
                        }
                    }
                }
            }

            // In custom theme (Dark OR Custom Light), disable visual styles and use owner-draw
            if (IsCustomThemeEnabled(m_configCopy))
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

                // Make bottom buttons owner-drawn for dark backgrounds
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_SETTINGS_BUTTON_OPEN_LOG));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDOK));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_SETTINGS_BUTTON_APPLY));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDCANCEL));
                DialogThemeHelper::ApplyDarkButton(GetDlgItem(hDlg, IDC_PORTABLE_MODE_BUTTON));

                // Make checkboxes owner-draw for dark theme
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_DATA_USAGE_ENABLE_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_NETWORK_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_CPU_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_RAM_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_PING_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_DATA_TODAY_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_SPARKLINE_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_VPN_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_FLOATING_SHOW_IP_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_TRAY_ANIMATION_CHECK));
                DialogThemeHelper::ApplyDarkCheckbox(GetDlgItem(hDlg, IDC_PORTABLE_MODE_CHECK));

                // Clear default button to prevent the system from drawing an
                // initial white default highlight before owner-draw kicks in.
                SendMessageW(hDlg, DM_SETDEFID, 0, 0);
            }
            return TRUE;
        }



        // Handle Edit Control Colors for Dark Theme
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HBRUSH hBrush = DialogThemeHelper::HandleEditControlColor(hdc, IsCustomThemeEnabled(m_configCopy));
        if (hBrush)
        {
            return reinterpret_cast<INT_PTR>(hBrush);
        }
        break;
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

                case IDC_PORTABLE_MODE_BUTTON:
                {
                    if (m_pConfigProvider)
                    {
                        if (m_pConfigProvider->HasPortableConfigFile())
                        {
                            // File already exists
                            std::wstring msg = LoadStringResource(IDS_PORTABLE_MODE_ALREADY);
                            if (msg.empty()) msg = L"Portable config file already exists";
                            MessageBoxW(hDlg, msg.c_str(), L"NetPulse", MB_OK | MB_ICONINFORMATION);
                        }
                        else
                        {
                            // Try to create portable config
                            if (m_pConfigProvider->EnablePortableMode(m_configCopy))
                            {
                                std::wstring msg = LoadStringResource(IDS_PORTABLE_MODE_SUCCESS);
                                if (msg.empty()) msg = L"Portable configuration created successfully.\nSettings will now be saved to netpulse.ini";
                                MessageBoxW(hDlg, msg.c_str(), L"NetPulse", MB_OK | MB_ICONINFORMATION);
                                
                                // Disable the button since file now exists
                                HWND hBtn = GetDlgItem(hDlg, IDC_PORTABLE_MODE_BUTTON);
                                if (hBtn) EnableWindow(hBtn, FALSE);
                                
                                // Enable and check the checkbox
                                HWND hCheck = GetDlgItem(hDlg, IDC_PORTABLE_MODE_CHECK);
                                if (hCheck)
                                {
                                    EnableWindow(hCheck, TRUE);
                                    Button_SetCheck(hCheck, BST_CHECKED);
                                    SetCheckboxState(IDC_PORTABLE_MODE_CHECK, true);
                                    if (IsCustomThemeEnabled(m_configCopy))
                                    {
                                        InvalidateRect(hCheck, nullptr, FALSE);
                                        UpdateWindow(hCheck);
                                    }
                                }
                            }
                            else
                            {
                                std::wstring msg = LoadStringResource(IDS_PORTABLE_MODE_FAILED);
                                if (msg.empty()) msg = L"Failed to create portable configuration file.";
                                MessageBoxW(hDlg, msg.c_str(), L"NetPulse", MB_OK | MB_ICONERROR);
                            }
                        }
                    }
                    return TRUE;
                }

                case IDC_PORTABLE_MODE_CHECK:
                {
                    if (HIWORD(wParam) == BN_CLICKED && m_pConfigProvider)
                    {
                        HWND hCheck = GetDlgItem(hDlg, IDC_PORTABLE_MODE_CHECK);
                        if (hCheck)
                        {
                            if (IsCustomThemeEnabled(m_configCopy))
                            {
                                ToggleCheckboxState(IDC_PORTABLE_MODE_CHECK);
                                InvalidateRect(hCheck, nullptr, FALSE);
                                UpdateWindow(hCheck);
                            }
                            // Note: Actual portable mode change is handled in ApplySettingsFromDialog
                        }
                    }
                    return TRUE;
                }


                case IDOK:
                {
                    // Apply settings (internally checks for changes)
                    if (ApplySettingsFromDialog(hDlg))
                    {
                        if (m_settingsChangedCallback)
                        {
                            m_settingsChangedCallback();
                        }
                    }
                    // Clear external handle storage before closing
                    if (m_pExternalHandle)
                    {
                        *m_pExternalHandle = nullptr;
                    }
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDC_SETTINGS_BUTTON_APPLY:
                {
                    // Remember current theme before applying
                    bool oldDarkTheme = m_configCopy.darkTheme;
                    ThemeMode oldThemeMode = m_configCopy.themeMode;
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
                            m_configCopy.themeMode != oldThemeMode ||
                            m_configCopy.language != oldLanguage)
                        {
                            // Clear external handle storage before closing
                            if (m_pExternalHandle)
                            {
                                *m_pExternalHandle = nullptr;
                            }
                            EndDialog(hDlg, IDAPPLY_REOPEN);
                        }
                    }
                    return TRUE;
                }

                case IDCANCEL:
                    // Clear external handle storage before closing
                    if (m_pExternalHandle)
                    {
                        *m_pExternalHandle = nullptr;
                    }
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;

                // Handle owner-draw checkbox clicks
                case IDC_AUTOSTART_CHECK:
                case IDC_AUTOSTART_ADMIN_CHECK:
                case IDC_ENABLE_LOGGING_CHECK:
                case IDC_DEBUG_LOGGING_CHECK:
                case IDC_CONNECTION_NOTIFY_CHECK:
                case IDC_DATA_USAGE_ENABLE_CHECK:
                case IDC_FLOATING_SHOW_NETWORK_CHECK:
                case IDC_FLOATING_SHOW_CPU_CHECK:
                case IDC_FLOATING_SHOW_RAM_CHECK:
                case IDC_FLOATING_SHOW_PING_CHECK:
                case IDC_FLOATING_SHOW_DATA_TODAY_CHECK:
                case IDC_FLOATING_SHOW_SPARKLINE_CHECK:
                case IDC_FLOATING_SHOW_VPN_CHECK:
                case IDC_FLOATING_SHOW_IP_CHECK:
                case IDC_TRAY_ANIMATION_CHECK:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        UINT ctrlId = LOWORD(wParam);
                        HWND hCheck = GetDlgItem(hDlg, ctrlId);
                        
                        if (hCheck && IsCustomThemeEnabled(m_configCopy))
                        {
                            // Toggle custom checkbox state (BS_OWNERDRAW doesn't store state)
                            ToggleCheckboxState(ctrlId);
                            
                            // Force immediate repaint to prevent paint coalescing
                            InvalidateRect(hCheck, nullptr, FALSE);
                            UpdateWindow(hCheck);
                        }
                        
                        // Special handling: Enable/disable Admin checkbox based on Autostart state
                        if (ctrlId == IDC_AUTOSTART_CHECK)
                        {
                            bool autoStartChecked = IsCustomThemeEnabled(m_configCopy) 
                                ? GetCheckboxState(IDC_AUTOSTART_CHECK)
                                : (Button_GetCheck(hCheck) == BST_CHECKED);
                            EnableWindow(GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK), autoStartChecked);
                            
                            // If unchecking Autostart, also uncheck Admin
                            if (!autoStartChecked)
                            {
                                if (IsCustomThemeEnabled(m_configCopy))
                                {
                                    SetCheckboxState(IDC_AUTOSTART_ADMIN_CHECK, false);
                                    HWND hAdminCheck = GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK);
                                    InvalidateRect(hAdminCheck, nullptr, FALSE);
                                    UpdateWindow(hAdminCheck);
                                }
                                else
                                {
                                    Button_SetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK), BST_UNCHECKED);
                                }
                            }
                        }
                        
                        if (IsCustomThemeEnabled(m_configCopy))
                            return TRUE;
                    }
                }

                default:
                    break;
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
            // Handle Tab Control custom draw for custom theme background
            if (pnmh->idFrom == IDC_SETTINGS_TAB && pnmh->code == NM_CUSTOMDRAW && IsCustomThemeEnabled(m_configCopy))
            {
                LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                switch (pNMCD->dwDrawStage)
                {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                    case CDDS_PREERASE:
                    {
                        // Fill entire tab control area with theme background
                        DialogThemeHelper::FillDarkBackground(pNMCD->hdc, pNMCD->rc);
                        return CDRF_SKIPDEFAULT;
                    }
                }
            }
            break;
        }

        case WM_ERASEBKGND:
        {
            if (IsCustomThemeEnabled(m_configCopy))
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                RECT rc;
                GetClientRect(hDlg, &rc);
                DialogThemeHelper::FillDarkBackground(hdc, rc);
                return TRUE;
            }
            break;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:

        {
            if (IsCustomThemeEnabled(m_configCopy))
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
                    (ctrlId == IDC_THEME_MODE_COMBO) ||
                    (ctrlId == IDC_PING_INTERVAL_COMBO) ||
                    (ctrlId == IDC_HOTKEY_COMBO) ||
                    (ctrlId == IDC_OVERLAY_FONT_SIZE_COMBO) ||
                    (ctrlId == IDC_OVERLAY_COLOR_COMBO) ||
                    (ctrlId == IDC_TRAY_ANIMATION_THRESHOLD) ||
                    (ctrlId == IDC_SPARKLINE_TIME_RANGE_COMBO);

                if (message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLOREDIT || isComboArea)
                {
                    // For combobox edit/dropdown areas: fill opaque theme background
                    const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());
                    SetTextColor(hdc, colors.dialogText);
                    SetBkColor(hdc, colors.dialogBackground);
                    SetBkMode(hdc, OPAQUE);
                }
                else
                {
                    // For labels, group boxes, buttons: transparent over dark dialog
                    HBRUSH hBrush = DialogThemeHelper::HandleControlColor(hdc, true);
                    if (hBrush) return reinterpret_cast<INT_PTR>(hBrush);
                }

                return reinterpret_cast<INT_PTR>(darkBrush);
            }
            break;
        }
        case WM_DRAWITEM:
        {
            if (IsCustomThemeEnabled(m_configCopy))
            {
                DRAWITEMSTRUCT* pDrawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (pDrawItem->CtlType == ODT_BUTTON)
                {
                    UINT id = pDrawItem->CtlID;
                    if (id == IDC_SETTINGS_BUTTON_OPEN_LOG || id == IDOK || 
                        id == IDC_SETTINGS_BUTTON_APPLY || id == IDCANCEL ||
                        id == IDC_PORTABLE_MODE_BUTTON)
                    {
                        DialogThemeHelper::DrawButton(pDrawItem, true);
                        return TRUE;
                    }
                }
                // Handle Tab Control owner-draw
                else if (pDrawItem->CtlType == ODT_TAB)
                {
                    DialogThemeHelper::DrawTabItem(pDrawItem, true);
                    return TRUE;
                }
                // Handle Checkbox owner-draw
                UINT ctlId = pDrawItem->CtlID;
                if (ctlId == IDC_AUTOSTART_CHECK || ctlId == IDC_AUTOSTART_ADMIN_CHECK ||
                    ctlId == IDC_ENABLE_LOGGING_CHECK || ctlId == IDC_DEBUG_LOGGING_CHECK ||
                    ctlId == IDC_CONNECTION_NOTIFY_CHECK || ctlId == IDC_DATA_USAGE_ENABLE_CHECK ||
                    ctlId == IDC_FLOATING_SHOW_NETWORK_CHECK || ctlId == IDC_FLOATING_SHOW_CPU_CHECK ||
                    ctlId == IDC_FLOATING_SHOW_RAM_CHECK || ctlId == IDC_FLOATING_SHOW_PING_CHECK ||
                    ctlId == IDC_FLOATING_SHOW_DATA_TODAY_CHECK || ctlId == IDC_FLOATING_SHOW_SPARKLINE_CHECK ||
                    ctlId == IDC_FLOATING_SHOW_VPN_CHECK || ctlId == IDC_FLOATING_SHOW_IP_CHECK ||
                    ctlId == IDC_TRAY_ANIMATION_CHECK || ctlId == IDC_PORTABLE_MODE_CHECK)
                {
                    HDC hdc = pDrawItem->hDC;
                    RECT rc = pDrawItem->rcItem;
                    bool disabled = (pDrawItem->itemState & ODS_DISABLED) != 0;

                    // Get check state from our custom map (BS_OWNERDRAW doesn't store state)
                    bool checked = GetCheckboxState(ctlId);

                    // Get theme colors for checkbox drawing
                    const auto& colors = ThemeHelper::GetColors(ThemeHelper::GetCurrentTheme());

                    // Fill background using cached brush
                    FillRect(hdc, &rc, DialogThemeHelper::GetDarkBackgroundBrush());

                    // Draw checkbox box (13x13 pixels)
                    int boxSize = 13;
                    int boxY = rc.top + (rc.bottom - rc.top - boxSize) / 2;
                    RECT boxRect = {rc.left, boxY, rc.left + boxSize, boxY + boxSize};

                    // Box background and border - using theme colors
                    SetDCBrushColor(hdc, colors.inputBackground);
                    FillRect(hdc, &boxRect, (HBRUSH)GetStockObject(DC_BRUSH));
                    SetDCBrushColor(hdc, colors.dialogBorder);
                    FrameRect(hdc, &boxRect, (HBRUSH)GetStockObject(DC_BRUSH));

                    // Draw checkmark if checked - use accent color from theme
                    if (checked)
                    {
                        SetDCPenColor(hdc, disabled ? RGB(120, 120, 120) : colors.download);
                        SelectObject(hdc, GetStockObject(DC_PEN));
                        int cx = boxRect.left + 6, cy = boxRect.top + 6;
                        MoveToEx(hdc, cx - 3, cy, NULL); LineTo(hdc, cx - 1, cy + 3); LineTo(hdc, cx + 4, cy - 3);
                        MoveToEx(hdc, cx - 3, cy + 1, NULL); LineTo(hdc, cx - 1, cy + 4); LineTo(hdc, cx + 4, cy - 2);
                    }

                    // Draw text - use cached text position
                    wchar_t text[64] = {0};
                    int textLen = GetWindowTextW(pDrawItem->hwndItem, text, 64);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, disabled ? RGB(120, 120, 120) : colors.dialogText);
                    int textX = boxRect.right + 6;
                    int textY = rc.top + (rc.bottom - rc.top - 16) / 2;  // Assume 16px font height
                    ExtTextOutW(hdc, textX, textY, 0, nullptr, text, textLen, nullptr);

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
            {ThemeMode::Dracula,       0},
            {ThemeMode::Cyberpunk,     0},
            {ThemeMode::Nord,          0},
            {ThemeMode::Forest,        0},
            {ThemeMode::OLED,          0},
            // New Light Presets
            {ThemeMode::SolarizedLight,0},
            {ThemeMode::MorningMist,   0},
            {ThemeMode::SoftPaper,     0},
            {ThemeMode::MintFresh,     0},
            {ThemeMode::Lavender,      0},
            {ThemeMode::RosePink,      0},
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
                case ThemeMode::Dracula:
                    label = L"Dracula";
                    break;
                case ThemeMode::Cyberpunk:
                    label = L"Cyberpunk / Neon";
                    break;
                case ThemeMode::Nord:
                    label = L"Nord";
                    break;
                case ThemeMode::Forest:
                    label = L"Forest / Matcha";
                    break;
                case ThemeMode::OLED:
                    label = L"OLED Black";
                    break;
                case ThemeMode::SolarizedLight:
                    label = L"Solarized Light";
                    break;
                case ThemeMode::MorningMist:
                    label = L"Morning Mist (Blue)";
                    break;
                case ThemeMode::SoftPaper:
                    label = L"Soft Paper (Warm)";
                    break;
                case ThemeMode::MintFresh:
                    label = L"Mint Fresh (Green)";
                    break;
                case ThemeMode::Lavender:
                    label = L"Lavender (Purple)";
                    break;
                case ThemeMode::RosePink:
                    label = L"Rose Pink";
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
    SetCheckboxState(IDC_AUTOSTART_CHECK, m_configCopy.autoStart);
    Button_SetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK), m_configCopy.autoStartAsAdmin ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_AUTOSTART_ADMIN_CHECK, m_configCopy.autoStartAsAdmin);
    EnableWindow(GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK), m_configCopy.autoStart);
    Button_SetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK), m_configCopy.enableLogging ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_ENABLE_LOGGING_CHECK, m_configCopy.enableLogging);
    Button_SetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK), m_configCopy.debugLogging ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_DEBUG_LOGGING_CHECK, m_configCopy.debugLogging);
    Button_SetCheck(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK), m_configCopy.enableConnectionNotification ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_CONNECTION_NOTIFY_CHECK, m_configCopy.enableConnectionNotification);
    Button_SetCheck(GetDlgItem(hDlg, IDC_DATA_USAGE_ENABLE_CHECK), m_configCopy.enableDataUsageAlerts ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_DATA_USAGE_ENABLE_CHECK, m_configCopy.enableDataUsageAlerts);

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

    // Apply theme styling to all comboboxes (for both dark and light custom themes)
    if (IsCustomThemeEnabled(m_configCopy))
    {
        bool useDarkScrollbar = IsDarkThemeEnabled(m_configCopy);
        
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
        HWND hTrayThresholdTheme = GetDlgItem(hDlg, IDC_TRAY_ANIMATION_THRESHOLD);
        HWND hSparklineTimeTheme = GetDlgItem(hDlg, IDC_SPARKLINE_TIME_RANGE_COMBO);

        if (hLangTheme)    ThemeHelper::ApplyDarkThemeToControl(hLangTheme, useDarkScrollbar);
        if (hIntTheme)     ThemeHelper::ApplyDarkThemeToControl(hIntTheme, useDarkScrollbar);
        if (hUnitTheme)    ThemeHelper::ApplyDarkThemeToControl(hUnitTheme, useDarkScrollbar);
        if (hIfaceTheme)   ThemeHelper::ApplyDarkThemeToControl(hIfaceTheme, useDarkScrollbar);
        if (hTrimTheme)    ThemeHelper::ApplyDarkThemeToControl(hTrimTheme, useDarkScrollbar);
        if (hThemeModeCB)  ThemeHelper::ApplyDarkThemeToControl(hThemeModeCB, useDarkScrollbar);
        if (hPingIntTheme) ThemeHelper::ApplyDarkThemeToControl(hPingIntTheme, useDarkScrollbar);
        if (hHotkeyTheme)  ThemeHelper::ApplyDarkThemeToControl(hHotkeyTheme, useDarkScrollbar);
        if (hFontSizeTheme) ThemeHelper::ApplyDarkThemeToControl(hFontSizeTheme, useDarkScrollbar);
        if (hOverlayColorTheme) ThemeHelper::ApplyDarkThemeToControl(hOverlayColorTheme, useDarkScrollbar);
        if (hTrayThresholdTheme) ThemeHelper::ApplyDarkThemeToControl(hTrayThresholdTheme, useDarkScrollbar);
        if (hSparklineTimeTheme) ThemeHelper::ApplyDarkThemeToControl(hSparklineTimeTheme, useDarkScrollbar);

        // Apply dark theme style to Edit controls (remove thick client edge)
        const int editControls[] = {
            IDC_PING_TARGET_EDIT,
            IDC_DATA_USAGE_QUOTA_EDIT, 
            IDC_DATA_USAGE_THRESHOLD1_EDIT, 
            IDC_DATA_USAGE_THRESHOLD2_EDIT
        };

        for (int id : editControls)
        {
            DialogThemeHelper::ApplyDarkEditControl(GetDlgItem(hDlg, id));
        }

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
        subclassComboBox(hTrayThresholdTheme);
        subclassComboBox(hSparklineTimeTheme);
    }

    // === Data Usage Alerts ===
    HWND hDataUsageEnable = GetDlgItem(hDlg, IDC_DATA_USAGE_ENABLE_CHECK);
    if (hDataUsageEnable)
    {
        Button_SetCheck(hDataUsageEnable, m_configCopy.enableDataUsageAlerts ? BST_CHECKED : BST_UNCHECKED);
    }

    // Initialize Floating Window checkboxes
    HWND hFloatNet = GetDlgItem(hDlg, IDC_FLOATING_SHOW_NETWORK_CHECK);
    if (hFloatNet) Button_SetCheck(hFloatNet, m_configCopy.floatingShowNetwork ? BST_CHECKED : BST_UNCHECKED);

    HWND hFloatCpu = GetDlgItem(hDlg, IDC_FLOATING_SHOW_CPU_CHECK);
    if (hFloatCpu) Button_SetCheck(hFloatCpu, m_configCopy.floatingShowCPU ? BST_CHECKED : BST_UNCHECKED);

    HWND hFloatRam = GetDlgItem(hDlg, IDC_FLOATING_SHOW_RAM_CHECK);
    if (hFloatRam) Button_SetCheck(hFloatRam, m_configCopy.floatingShowRAM ? BST_CHECKED : BST_UNCHECKED);

    // Track checkbox state for dark theme mode
    SetCheckboxState(IDC_FLOATING_SHOW_NETWORK_CHECK, m_configCopy.floatingShowNetwork);
    SetCheckboxState(IDC_FLOATING_SHOW_CPU_CHECK, m_configCopy.floatingShowCPU);
    SetCheckboxState(IDC_FLOATING_SHOW_RAM_CHECK, m_configCopy.floatingShowRAM);

    // Initialize Ping checkbox
    HWND hFloatPing = GetDlgItem(hDlg, IDC_FLOATING_SHOW_PING_CHECK);
    if (hFloatPing) Button_SetCheck(hFloatPing, m_configCopy.floatingShowPing ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_FLOATING_SHOW_PING_CHECK, m_configCopy.floatingShowPing);

    // Initialize Data Today checkbox
    HWND hFloatData = GetDlgItem(hDlg, IDC_FLOATING_SHOW_DATA_TODAY_CHECK);
    if (hFloatData) Button_SetCheck(hFloatData, m_configCopy.floatingShowDataToday ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_FLOATING_SHOW_DATA_TODAY_CHECK, m_configCopy.floatingShowDataToday);

    // Initialize Sparkline checkbox
    HWND hFloatSparkline = GetDlgItem(hDlg, IDC_FLOATING_SHOW_SPARKLINE_CHECK);
    if (hFloatSparkline) Button_SetCheck(hFloatSparkline, m_configCopy.floatingShowSparkline ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_FLOATING_SHOW_SPARKLINE_CHECK, m_configCopy.floatingShowSparkline);

    // Initialize VPN Status checkbox (Phase 3)
    HWND hFloatVpn = GetDlgItem(hDlg, IDC_FLOATING_SHOW_VPN_CHECK);
    if (hFloatVpn) Button_SetCheck(hFloatVpn, m_configCopy.floatingShowVpnStatus ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_FLOATING_SHOW_VPN_CHECK, m_configCopy.floatingShowVpnStatus);

    // Initialize Public IP checkbox (Phase 3)
    HWND hFloatIP = GetDlgItem(hDlg, IDC_FLOATING_SHOW_IP_CHECK);
    if (hFloatIP) Button_SetCheck(hFloatIP, m_configCopy.floatingShowPublicIP ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_FLOATING_SHOW_IP_CHECK, m_configCopy.floatingShowPublicIP);

    // Initialize Tray Animation checkbox
    HWND hTrayAnimCheck = GetDlgItem(hDlg, IDC_TRAY_ANIMATION_CHECK);
    if (hTrayAnimCheck) Button_SetCheck(hTrayAnimCheck, m_configCopy.trayAnimationEnabled ? BST_CHECKED : BST_UNCHECKED);
    SetCheckboxState(IDC_TRAY_ANIMATION_CHECK, m_configCopy.trayAnimationEnabled);
    
    // Initialize Tray Animation threshold combo
    HWND hTrayAnimThreshold = GetDlgItem(hDlg, IDC_TRAY_ANIMATION_THRESHOLD);
    if (hTrayAnimThreshold)
    {
        ComboBox_ResetContent(hTrayAnimThreshold);
        // Add threshold options: 256 KB/s, 512 KB/s, 1 MB/s, 2 MB/s, 5 MB/s
        const int thresholds[] = { 256, 512, 1024, 2048, 5120 };
        const wchar_t* labels[] = { L"256 KB/s", L"512 KB/s", L"1 MB/s", L"2 MB/s", L"5 MB/s" };
        int selectedIdx = 2; // default to 1 MB/s
        for (int i = 0; i < 5; i++)
        {
            ComboBox_AddString(hTrayAnimThreshold, labels[i]);
            ComboBox_SetItemData(hTrayAnimThreshold, i, thresholds[i]);
            if (m_configCopy.trayAnimationThresholdKB == thresholds[i])
            {
                selectedIdx = i;
            }
        }
        ComboBox_SetCurSel(hTrayAnimThreshold, selectedIdx);
    }
    
    // Initialize Sparkline Time Range combo
    HWND hSparklineTime = GetDlgItem(hDlg, IDC_SPARKLINE_TIME_RANGE_COMBO);
    if (hSparklineTime)
    {
        ComboBox_ResetContent(hSparklineTime);
        std::wstring opt30s = LoadStringResource(IDS_SPARKLINE_30S);
        std::wstring opt1m = LoadStringResource(IDS_SPARKLINE_1M);
        std::wstring opt5m = LoadStringResource(IDS_SPARKLINE_5M);
        ComboBox_AddString(hSparklineTime, opt30s.empty() ? L"30s" : opt30s.c_str());
        ComboBox_AddString(hSparklineTime, opt1m.empty() ? L"1m" : opt1m.c_str());
        ComboBox_AddString(hSparklineTime, opt5m.empty() ? L"5m" : opt5m.c_str());
        ComboBox_SetCurSel(hSparklineTime, m_configCopy.sparklineTimeRange);
    }

    HWND hDataQuota = GetDlgItem(hDlg, IDC_DATA_USAGE_QUOTA_EDIT);
    if (hDataQuota)
    {
        wchar_t buf[32];
        swprintf_s(buf, L"%.1f", m_configCopy.dataQuotaGB);
        SetWindowTextW(hDataQuota, buf);
    }

    // Force repaint of all comboboxes after population to apply dark theme arrow
    if (IsCustomThemeEnabled(m_configCopy))
    {
        const UINT comboIds[] = {
            IDC_LANGUAGE_COMBO, IDC_UPDATE_INTERVAL_COMBO, IDC_DISPLAY_UNIT_COMBO,
            IDC_INTERFACE_COMBO, IDC_HISTORY_AUTO_TRIM_COMBO, IDC_THEME_MODE_COMBO,
            IDC_PING_INTERVAL_COMBO, IDC_HOTKEY_COMBO, IDC_OVERLAY_FONT_SIZE_COMBO,
            IDC_OVERLAY_COLOR_COMBO, IDC_TRAY_ANIMATION_THRESHOLD, IDC_SPARKLINE_TIME_RANGE_COMBO
        };
        for (UINT id : comboIds)
        {
            HWND hCombo = GetDlgItem(hDlg, id);
            if (hCombo)
            {
                InvalidateRect(hCombo, nullptr, TRUE);
                UpdateWindow(hCombo);
            }
        }
    }
}

bool SettingsDialog::ApplySettingsFromDialog(HWND hDlg)
{
    // Create a temporary config to hold new values
    AppConfig tempConfig = m_configCopy;

    // Get update interval selection
    HWND hInterval = GetDlgItem(hDlg, IDC_UPDATE_INTERVAL_COMBO);
    if (hInterval)
    {
        int sel = static_cast<int>(SendMessageW(hInterval, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            tempConfig.updateInterval = static_cast<UINT>(SendMessageW(hInterval, CB_GETITEMDATA, sel, 0));
        }
    }

    // Get display unit selection
    HWND hUnit = GetDlgItem(hDlg, IDC_DISPLAY_UNIT_COMBO);
    if (hUnit)
    {
        int sel = static_cast<int>(SendMessageW(hUnit, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            tempConfig.displayUnit = static_cast<SpeedUnit>(SendMessageW(hUnit, CB_GETITEMDATA, sel, 0));
        }
    }

    // Get checkbox states - use custom state map for dark theme (owner-draw doesn't store state)
    // Note: We use IsCustomThemeEnabled (current state) to determine how to read
    if (IsCustomThemeEnabled(m_configCopy))
    {
        tempConfig.autoStart = GetCheckboxState(IDC_AUTOSTART_CHECK);
        tempConfig.autoStartAsAdmin = GetCheckboxState(IDC_AUTOSTART_ADMIN_CHECK);
        tempConfig.enableLogging = GetCheckboxState(IDC_ENABLE_LOGGING_CHECK);
        tempConfig.debugLogging = GetCheckboxState(IDC_DEBUG_LOGGING_CHECK);
        tempConfig.enableConnectionNotification = GetCheckboxState(IDC_CONNECTION_NOTIFY_CHECK);
    }
    else
    {
        tempConfig.autoStart = (Button_GetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_CHECK)) == BST_CHECKED);
        tempConfig.autoStartAsAdmin = (Button_GetCheck(GetDlgItem(hDlg, IDC_AUTOSTART_ADMIN_CHECK)) == BST_CHECKED);
        tempConfig.enableLogging = (Button_GetCheck(GetDlgItem(hDlg, IDC_ENABLE_LOGGING_CHECK)) == BST_CHECKED);
        tempConfig.debugLogging = (Button_GetCheck(GetDlgItem(hDlg, IDC_DEBUG_LOGGING_CHECK)) == BST_CHECKED);
        tempConfig.enableConnectionNotification = (Button_GetCheck(GetDlgItem(hDlg, IDC_CONNECTION_NOTIFY_CHECK)) == BST_CHECKED);
    }

    // Keep ThemeMode roughly in sync with the dark theme checkbox
    HWND hThemeModeDlg = GetDlgItem(hDlg, IDC_THEME_MODE_COMBO);
    if (hThemeModeDlg)
    {
        int selTheme = static_cast<int>(SendMessageW(hThemeModeDlg, CB_GETCURSEL, 0, 0));
        if (selTheme != CB_ERR)
        {
            LRESULT data = SendMessageW(hThemeModeDlg, CB_GETITEMDATA, selTheme, 0);
            if (data != CB_ERR)
            {
                tempConfig.themeMode = static_cast<ThemeMode>(static_cast<int>(data));
            }
        }
    }

    // Calculate effective dark theme based on mode
    tempConfig.darkTheme = IsDarkThemeEnabled(tempConfig);

    // Get interface selection
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
                tempConfig.selectedInterface.clear();
            }
            else
            {
                tempConfig.selectedInterface = buffer;
            }
        }
    }

    // Get history auto-trim selection
    HWND hTrim = GetDlgItem(hDlg, IDC_HISTORY_AUTO_TRIM_COMBO);
    if (hTrim)
    {
        int selTrim = static_cast<int>(SendMessageW(hTrim, CB_GETCURSEL, 0, 0));
        if (selTrim != CB_ERR)
        {
            LRESULT days = SendMessageW(hTrim, CB_GETITEMDATA, selTrim, 0);
            if (days != CB_ERR)
            {
                tempConfig.historyAutoTrimDays = static_cast<int>(days);
            }
        }
    }

    // Get language selection
    HWND hLanguage = GetDlgItem(hDlg, IDC_LANGUAGE_COMBO);
    if (hLanguage)
    {
        int selLang = static_cast<int>(SendMessageW(hLanguage, CB_GETCURSEL, 0, 0));
        if (selLang != CB_ERR)
        {
            LRESULT langVal = SendMessageW(hLanguage, CB_GETITEMDATA, selLang, 0);
            if (langVal != CB_ERR)
            {
                tempConfig.language = static_cast<AppLanguage>(langVal);
            }
        }
    }

    // Get ping target
    HWND hPingTarget = GetDlgItem(hDlg, IDC_PING_TARGET_EDIT);
    if (hPingTarget)
    {
        wchar_t buffer[256] = {0};
        GetWindowTextW(hPingTarget, buffer, 256);
        tempConfig.pingTarget = buffer;
        if (tempConfig.pingTarget.empty())
        {
            tempConfig.pingTarget = L"8.8.8.8";
        }
    }

    // Get ping interval
    HWND hPingInterval = GetDlgItem(hDlg, IDC_PING_INTERVAL_COMBO);
    if (hPingInterval)
    {
        int sel = static_cast<int>(SendMessageW(hPingInterval, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hPingInterval, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                tempConfig.pingIntervalMs = static_cast<UINT>(data);
            }
        }
    }

    // Get hotkey
    HWND hHotkeyCombo = GetDlgItem(hDlg, IDC_HOTKEY_COMBO);
    if (hHotkeyCombo)
    {
        int sel = static_cast<int>(SendMessageW(hHotkeyCombo, CB_GETCURSEL, 0, 0));
        if (sel != CB_ERR)
        {
            LRESULT data = SendMessageW(hHotkeyCombo, CB_GETITEMDATA, sel, 0);
            if (data != CB_ERR)
            {
                tempConfig.hotkeyKey = LOWORD(data);
                tempConfig.hotkeyModifier = HIWORD(data);
            }
        }
    }

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
                tempConfig.overlayFontSize = static_cast<int>(data);
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
            tempConfig.overlayDownloadColor = s_overlayColorPresets[sel].down;
            tempConfig.overlayUploadColor = s_overlayColorPresets[sel].up;
        }
    }

    // === Data Usage Alerts ===
    if (IsCustomThemeEnabled(m_configCopy))
    {
        tempConfig.enableDataUsageAlerts = GetCheckboxState(IDC_DATA_USAGE_ENABLE_CHECK);
    }
    else
    {
        tempConfig.enableDataUsageAlerts = (Button_GetCheck(GetDlgItem(hDlg, IDC_DATA_USAGE_ENABLE_CHECK)) == BST_CHECKED);
    }
    
    // === Floating Window Settings ===
    if (IsCustomThemeEnabled(m_configCopy))
    {
        tempConfig.floatingShowNetwork = GetCheckboxState(IDC_FLOATING_SHOW_NETWORK_CHECK);
        tempConfig.floatingShowCPU = GetCheckboxState(IDC_FLOATING_SHOW_CPU_CHECK);
        tempConfig.floatingShowRAM = GetCheckboxState(IDC_FLOATING_SHOW_RAM_CHECK);
        tempConfig.floatingShowPing = GetCheckboxState(IDC_FLOATING_SHOW_PING_CHECK);
        tempConfig.floatingShowDataToday = GetCheckboxState(IDC_FLOATING_SHOW_DATA_TODAY_CHECK);
        tempConfig.floatingShowSparkline = GetCheckboxState(IDC_FLOATING_SHOW_SPARKLINE_CHECK);
        tempConfig.trayAnimationEnabled = GetCheckboxState(IDC_TRAY_ANIMATION_CHECK);
        tempConfig.floatingShowVpnStatus = GetCheckboxState(IDC_FLOATING_SHOW_VPN_CHECK);
        tempConfig.floatingShowPublicIP = GetCheckboxState(IDC_FLOATING_SHOW_IP_CHECK);
    }
    else
    {
        tempConfig.floatingShowNetwork = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_NETWORK_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowCPU = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_CPU_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowRAM = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_RAM_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowPing = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_PING_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowDataToday = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_DATA_TODAY_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowSparkline = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_SPARKLINE_CHECK)) == BST_CHECKED);
        tempConfig.trayAnimationEnabled = (Button_GetCheck(GetDlgItem(hDlg, IDC_TRAY_ANIMATION_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowVpnStatus = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_VPN_CHECK)) == BST_CHECKED);
        tempConfig.floatingShowPublicIP = (Button_GetCheck(GetDlgItem(hDlg, IDC_FLOATING_SHOW_IP_CHECK)) == BST_CHECKED);
    }
    
    // Get tray animation threshold from combo
    HWND hTrayThreshold = GetDlgItem(hDlg, IDC_TRAY_ANIMATION_THRESHOLD);
    if (hTrayThreshold)
    {
        int sel = ComboBox_GetCurSel(hTrayThreshold);
        if (sel != CB_ERR)
        {
            tempConfig.trayAnimationThresholdKB = static_cast<int>(ComboBox_GetItemData(hTrayThreshold, sel));
        }
    }
    
    // Get sparkline time range from combo
    HWND hSparklineTime = GetDlgItem(hDlg, IDC_SPARKLINE_TIME_RANGE_COMBO);
    if (hSparklineTime)
    {
        int sel = ComboBox_GetCurSel(hSparklineTime);
        if (sel != CB_ERR)
        {
            tempConfig.sparklineTimeRange = sel;
        }
    }
    
    HWND hDataQuota = GetDlgItem(hDlg, IDC_DATA_USAGE_QUOTA_EDIT);
    if (hDataQuota)
    {
        wchar_t buf[64] = {0};
        GetWindowTextW(hDataQuota, buf, 64);
        tempConfig.dataQuotaGB = _wtof(buf);
        if (tempConfig.dataQuotaGB < 0.0)
        {
            tempConfig.dataQuotaGB = 0.0;
        }
    }

    // === CHECK FOR CHANGES ===
    // If nothing changed, return success immediately without saving
    // === CHECK FOR CHANGES ===
    
    // Fix precision issue for data quota (round to 1 decimal place to match UI)
    tempConfig.dataQuotaGB = round(tempConfig.dataQuotaGB * 10.0) / 10.0;
    
    // === CHECK FOR CHANGES ===
    
    // Fix precision issue for data quota (round to 1 decimal place to match UI)
    tempConfig.dataQuotaGB = round(tempConfig.dataQuotaGB * 10.0) / 10.0;
    
    // Check portable mode state
    bool portableModeChanged = false;
    bool newPortableMode = false;
    if (m_pConfigProvider && m_pConfigProvider->HasPortableConfigFile())
    {
        HWND hCheck = GetDlgItem(hDlg, IDC_PORTABLE_MODE_CHECK);
        if (hCheck)
        {
            if (IsCustomThemeEnabled(m_configCopy))
            {
                newPortableMode = GetCheckboxState(IDC_PORTABLE_MODE_CHECK);
            }
            else
            {
                newPortableMode = (Button_GetCheck(hCheck) == BST_CHECKED);
            }
            
            bool currentMode = m_pConfigProvider->IsPortableMode();
            if (newPortableMode != currentMode)
            {
                portableModeChanged = true;
            }
        }
    }

    // Compare and log differences
    if (tempConfig == m_configCopy && !portableModeChanged)
    {
        LogDebug(L"ApplySettings: No changes detected.");
        return true;
    }

    // Log what changed for debugging
    if (tempConfig.updateInterval != m_configCopy.updateInterval) LogDebug(L"Settings Changed: updateInterval");
    if (tempConfig.displayUnit != m_configCopy.displayUnit) LogDebug(L"Settings Changed: displayUnit");
    if (tempConfig.autoStart != m_configCopy.autoStart) LogDebug(L"Settings Changed: autoStart");
    if (tempConfig.autoStartAsAdmin != m_configCopy.autoStartAsAdmin) LogDebug(L"Settings Changed: autoStartAsAdmin");
    if (tempConfig.enableLogging != m_configCopy.enableLogging) LogDebug(L"Settings Changed: enableLogging");
    if (tempConfig.debugLogging != m_configCopy.debugLogging) LogDebug(L"Settings Changed: debugLogging");
    if (tempConfig.darkTheme != m_configCopy.darkTheme) LogDebug(L"Settings Changed: darkTheme");
    if (tempConfig.themeMode != m_configCopy.themeMode) LogDebug(L"Settings Changed: themeMode");
    if (tempConfig.selectedInterface != m_configCopy.selectedInterface) LogDebug(L"Settings Changed: selectedInterface");
    if (tempConfig.historyAutoTrimDays != m_configCopy.historyAutoTrimDays) LogDebug(L"Settings Changed: historyAutoTrimDays");
    if (tempConfig.language != m_configCopy.language) LogDebug(L"Settings Changed: language");
    if (tempConfig.pingTarget != m_configCopy.pingTarget) LogDebug(L"Settings Changed: pingTarget");
    if (tempConfig.pingIntervalMs != m_configCopy.pingIntervalMs) LogDebug(L"Settings Changed: pingIntervalMs");
    if (tempConfig.enableDataUsageAlerts != m_configCopy.enableDataUsageAlerts) LogDebug(L"Settings Changed: enableDataUsageAlerts");
    if (tempConfig.dataQuotaGB != m_configCopy.dataQuotaGB) 
    {
        LogDebug(L"Settings Changed: dataQuotaGB");
    }
    if (tempConfig.floatingShowNetwork != m_configCopy.floatingShowNetwork) LogDebug(L"Settings Changed: floatingShowNetwork");
    if (tempConfig.floatingShowCPU != m_configCopy.floatingShowCPU) LogDebug(L"Settings Changed: floatingShowCPU");
    if (tempConfig.floatingShowRAM != m_configCopy.floatingShowRAM) LogDebug(L"Settings Changed: floatingShowRAM");
    if (tempConfig.floatingShowPing != m_configCopy.floatingShowPing) LogDebug(L"Settings Changed: floatingShowPing");
    if (tempConfig.floatingShowDataToday != m_configCopy.floatingShowDataToday) LogDebug(L"Settings Changed: floatingShowDataToday");
    if (tempConfig.floatingShowSparkline != m_configCopy.floatingShowSparkline) LogDebug(L"Settings Changed: floatingShowSparkline");
    if (portableModeChanged) LogDebug(L"Settings Changed: portableMode");

    // Capture old auto-start state before updating m_configCopy
    bool oldAutoStart = m_configCopy.autoStart;
    bool oldAutoStartAsAdmin = m_configCopy.autoStartAsAdmin;

    // Apply changes
    m_configCopy = tempConfig;
    
    // Update global theme state immediately so UI refresh uses new palette
    ThemeHelper::SetCurrentTheme(m_configCopy.themeMode);
    
    // Clear cached brushes in DialogThemeHelper to ensure new colors are used
    DialogThemeHelper::Cleanup();

    // Save to registry via config provider (ignore errors for now)
    if (m_pConfigProvider)
    {
        if (portableModeChanged)
        {
            m_pConfigProvider->SetPortableMode(newPortableMode);
        }

        m_pConfigProvider->SaveConfig(m_configCopy);

        // Explicitly update auto-start if changed
        // This avoids UAC prompts when only other settings (like language) are changed
        if (m_configCopy.autoStart != oldAutoStart || 
            m_configCopy.autoStartAsAdmin != oldAutoStartAsAdmin)
        {
            m_pConfigProvider->SetAutoStart(m_configCopy.autoStart, m_configCopy.autoStartAsAdmin);
        }
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
        IDC_AUTOSTART_CHECK, IDC_AUTOSTART_ADMIN_CHECK, IDC_ENABLE_LOGGING_CHECK,
        IDC_DEBUG_LOGGING_CHECK, IDC_CONNECTION_NOTIFY_CHECK,
        IDC_PORTABLE_MODE_CHECK, IDC_PORTABLE_MODE_BUTTON
    };

    // Display tab controls
    const int displayControls[] = {
        IDC_SETTINGS_LABEL_THEME, IDC_THEME_MODE_COMBO,
        IDC_SETTINGS_LABEL_INTERVAL, IDC_UPDATE_INTERVAL_COMBO,
        IDC_SETTINGS_LABEL_SPEED_UNIT, IDC_DISPLAY_UNIT_COMBO,
        IDC_FONT_SIZE_LABEL, IDC_OVERLAY_FONT_SIZE_COMBO,
        IDC_OVERLAY_COLOR_LABEL, IDC_OVERLAY_COLOR_COMBO,
        IDC_SETTINGS_GROUP_FLOATING,
        IDC_FLOATING_SHOW_NETWORK_CHECK, IDC_FLOATING_SHOW_CPU_CHECK, IDC_FLOATING_SHOW_RAM_CHECK,
        IDC_FLOATING_SHOW_PING_CHECK, IDC_FLOATING_SHOW_DATA_TODAY_CHECK, IDC_FLOATING_SHOW_SPARKLINE_CHECK,
        IDC_FLOATING_SHOW_VPN_CHECK, IDC_FLOATING_SHOW_IP_CHECK,
        IDC_SETTINGS_GROUP_TRAY, IDC_TRAY_ANIMATION_CHECK, IDC_TRAY_ANIMATION_THRESHOLD_LABEL, IDC_TRAY_ANIMATION_THRESHOLD,
        IDC_SPARKLINE_TIME_RANGE_LABEL, IDC_SPARKLINE_TIME_RANGE_COMBO
    };

    // Advanced tab controls
    const int advancedControls[] = {
        IDC_SETTINGS_LABEL_MONITOR, IDC_INTERFACE_COMBO,
        IDC_HISTORY_TRIM_LABEL, IDC_HISTORY_AUTO_TRIM_COMBO,
        IDC_PING_TARGET_LABEL, IDC_PING_TARGET_EDIT,
        IDC_PING_INTERVAL_LABEL, IDC_PING_INTERVAL_COMBO,
        IDC_HOTKEY_LABEL, IDC_HOTKEY_COMBO,
        IDC_DATA_USAGE_ENABLE_CHECK, IDC_DATA_USAGE_QUOTA_LABEL, IDC_DATA_USAGE_QUOTA_EDIT
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

} // namespace NetPulse
