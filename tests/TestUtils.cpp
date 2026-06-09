#include "TestUtils.h"

#include <cstdio>
#include <string>

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
#endif
}

void LogTestMessage(const wchar_t* message)
{
    if (!message)
    {
        return;
    }
#if defined(_WIN32)
    _putws(message);
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

} // namespace NetPulseTests
