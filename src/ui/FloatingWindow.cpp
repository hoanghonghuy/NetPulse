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
    , m_downloadSpeed(0.0)
    , m_uploadSpeed(0.0)
    , m_speedUnit(SpeedUnit::KiloBytesPerSecond)
    , m_cpuPercent(0.0)
    , m_ramPercent(0.0)
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

void FloatingWindow::UpdateSpeed(double downloadBytesPerSec, double uploadBytesPerSec, SpeedUnit unit)
{
    m_downloadSpeed = downloadBytesPerSec;
    m_uploadSpeed = uploadBytesPerSec;
    m_speedUnit = unit;
    Invalidate();
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
        // Allow dragging from anywhere on the window
        return HTCAPTION;

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
        TextOutW(hdc, PADDING + 70, y, upText.c_str(), static_cast<int>(upText.length()));
        y += LINE_HEIGHT;
    }

    // Draw CPU
    if (m_showCPU)
    {
        wchar_t buf[32];
        swprintf_s(buf, L"CPU: %.0f%%", m_cpuPercent);
        SetTextColor(hdc, cpuColor);
        TextOutW(hdc, PADDING, y, buf, static_cast<int>(wcslen(buf)));
        y += LINE_HEIGHT;
    }

    // Draw RAM
    if (m_showRAM)
    {
        wchar_t buf[32];
        swprintf_s(buf, L"RAM: %.0f%%", m_ramPercent);
        SetTextColor(hdc, ramColor);
        TextOutW(hdc, PADDING, y, buf, static_cast<int>(wcslen(buf)));
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

std::wstring FloatingWindow::FormatSpeed(double bytesPerSec, SpeedUnit unit) const
{
    double value = bytesPerSec;
    const wchar_t* suffix = L"B/s";

    switch (unit)
    {
    case SpeedUnit::BytesPerSecond:
        suffix = L"B/s";
        break;
    case SpeedUnit::KiloBytesPerSecond:
        value = bytesPerSec / 1024.0;
        suffix = L"KB/s";
        break;
    case SpeedUnit::MegaBytesPerSecond:
        value = bytesPerSec / (1024.0 * 1024.0);
        suffix = L"MB/s";
        break;
    case SpeedUnit::MegaBitsPerSecond:
        value = (bytesPerSec * 8.0) / (1024.0 * 1024.0);
        suffix = L"Mbps";
        break;
    }

    wchar_t buf[32];
    if (value < 10.0)
    {
        swprintf_s(buf, L"%.1f", value);
    }
    else
    {
        swprintf_s(buf, L"%.0f", value);
    }

    return std::wstring(buf) + suffix;
}

} // namespace NetworkMonitor
