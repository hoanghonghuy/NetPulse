// ImGuiHistory.h - History Panel using Dear ImGui
#ifndef NETWORK_MONITOR_IMGUI_HISTORY_H
#define NETWORK_MONITOR_IMGUI_HISTORY_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/HistoryLogger.h"
#include "imgui.h"
#include <vector>
#include <string>

namespace NetworkMonitor
{

class ImGuiHistory
{
public:
    ImGuiHistory();
    ~ImGuiHistory();

    // Update data from HistoryLogger
    void Update();

    // Render the history panel
    void Render();

private:
    std::string FormatBytesNarrow(uint64_t bytes);
    std::string FormatTimestamp(std::time_t timestamp);

    // Cached data
    std::vector<HistorySample> m_recentSamples;
    uint64_t m_todayDown;
    uint64_t m_todayUp;
    uint64_t m_monthDown;
    uint64_t m_monthUp;

    // UI state
    bool m_showOnlyToday;
    int m_sampleLimit;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IMGUI_HISTORY_H
