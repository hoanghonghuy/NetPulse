// ImGuiMainMenu.cpp - Main Menu Bar Implementation
#include "NetPulse/ImGuiMainMenu.h"

namespace NetPulse
{

ImGuiMainMenu::ImGuiMainMenu()
    : m_showAbout(false)
{
}

ImGuiMainMenu::~ImGuiMainMenu()
{
}

bool ImGuiMainMenu::Render(PanelVisibility& visibility)
{
    bool keepRunning = true;

    if (ImGui::BeginMainMenuBar())
    {
        // File Menu
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                keepRunning = false;
                if (m_exitCallback)
                {
                    m_exitCallback();
                }
            }
            ImGui::EndMenu();
        }

        // View Menu
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Dashboard", nullptr, &visibility.showDashboard);
            ImGui::MenuItem("Settings", nullptr, &visibility.showSettings);
            ImGui::MenuItem("History", nullptr, &visibility.showHistory);
            ImGui::MenuItem("Per-App Usage", nullptr, &visibility.showPerApp);
            ImGui::EndMenu();
        }

        // Help Menu
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About Network Monitor"))
            {
                m_showAbout = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // About Dialog
    if (m_showAbout)
    {
        ImGui::SetNextWindowSize(ImVec2(350, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("About Network Monitor", &m_showAbout, ImGuiWindowFlags_NoResize))
        {
            ImGui::Text("Network Monitor");
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Version: 1.3.1 (Modern UI)");
            ImGui::Spacing();
            
            ImGui::TextWrapped("A lightweight network monitoring tool with real-time statistics, "
                              "per-application usage tracking, and data usage alerts.");
            ImGui::Spacing();
            
            ImGui::Text("Built with Dear ImGui + DirectX 11");
            ImGui::Spacing();
            
            if (ImGui::Button("Close", ImVec2(80, 0)))
            {
                m_showAbout = false;
            }
        }
        ImGui::End();
    }

    return keepRunning;
}

} // namespace NetPulse
