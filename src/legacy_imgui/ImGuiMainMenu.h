// ImGuiMainMenu.h - Main Menu Bar using Dear ImGui
#ifndef NETWORK_MONITOR_IMGUI_MAIN_MENU_H
#define NETWORK_MONITOR_IMGUI_MAIN_MENU_H

#include "imgui.h"
#include <functional>

namespace NetPulse
{

// Panel visibility flags
struct PanelVisibility
{
    bool showDashboard = true;
    bool showSettings = true;
    bool showHistory = true;
    bool showPerApp = true;
};

class ImGuiMainMenu
{
public:
    ImGuiMainMenu();
    ~ImGuiMainMenu();

    // Render the main menu bar
    // Returns false if user requested exit
    bool Render(PanelVisibility& visibility);

    // Set exit callback
    void SetExitCallback(std::function<void()> callback) { m_exitCallback = callback; }

private:
    std::function<void()> m_exitCallback;
    bool m_showAbout;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_IMGUI_MAIN_MENU_H
