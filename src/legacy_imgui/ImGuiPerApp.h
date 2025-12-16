// ImGuiPerApp.h - Per-App Network Usage Panel using Dear ImGui
#ifndef NETWORK_MONITOR_IMGUI_PERAPP_H
#define NETWORK_MONITOR_IMGUI_PERAPP_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NetPulse/Common.h"
#include "NetPulse/PerAppMonitor.h"
#include "imgui.h"
#include <vector>

namespace NetPulse
{

class ImGuiPerApp
{
public:
    ImGuiPerApp();
    ~ImGuiPerApp();

    // Update data from PerAppMonitor
    void Update(PerAppMonitor* perAppMonitor);

    // Render the per-app panel
    void Render();

private:
    // Helper to format bytes
    std::string FormatBytesNarrow(uint64_t bytes);

    // Cached data
    std::vector<AppNetworkUsage> m_appUsage;
    
    // Sorting state
    int m_sortColumn;
    bool m_sortAscending;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_IMGUI_PERAPP_H
