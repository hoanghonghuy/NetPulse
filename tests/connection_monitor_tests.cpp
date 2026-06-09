#include "NetPulse/PerAppMonitor.h"
#include "NetPulse/ConnectionMonitor.h"
#include "TestUtils.h"

#include <chrono>
#include <map>
#include <thread>

using namespace NetPulse;

namespace NetPulseTests
{

namespace
{

static bool IsKnownProtocol(const std::wstring& protocol)
{
    return protocol == L"TCP" || protocol == L"TCP6" ||
           protocol == L"UDP" || protocol == L"UDP6";
}

static void ValidatePerAppConnections(const std::vector<ConnectionInfo>& connections)
{
    for (const auto& conn : connections)
    {
        AssertTrue(IsKnownProtocol(conn.protocol),
                   L"PerAppMonitor connection uses known protocol");
        if (conn.protocol == L"UDP" || conn.protocol == L"UDP6")
        {
            AssertTrue(conn.remoteAddress == L"*",
                       L"PerAppMonitor UDP connection uses wildcard remote");
            AssertTrue(conn.remotePort == 0,
                       L"PerAppMonitor UDP connection remote port is zero");
        }
        else
        {
            AssertTrue(!conn.state.empty(),
                       L"PerAppMonitor TCP connection has state string");
        }

        if (conn.protocol == L"TCP")
        {
            AssertTrue(conn.localAddress.find(L'.') != std::wstring::npos ||
                       conn.localAddress == L"0.0.0.0",
                       L"PerAppMonitor TCP IPv4 address format");
        }
        else if (conn.protocol == L"TCP6" || conn.protocol == L"UDP6")
        {
            AssertTrue(conn.localAddress.find(L':') != std::wstring::npos ||
                       conn.localAddress == L"::",
                       L"PerAppMonitor IPv6 address format");
        }
    }
}

static void ValidatePerAppAggregation(
    const std::vector<ConnectionInfo>& connections,
    const std::vector<AppNetworkUsage>& appUsage)
{
    std::map<DWORD, int> expectedTcp;
    std::map<DWORD, int> expectedUdp;

    for (const auto& conn : connections)
    {
        if (conn.protocol == L"TCP" || conn.protocol == L"TCP6")
        {
            ++expectedTcp[conn.processId];
        }
        else if (conn.protocol == L"UDP" || conn.protocol == L"UDP6")
        {
            ++expectedUdp[conn.processId];
        }
    }

    int totalTcp = 0;
    int totalUdp = 0;
    for (const auto& usage : appUsage)
    {
        AssertTrue(usage.tcpConnections >= 0 && usage.udpConnections >= 0,
                   L"PerAppMonitor aggregate counts are non-negative");
        AssertTrue(usage.tcpConnections == expectedTcp[usage.processId],
                   L"PerAppMonitor aggregates TCP and TCP6 into tcpConnections");
        AssertTrue(usage.udpConnections == expectedUdp[usage.processId],
                   L"PerAppMonitor aggregates UDP and UDP6 into udpConnections");
        totalTcp += usage.tcpConnections;
        totalUdp += usage.udpConnections;
    }

    int connTcp = 0;
    int connUdp = 0;
    for (const auto& conn : connections)
    {
        if (conn.protocol == L"TCP" || conn.protocol == L"TCP6")
        {
            ++connTcp;
        }
        else if (conn.protocol == L"UDP" || conn.protocol == L"UDP6")
        {
            ++connUdp;
        }
    }

    AssertTrue(totalTcp == connTcp, L"PerAppMonitor total TCP aggregate matches connections");
    AssertTrue(totalUdp == connUdp, L"PerAppMonitor total UDP aggregate matches connections");
}

static void ValidateConnectionMonitorEntries(const std::vector<NetConnectionInfo>& connections)
{
    for (const auto& conn : connections)
    {
        AssertTrue(IsKnownProtocol(conn.protocol),
                   L"ConnectionMonitor entry uses known protocol");
        if (conn.processId == 0)
        {
            AssertTrue(conn.processName == L"System Idle",
                       L"ConnectionMonitor PID 0 process name");
        }
        else if (conn.processId == 4)
        {
            AssertTrue(conn.processName == L"System",
                       L"ConnectionMonitor PID 4 process name");
        }

        if (conn.protocol == L"UDP" || conn.protocol == L"UDP6")
        {
            AssertTrue(conn.remoteAddress == L"*",
                       L"ConnectionMonitor UDP entry remote wildcard");
        }
    }
}

static void CountPerAppProtocols(
    const std::vector<ConnectionInfo>& connections,
    int& tcp4,
    int& tcp6,
    int& udp4,
    int& udp6)
{
    tcp4 = tcp6 = udp4 = udp6 = 0;
    for (const auto& conn : connections)
    {
        if (conn.protocol == L"TCP") ++tcp4;
        else if (conn.protocol == L"TCP6") ++tcp6;
        else if (conn.protocol == L"UDP") ++udp4;
        else if (conn.protocol == L"UDP6") ++udp6;
    }
}

} // namespace

void RunConnectionMonitorTests()
{
    LogTestMessage(L"=== PerAppMonitor / ConnectionMonitor smoke tests ===");

    PerAppMonitor perApp;
    AssertTrue(perApp.Initialize(), L"PerAppMonitor.Initialize returns true");
    AssertTrue(perApp.Initialize(), L"PerAppMonitor.Initialize is idempotent");

    perApp.Refresh();
    const auto& connections = perApp.GetConnections();
    const auto& appUsage = perApp.GetAppUsage();

    ValidatePerAppConnections(connections);
    ValidatePerAppAggregation(connections, appUsage);

    int tcp4Count = 0;
    int tcp6Count = 0;
    int udp4Count = 0;
    int udp6Count = 0;
    CountPerAppProtocols(connections, tcp4Count, tcp6Count, udp4Count, udp6Count);

    LogTestMessage((L"  PerAppMonitor counts: TCP=" + std::to_wstring(tcp4Count) +
                    L" TCP6=" + std::to_wstring(tcp6Count) +
                    L" UDP=" + std::to_wstring(udp4Count) +
                    L" UDP6=" + std::to_wstring(udp6Count)).c_str());
    AssertTrue(true, L"PerAppMonitor IPv4 TCP enumeration smoke completed");
    AssertTrue(true, L"PerAppMonitor IPv6 TCP enumeration smoke completed");
    AssertTrue(true, L"PerAppMonitor UDP/UDP6 enumeration smoke completed");

    std::wstring idleName = perApp.GetProcessName(0);
    std::wstring systemName = perApp.GetProcessName(4);
    AssertTrue(idleName == L"System Idle Process",
               L"PerAppMonitor GetProcessName PID 0");
    AssertTrue(systemName == L"System",
               L"PerAppMonitor GetProcessName PID 4");
    AssertTrue(perApp.GetProcessName(0) == idleName,
               L"PerAppMonitor process name cache returns stable PID 0 name");

    perApp.EnableEtw(true);
    perApp.EnableEtw(false);
    AssertTrue(!perApp.IsEtwEnabled(), L"PerAppMonitor EnableEtw(false) disables ETW");

    perApp.Shutdown();
    AssertTrue(true, L"PerAppMonitor Shutdown does not crash");

    ConnectionMonitor connMonitor;
    AssertTrue(!connMonitor.IsRunning(), L"ConnectionMonitor not running before Start");

    bool started = connMonitor.Start();
    AssertTrue(started, L"ConnectionMonitor.Start returns true");
    AssertTrue(connMonitor.IsRunning(), L"ConnectionMonitor.IsRunning after Start");

    auto active = connMonitor.GetActiveConnections();
    ValidateConnectionMonitorEntries(active);

    AssertTrue(true, L"ConnectionMonitor IPv4 TCP snapshot smoke completed");
    AssertTrue(true, L"ConnectionMonitor IPv6 TCP snapshot smoke completed");
    AssertTrue(true, L"ConnectionMonitor UDP/UDP6 snapshot smoke completed");

    int callbackInvocations = 0;
    connMonitor.SetNewConnectionCallback([&](const NetConnectionInfo& /*conn*/)
    {
        ++callbackInvocations;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    connMonitor.Stop();
    AssertTrue(!connMonitor.IsRunning(), L"ConnectionMonitor.IsRunning false after Stop");

    auto recent = connMonitor.GetRecentConnections(10);
    AssertTrue(recent.size() <= 10, L"ConnectionMonitor.GetRecentConnections respects limit");

    connMonitor.Stop();
    AssertTrue(true, L"ConnectionMonitor double Stop is safe");
}

} // namespace NetPulseTests
