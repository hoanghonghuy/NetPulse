#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/Application.h"
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

    // Enable GDI-scaled DPI awareness: sharp GDI text + Windows handles bitmap scaling
    // This requires Windows 10 1703+ but is the best balance of quality and compatibility
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);

    NetPulse::LogDebug(L"WinMain: NetPulse starting");

    // Check if another instance is already running
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, APP_MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        NetPulse::LogError(L"WinMain: another instance is already running");
        std::wstring msg = NetPulse::LoadStringResource(IDS_ERROR_ALREADY_RUNNING);
        std::wstring title = NetPulse::LoadStringResource(IDS_APP_TITLE);
        if (title.empty())
        {
            title = APP_NAME;
        }
        if (msg.empty())
        {
            msg = L"NetPulse is already running!";
        }
        NetPulse::ShowDarkMessageBox(nullptr, msg, title, MB_OK | MB_ICONINFORMATION, true);
        return 0;
    }

    // Use Application class for initialization and message loop
    NetPulse::Application app;
    if (!app.Initialize(hInstance))
    {
        // Initialization failed; Application will show any relevant error messages
        NetPulse::LogError(L"WinMain: Application::Initialize failed");
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

    NetPulse::LogDebug(L"WinMain: exiting with code " + std::to_wstring(result));
    return result;
}

