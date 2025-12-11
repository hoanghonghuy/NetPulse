// ImGuiSettings.h - Modern Settings Panel using Dear ImGui
#ifndef NETWORK_MONITOR_IMGUI_SETTINGS_H
#define NETWORK_MONITOR_IMGUI_SETTINGS_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/Interfaces/IConfigProvider.h"
#include "imgui.h"

namespace NetworkMonitor
{

class ImGuiSettings
{
public:
    ImGuiSettings();
    ~ImGuiSettings();

    // Initialize with current config
    void SetConfig(const AppConfig& config);

    // Get modified config
    AppConfig GetConfig() const { return m_config; }

    // Check if settings were modified
    bool IsModified() const { return m_modified; }

    // Render the settings panel (returns true if Save was clicked)
    bool Render();

    // Reset modified flag after saving
    void ResetModified() { m_modified = false; }

private:
    AppConfig m_config;
    AppConfig m_originalConfig;
    bool m_modified;
    bool m_isOpen;

    // Tab rendering helpers
    void RenderGeneralTab();
    void RenderDisplayTab();
    void RenderAdvancedTab();
    void RenderFloatingTab();
    void RenderDataUsageTab();
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IMGUI_SETTINGS_H
