// ============================================================================
// File: main_tests.cpp
// Description: Test runner for NetworkMonitor
// Author: NetworkMonitor Project (tests)
// ============================================================================

#include "TestUtils.h"

namespace NetworkMonitorTests
{
void RunHistoryLoggerTests();
void RunNetworkMonitorTests();
void RunUtilsTests();
void RunNetworkCalculatorTests();
void RunConfigManagerTests();
void RunTrayIconTests();
void RunTaskbarOverlayTests();
void RunFloatingWindowTests();
}

using namespace NetworkMonitorTests;

int main()
{
    LogTestMessage(L"Running NetworkMonitor tests...");

    RunHistoryLoggerTests();
    RunNetworkMonitorTests();
    RunUtilsTests();
    RunNetworkCalculatorTests();
    RunConfigManagerTests();
    RunTrayIconTests();
    RunTaskbarOverlayTests();
    RunFloatingWindowTests();

    int failures = GetFailureCount();
    if (failures == 0)
    {
        LogTestMessage(L"All tests passed.");
        return 0;
    }

    std::wstring summary = L"Tests failed: ";
    summary += std::to_wstring(static_cast<unsigned long long>(failures));
    LogTestMessage(summary.c_str());
    return 1;
}
