#include "TestUtils.h"

#include <cstdio>
#include <chrono>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#endif

namespace NetPulseTests
{

namespace
{
    int g_failures = 0;
}

void EnableTestSandbox()
{
#ifdef _WIN32
    wchar_t tempPath[MAX_PATH] = {0};
    if (GetTempPathW(MAX_PATH, tempPath) == 0)
    {
        return;
    }

    std::wstring sandbox = tempPath;
    sandbox += L"NetPulseTest_";
    sandbox += std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(sandbox.c_str(), nullptr);

    _wputenv_s(L"NETPULSE_TEST_DATA_DIR", sandbox.c_str());
    _wputenv_s(L"NETPULSE_TEST_REGISTRY_PATH", L"Software\\NetworkMonitor\\Test");
    _wputenv_s(L"NETPULSE_TEST_MODE", L"1");
#endif
}

bool ClearTestRegistrySandbox()
{
#ifdef _WIN32
    const wchar_t* registryPath = _wgetenv(L"NETPULSE_TEST_REGISTRY_PATH");
    if (!registryPath || registryPath[0] == L'\0')
    {
        return false;
    }

    return RegDeleteTreeW(HKEY_CURRENT_USER, registryPath) == ERROR_SUCCESS
        || RegDeleteTreeW(HKEY_CURRENT_USER, registryPath) == ERROR_FILE_NOT_FOUND;
#else
    return false;
#endif
}

bool PumpWinMessages(unsigned int timeoutMs)
{
#ifdef _WIN32
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    MSG msg = {};

    while (std::chrono::steady_clock::now() < deadline)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
#else
    (void)timeoutMs;
    return false;
#endif
}

HWND WaitForWindow(const std::wstring& className, unsigned int timeoutMs)
{
#ifdef _WIN32
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline)
    {
        HWND hwnd = FindWindowW(className.c_str(), nullptr);
        if (hwnd)
        {
            return hwnd;
        }
        PumpWinMessages(25);
    }
#endif
    (void)className;
    (void)timeoutMs;
    return nullptr;
}

HWND FindProcessDialogHwnd(unsigned int timeoutMs)
{
#ifdef _WIN32
    struct EnumData
    {
        DWORD pid = 0;
        HWND result = nullptr;
    };

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline)
    {
        EnumData data;
        data.pid = GetCurrentProcessId();

        EnumWindows(
            [](HWND hwnd, LPARAM lParam) -> BOOL {
                auto* search = reinterpret_cast<EnumData*>(lParam);
                if (!IsWindowVisible(hwnd))
                {
                    return TRUE;
                }

                DWORD windowPid = 0;
                GetWindowThreadProcessId(hwnd, &windowPid);
                if (windowPid != search->pid)
                {
                    return TRUE;
                }

                wchar_t className[16] = {};
                if (GetClassNameW(hwnd, className, static_cast<int>(sizeof(className) / sizeof(className[0]))) == 0)
                {
                    return TRUE;
                }

                if (wcscmp(className, L"#32770") == 0)
                {
                    search->result = hwnd;
                    return FALSE;
                }

                return TRUE;
            },
            reinterpret_cast<LPARAM>(&data));

        if (data.result)
        {
            return data.result;
        }

        PumpWinMessages(25);
    }
#endif
    (void)timeoutMs;
    return nullptr;
}

std::wstring GetTestSandboxDir()
{
#ifdef _WIN32
    const wchar_t* testDataDir = _wgetenv(L"NETPULSE_TEST_DATA_DIR");
    if (testDataDir && testDataDir[0] != L'\0')
    {
        return std::wstring(testDataDir);
    }
#endif
    return std::wstring();
}

void LogTestMessage(const wchar_t* message)
{
    if (!message)
    {
        return;
    }
#if defined(_WIN32)
    _putws(message);
    fflush(stdout);
#else
    std::wprintf(L"%ls\n", message);
#endif
}

void AssertTrue(bool condition, const wchar_t* testName)
{
    if (!condition)
    {
        ++g_failures;
        std::wstring msg = L"[FAIL] ";
        if (testName)
        {
            msg += testName;
        }
        else
        {
            msg += L"Unnamed test";
        }
        LogTestMessage(msg.c_str());
    }
    else if (testName)
    {
        std::wstring msg = L"[ OK ] ";
        msg += testName;
        LogTestMessage(msg.c_str());
    }
}

int GetFailureCount()
{
    return g_failures;
}

void ResetFailureCount()
{
    g_failures = 0;
}

NetPulse::NetworkStats MakeNetworkStats(const std::wstring& name,
                                       unsigned long long bytesDown,
                                       unsigned long long bytesUp,
                                       bool isActive,
                                       bool isPhysical)
{
    NetPulse::NetworkStats stats;
    stats.interfaceName = name;
    stats.bytesReceived = bytesDown;
    stats.bytesSent = bytesUp;
    stats.isActive = isActive;
    stats.isPhysicalHardware = isPhysical;
    return stats;
}

} // namespace NetPulseTests

