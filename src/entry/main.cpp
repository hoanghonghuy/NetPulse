#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/Utils.h"
#include "NetworkMonitor/Application.h"
#include "../../resources/resource.h"
#include <windows.h>
// ============================================================================
// WINMAIN - APPLICATION ENTRY POINT
// ============================================================================

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    NetworkMonitor::LogDebug(L"WinMain: NetworkMonitor starting");

    // Check if another instance is already running
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"NetworkMonitor_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        NetworkMonitor::LogError(L"WinMain: another instance is already running");
        std::wstring msg = NetworkMonitor::LoadStringResource(IDS_ERROR_ALREADY_RUNNING);
        std::wstring title = NetworkMonitor::LoadStringResource(IDS_APP_TITLE);
        if (title.empty())
        {
            title = APP_NAME;
        }
        if (msg.empty())
        {
            msg = L"NetworkMonitor is already running!";
        }
        NetworkMonitor::ShowDarkMessageBox(nullptr, msg, title, MB_OK | MB_ICONINFORMATION, true);
        return 0;
    }

    // Use Application class for initialization and message loop
    NetworkMonitor::Application app;
    if (!app.Initialize(hInstance))
    {
        // Initialization failed; Application will show any relevant error messages
        NetworkMonitor::LogError(L"WinMain: Application::Initialize failed");
        if (hMutex)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return -1;
    }

    int result = app.Run();

    // Explicit cleanup (guarded internally by m_initialized)
    app.Cleanup();

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    NetworkMonitor::LogDebug(L"WinMain: exiting with code " + std::to_wstring(result));
    return result;
}

