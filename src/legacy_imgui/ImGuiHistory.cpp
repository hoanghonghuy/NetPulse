// ImGuiHistory.cpp - History Panel Implementation
#include "NetPulse/ImGuiHistory.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace NetPulse
{

ImGuiHistory::ImGuiHistory()
    : m_todayDown(0)
    , m_todayUp(0)
    , m_monthDown(0)
    , m_monthUp(0)
    , m_showOnlyToday(false)
    , m_sampleLimit(100)
{
}

ImGuiHistory::~ImGuiHistory()
{
}

void ImGuiHistory::Update()
{
    HistoryLogger& logger = HistoryLogger::Instance();

    // Get totals
    unsigned long long todayDown = 0, todayUp = 0;
    unsigned long long monthDown = 0, monthUp = 0;
    
    logger.GetTotalsToday(todayDown, todayUp, nullptr);
    logger.GetTotalsThisMonth(monthDown, monthUp, nullptr);

    m_todayDown = todayDown;
    m_todayUp = todayUp;
    m_monthDown = monthDown;
    m_monthUp = monthUp;

    // Get recent samples
    m_recentSamples.clear();
    logger.GetRecentSamples(m_sampleLimit, m_recentSamples, nullptr, m_showOnlyToday);
}

void ImGuiHistory::Render()
{
    ImGui::SetNextWindowPos(ImVec2(620, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 280), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(380, 220), ImVec2(FLT_MAX, FLT_MAX));

    if (!ImGui::Begin("Usage History", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }
    
    // === Summary using Table ===
    ImGui::Text("Usage Summary");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTable("SummaryTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("Period", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Downloaded", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Uploaded", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Today");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", FormatBytesNarrow(m_todayDown).c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", FormatBytesNarrow(m_todayUp).c_str());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("This Month");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%s", FormatBytesNarrow(m_monthDown).c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", FormatBytesNarrow(m_monthUp).c_str());

        ImGui::EndTable();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Filter Controls ===
    ImGui::Checkbox("Today only", &m_showOnlyToday);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("##Limit", &m_sampleLimit, 0, 0))
    {
        if (m_sampleLimit < 10) m_sampleLimit = 10;
        if (m_sampleLimit > 500) m_sampleLimit = 500;
    }
    ImGui::SameLine();
    ImGui::Text("rows");
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
    {
        Update();
    }

    ImGui::Spacing();

        // === Recent Samples Table ===
        if (ImGui::BeginTable("HistoryTable", 4,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_ScrollY, ImVec2(0, -30)))
        {
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Down", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Up", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (const auto& sample : m_recentSamples)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", FormatTimestamp(sample.timestamp).c_str());

                ImGui::TableSetColumnIndex(1);
                char interfaceNarrow[128];
                WideCharToMultiByte(CP_UTF8, 0, sample.interfaceName.c_str(), -1, interfaceNarrow, 128, nullptr, nullptr);
                ImGui::Text("%s", interfaceNarrow);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", FormatBytesNarrow(sample.bytesDown).c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", FormatBytesNarrow(sample.bytesUp).c_str());
            }

        ImGui::EndTable();
    }

    // === Actions ===
    ImGui::Spacing();
    if (ImGui::Button("Delete All"))
    {
        HistoryLogger::Instance().DeleteAll();
        Update();
    }
    ImGui::SameLine();
    if (ImGui::Button("Keep 30 Days"))
    {
        HistoryLogger::Instance().TrimToRecentDays(30);
        Update();
    }
    ImGui::SameLine();
    if (ImGui::Button("Keep 90 Days"))
    {
        HistoryLogger::Instance().TrimToRecentDays(90);
        Update();
    }
    
    ImGui::End();
}

std::string ImGuiHistory::FormatBytesNarrow(uint64_t bytes)
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
    else if (bytes < 1024ULL * 1024 * 1024)
    {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0) << " MB";
    }
    else
    {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0 / 1024.0) << " GB";
    }
    return oss.str();
}

std::string ImGuiHistory::FormatTimestamp(std::time_t timestamp)
{
    std::tm tm;
    if (localtime_s(&tm, &timestamp) != 0)
        return "Invalid";

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (tm.tm_year + 1900) << "-"
        << std::setw(2) << (tm.tm_mon + 1) << "-"
        << std::setw(2) << tm.tm_mday << " "
        << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min;
    return oss.str();
}

} // namespace NetPulse
