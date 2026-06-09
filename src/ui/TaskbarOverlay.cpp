// NOTE: This class manages its own GDI resources (Fonts, Brushes, Bitmaps) for performance and stability.
// Refactoring to shared helpers is discouraged to avoid object lifetime issues.
#include "NetPulse/TaskbarOverlay.h"
#include "NetPulse/ThemeHelper.h"
#include "NetPulse/Utils.h"
#include "../../resources/resource.h"
#include <dwmapi.h>

#pragma comment(lib, "Dwmapi.lib")

namespace NetPulse
{

TaskbarOverlay::TaskbarOverlay()
    : m_hInstance(nullptr)
    , m_hwnd(nullptr)
    , m_hTaskbar(nullptr)
    , m_userWantsVisible(false)
    , m_initialized(false)
    , m_timerId(0)
    , m_downloadSpeed(0.0)
    , m_uploadSpeed(0.0)
    , m_displayUnit(SpeedUnit::KiloBytesPerSecond)
    , m_darkTheme(false)
    , m_pingLatency(-1)
    , m_wasHiddenByFullscreen(false)
    , m_fontSize(13)
    , m_downloadColor(0) // Will be set in OnPaint or Update
    , m_uploadColor(0)
    , m_memDC(nullptr)
    , m_memBitmap(nullptr)
    , m_oldBitmap(nullptr)
    , m_font(nullptr)
    , m_bitmapWidth(0)
    , m_bitmapHeight(0)
{
}

TaskbarOverlay::~TaskbarOverlay()
{
    Cleanup();
}

bool TaskbarOverlay::Initialize(HINSTANCE hInstance)
{
    if (m_initialized)
    {
        return true;
    }

    m_hInstance = hInstance;

    if (!RegisterWindowClass(hInstance))
    {
        return false;
    }

    if (!CreateOverlayWindow(hInstance))
    {
        return false;
    }

    m_hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    
    PositionOnTaskbar();

    // Start timer - 150ms for checking visibility and position
    m_timerId = SetTimer(m_hwnd, TIMER_CHECK_VISIBILITY, 150, nullptr);

    m_initialized = true;
    return true;
}

void TaskbarOverlay::Cleanup()
{
    ReleaseGraphicsResources();

    if (m_hwnd && m_timerId)
    {
        KillTimer(m_hwnd, TIMER_CHECK_VISIBILITY);
        m_timerId = 0;
    }

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    if (m_hInstance)
    {
        UnregisterClassW(WINDOW_CLASS_NAME, m_hInstance);
        m_hInstance = nullptr;
    }

    m_initialized = false;
}

void TaskbarOverlay::UpdateSpeed(double downloadSpeed, double uploadSpeed, SpeedUnit unit)
{
    m_downloadSpeed = downloadSpeed;
    m_uploadSpeed = uploadSpeed;
    m_displayUnit = unit;

    if (m_hwnd && m_userWantsVisible)
    {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

bool TaskbarOverlay::IsVisible() const
{
    return m_hwnd != nullptr && IsWindowVisible(m_hwnd);
}

void TaskbarOverlay::Show(bool show)
{
    if (!m_hwnd)
    {
        return;
    }

    m_userWantsVisible = show;
    
    if (show)
    {
        ForceShow();
    }
    else
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void TaskbarOverlay::SetDarkTheme(bool dark)
{
    m_darkTheme = dark;

    if (m_hwnd && m_userWantsVisible)
    {
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void TaskbarOverlay::SetRightClickCallback(std::function<void()> callback)
{
    m_rightClickCallback = callback;
}

bool TaskbarOverlay::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    if (!RegisterClassExW(&wc))
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_REGISTER_OVERLAY));
        return false;
    }

    return true;
}

bool TaskbarOverlay::CreateOverlayWindow(HINSTANCE hInstance)
{
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        WINDOW_CLASS_NAME,
        L"NetPulse Overlay",
        WS_POPUP,
        0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd)
    {
        ShowErrorMessage(LoadStringResource(IDS_ERR_CREATE_OVERLAY));
        return false;
    }

    ShowWindow(m_hwnd, SW_HIDE);

    // Set transparency - RGB(1,1,1) is transparent color key
    SetLayeredWindowAttributes(m_hwnd, RGB(1, 1, 1), 0, LWA_COLORKEY);

    return true;
}

void TaskbarOverlay::PositionOnTaskbar()
{
    if (!m_hwnd)
    {
        return;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    UINT posFlags = SWP_NOACTIVATE;
    if (m_userWantsVisible && !m_wasHiddenByFullscreen)
    {
        posFlags |= SWP_SHOWWINDOW;
    }

    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hTaskbar)
    {
        // Fallback: bottom-right corner
        int x = screenWidth - WINDOW_WIDTH - 60;
        int y = screenHeight - WINDOW_HEIGHT - 7;
        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, WINDOW_WIDTH, WINDOW_HEIGHT, posFlags);
        return;
    }

    // Get taskbar rect
    RECT taskbarRect;
    GetWindowRect(hTaskbar, &taskbarRect);
    int taskbarHeight = taskbarRect.bottom - taskbarRect.top;

    // Find notification area (system tray)
    HWND hTrayNotify = FindWindowExW(hTaskbar, nullptr, L"TrayNotifyWnd", nullptr);
    
    int x, y;
    int dynamicGap = 10; // Default gap
    
    if (hTrayNotify)
    {
        RECT trayRect;
        GetWindowRect(hTrayNotify, &trayRect);
        
        // Try to find chevron button ("^" overflow button)
        HWND hChevron = FindWindowExW(hTrayNotify, nullptr, L"Button", nullptr);
        
        if (hChevron && IsWindowVisible(hChevron))
        {
            // Chevron is visible - position right before it
            RECT chevronRect;
            GetWindowRect(hChevron, &chevronRect);
            
            // Position with safe gap from chevron
            x = chevronRect.left - WINDOW_WIDTH - 8; // 8px gap
        }
        else
        {
            // Chevron is hidden or not found - use tray left edge
            // Calculate dynamic gap based on tray width
            int trayWidth = trayRect.right - trayRect.left;
            
            // If tray is narrow, chevron is likely hidden - use smaller gap
            // If tray is wide, chevron might appear - use larger gap
            dynamicGap = (trayWidth < 80) ? 5 : 12;
            
            x = trayRect.left - WINDOW_WIDTH - dynamicGap;
        }
        
        // Center vertically in taskbar
        y = taskbarRect.top + ((taskbarHeight - WINDOW_HEIGHT) / 2);
        
        // Safety bounds check
        if (x < 0)
        {
            x = 10;
        }
    }
    else
    {
        // Fallback if tray not found
        x = screenWidth - WINDOW_WIDTH - 180;
        y = taskbarRect.top + ((taskbarHeight - WINDOW_HEIGHT) / 2);
    }

    // Check if position needs update
    RECT currentRect;
    GetWindowRect(m_hwnd, &currentRect);
    
    int currentX = currentRect.left;
    int currentY = currentRect.top;
    
    // Only update position if it has changed significantly (more than 5 pixels)
    if (abs(currentX - x) > 5 || abs(currentY - y) > 5)
    {
        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, WINDOW_WIDTH, WINDOW_HEIGHT, posFlags);
    }
}

void TaskbarOverlay::ForceShow()
{
    if (!m_hwnd || !m_userWantsVisible)
    {
        return;
    }

    // Multiple strategies to ensure window stays visible
    
    // 1. Show window
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    
    // 2. Force to topmost
    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    // 3. Update position (in case taskbar moved)
    PositionOnTaskbar();
    
    // 4. Invalidate to force repaint
    InvalidateRect(m_hwnd, nullptr, TRUE);
    UpdateWindow(m_hwnd);
}

LRESULT CALLBACK TaskbarOverlay::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TaskbarOverlay* pThis = nullptr;

    if (message == WM_CREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<TaskbarOverlay*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        return 0;
    }
    else
    {
        pThis = reinterpret_cast<TaskbarOverlay*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        switch (message)
        {
            case WM_TIMER:
            {
                if (wParam == TIMER_CHECK_VISIBILITY)
                {
                    // Check if fullscreen app is running
                    bool isFullscreen = IsForegroundWindowFullscreen();
                    
                    if (isFullscreen && pThis->m_userWantsVisible && !pThis->m_wasHiddenByFullscreen)
                    {
                        // Hide overlay while fullscreen app is running
                        ShowWindow(hwnd, SW_HIDE);
                        pThis->m_wasHiddenByFullscreen = true;
                    }
                    else if (!isFullscreen && pThis->m_wasHiddenByFullscreen)
                    {
                        // Restore overlay when fullscreen app exits
                        pThis->ForceShow();
                        pThis->m_wasHiddenByFullscreen = false;
                    }
                    else if (pThis->m_userWantsVisible && !pThis->m_wasHiddenByFullscreen)
                    {
                        // Normal visibility check
                        BOOL isVisible = IsWindowVisible(hwnd);
                        
                        if (!isVisible)
                        {
                            // Window is hidden but should be visible - force show
                            pThis->ForceShow();
                        }
                        else
                        {
                            // Window is visible - check position and update if needed
                            pThis->PositionOnTaskbar();
                            
                            // Ensure topmost
                            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                        }
                    }
                }
                return 0;
            }

            case WM_MOUSEACTIVATE:
            {
                // Prevent window from being activated
                return MA_NOACTIVATE;
            }

            case WM_NCACTIVATE:
            {
                // Keep window visible but don't activate
                return TRUE;
            }

            case WM_ACTIVATE:
            {
                // Don't activate
                return 0;
            }

            case WM_PAINT:
            {
                pThis->OnPaint();
                return 0;
            }

            case WM_RBUTTONUP:
            {
                pThis->OnRightButtonUp();
                return 0;
            }

            case WM_DISPLAYCHANGE:
            case WM_SETTINGCHANGE:
            case WM_WINDOWPOSCHANGING:
            case WM_WINDOWPOSCHANGED:
            {
                // Reposition when display or window position changes
                pThis->OnDisplayChange();
                return 0;
            }

            case WM_DESTROY:
            {
                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void TaskbarOverlay::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT rect;
    GetClientRect(m_hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (!EnsureGraphicsResources(hdc, width, height))
    {
        EndPaint(m_hwnd, &ps);
        return;
    }

    HDC hdcMem = m_memDC;

    // Fill with RGB(1,1,1) - this will be transparent
    HBRUSH hBrush = CreateSolidBrush(RGB(1, 1, 1));
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);

    // Set transparent text background
    SetBkMode(hdcMem, TRANSPARENT);

    HFONT hOldFont = nullptr;
    if (m_font)
    {
        hOldFont = (HFONT)SelectObject(hdcMem, m_font);
    }

    // Format speeds
    std::wstring downStr = FormatSpeed(m_downloadSpeed, m_displayUnit);
    std::wstring upStr = FormatSpeed(m_uploadSpeed, m_displayUnit);

    // Create 2 lines with localized prefixes
    std::wstring downPrefix = LoadStringResource(IDS_OVERLAY_DOWN_PREFIX);
    if (downPrefix.empty())
    {
        downPrefix = L"Down: ";
    }

    std::wstring upPrefix = LoadStringResource(IDS_OVERLAY_UP_PREFIX);
    if (upPrefix.empty())
    {
        upPrefix = L"Up: ";
    }

    std::wstring line1 = downPrefix + downStr;
    std::wstring line2 = upPrefix + upStr;

    // Calculate line positions
    int lineHeight = 16;
    int startY = (rect.bottom - (lineHeight * 2)) / 2;

    RECT line1Rect = {5, startY, rect.right - 5, startY + lineHeight};
    RECT line2Rect = {5, startY + lineHeight, rect.right - 5, startY + lineHeight * 2};

    // Draw Download line
    // Use colors from SetOverlayStyle (set from config.overlayDownloadColor/overlayUploadColor)
    // NOT from theme presets - overlay colors are configured separately in Settings
    COLORREF downColor = m_downloadColor;
    COLORREF upColor = m_uploadColor;

    SetTextColor(hdcMem, downColor);
    DrawTextW(hdcMem, line1.c_str(), -1, &line1Rect,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER);

    // Draw Upload line
    SetTextColor(hdcMem, upColor);
    DrawTextW(hdcMem, line2.c_str(), -1, &line2Rect,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER);

    // Draw Ping latency on the right side
    // Ping colors still use theme (no separate config setting for ping colors)
    {
        const auto& colors = ThemeHelper::GetColors(m_darkTheme);
        wchar_t pingBuffer[32];
        COLORREF pingColor;

        if (m_pingLatency < 0)
        {
            // Timeout or unavailable - show "---" in gray
            wcscpy_s(pingBuffer, L"---");
            pingColor = colors.pingNone;
        }
        else if (m_pingLatency < 50)
        {
            // Good latency - green
            swprintf_s(pingBuffer, L"%dms", m_pingLatency);
            pingColor = colors.pingLow;
        }
        else if (m_pingLatency < 100)
        {
            // Medium latency - yellow/orange
            swprintf_s(pingBuffer, L"%dms", m_pingLatency);
            pingColor = colors.pingMed;
        }
        else
        {
            // High latency - red
            swprintf_s(pingBuffer, L"%dms", m_pingLatency);
            pingColor = colors.pingHigh;
        }

        SetTextColor(hdcMem, pingColor);

        RECT pingRect = {rect.right - 45, startY, rect.right - 2, startY + lineHeight * 2};
        DrawTextW(hdcMem, pingBuffer, -1, &pingRect,
                  DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
    }

    // Copy to screen
    BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

    // Cleanup
    if (hOldFont)
    {
        SelectObject(hdcMem, hOldFont);
    }

    EndPaint(m_hwnd, &ps);
}

bool TaskbarOverlay::EnsureGraphicsResources(HDC referenceDC, int width, int height)
{
    if (!referenceDC || width <= 0 || height <= 0)
    {
        return false;
    }

    if (!m_memDC)
    {
        m_memDC = CreateCompatibleDC(referenceDC);
        if (!m_memDC)
        {
            return false;
        }
    }

    if (!m_memBitmap || width != m_bitmapWidth || height != m_bitmapHeight)
    {
        HBITMAP newBitmap = CreateCompatibleBitmap(referenceDC, width, height);
        if (!newBitmap)
        {
            return false;
        }

        if (m_memBitmap)
        {
            SelectObject(m_memDC, m_oldBitmap);
            DeleteObject(m_memBitmap);
        }

        m_memBitmap = newBitmap;
        m_oldBitmap = (HBITMAP)SelectObject(m_memDC, m_memBitmap);
        m_bitmapWidth = width;
        m_bitmapHeight = height;
    }

    if (!m_font)
    {
        m_font = CreateFontW(
            m_fontSize,                  // Height - use configurable size
            0,                           // Width
            0,                           // Escapement
            0,                           // Orientation
            FW_NORMAL,                   // Weight
            FALSE,                       // Italic
            FALSE,                       // Underline
            FALSE,                       // StrikeOut
            DEFAULT_CHARSET,             // CharSet
            OUT_DEFAULT_PRECIS,          // OutPrecision
            CLIP_DEFAULT_PRECIS,         // ClipPrecision
            CLEARTYPE_QUALITY,           // Quality
            DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
            L"Segoe UI"                  // FaceName
        );

        if (!m_font)
        {
            return false;
        }
    }

    return true;
}

void TaskbarOverlay::ReleaseGraphicsResources()
{
    if (m_memDC)
    {
        if (m_memBitmap)
        {
            SelectObject(m_memDC, m_oldBitmap);
            DeleteObject(m_memBitmap);
            m_memBitmap = nullptr;
        }

        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }

    if (m_font)
    {
        DeleteObject(m_font);
        m_font = nullptr;
    }

    m_oldBitmap = nullptr;
    m_bitmapWidth = 0;
    m_bitmapHeight = 0;
}

void TaskbarOverlay::OnRightButtonUp()
{
    if (m_rightClickCallback)
    {
        m_rightClickCallback();
    }
}

void TaskbarOverlay::OnDisplayChange()
{
    PositionOnTaskbar();
}

void TaskbarOverlay::SetPingLatency(int latencyMs)
{
    if (m_pingLatency != latencyMs)
    {
        m_pingLatency = latencyMs;
        // Trigger repaint to update display
        if (m_hwnd && m_userWantsVisible)
        {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
}

void TaskbarOverlay::SetOverlayStyle(int fontSize, COLORREF downloadColor, COLORREF uploadColor)
{
    bool needFontRecreate = (m_fontSize != fontSize);
    m_fontSize = fontSize;
    m_downloadColor = downloadColor;
    m_uploadColor = uploadColor;

    // Recreate font if size changed
    if (needFontRecreate && m_font)
    {
        DeleteObject(m_font);
        m_font = nullptr;
    }

    // Trigger repaint
    if (m_hwnd && m_userWantsVisible)
    {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

bool TaskbarOverlay::IsForegroundWindowFullscreen()
{
    HWND hForeground = GetForegroundWindow();
    if (!hForeground)
    {
        return false;
    }

    // Get foreground window rect
    RECT windowRect;
    if (!GetWindowRect(hForeground, &windowRect))
    {
        return false;
    }

    // Get monitor info for the foreground window
    HMONITOR hMonitor = MonitorFromWindow(hForeground, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor)
    {
        return false;
    }

    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(hMonitor, &monitorInfo))
    {
        return false;
    }

    // Check if window covers the entire monitor
    return (windowRect.left <= monitorInfo.rcMonitor.left &&
            windowRect.top <= monitorInfo.rcMonitor.top &&
            windowRect.right >= monitorInfo.rcMonitor.right &&
            windowRect.bottom >= monitorInfo.rcMonitor.bottom);
}

} // namespace NetPulse
