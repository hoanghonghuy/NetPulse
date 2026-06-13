#ifndef NETPULSE_CONNECTION_MONITOR_H
#define NETPULSE_CONNECTION_MONITOR_H

#include "NetPulse/Common.h"
#include <windows.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

#ifdef _MSC_VER
#pragma comment(lib, "Iphlpapi.lib")
#endif

namespace NetPulse
{

// Represents a single network connection (named differently from PerAppMonitor's ConnectionInfo)
struct NetConnectionInfo
{
    DWORD processId = 0;
    std::wstring processName;
    std::wstring localAddress;
    USHORT localPort = 0;
    std::wstring remoteAddress;
    USHORT remotePort = 0;
    std::wstring protocol;  // "TCP" or "UDP"
    std::wstring state;     // "ESTABLISHED", "LISTENING", etc.
    time_t timestamp = 0;
    
    // For use in set comparisons
    bool operator<(const NetConnectionInfo& other) const
    {
        if (processId != other.processId) return processId < other.processId;
        if (localPort != other.localPort) return localPort < other.localPort;
        if (remotePort != other.remotePort) return remotePort < other.remotePort;
        if (remoteAddress != other.remoteAddress) return remoteAddress < other.remoteAddress;
        return protocol < other.protocol;
    }
    
    bool operator==(const NetConnectionInfo& other) const
    {
        return processId == other.processId &&
               localPort == other.localPort &&
               remotePort == other.remotePort &&
               remoteAddress == other.remoteAddress &&
               protocol == other.protocol;
    }
};

// Callback for new connection events
using ConnectionCallback = std::function<void(const NetConnectionInfo&)>;

class ConnectionMonitor
{
public:
    ConnectionMonitor();
    ~ConnectionMonitor();

    // Start monitoring connections
    bool Start();
    
    // Stop monitoring
    void Stop();
    
    // Check if running
    bool IsRunning() const { return m_running; }
    
    // Set callback for new connections
    void SetNewConnectionCallback(ConnectionCallback callback) { m_callback = callback; }
    
    // Get current active connections
    std::vector<NetConnectionInfo> GetActiveConnections() const;
    
    // Get connection history (from memory, not DB)
    std::vector<NetConnectionInfo> GetRecentConnections(size_t maxCount = 100) const;

private:
    // Worker thread
    void MonitorThreadProc();
    
    // Snapshot current connections
    std::set<NetConnectionInfo> SnapshotConnections();
    
    // Get TCP connections
    std::vector<NetConnectionInfo> GetTcpConnections();
    
    // Get UDP connections
    std::vector<NetConnectionInfo> GetUdpConnections();
    
    // Get process name by PID
    std::wstring GetProcessName(DWORD pid);
    
    // Convert IP address to string
    static std::wstring IpToString(DWORD ip);
    
    // Convert IPv6 address to string
    static std::wstring Ip6ToString(const UCHAR addr[16]);
    
    // Convert TCP state to string
    static std::wstring TcpStateToString(DWORD state);

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
    std::thread m_monitorThread;
    
    // Previous snapshot for diffing
    std::set<NetConnectionInfo> m_previousSnapshot;
    mutable std::mutex m_snapshotMutex;
    
    // Recent connections (in-memory buffer)
    std::vector<NetConnectionInfo> m_recentConnections;
    mutable std::mutex m_recentMutex;
    static const size_t MAX_RECENT_CONNECTIONS = 1000;
    
    // Process name cache
    std::map<DWORD, std::wstring> m_processNameCache;
    mutable std::mutex m_cacheMutex;
    static const size_t MAX_CACHE_SIZE = 500;
    
    // Callback for new connections
    ConnectionCallback m_callback;
    
    // Polling interval in milliseconds
    static const DWORD POLL_INTERVAL_MS = 1000;
};

} // namespace NetPulse

#endif // NETPULSE_CONNECTION_MONITOR_H
