#ifndef NETWORK_MONITOR_FLOATING_WINDOW_H
#define NETWORK_MONITOR_FLOATING_WINDOW_H

#include "NetworkMonitor/Common.h"
#include <string>

namespace NetworkMonitor
{

/**
 * FloatingWindow - Desktop widget showing network speed, CPU, and RAM usage
 * 
 * Features:
 * - Always on top, semi-transparent layered window
 * - Draggable from anywhere on the window
 * - Configurable opacity and visibility of each metric
 */
class FloatingWindow
{
public:
    FloatingWindow();
    ~FloatingWindow();

    /**
     * Create and register the floating window
     * @param hInstance Application instance
     * @return true if created successfully
     */
    bool Create(HINSTANCE hInstance);

    /**
     * Destroy the window
     */
    void Destroy();

    /**
     * Show or hide the window
     */
    void Show(bool visible);

    /**
     * Check if window is visible
     */
    bool IsVisible() const;

    /**
     * Update displayed network speed
     */
    void UpdateSpeed(double downloadBytesPerSec, double uploadBytesPerSec, SpeedUnit unit);

    /**
     * Update displayed CPU usage percentage
     */
    void UpdateCPU(double cpuPercent);

    /**
     * Update displayed RAM usage percentage
     */
    void UpdateRAM(double ramPercent);

    /**
     * Set window position
     */
    void SetPosition(int x, int y);

    /**
     * Get current window position
     */
    void GetPosition(int& x, int& y) const;

    /**
     * Set window opacity (0-255)
     */
    void SetOpacity(BYTE alpha);

    /**
     * Set dark theme mode
     */
    void SetDarkTheme(bool dark);

    /**
     * Configure which metrics to display
     */
    void SetShowNetwork(bool show) { m_showNetwork = show; Invalidate(); }
    void SetShowCPU(bool show) { m_showCPU = show; Invalidate(); }
    void SetShowRAM(bool show) { m_showRAM = show; Invalidate(); }

    /**
     * Get window handle
     */
    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void RegisterWindowClass(HINSTANCE hInstance);
    void Paint(HDC hdc);
    void Invalidate();
    std::wstring FormatSpeed(double bytesPerSec, SpeedUnit unit) const;

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    bool m_darkTheme;
    BYTE m_opacity;

    // Display flags
    bool m_showNetwork;
    bool m_showCPU;
    bool m_showRAM;

    // Current values
    double m_downloadSpeed;
    double m_uploadSpeed;
    SpeedUnit m_speedUnit;
    double m_cpuPercent;
    double m_ramPercent;

    // Window dimensions
    static constexpr int WINDOW_WIDTH = 150;
    static constexpr int WINDOW_HEIGHT = 70;
    static constexpr int PADDING = 8;
    static constexpr int LINE_HEIGHT = 16;

    // Class name
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"NetworkMonitorFloatingWindow";
    static bool s_classRegistered;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_FLOATING_WINDOW_H
