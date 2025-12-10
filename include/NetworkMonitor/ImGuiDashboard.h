// ImGuiDashboard.h - Modern Dashboard UI using Dear ImGui
#ifndef NETWORK_MONITOR_IMGUI_DASHBOARD_H
#define NETWORK_MONITOR_IMGUI_DASHBOARD_H

#ifndef NOMINMAX
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#endif

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/Interfaces/INetworkStatsProvider.h"
#include "NetworkMonitor/SystemMonitor.h"
#include "imgui.h"
#include <vector>
#include <deque>

namespace NetworkMonitor
{

class ImGuiDashboard
{
public:
    ImGuiDashboard();
    ~ImGuiDashboard();

    // Update data from monitors
    void Update(INetworkStatsProvider* networkMonitor, SystemMonitor* systemMonitor);

    // Render the dashboard UI
    void Render();

private:
    // Helper to draw a scrolling line graph
    void RenderSpeedGraph(const char* label, const std::deque<float>& data, float maxValue, ImU32 color);

    // Data history for scrolling graphs
    static constexpr int HISTORY_SIZE = 120; // 2 minutes at 1 update/sec
    std::deque<float> m_downloadHistory;
    std::deque<float> m_uploadHistory;

    // Current values
    double m_currentDownloadMbps;
    double m_currentUploadMbps;
    double m_cpuPercent;
    double m_ramPercent;
    uint64_t m_totalBytesReceived;
    uint64_t m_totalBytesSent;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IMGUI_DASHBOARD_H
