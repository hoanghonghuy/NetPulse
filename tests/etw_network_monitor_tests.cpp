#include "TestUtils.h"
#include "NetPulse/EtwNetworkMonitor.h"
#include "test_fakes/FakeEtwSession.h"
#include <thread>
#include <chrono>

namespace NetPulseTests
{

using namespace NetPulse;

struct EtwNetworkMonitorTestFriend
{
    static EtwNetworkMonitor* GetStaticInstance()
    {
        return EtwNetworkMonitor::s_instance;
    }
    static TRACEHANDLE GetSessionHandle(const EtwNetworkMonitor& m)
    {
        return m.m_sessionHandle;
    }
    static TRACEHANDLE GetTraceHandle(const EtwNetworkMonitor& m)
    {
        return m.m_traceHandle;
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

void TestEtwStaticInstanceClearedOnStopWithFake()
{
    LogTestMessage(L"  Running TestEtwStaticInstanceClearedOnStopWithFake...");

    FakeEtwSession fake;
    EtwNetworkMonitor monitor(&fake);
    AssertTrue(monitor.Start(), L"Start succeeds with fake session");
    AssertTrue(fake.m_startCalls == 1, L"Start calls fake Start");
    AssertTrue(fake.m_enableCalls == 1, L"Start calls fake EnableTraceEx2");
    AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() != nullptr, L"Start sets static instance");
    AssertTrue(monitor.IsRunning(), L"Monitor is marked running");
    
    std::wstring debugStart = L"    SessionHandle after start: " + std::to_wstring(EtwNetworkMonitorTestFriend::GetSessionHandle(monitor)) +
                             L", TraceHandle: " + std::to_wstring(EtwNetworkMonitorTestFriend::GetTraceHandle(monitor));
    LogTestMessage(debugStart.c_str());

    // Wait a brief moment to ensure ProcessThreadProc has run and set m_traceHandle
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    monitor.Stop();
    
    std::wstring debugStop = L"    Stop calls: " + std::to_wstring(fake.m_stopCalls) +
                            L", Close calls: " + std::to_wstring(fake.m_closeCalls);
    LogTestMessage(debugStop.c_str());

    AssertTrue(fake.m_stopCalls == 2, L"Stop calls fake Stop twice (once on cleanup, once on stop)");
    AssertTrue(fake.m_closeCalls == 1, L"Stop calls fake CloseTrace");
    AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() == nullptr, L"Stop clears static instance");
    AssertTrue(!monitor.IsRunning(), L"Stop clears running flag");
}

void TestEtwStartFailWithFake()
{
    LogTestMessage(L"  Running TestEtwStartFailWithFake...");

    FakeEtwSession fake;
    fake.m_startResult = ERROR_ACCESS_DENIED;
    EtwNetworkMonitor monitor(&fake);
    AssertTrue(!monitor.Start(), L"Start fails when StartTraceW returns error");
    AssertTrue(fake.m_startCalls == 1, L"Start calls fake Start");
    AssertTrue(fake.m_enableCalls == 0, L"Start does not call EnableProvider if StartTrace fails");
    AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() == nullptr, L"Static instance remains null");
}

void TestEtwEnableProviderFailWithFake()
{
    LogTestMessage(L"  Running TestEtwEnableProviderFailWithFake...");

    FakeEtwSession fake;
    fake.m_enableResult = ERROR_INVALID_PARAMETER;
    EtwNetworkMonitor monitor(&fake);
    AssertTrue(!monitor.Start(), L"Start fails when EnableTraceEx2 returns error");
    AssertTrue(fake.m_startCalls == 1, L"Start calls fake Start");
    AssertTrue(fake.m_enableCalls == 1, L"Start calls fake EnableProvider");
    AssertTrue(fake.m_stopCalls == 2, L"Start stops the session if enable provider fails (including initial cleanup)");
    AssertTrue(EtwNetworkMonitorTestFriend::GetStaticInstance() == nullptr, L"Static instance remains null");
}

void RunEtwNetworkMonitorTests()
{
    LogTestMessage(L"=== EtwNetworkMonitor tests ===");

    TestEtwStopIdempotent();
    TestEtwTrafficAggregation();
    TestEtwResetStats();
    TestEtwProcessNameSpecialPids();
    TestEtwStaticInstanceClearedOnStopWithFake();
    TestEtwStartFailWithFake();
    TestEtwEnableProviderFailWithFake();

    LogTestMessage(L"EtwNetworkMonitor tests completed.");
}

} // namespace NetPulseTests
