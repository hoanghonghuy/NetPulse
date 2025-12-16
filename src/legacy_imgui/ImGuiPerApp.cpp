// ImGuiPerApp.cpp - Per-App Network Usage Panel Implementation
#include "NetPulse/ImGuiPerApp.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace NetPulse
{

ImGuiPerApp::ImGuiPerApp()
    : m_sortColumn(0)
    , m_sortAscending(false) // Default descending (highest first)
{
}

ImGuiPerApp::~ImGuiPerApp()
{
}

void ImGuiPerApp::Update(PerAppMonitor* perAppMonitor)
{
    if (!perAppMonitor)
        return;

    perAppMonitor->Refresh();
    m_appUsage = perAppMonitor->GetAppUsage();

    // Apply sorting
    std::sort(m_appUsage.begin(), m_appUsage.end(),
        [this](const AppNetworkUsage& a, const AppNetworkUsage& b) -> bool
        {
            bool less = false;
            switch (m_sortColumn)
            {
            case 0: // Process Name
                less = a.processName < b.processName;
                break;
            case 1: // TCP Connections
                less = a.tcpConnections < b.tcpConnections;
                break;
            case 2: // UDP Connections
                less = a.udpConnections < b.udpConnections;
                break;
            case 3: // Bytes Sent
                less = a.bytesSent < b.bytesSent;
                break;
            case 4: // Bytes Received
                less = a.bytesReceived < b.bytesReceived;
                break;
            default:
                less = a.processName < b.processName;
                break;
            }
            return m_sortAscending ? less : !less;
        });
}

void ImGuiPerApp::Render()
{
    ImGui::SetNextWindowPos(ImVec2(10, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 250), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(450, 180), ImVec2(FLT_MAX, FLT_MAX));

    if (!ImGui::Begin("Per-App Network Usage", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }
    
    ImGui::Text("Active Network Connections by Application");
    ImGui::Separator();

        // Table
        if (ImGui::BeginTable("AppUsageTable", 5, 
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | 
            ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_ScrollY, ImVec2(0, -1)))
        {
            // Headers
            ImGui::TableSetupColumn("Process", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("TCP", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("UDP", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Sent", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Received", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupScrollFreeze(0, 1); // Freeze header row
            ImGui::TableHeadersRow();

            // Handle sorting
            ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
            if (sortSpecs && sortSpecs->SpecsDirty)
            {
                if (sortSpecs->SpecsCount > 0)
                {
                    m_sortColumn = sortSpecs->Specs[0].ColumnIndex;
                    m_sortAscending = (sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);
                }
                sortSpecs->SpecsDirty = false;
            }

            // Rows
            for (const auto& app : m_appUsage)
            {
                ImGui::TableNextRow();

                // Process Name with text clipping
                ImGui::TableSetColumnIndex(0);
                char processNameNarrow[256];
                WideCharToMultiByte(CP_UTF8, 0, app.processName.c_str(), -1, processNameNarrow, 256, nullptr, nullptr);
                ImGui::TextUnformatted(processNameNarrow);
                if (ImGui::IsItemHovered() && strlen(processNameNarrow) > 20)
                {
                    ImGui::SetTooltip("%s", processNameNarrow);
                }

                // TCP Connections
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", app.tcpConnections);

                // UDP Connections
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", app.udpConnections);

                // Bytes Sent
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", FormatBytesNarrow(app.bytesSent).c_str());

                // Bytes Received
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", FormatBytesNarrow(app.bytesReceived).c_str());
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Text("Total: %zu processes", m_appUsage.size());

    ImGui::End();
}

std::string ImGuiPerApp::FormatBytesNarrow(uint64_t bytes)
{
    std::ostringstream oss;
    if (bytes < 1024)
    {
        oss << bytes << " B";
    }
    else if (bytes < 1024 * 1024)
    {
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    }
    else if (bytes < 1024 * 1024 * 1024)
    {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0) << " MB";
    }
    else
    {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0 / 1024.0) << " GB";
    }
    return oss.str();
}

} // namespace NetPulse
