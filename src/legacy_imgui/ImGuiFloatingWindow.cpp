// ImGuiFloatingWindow.cpp - Floating Widget Implementation
#include "NetPulse/ImGuiFloatingWindow.h"
#include "NetPulse/Utils.h"
#include <sstream>
#include <iomanip>

namespace NetPulse
{

ImGuiFloatingWindow::ImGuiFloatingWindow()
    : m_downloadSpeed(0.0)
    , m_uploadSpeed(0.0)
    , m_cpuPercent(0.0)
    , m_ramPercent(0.0)
    , m_posX(100)
    , m_posY(100)
    , m_isOpen(true)
    , m_isDragging(false)
{
}

ImGuiFloatingWindow::~ImGuiFloatingWindow()
{
}

void ImGuiFloatingWindow::Update(INetworkStatsProvider* networkMonitor, SystemMonitor* systemMonitor)
{
    if (networkMonitor)
    {
        NetworkStats stats = networkMonitor->GetAggregatedStats();
        m_downloadSpeed = stats.currentDownloadSpeed;
        m_uploadSpeed = stats.currentUploadSpeed;
    }

    if (systemMonitor)
    {
        m_cpuPercent = systemMonitor->GetCPUPercent();
        m_ramPercent = systemMonitor->GetRAMPercent();
    }
}

bool ImGuiFloatingWindow::Render(const AppConfig& config)
{
    if (!config.showFloatingWindow)
        return false;

    // Format speed strings
    auto FormatSpeed = [](double bytesPerSec) -> std::string {
        std::ostringstream oss;
        if (bytesPerSec < 1024.0)
        {
            oss << std::fixed << std::setprecision(0) << bytesPerSec << " B/s";
        }
        else if (bytesPerSec < 1024.0 * 1024.0)
        {
            oss << std::fixed << std::setprecision(1) << (bytesPerSec / 1024.0) << " KB/s";
        }
        else
        {
            oss << std::fixed << std::setprecision(2) << (bytesPerSec / 1024.0 / 1024.0) << " MB/s";
        }
        return oss.str();
    };

    // Window flags for floating widget
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize;

    // Set position
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(m_posX), static_cast<float>(m_posY)), ImGuiCond_FirstUseEver);
    
    // Custom styling for floating window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.70f, 1.00f, 0.50f));

    if (ImGui::Begin("##FloatingWidget", &m_isOpen, flags))
    {
        // Update position from window
        ImVec2 pos = ImGui::GetWindowPos();
        m_posX = static_cast<int>(pos.x);
        m_posY = static_cast<int>(pos.y);

        // Network Speed (if enabled)
        if (config.floatingShowNetwork)
        {
            // Download
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "D");
            ImGui::SameLine();
            ImGui::Text("%s", FormatSpeed(m_downloadSpeed).c_str());
            
            // Upload
            ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "U");
            ImGui::SameLine();
            ImGui::Text("%s", FormatSpeed(m_uploadSpeed).c_str());
        }

        // CPU (if enabled)
        if (config.floatingShowCPU)
        {
            if (config.floatingShowNetwork) ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "CPU");
            ImGui::SameLine();
            ImGui::Text("%.0f%%", m_cpuPercent);
        }

        // RAM (if enabled)
        if (config.floatingShowRAM)
        {
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "RAM");
            ImGui::SameLine();
            ImGui::Text("%.0f%%", m_ramPercent);
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    return !m_isOpen; // Return true if window was closed
}

} // namespace NetPulse
