#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/Application.h"
#include "NetPulse/ApplicationRuntime.h"
#include "../../resources/resource.h"
#include <windows.h>

#if defined(_MSC_VER)
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    NetPulse::ApplicationRuntime::ParseCommandLine();
    NetPulse::ApplicationRuntime::ApplySandboxEnvironment();

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32)
    {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
        const DPI_AWARENESS_CONTEXT CTX_UNAWARE_GDISCALED = (DPI_AWARENESS_CONTEXT)-5;

        SetProcessDpiAwarenessContextFunc pSetDpi =
            reinterpret_cast<SetProcessDpiAwarenessContextFunc>(reinterpret_cast<void*>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")));

        if (pSetDpi)
        {
            pSetDpi(CTX_UNAWARE_GDISCALED);
        }
    }

    NetPulse::LogDebug(L"WinMain: NetPulse starting");

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, NetPulse::ApplicationRuntime::GetMutexName());
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (!NetPulse::ApplicationRuntime::IsTestMode())
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

        if (hMutex)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        hMutex = CreateMutexW(nullptr, TRUE, NetPulse::ApplicationRuntime::GetMutexName());
    }

    NetPulse::Application app;
    if (!app.Initialize(hInstance))
    {
        NetPulse::LogError(L"WinMain: Application::Initialize failed");
        if (hMutex)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return -1;
    }

    const auto& runtimeOptions = NetPulse::ApplicationRuntime::Options();
    if (!runtimeOptions.testScenario.empty())
    {
        int scenarioResult = NetPulse::ApplicationRuntime::RunTestScenario(app);
        app.Cleanup();
        if (hMutex)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        NetPulse::LogDebug(L"WinMain: test scenario exit code " + std::to_wstring(scenarioResult));
        return scenarioResult;
    }

    if (runtimeOptions.autoExit)
    {
        app.Cleanup();
        if (hMutex)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return 0;
    }

    int result = app.Run();

    app.Cleanup();

    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    NetPulse::LogDebug(L"WinMain: exiting with code " + std::to_wstring(result));
    return result;
}
