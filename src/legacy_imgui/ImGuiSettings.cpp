// ImGuiSettings.cpp - Modern Settings Panel Implementation
#include "NetPulse/ImGuiSettings.h"

namespace NetPulse
{

ImGuiSettings::ImGuiSettings()
    : m_modified(false)
    , m_isOpen(true)
{
}

ImGuiSettings::~ImGuiSettings()
{
}

void ImGuiSettings::SetConfig(const AppConfig& config)
{
    m_config = config;
    m_originalConfig = config;
    m_modified = false;
}

bool ImGuiSettings::Render()
{
    bool saveClicked = false;

    ImGui::SetNextWindowPos(ImVec2(620, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(350, 250), ImVec2(FLT_MAX, FLT_MAX));
    
    if (!ImGui::Begin("Settings", &m_isOpen, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return false;
    }
    {
        // Tab bar for settings categories
        if (ImGui::BeginTabBar("SettingsTabs"))
        {
            if (ImGui::BeginTabItem("General"))
            {
                RenderGeneralTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Display"))
            {
                RenderDisplayTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Floating"))
            {
                RenderFloatingTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Data Usage"))
            {
                RenderDataUsageTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Advanced"))
            {
                RenderAdvancedTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Buttons
        if (ImGui::Button("Save", ImVec2(80, 0)))
        {
            saveClicked = true;
            m_modified = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(80, 0)))
        {
            m_config = m_originalConfig;
            m_modified = false;
        }
    }
    ImGui::End();

    // Check for modifications
    if (m_config != m_originalConfig)
    {
        m_modified = true;
    }

    return saveClicked;
}

void ImGuiSettings::RenderGeneralTab()
{
    ImGui::Spacing();

    // Update Interval
    const char* intervals[] = { "Fast (1s)", "Normal (2s)", "Slow (5s)" };
    int intervalIdx = (m_config.updateInterval == 1000) ? 0 :
                      (m_config.updateInterval == 2000) ? 1 : 2;
    if (ImGui::Combo("Update Interval", &intervalIdx, intervals, 3))
    {
        m_config.updateInterval = (intervalIdx == 0) ? 1000 :
                                   (intervalIdx == 1) ? 2000 : 5000;
    }

    ImGui::Spacing();

    // Auto-start
    ImGui::Checkbox("Start with Windows", &m_config.autoStart);
    if (m_config.autoStart)
    {
        ImGui::Indent();
        ImGui::Checkbox("Run as Administrator", &m_config.autoStartAsAdmin);
        ImGui::Unindent();
    }

    ImGui::Spacing();

    // Language
    const char* languages[] = { "System Default", "English", "Vietnamese", 
                                "Japanese", "Korean", "Chinese (Simplified)" };
    int langIdx = static_cast<int>(m_config.language);
    if (ImGui::Combo("Language", &langIdx, languages, 6))
    {
        m_config.language = static_cast<AppLanguage>(langIdx);
    }

    ImGui::Spacing();

    // History logging
    ImGui::Checkbox("Enable History Logging", &m_config.enableLogging);
    
    if (m_config.enableLogging)
    {
        ImGui::Indent();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 20);
        ImGui::SliderInt("##HistoryDays", &m_config.historyAutoTrimDays, 0, 365, "Auto-delete: %d days");
        if (m_config.historyAutoTrimDays == 0)
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Keep forever)");
        }
        ImGui::Unindent();
    }
}

void ImGuiSettings::RenderDisplayTab()
{
    ImGui::Spacing();

    // Theme
    const char* themes[] = { "System Default", "Light", "Dark" };
    int themeIdx = static_cast<int>(m_config.themeMode);
    if (ImGui::Combo("Theme", &themeIdx, themes, 3))
    {
        m_config.themeMode = static_cast<ThemeMode>(themeIdx);
        m_config.darkTheme = (m_config.themeMode == ThemeMode::Dark);
    }

    ImGui::Spacing();

    // Speed Unit
    const char* units[] = { "KB/s", "MB/s", "Mbps" };
    int unitIdx = static_cast<int>(m_config.displayUnit);
    if (ImGui::Combo("Speed Unit", &unitIdx, units, 3))
    {
        m_config.displayUnit = static_cast<SpeedUnit>(unitIdx);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Overlay settings
    ImGui::Text("Tray Icon Overlay");
    ImGui::SliderInt("Font Size", &m_config.overlayFontSize, 8, 20);

    // Color pickers (simplified - ImGui uses float RGBA)
    float downloadColor[3] = {
        static_cast<float>(GetRValue(m_config.overlayDownloadColor)) / 255.0f,
        static_cast<float>(GetGValue(m_config.overlayDownloadColor)) / 255.0f,
        static_cast<float>(GetBValue(m_config.overlayDownloadColor)) / 255.0f
    };
    if (ImGui::ColorEdit3("Download Color", downloadColor))
    {
        m_config.overlayDownloadColor = RGB(
            static_cast<BYTE>(downloadColor[0] * 255),
            static_cast<BYTE>(downloadColor[1] * 255),
            static_cast<BYTE>(downloadColor[2] * 255)
        );
    }

    float uploadColor[3] = {
        static_cast<float>(GetRValue(m_config.overlayUploadColor)) / 255.0f,
        static_cast<float>(GetGValue(m_config.overlayUploadColor)) / 255.0f,
        static_cast<float>(GetBValue(m_config.overlayUploadColor)) / 255.0f
    };
    if (ImGui::ColorEdit3("Upload Color", uploadColor))
    {
        m_config.overlayUploadColor = RGB(
            static_cast<BYTE>(uploadColor[0] * 255),
            static_cast<BYTE>(uploadColor[1] * 255),
            static_cast<BYTE>(uploadColor[2] * 255)
        );
    }
}

void ImGuiSettings::RenderFloatingTab()
{
    ImGui::Spacing();

    ImGui::Checkbox("Show Floating Window", &m_config.showFloatingWindow);

    if (m_config.showFloatingWindow)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Display Options:");
        ImGui::Checkbox("Show Network Speed", &m_config.floatingShowNetwork);
        ImGui::Checkbox("Show CPU Usage", &m_config.floatingShowCPU);
        ImGui::Checkbox("Show RAM Usage", &m_config.floatingShowRAM);

        ImGui::Spacing();

        int opacity = m_config.floatingWindowOpacity;
        if (ImGui::SliderInt("Opacity", &opacity, 50, 255))
        {
            m_config.floatingWindowOpacity = static_cast<BYTE>(opacity);
        }

        ImGui::Spacing();
        ImGui::Text("Position: %d, %d", m_config.floatingWindowX, m_config.floatingWindowY);
    }
}

void ImGuiSettings::RenderDataUsageTab()
{
    ImGui::Spacing();

    ImGui::Checkbox("Enable Data Usage Alerts", &m_config.enableDataUsageAlerts);

    if (m_config.enableDataUsageAlerts)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float quota = static_cast<float>(m_config.dataQuotaGB);
        if (ImGui::InputFloat("Monthly Quota (GB)", &quota, 1.0f, 10.0f, "%.1f"))
        {
            m_config.dataQuotaGB = static_cast<double>(quota);
        }

        ImGui::Spacing();

        ImGui::SliderInt("Warning Threshold (%)", &m_config.dataAlertThreshold1, 50, 100);
        ImGui::SliderInt("Critical Threshold (%)", &m_config.dataAlertThreshold2, 50, 100);

        // Ensure threshold1 <= threshold2
        if (m_config.dataAlertThreshold1 > m_config.dataAlertThreshold2)
        {
            m_config.dataAlertThreshold1 = m_config.dataAlertThreshold2;
        }
    }
}

void ImGuiSettings::RenderAdvancedTab()
{
    ImGui::Spacing();

    ImGui::Checkbox("Enable Debug Logging", &m_config.debugLogging);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Logs to %%LOCALAPPDATA%\\NetPulse\\NetPulse.log");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Ping settings
    ImGui::Text("Ping Monitor");
    
    char pingTarget[256];
    WideCharToMultiByte(CP_UTF8, 0, m_config.pingTarget.c_str(), -1, pingTarget, 256, nullptr, nullptr);
    if (ImGui::InputText("Ping Target", pingTarget, 256))
    {
        wchar_t widePingTarget[256];
        MultiByteToWideChar(CP_UTF8, 0, pingTarget, -1, widePingTarget, 256);
        m_config.pingTarget = widePingTarget;
    }

    int pingIntervalSec = m_config.pingIntervalMs / 1000;
    if (ImGui::SliderInt("Ping Interval (sec)", &pingIntervalSec, 1, 60))
    {
        m_config.pingIntervalMs = pingIntervalSec * 1000;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Hotkey
    ImGui::Text("Global Hotkey (to toggle overlay)");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Win + Shift + N (default)");
}

} // namespace NetPulse
