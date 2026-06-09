// NOTE: This class manages its own GDI resources (Fonts, Brushes, Bitmaps) for performance and stability.
// Refactoring to shared helpers is discouraged to avoid object lifetime issues.
#include "NetPulse/FloatingWindow.h"
#include "NetPulse/ThemeHelper.h"
#include "NetPulse/Utils.h"
#include "../../resources/resource.h"
#include <windowsx.h>
#include <commdlg.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace NetPulse
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
    , m_showSparkline(true)    // Show sparkline by default
    , m_sparklineTimeRange(0)  // Default 30s
    , m_downloadSparkline(std::make_unique<SparklineRenderer>(30))
    , m_uploadSparkline(std::make_unique<SparklineRenderer>(30))
    , m_showVpnStatus(true)    // Show VPN status by default
    , m_showPublicIP(true)     // Show public IP by default
    , m_isVpnActive(false)
    , m_isProxyActive(false)
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
    // Add data points to sparklines (always, for smooth graph)
    if (m_downloadSparkline && m_uploadSparkline)
    {
        m_downloadSparkline->AddDataPoint(downloadSpeed);
        m_uploadSparkline->AddDataPoint(uploadSpeed);
    }
    
    // Check if values changed significantly to avoid unnecessary repaints
    if (m_downloadSpeed != downloadSpeed || m_uploadSpeed != uploadSpeed || m_speedUnit != unit)
    {
        m_downloadSpeed = downloadSpeed;
        m_uploadSpeed = uploadSpeed;
        m_speedUnit = unit;
        Invalidate();
    }
    else if (m_showSparkline)
    {
        // Force repaint for sparkline even if speed didn't change
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

    WNDCLASSEXW wc = {};
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
    // Double buffering to prevent flicker
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Create memory DC and bitmap
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Paint to memory DC
    if (m_miniMode)
    {
        PaintMiniMode(hdcMem);
    }
    else
    {
        PaintNormal(hdcMem);
    }

    // copy to screen
    BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

void FloatingWindow::PaintMiniMode(HDC hdc)
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Get theme colors
    const auto& colors = ThemeHelper::GetColors(m_darkTheme);

    // Fill background
    HBRUSH hBgBrush = CreateSolidBrush(colors.background);
    FillRect(hdc, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw thin border
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, colors.border);
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
    
    SetTextColor(hdc, colors.download);
    TextOutW(hdc, 4, 4, text.c_str(), static_cast<int>(text.length()));

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void FloatingWindow::PaintNormal(HDC hdc)
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    // Get theme colors
    const auto& colors = ThemeHelper::GetColors(m_darkTheme);

    // Fill background with rounded rectangle effect
    HBRUSH hBgBrush = CreateSolidBrush(colors.background);
    FillRect(hdc, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw border
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, colors.border);
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
        
        SetTextColor(hdc, colors.download);
        TextOutW(hdc, PADDING, y, downText.c_str(), static_cast<int>(downText.length()));
        
        SetTextColor(hdc, colors.upload);
        TextOutW(hdc, PADDING + 85, y, upText.c_str(), static_cast<int>(upText.length()));
        y += LINE_HEIGHT;
        
        // Draw sparkline graph below network speeds
        if (m_showSparkline && m_downloadSparkline && m_uploadSparkline)
        {
            RECT sparkRect;
            sparkRect.left = PADDING;
            sparkRect.right = rc.right - PADDING;
            sparkRect.top = y;
            sparkRect.bottom = y + SPARKLINE_HEIGHT;
            
            // Use semi-transparent fill for download sparkline
            m_downloadSparkline->Render(hdc, sparkRect, colors.download, colors.downloadFill);
            
            // Overlay upload sparkline (line only, no fill)
            m_uploadSparkline->Render(hdc, sparkRect, colors.upload, 0);
            
            y += SPARKLINE_HEIGHT + 4;  // +4 for spacing
        }
    }

    // Draw CPU and RAM on the same line
    if (m_showCPU || m_showRAM)
    {
        int xPos = PADDING;
        
        if (m_showCPU)
        {
            wchar_t buf[32];
            swprintf_s(buf, L"CPU: %.0f%%", m_cpuPercent);
            SetTextColor(hdc, colors.cpu);
            TextOutW(hdc, xPos, y, buf, static_cast<int>(wcslen(buf)));
            xPos = PADDING + 85; // Move to second column for RAM
        }

        if (m_showRAM)
        {
            wchar_t buf[32];
            swprintf_s(buf, L"RAM: %.0f%%", m_ramPercent);
            SetTextColor(hdc, colors.ram);
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
            
            std::wstring pingLabel = LoadStringResource(IDS_SPEED_PING); // Reuse "Ping:"
            if (pingLabel.empty()) pingLabel = L"Ping:";

            if (m_pingLatency < 0)
            {
                swprintf_s(buf, L"%s --", pingLabel.c_str());
                pingColor = colors.pingNone;
            }
            else if (m_pingLatency < 50)
            {
                swprintf_s(buf, L"%s %dms", pingLabel.c_str(), m_pingLatency);
                pingColor = colors.pingLow;
            }
            else if (m_pingLatency < 100)
            {
                swprintf_s(buf, L"%s %dms", pingLabel.c_str(), m_pingLatency);
                pingColor = colors.pingMed;
            }
            else
            {
                swprintf_s(buf, L"%s %dms", pingLabel.c_str(), m_pingLatency);
                pingColor = colors.pingHigh;
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
            std::wstring todayLabel = LoadStringResource(IDS_DASHBOARD_LABEL_TODAY); // Reuse "Today:"
            if (todayLabel.empty()) todayLabel = L"Today:";
            
            std::wstring todayStr = todayLabel + L" " + FormatBytes(m_todayBytesDown + m_todayBytesUp);
            SetTextColor(hdc, colors.textSecondary);
            
            // If Ping is hidden, Data Today aligns left (startX = PADDING)
            // If Ping is shown, Data Today aligns to second column (startX = PADDING + 85)
            TextOutW(hdc, startX, y, todayStr.c_str(), static_cast<int>(todayStr.length()));
        }
        
        y += LINE_HEIGHT;
    }

    // Draw VPN Status and Public IP (Phase 3)
    if (m_showVpnStatus || m_showPublicIP)
    {
        int startX = PADDING;
        
        // Draw VPN/Proxy indicator
        if (m_showVpnStatus)
        {
            wchar_t vpnBuf[32];
            COLORREF vpnColor;
            
            if (m_isVpnActive)
            {
                std::wstring vpnOn = LoadStringResource(IDS_FLOATING_VPN_STATE_ON);
                wcscpy_s(vpnBuf, vpnOn.empty() ? L"VPN: ON" : vpnOn.c_str());
                vpnColor = colors.vpnOn;
            }
            else if (m_isProxyActive)
            {
                std::wstring proxyOn = LoadStringResource(IDS_FLOATING_PROXY_STATE_ON);
                wcscpy_s(vpnBuf, proxyOn.empty() ? L"Proxy: ON" : proxyOn.c_str());
                vpnColor = colors.vpnProxy;
            }
            else
            {
                std::wstring vpnOff = LoadStringResource(IDS_FLOATING_VPN_STATE_OFF);
                wcscpy_s(vpnBuf, vpnOff.empty() ? L"VPN: OFF" : vpnOff.c_str());
                vpnColor = colors.vpnOff;
            }
            
            SetTextColor(hdc, vpnColor);
            TextOutW(hdc, startX, y, vpnBuf, static_cast<int>(wcslen(vpnBuf)));
            
            if (m_showPublicIP)
            {
                startX += 85; // Offset for IP (aligned with Upload/RAM/Data Today)
            }
        }
        
        // Draw Public IP
        if (m_showPublicIP && !m_publicIP.empty())
        {
            std::wstring ipLabel = LoadStringResource(IDS_FLOATING_IP_LABEL);
            if (ipLabel.empty()) ipLabel = L"IP: ";
            std::wstring ipStr = ipLabel + m_publicIP;
            SetTextColor(hdc, colors.textSecondary); // Using Secondary here for consistency, effectively ipColor
            TextOutW(hdc, startX, y, ipStr.c_str(), static_cast<int>(ipStr.length()));
        }
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
        if (m_showVpnStatus || m_showPublicIP) visibleRows++;  // Phase 3: VPN row
        
        newWidth = WINDOW_WIDTH;
        newHeight = (PADDING * 2) + (visibleRows * LINE_HEIGHT);
        
        // Add sparkline height if enabled
        if (m_showSparkline && m_showNetwork)
        {
            newHeight += SPARKLINE_HEIGHT + 4;  // +4 for spacing
        }
    }
    
    // Resize window but keep position
    if (m_hwnd)
    {
        SetWindowPos(m_hwnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// ========== PHASE 1: SNAP-TO-EDGE ==========

void FloatingWindow::ApplySnapToEdge(RECT* pRect) const
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

// ========== PHASE 2: SPARKLINE ==========

void FloatingWindow::SetShowSparkline(bool enabled)
{
    if (m_showSparkline == enabled) return;
    
    m_showSparkline = enabled;
    RecalculateWindowSize();
    Invalidate();
}

void FloatingWindow::SetSparklineTimeRange(int range)
{
    // Convert range to points: 0=30pts(30s), 1=60pts(1m), 2=300pts(5m)
    static const size_t pointCounts[] = { 30, 60, 300 };
    if (range < 0 || range > 2) range = 0;
    
    m_sparklineTimeRange = range;
    size_t points = pointCounts[range];
    
    if (m_downloadSparkline) m_downloadSparkline->SetMaxPoints(points);
    if (m_uploadSparkline) m_uploadSparkline->SetMaxPoints(points);
    
    Invalidate();
}

bool FloatingWindow::ExportChartAsBMP(const std::wstring& filePath)
{
    if (!m_downloadSparkline || !m_uploadSparkline) return false;
    
    // Create a bitmap for the chart
    const int width = 400;
    const int height = 150;
    
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hBitmap));
    
    // Fill background
    RECT rect = { 0, 0, width, height };
    HBRUSH hBrush = CreateSolidBrush(m_darkTheme ? RGB(30, 30, 30) : RGB(255, 255, 255));
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);
    
    // Draw title
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, m_darkTheme ? RGB(200, 200, 200) : RGB(50, 50, 50));
    HFONT hFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdcMem, hFont));
    
    std::wstring chartTitle = LoadStringResource(IDS_FLOATING_CHART_TITLE);
    if (chartTitle.empty()) chartTitle = L"Network Speed Chart";
    TextOutW(hdcMem, 10, 5, chartTitle.c_str(), static_cast<int>(chartTitle.length()));
    
    // Draw sparklines
    RECT dlRect = { 10, 30, width - 10, 85 };
    RECT ulRect = { 10, 95, width - 10, height - 10 };
    
    m_downloadSparkline->Render(hdcMem, dlRect, RGB(0, 180, 255), RGB(0, 100, 150));
    m_uploadSparkline->Render(hdcMem, ulRect, RGB(0, 200, 100), RGB(0, 120, 60));
    
    // Draw labels
    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);
    hFont = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    SelectObject(hdcMem, hFont);
    SetTextColor(hdcMem, RGB(0, 180, 255));
    std::wstring dlLabel = LoadStringResource(IDS_FLOATING_CHART_DOWNLOAD);
    if (dlLabel.empty()) dlLabel = L"Download";
    TextOutW(hdcMem, 12, 30, dlLabel.c_str(), static_cast<int>(dlLabel.length()));

    SetTextColor(hdcMem, RGB(0, 200, 100));
    std::wstring ulLabel = LoadStringResource(IDS_FLOATING_CHART_UPLOAD);
    if (ulLabel.empty()) ulLabel = L"Upload";
    TextOutW(hdcMem, 12, 95, ulLabel.c_str(), static_cast<int>(ulLabel.length()));
    
    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);
    
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Top-down
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    
    DWORD dwBmpSize = ((width * 3 + 3) & ~3) * height;
    std::vector<BYTE> pixels(dwBmpSize);
    GetDIBits(hdcMem, hBitmap, 0, height, pixels.data(), reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
    
    std::wstring bmpPath = filePath;
    if (bmpPath.size() > 4 && bmpPath.substr(bmpPath.size() - 4) == L".png")
    {
        bmpPath.replace(bmpPath.size() - 4, 4, L".bmp");
    }

    bool saved = false;
    HANDLE hFile = CreateFileW(bmpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        BITMAPFILEHEADER bmfHeader = {};
        bmfHeader.bfType = 0x4D42;
        bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        DWORD written = 0;
        bool ok = WriteFile(hFile, &bmfHeader, sizeof(bmfHeader), &written, nullptr) != FALSE;
        bi.biHeight = height;
        ok = ok && WriteFile(hFile, &bi, sizeof(bi), &written, nullptr) != FALSE;
        ok = ok && WriteFile(hFile, pixels.data(), dwBmpSize, &written, nullptr) != FALSE;
        CloseHandle(hFile);
        saved = ok;
    }

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    return saved;
}

// ========== PHASE 3: VPN/PROXY DETECTION ==========

void FloatingWindow::UpdateVpnStatus(bool isVpnActive, bool isProxyActive)
{
    if (m_isVpnActive != isVpnActive || m_isProxyActive != isProxyActive)
    {
        m_isVpnActive = isVpnActive;
        m_isProxyActive = isProxyActive;
        if (m_showVpnStatus)
        {
            Invalidate();
        }
    }
}

void FloatingWindow::UpdatePublicIP(const std::wstring& ip)
{
    if (m_publicIP != ip)
    {
        m_publicIP = ip;
        if (m_showPublicIP)
        {
            Invalidate();
        }
    }
}

void FloatingWindow::SetShowVpnStatus(bool show)
{
    if (m_showVpnStatus != show)
    {
        m_showVpnStatus = show;
        RecalculateWindowSize();
        Invalidate();
    }
}

void FloatingWindow::SetShowPublicIP(bool show)
{
    if (m_showPublicIP != show)
    {
        m_showPublicIP = show;
        RecalculateWindowSize();
        Invalidate();
    }
}

} // namespace NetPulse


