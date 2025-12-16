// ImGuiDashboard.cpp - Modern Dashboard UI Implementation
#include "NetPulse/ImGuiDashboard.h"
#include "NetPulse/Utils.h"
#include "imgui.h"
#include <algorithm>

namespace NetPulse
{

ImGuiDashboard::ImGuiDashboard()
    : m_currentDownloadMbps(0.0)
    , m_currentUploadMbps(0.0)
    , m_cpuPercent(0.0)
    , m_ramPercent(0.0)
    , m_totalBytesReceived(0)
    , m_totalBytesSent(0)
{
}

ImGuiDashboard::~ImGuiDashboard()
{
}

void ImGuiDashboard::Update(INetworkStatsProvider* networkMonitor, SystemMonitor* systemMonitor)
{
    if (networkMonitor)
    {
        NetworkStats stats = networkMonitor->GetAggregatedStats();
        
        // Convert to Mbps for display
        m_currentDownloadMbps = (stats.currentDownloadSpeed * 8.0) / 1000000.0;
        m_currentUploadMbps = (stats.currentUploadSpeed * 8.0) / 1000000.0;
        m_totalBytesReceived = static_cast<uint64_t>(stats.bytesReceived);
        m_totalBytesSent = static_cast<uint64_t>(stats.bytesSent);

        // Add to history
        m_downloadHistory.push_back(static_cast<float>(m_currentDownloadMbps));
        m_uploadHistory.push_back(static_cast<float>(m_currentUploadMbps));

        // Trim history
        while (m_downloadHistory.size() > HISTORY_SIZE)
        {
            m_downloadHistory.pop_front();
        }
        while (m_uploadHistory.size() > HISTORY_SIZE)
        {
            m_uploadHistory.pop_front();
        }
    }

    if (systemMonitor)
    {
        systemMonitor->Update();
        m_cpuPercent = systemMonitor->GetCPUPercent();
        m_ramPercent = systemMonitor->GetRAMPercent();
    }
}

void ImGuiDashboard::Render()
{
    // Main Dashboard Window with minimum size
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
    
    if (!ImGui::Begin("Network Dashboard", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    // === Header ===
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Network Monitor - Real-time Statistics");
    ImGui::Separator();
    ImGui::Spacing();

    // === Speed and Data Summary (2-column layout) ===
    float availWidth = ImGui::GetContentRegionAvail().x;
    float columnWidth = availWidth * 0.5f;
    
    ImGui::BeginChild("SpeedColumn", ImVec2(columnWidth - 10, 80), false);
    {
        ImGui::Text("Current Speed");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Download:");
        ImGui::SameLine();
        ImGui::Text("%.2f Mbps", m_currentDownloadMbps);
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.2f, 1.0f), "Upload:");
        ImGui::SameLine();
        ImGui::Text("%.2f Mbps", m_currentUploadMbps);
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("DataColumn", ImVec2(0, 80), false);
    {
        ImGui::Text("Total Transferred");
        ImGui::Spacing();
        std::wstring downloadedStr = FormatBytes(m_totalBytesReceived);
        std::wstring uploadedStr = FormatBytes(m_totalBytesSent);
        char downloadedNarrow[64], uploadedNarrow[64];
        WideCharToMultiByte(CP_UTF8, 0, downloadedStr.c_str(), -1, downloadedNarrow, 64, nullptr, nullptr);
        WideCharToMultiByte(CP_UTF8, 0, uploadedStr.c_str(), -1, uploadedNarrow, 64, nullptr, nullptr);
        ImGui::Text("Downloaded: %s", downloadedNarrow);
        ImGui::Text("Uploaded:   %s", uploadedNarrow);
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Speed Graphs ===
    ImGui::Text("Speed History (Last 2 Minutes)");
    ImGui::Spacing();

    // Calculate max value for scaling
    float maxDownload = 1.0f; // Minimum 1 Mbps scale
    float maxUpload = 1.0f;
    for (float v : m_downloadHistory) maxDownload = std::max(maxDownload, v);
    for (float v : m_uploadHistory) maxUpload = std::max(maxUpload, v);

    // Download Graph
    RenderSpeedGraph("Download (Mbps)", m_downloadHistory, maxDownload * 1.2f, IM_COL32(50, 200, 50, 255));
    ImGui::Spacing();
    
    // Upload Graph
    RenderSpeedGraph("Upload (Mbps)", m_uploadHistory, maxUpload * 1.2f, IM_COL32(200, 100, 50, 255));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === System Stats ===
    ImGui::Text("System Resources");
    ImGui::Spacing();

    // CPU Progress Bar with overlay text
    char cpuOverlay[32];
    snprintf(cpuOverlay, sizeof(cpuOverlay), "CPU: %.1f%%", m_cpuPercent);
    ImGui::ProgressBar(static_cast<float>(m_cpuPercent / 100.0), ImVec2(-1, 20), cpuOverlay);
    ImGui::Spacing();

    // RAM Progress Bar with overlay text
    char ramOverlay[32];
    snprintf(ramOverlay, sizeof(ramOverlay), "RAM: %.1f%%", m_ramPercent);
    ImGui::ProgressBar(static_cast<float>(m_ramPercent / 100.0), ImVec2(-1, 20), ramOverlay);

    ImGui::End();
}

void ImGuiDashboard::RenderSpeedGraph(const char* label, const std::deque<float>& data, float maxValue, ImU32 color)
{
    // Convert deque to vector for ImGui
    std::vector<float> plotData(data.begin(), data.end());
    
    // Pad with zeros if not enough data
    while (plotData.size() < HISTORY_SIZE)
    {
        plotData.insert(plotData.begin(), 0.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PlotLines(label, plotData.data(), static_cast<int>(plotData.size()), 0, nullptr, 0.0f, maxValue, ImVec2(-1, 80));
    ImGui::PopStyleColor();
}

} // namespace NetPulse
