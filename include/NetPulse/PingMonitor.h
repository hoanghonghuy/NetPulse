#ifndef NETWORK_MONITOR_PING_MONITOR_H
#define NETWORK_MONITOR_PING_MONITOR_H

#include "NetPulse/Common.h"
#include "NetPulse/Interfaces/IPingProvider.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <string>

#pragma comment(lib, "iphlpapi.lib")

namespace NetPulse
{

class PingMonitor : public IPingProvider
{
public:
    PingMonitor();
    ~PingMonitor() override;

    // Initialize with target IP/domain (default: 8.8.8.8)
    bool Initialize(const std::wstring& target = L"8.8.8.8");
    void Cleanup();

    // Perform ping and update latency (call from timer)
    void Update() override;

    // Get last measured latency in milliseconds (-1 if failed/timeout)
    int GetLatency() const override { return m_latency; }

    // Check if ping is available
    bool IsAvailable() const override { return m_initialized; }

    // Set new target (will resolve on next Update)
    void SetTarget(const std::wstring& target) override;

private:
    HANDLE m_hIcmp;
    bool m_initialized;
    int m_latency;  // -1 = unavailable/timeout
    std::wstring m_target;
    ULONG m_targetIP;

    // Resolve hostname/IP to IP address
    bool ResolveTarget();

    static constexpr DWORD TIMEOUT_MS = 1000;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_PING_MONITOR_H

