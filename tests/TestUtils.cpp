#include "TestUtils.h"

#include <cstdio>

namespace NetPulseTests
{

namespace
{
    int g_failures = 0;
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
