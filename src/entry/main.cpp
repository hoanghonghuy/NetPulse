#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/Application.h"
#include "../../resources/resource.h"
#include <windows.h>

// For MSVC (Visual Studio), embed the manifest dependency via pragma to avoid
// collisions with auto-generated manifests. MinGW uses app.rc instead.
#if defined(_MSC_VER)
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

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
    // This requires Windows 10 1703+ but is the best balance of quality and compatibility.
    // We load it dynamically to maintain Windows 7 compatibility (where it will just fail gracefully).
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32)
    {
        // Define function pointer type
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
        
        // This constant is available in headers if _WIN32_WINNT >= 0x0A00, but we define it here for Win7 compat build
        // DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED is (DPI_AWARENESS_CONTEXT)-5
        const DPI_AWARENESS_CONTEXT CTX_UNAWARE_GDISCALED = (DPI_AWARENESS_CONTEXT)-5;

        SetProcessDpiAwarenessContextFunc pSetDpi = 
            reinterpret_cast<SetProcessDpiAwarenessContextFunc>(reinterpret_cast<void*>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")));
            
        if (pSetDpi)
        {
            pSetDpi(CTX_UNAWARE_GDISCALED);
        }
    }

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

