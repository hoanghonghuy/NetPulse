#include "TestUtils.h"
#include "NetPulse/EtwNetworkMonitor.h"

namespace NetPulseTests
{

using namespace NetPulse;

struct EtwNetworkMonitorTestFriend
{
    static EtwNetworkMonitor* GetStaticInstance()
    {
        return EtwNetworkMonitor::s_instance;
    }
};

void TestEtwStopIdempotent()
{
    LogTestMessage(L"  Running TestEtwStopIdempotent...");

    EtwNetworkMonitor monitor;
    monitor.Stop();
    monitor.Stop();

    AssertTrue(!monitor.IsRunning(), L"Stop on idle monitor leaves IsRunning false");
}

void TestEtwTrafficAggregation()
{
    LogTestMessage(L"  Running TestEtwTrafficAggregation...");

    EtwNetworkMonitor monitor;
    const DWORD pid = 4242;

    monitor.ApplyTrafficEventForTest(EtwNetworkMonitor::EVENT_ID_TCP_SEND, pid, 100);
    monitor.ApplyTrafficEventForTest(EtwNetworkMonitor::EVENT_ID_TCP_RECV, pid, 250);
    monitor.ApplyTrafficEventForTest(EtwNetworkMonitor::EVENT_ID_UDP_SEND, pid, 50);

    ProcessTrafficStats stats = monitor.GetProcessStats(pid);
    AssertTrue(stats.bytesSent.load() == 150ULL, L"ETW aggregation sums send bytes");
    AssertTrue(stats.bytesReceived.load() == 250ULL, L"ETW aggregation sums receive bytes");

    auto allStats = monitor.GetAllStats();
    AssertTrue(allStats.size() == 1, L"ETW aggregation tracks one PID");
}

void TestEtwResetStats()
{
    LogTestMessage(L"  Running TestEtwResetStats...");

    EtwNetworkMonitor monitor;
    monitor.ApplyTrafficEventForTest(EtwNetworkMonitor::EVENT_ID_TCP_SEND, 99, 10);
    AssertTrue(!monitor.GetAllStats().empty(), L"Stats populated before reset");

    monitor.ResetStats();
    AssertTrue(monitor.GetAllStats().empty(), L"ResetStats clears all process stats");
}

void TestEtwProcessNameSpecialPids()
{
    LogTestMessage(L"  Running TestEtwProcessNameSpecialPids...");

    EtwNetworkMonitor monitor;
    AssertTrue(monitor.GetProcessName(0) == L"System Idle Process",
               L"PID 0 maps to System Idle Process");
    AssertTrue(monitor.GetProcessName(4) == L"System",
               L"PID 4 maps to System");
}

void TestEtwStaticInstanceClearedOnStop()
{
    LogTestMessage(L"  Running TestEtwStaticInstanceClearedOnStop...");

    EtwNetworkMonitor monitor;
    if (monitor.Start())
    {
        AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() != nullptr,
                   L"Start sets static instance when successful");
        monitor.Stop();
        AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() == nullptr,
                   L"Stop clears static instance");
        AssertTrue(!monitor.IsRunning(), L"Stop clears running flag");
    }
    else
    {
        LogTestMessage(L"    Start failed (permissions?) — verifying fail path clears instance");
        AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() == nullptr,
                   L"Start fail path clears static instance");
    }
}

void RunEtwNetworkMonitorTests()
{
    LogTestMessage(L"=== EtwNetworkMonitor tests ===");

    TestEtwStopIdempotent();
    TestEtwTrafficAggregation();
    TestEtwResetStats();
    TestEtwProcessNameSpecialPids();
    TestEtwStaticInstanceClearedOnStop();

    LogTestMessage(L"EtwNetworkMonitor tests completed.");
}

} // namespace NetPulseTests
