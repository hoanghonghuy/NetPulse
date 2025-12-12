#include "NetworkMonitor/FloatingWindow.h"
#include "NetworkMonitor/Utils.h"
#include <windowsx.h>
#include <sstream>
#include <iomanip>

namespace NetworkMonitor
{

bool FloatingWindow::s_classRegistered = false;

FloatingWindow::FloatingWindow()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_darkTheme(true)
    , m_opacity(200)
    , m_showNetwork(true)
    , m_showCPU(true)
    , m_showRAM(true)
    , m_showPing(true)
    , m_showDataToday(true)
    , m_downloadSpeed(0.0)
    , m_uploadSpeed(0.0)
    , m_speedUnit(SpeedUnit::KiloBytesPerSecond)
    , m_cpuPercent(0.0)
    , m_ramPercent(0.0)
    , m_pingLatency(-1)
    , m_todayBytesDown(0)
    , m_todayBytesUp(0)
    , m_snapToEdge(true)       // Enable snap-to-edge by default
    , m_snapDistance(20)       // Default 20px snap distance
    , m_clickThrough(false)    // Click-through disabled by default
    , m_miniMode(false)        // Normal mode by default
{
}

FloatingWindow::~FloatingWindow()
{
    Destroy();
}

bool FloatingWindow::Create(HINSTANCE hInstance)
{
    if (m_hwnd)
    {
        return true; // Already created
    }

    m_hInstance = hInstance;
    RegisterWindowClass(hInstance);

    // Create layered, topmost, tool window (no taskbar button)
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP;

    // Get default position (top-right corner of primary monitor)
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int x = screenWidth - WINDOW_WIDTH - 20;
    int y = 20;

    m_hwnd = CreateWindowExW(
        exStyle,
        WINDOW_CLASS_NAME,
        L"Network Monitor",
        style,
        x, y,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!m_hwnd)
    {
        LogError(L"FloatingWindow::Create: CreateWindowExW failed: " + GetLastErrorString());
        return false;
    }

    // Set initial opacity
    SetLayeredWindowAttributes(m_hwnd, 0, m_opacity, LWA_ALPHA);

    LogDebug(L"FloatingWindow::Create: Window created successfully");
    return true;
}

void FloatingWindow::Destroy()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void FloatingWindow::Show(bool visible)
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
}

bool FloatingWindow::IsVisible() const
{
    return m_hwnd && IsWindowVisible(m_hwnd);
}

void FloatingWindow::SetShowNetwork(bool show)
{
    m_showNetwork = show;
    RecalculateWindowSize();
    Invalidate();
}

void FloatingWindow::SetShowCPU(bool show)
{
    m_showCPU = show;
    RecalculateWindowSize();
    Invalidate();
}

void FloatingWindow::SetShowRAM(bool show)
{
    m_showRAM = show;
    RecalculateWindowSize();
    Invalidate();
}

void FloatingWindow::SetShowPing(bool show)
{
    m_showPing = show;
    RecalculateWindowSize();
    Invalidate();
}

void FloatingWindow::SetShowDataToday(bool show)
{
    m_showDataToday = show;
    RecalculateWindowSize();
    Invalidate();
}


void FloatingWindow::UpdateSpeed(double downloadSpeed, double uploadSpeed, SpeedUnit unit)
{
    // Check if values changed significantly to avoid unnecessary repaints
    if (m_downloadSpeed != downloadSpeed || m_uploadSpeed != uploadSpeed || m_speedUnit != unit)
    {
        m_downloadSpeed = downloadSpeed;
        m_uploadSpeed = uploadSpeed;
        m_speedUnit = unit;
        Invalidate();
    }
}

void FloatingWindow::UpdateDataToday(uint64_t bytesDown, uint64_t bytesUp)
{
    if (m_todayBytesDown != bytesDown || m_todayBytesUp != bytesUp)
    {
        m_todayBytesDown = bytesDown;
        m_todayBytesUp = bytesUp;
        if (m_showDataToday)
        {
            Invalidate();
        }
    }
}
void FloatingWindow::UpdateCPU(double cpuPercent)
{
    m_cpuPercent = cpuPercent;
    Invalidate();
}

void FloatingWindow::UpdateRAM(double ramPercent)
{
    m_ramPercent = ramPercent;
    Invalidate();
}

void FloatingWindow::UpdatePing(int latencyMs)
{
    m_pingLatency = latencyMs;
    Invalidate();
}

void FloatingWindow::SetPosition(int x, int y)
{
    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void FloatingWindow::GetPosition(int& x, int& y) const
{
    if (m_hwnd)
    {
        RECT rc;
        GetWindowRect(m_hwnd, &rc);
        x = rc.left;
        y = rc.top;
    }
    else
    {
        x = y = 0;
    }
}

void FloatingWindow::SetOpacity(BYTE alpha)
{
    m_opacity = alpha;
    if (m_hwnd)
    {
        SetLayeredWindowAttributes(m_hwnd, 0, m_opacity, LWA_ALPHA);
    }
}

void FloatingWindow::SetDarkTheme(bool dark)
{
    m_darkTheme = dark;
    Invalidate();
}

void FloatingWindow::Invalidate()
{
    if (m_hwnd)
    {
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void FloatingWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    if (s_classRegistered)
    {
        return;
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // We'll paint our own background
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (RegisterClassExW(&wc))
    {
        s_classRegistered = true;
    }
}

LRESULT CALLBACK FloatingWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    FloatingWindow* pThis = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<FloatingWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    }
    else
    {
        pThis = reinterpret_cast<FloatingWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT FloatingWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            Paint(hdc);
            EndPaint(m_hwnd, &ps);
        }
        return 0;

    case WM_NCHITTEST:
        // Allow dragging from anywhere on the window (unless click-through)
        if (m_clickThrough)
        {
            return HTTRANSPARENT;  // Pass clicks through
        }
        return HTCAPTION;

    case WM_MOVING:
        // Apply snap-to-edge when dragging
        if (m_snapToEdge)
        {
            ApplySnapToEdge(reinterpret_cast<RECT*>(lParam));
        }
        return TRUE;

    case WM_NCLBUTTONDBLCLK:
        // Double-click to toggle mini-mode
        ToggleMiniMode();
        return 0;

    case WM_ERASEBKGND:
        return 1; // We handle background in WM_PAINT

    case WM_DESTROY:
        m_hwnd = nullptr;
        return 0;

    default:
        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }
}

void FloatingWindow::Paint(HDC hdc)
{
    if (m_miniMode)
    {
        PaintMiniMode(hdc);
    }
    else
    {
        PaintNormal(hdc);
    }
}

void FloatingWindow::PaintMiniMode(HDC hdc)
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Colors
    COLORREF bgColor = m_darkTheme ? RGB(30, 30, 30) : RGB(245, 245, 245);
    COLORREF downColor = m_darkTheme ? RGB(0, 200, 255) : RGB(0, 120, 180);

    // Fill background
    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw thin border
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, m_darkTheme ? RGB(80, 80, 80) : RGB(180, 180, 180));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 4, 4);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBorderPen);

    // Create smaller font for mini mode
    HFONT hFont = CreateFontW(
        12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    // Show primary metric: download speed (most relevant)
    std::wstring text = L"\u2193" + FormatSpeed(m_downloadSpeed, m_speedUnit);
    
    SetTextColor(hdc, downColor);
    TextOutW(hdc, 4, 4, text.c_str(), static_cast<int>(text.length()));

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void FloatingWindow::PaintNormal(HDC hdc)
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Colors
    COLORREF bgColor = m_darkTheme ? RGB(30, 30, 30) : RGB(245, 245, 245);
    COLORREF downColor = m_darkTheme ? RGB(0, 200, 255) : RGB(0, 120, 180);
    COLORREF upColor = m_darkTheme ? RGB(0, 220, 100) : RGB(0, 150, 60);
    COLORREF cpuColor = m_darkTheme ? RGB(255, 180, 50) : RGB(200, 120, 0);
    COLORREF ramColor = m_darkTheme ? RGB(200, 100, 255) : RGB(140, 60, 180);

    // Fill background with rounded rectangle effect
    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw border
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, m_darkTheme ? RGB(80, 80, 80) : RGB(180, 180, 180));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBorderPen);

    // Create font
    HFONT hFont = CreateFontW(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    int y = PADDING;

    // Draw network speeds
    if (m_showNetwork)
    {
        std::wstring downText = L"\u2193 " + FormatSpeed(m_downloadSpeed, m_speedUnit);
        std::wstring upText = L"\u2191 " + FormatSpeed(m_uploadSpeed, m_speedUnit);
        
        SetTextColor(hdc, downColor);
        TextOutW(hdc, PADDING, y, downText.c_str(), static_cast<int>(downText.length()));
        
        SetTextColor(hdc, upColor);
        TextOutW(hdc, PADDING + 85, y, upText.c_str(), static_cast<int>(upText.length()));
        y += LINE_HEIGHT;
    }

    // Draw CPU and RAM on the same line
    if (m_showCPU || m_showRAM)
    {
        int xPos = PADDING;
        
        if (m_showCPU)
        {
            wchar_t buf[32];
            swprintf_s(buf, L"CPU: %.0f%%", m_cpuPercent);
            SetTextColor(hdc, cpuColor);
            TextOutW(hdc, xPos, y, buf, static_cast<int>(wcslen(buf)));
            xPos = PADDING + 85; // Move to second column for RAM
        }

        if (m_showRAM)
        {
            wchar_t buf[32];
            swprintf_s(buf, L"RAM: %.0f%%", m_ramPercent);
            SetTextColor(hdc, ramColor);
            // If CPU is hidden, RAM aligns left; otherwise aligns to second column
            TextOutW(hdc, xPos, y, buf, static_cast<int>(wcslen(buf)));
        }
        y += LINE_HEIGHT;
    }

    // Draw Ping and Data Today
    if (m_showPing || m_showDataToday)
    {
        int startX = PADDING;
        
        // Draw Ping
        if (m_showPing)
        {
            wchar_t buf[32];
            COLORREF pingColor;
            
            if (m_pingLatency < 0)
            {
                wcscpy_s(buf, L"Ping: --");
                pingColor = m_darkTheme ? RGB(128, 128, 128) : RGB(150, 150, 150);
            }
            else if (m_pingLatency < 50)
            {
                swprintf_s(buf, L"Ping: %dms", m_pingLatency);
                pingColor = m_darkTheme ? RGB(0, 220, 100) : RGB(0, 150, 60);
            }
            else if (m_pingLatency < 100)
            {
                swprintf_s(buf, L"Ping: %dms", m_pingLatency);
                pingColor = m_darkTheme ? RGB(255, 200, 50) : RGB(200, 140, 0);
            }
            else
            {
                swprintf_s(buf, L"Ping: %dms", m_pingLatency);
                pingColor = m_darkTheme ? RGB(255, 80, 80) : RGB(200, 40, 40);
            }
            
            SetTextColor(hdc, pingColor);
            TextOutW(hdc, startX, y, buf, static_cast<int>(wcslen(buf)));
            
            // If showing ping, move Data Today to the right
            if (m_showDataToday) 
            {
                startX += 85; // Offset for Data Today (aligned with Upload/RAM)
            }
        }
        
        // Draw Data Today
        if (m_showDataToday)
        {
            std::wstring todayStr = L"Today: " + FormatBytes(m_todayBytesDown + m_todayBytesUp);
            // Use a subtler color (e.g., CPU/RAM color or gray)
            COLORREF dateColor = m_darkTheme ? RGB(200, 200, 200) : RGB(80, 80, 80);
            SetTextColor(hdc, dateColor);
            
            // If Ping is hidden, Data Today aligns left (startX = PADDING)
            // If Ping is shown, Data Today aligns to second column (startX = PADDING + 85)
            TextOutW(hdc, startX, y, todayStr.c_str(), static_cast<int>(todayStr.length()));
        }
        
        y += LINE_HEIGHT;
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void FloatingWindow::RecalculateWindowSize()
{
    int newWidth, newHeight;
    
    if (m_miniMode)
    {
        // Mini mode: single compact line
        newWidth = WINDOW_WIDTH_MINI;
        newHeight = WINDOW_HEIGHT_MINI;
    }
    else
    {
        // Normal mode: calculate based on visible rows
        int visibleRows = 0;
        
        if (m_showNetwork) visibleRows++;
        if (m_showCPU || m_showRAM) visibleRows++;
        if (m_showPing || m_showDataToday) visibleRows++;
        
        newWidth = WINDOW_WIDTH;
        newHeight = (PADDING * 2) + (visibleRows * LINE_HEIGHT);
    }
    
    // Resize window but keep position
    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// ========== PHASE 1: SNAP-TO-EDGE ==========

void FloatingWindow::ApplySnapToEdge(RECT* pRect)
{
    if (!pRect) return;
    
    // Get work area (excludes taskbar)
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    
    int windowWidth = pRect->right - pRect->left;
    int windowHeight = pRect->bottom - pRect->top;
    
    // Snap to left edge
    if (abs(pRect->left - workArea.left) < m_snapDistance)
    {
        pRect->left = workArea.left;
        pRect->right = pRect->left + windowWidth;
    }
    
    // Snap to right edge
    if (abs(pRect->right - workArea.right) < m_snapDistance)
    {
        pRect->right = workArea.right;
        pRect->left = pRect->right - windowWidth;
    }
    
    // Snap to top edge
    if (abs(pRect->top - workArea.top) < m_snapDistance)
    {
        pRect->top = workArea.top;
        pRect->bottom = pRect->top + windowHeight;
    }
    
    // Snap to bottom edge
    if (abs(pRect->bottom - workArea.bottom) < m_snapDistance)
    {
        pRect->bottom = workArea.bottom;
        pRect->top = pRect->bottom - windowHeight;
    }
}

void FloatingWindow::SetSnapToEdge(bool enabled)
{
    m_snapToEdge = enabled;
}

void FloatingWindow::SetSnapDistance(int pixels)
{
    m_snapDistance = (pixels > 0) ? pixels : 20;
}

// ========== PHASE 1: CLICK-THROUGH MODE ==========

void FloatingWindow::SetClickThrough(bool enabled)
{
    if (m_clickThrough == enabled) return;
    
    m_clickThrough = enabled;
    
    if (m_hwnd)
    {
        LONG_PTR exStyle = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
        
        if (enabled)
        {
            // Add WS_EX_TRANSPARENT to pass clicks through
            exStyle |= WS_EX_TRANSPARENT;
        }
        else
        {
            // Remove WS_EX_TRANSPARENT
            exStyle &= ~WS_EX_TRANSPARENT;
        }
        
        SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, exStyle);
    }
}

// ========== PHASE 1: MINI-MODE ==========

void FloatingWindow::SetMiniMode(bool enabled)
{
    if (m_miniMode == enabled) return;
    
    m_miniMode = enabled;
    RecalculateWindowSize();
    Invalidate();
}

void FloatingWindow::ToggleMiniMode()
{
    SetMiniMode(!m_miniMode);
}

} // namespace NetworkMonitor

