#ifndef NETWORK_MONITOR_FLOATING_WINDOW_H
#define NETWORK_MONITOR_FLOATING_WINDOW_H

#include "NetPulse/Common.h"
#include "NetPulse/SparklineRenderer.h"
#include <string>
#include <memory>
#include <functional>

namespace NetPulse
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

    // ========== PHASE 1 NEW FEATURES ==========
    
    /**
     * Enable/disable snap-to-edge behavior
     * When enabled, window will snap to screen edges when dragged within snap distance
     */
    void SetSnapToEdge(bool enabled);
    bool IsSnapToEdge() const { return m_snapToEdge; }
    
    /**
     * Set the snap distance in pixels (default: 20)
     */
    void SetSnapDistance(int pixels);
    int GetSnapDistance() const { return m_snapDistance; }
    
    /**
     * Enable/disable click-through mode
     * When enabled, mouse clicks pass through the window
     */
    void SetClickThrough(bool enabled);
    bool IsClickThrough() const { return m_clickThrough; }
    
    /**
     * Toggle mini-mode (compact single-line display)
     */
    void SetMiniMode(bool enabled);
    bool IsMiniMode() const { return m_miniMode; }
    void ToggleMiniMode();
    
    // ========== PHASE 2 FEATURES ==========
    
    /**
     * Enable/disable sparkline graph display
     */
    void SetShowSparkline(bool enabled);
    bool IsShowSparkline() const { return m_showSparkline; }
    
    /**
     * Set sparkline time range (0=30s, 1=1m, 2=5m)
     */
    void SetSparklineTimeRange(int range);
    int GetSparklineTimeRange() const { return m_sparklineTimeRange; }
    
    /**
     * Export sparkline chart as PNG image
     * @return true if export successful
     */
    bool ExportChartAsPNG(const std::wstring& filePath);
    
    /**
     * Set callback for config changes (to save updated time range)
     */
    void SetConfigChangeCallback(std::function<void(int timeRange)> callback);

    // ========== PHASE 3 FEATURES ==========
    
    /**
     * Update VPN connection status
     * @param isVpnActive true if VPN is connected
     * @param isProxyActive true if proxy is enabled
     */
    void UpdateVpnStatus(bool isVpnActive, bool isProxyActive);
    
    /**
     * Update public IP address display
     * @param ip Public IP string, empty if unavailable
     */
    void UpdatePublicIP(const std::wstring& ip);
    
    /**
     * Configure VPN/IP display
     */
    void SetShowVpnStatus(bool show);
    void SetShowPublicIP(bool show);
    bool IsShowVpnStatus() const { return m_showVpnStatus; }
    bool IsShowPublicIP() const { return m_showPublicIP; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void RegisterWindowClass(HINSTANCE hInstance);
    void Paint(HDC hdc);
    void PaintNormal(HDC hdc);
    void PaintMiniMode(HDC hdc);
    void Invalidate();
    void RecalculateWindowSize();
    void ApplySnapToEdge(RECT* pRect);


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

    // Phase 1 Features
    bool m_snapToEdge;        // Enable snap-to-edge
    int m_snapDistance;       // Snap distance in pixels (default: 20)
    bool m_clickThrough;      // Click-through mode
    bool m_miniMode;          // Mini/compact mode

    // Phase 2 Features - Sparkline
    bool m_showSparkline;     // Show sparkline graph
    int m_sparklineTimeRange; // 0=30s, 1=1m, 2=5m
    std::unique_ptr<SparklineRenderer> m_downloadSparkline;
    std::unique_ptr<SparklineRenderer> m_uploadSparkline;
    std::function<void(int)> m_configChangeCallback;

    // Phase 3 Features - VPN/Proxy Detection
    bool m_showVpnStatus;     // Show VPN indicator
    bool m_showPublicIP;      // Show public IP
    bool m_isVpnActive;       // Current VPN status
    bool m_isProxyActive;     // Current proxy status
    std::wstring m_publicIP;  // Current public IP

    // Window dimensions
    static constexpr int WINDOW_WIDTH = 190;
    static constexpr int WINDOW_WIDTH_MINI = 100;  // Compact width for mini mode
    static constexpr int WINDOW_HEIGHT = 90; // Increased base height, will be dynamic
    static constexpr int WINDOW_HEIGHT_MINI = 24;  // Single line height for mini mode
    static constexpr int PADDING = 8;
    static constexpr int LINE_HEIGHT = 16;
    static constexpr int SPARKLINE_HEIGHT = 20;  // Height for sparkline graph

    // Class name
    static constexpr const wchar_t* WINDOW_CLASS_NAME = L"NetworkMonitorFloatingWindow";
    static bool s_classRegistered;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_FLOATING_WINDOW_H
