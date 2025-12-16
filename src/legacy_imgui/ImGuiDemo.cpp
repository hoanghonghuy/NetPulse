// ImGuiDemo.cpp - Standalone demo entry point
// This file will be used *temporarily* for testing.
// Build with: cmake --build build --config Release, then Run NetworkMonitor.exe
// The actual Application entry point will call this later.

#include "NetPulse/ImGuiApp.h"
#include "imgui.h"

// Demo callback - draws a simple dashboard mockup
void RenderDemoUI()
{
    // Main window that fills the viewport
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("NetPulse Dashboard", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::Text("Network Monitor - Modern UI Demo");
    ImGui::Separator();

    // Placeholder stats
    ImGui::Text("Download Speed: 12.5 MB/s");
    ImGui::Text("Upload Speed: 3.2 MB/s");
    ImGui::Spacing();

    // Simple progress bars
    ImGui::Text("CPU Usage:");
    ImGui::ProgressBar(0.45f, ImVec2(-1, 0), "45%%");

    ImGui::Text("RAM Usage:");
    ImGui::ProgressBar(0.72f, ImVec2(-1, 0), "72%%");

    ImGui::End();
}

#ifdef IMGUI_DEMO_STANDALONE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    NetPulse::ImGuiApp app;
    if (!app.Initialize(hInstance, nCmdShow))
    {
        return 1;
    }

    app.Run(RenderDemoUI);
    app.Shutdown();
    return 0;
}
#endif
