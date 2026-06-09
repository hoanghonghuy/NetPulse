// ============================================================================
// File: main_tests.cpp
// Description: Test runner for NetworkMonitor
// Author: NetworkMonitor Project (tests)
// ============================================================================

#include "TestUtils.h"

namespace NetPulseTests
{
void RunHistoryLoggerTests();
void RunNetworkMonitorTests();
void RunUtilsTests();
void RunNetworkCalculatorTests();
void RunConfigManagerTests();
void RunTrayIconTests();
void RunTaskbarOverlayTests();
void RunFloatingWindowTests();
void RunVpnProxyDetectorTests();
void RunSpeedTestTests();
void RunDataUsageMonitorTests();
void RunUpdateCoordinatorTests();
void RunMenuHandlerTests();
void RunLanguageManagerTests();
void RunSpeedTestHistoryPersistenceTests();
void RunDialogManagerTests();
void RunConnectionMonitorTests();
void RunUpdateCheckerTests();
void RunEtwNetworkMonitorTests();
void RunComponentRendererTests();
}

using namespace NetPulseTests;

int main()
{
    EnableTestSandbox();
    LogTestMessage(L"Running NetworkMonitor tests...");

    RunHistoryLoggerTests();
    RunNetworkMonitorTests();
    RunUtilsTests();
    RunNetworkCalculatorTests();
    RunDataUsageMonitorTests();
    RunUpdateCoordinatorTests();
    RunMenuHandlerTests();
    RunLanguageManagerTests();
    RunSpeedTestHistoryPersistenceTests();
    RunConfigManagerTests();
    RunDialogManagerTests();
    RunConnectionMonitorTests();
    RunUpdateCheckerTests();
    RunTrayIconTests();
    RunTaskbarOverlayTests();
    RunFloatingWindowTests();
    RunVpnProxyDetectorTests();
    RunEtwNetworkMonitorTests();
    RunSpeedTestTests();
    RunComponentRendererTests();

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
