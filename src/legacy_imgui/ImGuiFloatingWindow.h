// ImGuiFloatingWindow.h - Always-on-top Floating Widget
#ifndef NETWORK_MONITOR_IMGUI_FLOATING_WINDOW_H
#define NETWORK_MONITOR_IMGUI_FLOATING_WINDOW_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/Interfaces/INetworkStatsProvider.h"
#include "NetworkMonitor/SystemMonitor.h"
#include "imgui.h"

namespace NetworkMonitor
{

class ImGuiFloatingWindow
{
public:
    ImGuiFloatingWindow();
    ~ImGuiFloatingWindow();

    // Update stats
    void Update(INetworkStatsProvider* networkMonitor, SystemMonitor* systemMonitor);

    // Render the floating widget
    // Returns true if user closed the window
    bool Render(const AppConfig& config);

    // Position
    void SetPosition(int x, int y) { m_posX = x; m_posY = y; }
    int GetPosX() const { return m_posX; }
    int GetPosY() const { return m_posY; }

private:
    double m_downloadSpeed;  // bytes/sec
    double m_uploadSpeed;    // bytes/sec
    double m_cpuPercent;
    double m_ramPercent;

    int m_posX;
    int m_posY;
    bool m_isOpen;
    bool m_isDragging;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IMGUI_FLOATING_WINDOW_H
