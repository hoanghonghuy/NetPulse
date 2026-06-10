#include "NetPulse/PingMonitor.h"
#include "test_fakes/FakeIcmpProvider.h"
#include "TestUtils.h"

using namespace NetPulse;

namespace NetPulseTests
{

void RunPingMonitorTests()
{
    LogTestMessage(L"=== PingMonitor tests ===");

    // 1. Test Initialize success
    {
        FakeIcmpProvider fake;
        PingMonitor monitor(&fake);
        AssertTrue(monitor.Initialize(L"8.8.8.8"), L"PingMonitor::Initialize succeeds on valid target");
        AssertTrue(fake.m_resolveCalls == 1, L"PingMonitor::Initialize resolves target");
        AssertTrue(fake.m_createHandleCalls == 1, L"PingMonitor::Initialize creates ICMP handle");
        AssertTrue(monitor.IsAvailable(), L"PingMonitor is available after initialize");
    }

    // 2. Test Initialize fail on resolve target
    {
        FakeIcmpProvider fake;
        PingMonitor monitor(&fake);
        AssertTrue(!monitor.Initialize(L"fail.com"), L"PingMonitor::Initialize fails on unresolvable target");
        AssertTrue(fake.m_resolveCalls == 1, L"PingMonitor::Initialize calls ResolveTarget");
        AssertTrue(fake.m_createHandleCalls == 0, L"PingMonitor does not create handle if resolve fails");
        AssertTrue(!monitor.IsAvailable(), L"PingMonitor is not available if initialize fails");
    }

    // 3. Test Initialize fail on handle creation
    {
        FakeIcmpProvider fake;
        fake.m_mockHandle = INVALID_HANDLE_VALUE;
        PingMonitor monitor(&fake);
        AssertTrue(!monitor.Initialize(L"8.8.8.8"), L"PingMonitor::Initialize fails when CreateIcmpHandle fails");
        AssertTrue(fake.m_resolveCalls == 1, L"PingMonitor resolves target");
        AssertTrue(fake.m_createHandleCalls == 1, L"PingMonitor calls CreateIcmpHandle");
        AssertTrue(!monitor.IsAvailable(), L"PingMonitor is not available");
    }

    // 4. Test Update success and latency getter
    {
        FakeIcmpProvider fake;
        fake.m_echoReplyRTT = 45;
        PingMonitor monitor(&fake);
        
        AssertTrue(monitor.Initialize(L"8.8.8.8"), L"Initialize monitor for update test");
        monitor.Update();
        AssertTrue(fake.m_sendEchoCalls == 1, L"Update calls SendEcho");
        AssertTrue(monitor.GetLatency() == 45, L"GetLatency returns mock RTT");
    }

    // 5. Test Update failure/timeout
    {
        FakeIcmpProvider fake;
        fake.m_echoReplyStatus = IP_REQ_TIMED_OUT;
        fake.m_echoReplyRTT = 0;
        PingMonitor monitor(&fake);
        
        AssertTrue(monitor.Initialize(L"8.8.8.8"), L"Initialize monitor for timeout test");
        monitor.Update();
        AssertTrue(monitor.GetLatency() == -1, L"GetLatency returns -1 on timeout status");
        
        // Test when SendEcho returns 0 (API error)
        fake.m_sendEchoResult = 0;
        monitor.Update();
        AssertTrue(monitor.GetLatency() == -1, L"GetLatency returns -1 on SendEcho API failure");
    }

    // 6. Test SetTarget
    {
        FakeIcmpProvider fake;
        PingMonitor monitor(&fake);
        
        AssertTrue(monitor.Initialize(L"8.8.8.8"), L"Initialize for SetTarget test");
        AssertTrue(fake.m_resolveCalls == 1, L"Initial resolve call");
        
        monitor.SetTarget(L"8.8.8.8");
        AssertTrue(fake.m_resolveCalls == 1, L"Setting same target does not trigger re-resolve");

        monitor.SetTarget(L"1.1.1.1");
        AssertTrue(fake.m_resolveCalls == 2, L"Setting new target triggers immediate resolve");
    }

    // 7. Test Cleanup
    {
        FakeIcmpProvider fake;
        PingMonitor monitor(&fake);
        
        AssertTrue(monitor.Initialize(L"8.8.8.8"), L"Initialize for Cleanup test");
        monitor.Cleanup();
        AssertTrue(fake.m_closeHandleCalls == 1, L"Cleanup closes ICMP handle");
        AssertTrue(!monitor.IsAvailable(), L"Monitor not available after cleanup");
        AssertTrue(monitor.GetLatency() == -1, L"Latency reset to -1 after cleanup");
    }
}

} // namespace NetPulseTests
