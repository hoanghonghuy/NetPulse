#include "NetPulse/SystemMonitor.h"
#include "TestUtils.h"
#include <thread>
#include <chrono>

using namespace NetPulse;

namespace NetPulseTests
{

void RunSystemMonitorTests()
{
    LogTestMessage(L"=== SystemMonitor tests ===");

    // 1. Test Initialize/Shutdown lifecycle
    {
        SystemMonitor monitor;
        AssertTrue(monitor.Initialize(), L"SystemMonitor::Initialize returns true");
        AssertTrue(monitor.Initialize(), L"SystemMonitor::Initialize when already initialized returns true");
        
        monitor.Shutdown();
        // re-initialize
        AssertTrue(monitor.Initialize(), L"SystemMonitor::Initialize after Shutdown returns true");
    }

    // 2. Test standard Update and getters
    {
        SystemMonitor monitor;
        AssertTrue(monitor.Initialize(), L"SystemMonitor::Initialize for update tests");

        // First update establishes baseline for CPU times
        monitor.Update();

        // Sleep to let system times tick
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        monitor.Update();

        double cpu = monitor.GetCPUPercent();
        double ram = monitor.GetRAMPercent();
        uint64_t totalRam = monitor.GetTotalRAM();
        uint64_t usedRam = monitor.GetUsedRAM();

        AssertTrue(cpu >= 0.0 && cpu <= 100.0, L"SystemMonitor::GetCPUPercent is in [0, 100] range");
        AssertTrue(ram >= 0.0 && ram <= 100.0, L"SystemMonitor::GetRAMPercent is in [0, 100] range");
        AssertTrue(totalRam > 0, L"SystemMonitor::GetTotalRAM returns non-zero value");
        AssertTrue(usedRam <= totalRam, L"SystemMonitor::GetUsedRAM is less than or equal to GetTotalRAM");
    }

    // 3. Test auto-initialization on Update
    {
        SystemMonitor monitor;
        // Do not call Initialize()
        monitor.Update(); // This should call Initialize() internally
        
        // Wait and update again
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        monitor.Update();

        double cpu = monitor.GetCPUPercent();
        double ram = monitor.GetRAMPercent();
        uint64_t totalRam = monitor.GetTotalRAM();
        uint64_t usedRam = monitor.GetUsedRAM();

        AssertTrue(cpu >= 0.0 && cpu <= 100.0, L"Auto-init CPU percent is valid");
        AssertTrue(ram >= 0.0 && ram <= 100.0, L"Auto-init RAM percent is valid");
        AssertTrue(totalRam > 0, L"Auto-init RAM total is valid");
        AssertTrue(usedRam <= totalRam, L"Auto-init RAM used is valid");
    }
}

} // namespace NetPulseTests
