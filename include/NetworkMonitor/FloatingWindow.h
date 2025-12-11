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
     * Update displayed ping latency
     * @param latencyMs Ping latency in milliseconds, -1 for unavailable/timeout
     */
    void UpdatePing(int latencyMs);

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
    void SetShowNetwork(bool show);
    void SetShowCPU(bool show);
    void SetShowRAM(bool show);
    void SetShowPing(bool show);
    void SetShowDataToday(bool show);
    
    void UpdateDataToday(uint64_t bytesDown, uint64_t bytesUp);

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
    void RecalculateWindowSize();

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    bool m_darkTheme;
    BYTE m_opacity;

    // Display flags
    bool m_showNetwork;
    bool m_showCPU;
    bool m_showRAM;
    bool m_showPing;
    bool m_showDataToday;

    // Current values
    double m_downloadSpeed;
    double m_uploadSpeed;
    SpeedUnit m_speedUnit;
    double m_cpuPercent;
    double m_ramPercent;
    int m_pingLatency;  // -1 = unavailable
    uint64_t m_todayBytesDown;
    uint64_t m_todayBytesUp;

    // Window dimensions
    static constexpr int WINDOW_WIDTH = 190;
    static constexpr int WINDOW_HEIGHT = 90; // Increased base height, will be dynamic
    static constexpr int PADDING = 8;
    static constexpr int LINE_HEIGHT = 16;

    // Class name
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"NetworkMonitorFloatingWindow";
    static bool s_classRegistered;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_FLOATING_WINDOW_H
