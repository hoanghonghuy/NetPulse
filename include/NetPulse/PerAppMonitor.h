#ifndef NETWORK_MONITOR_PERAPP_MONITOR_H
#define NETWORK_MONITOR_PERAPP_MONITOR_H

#include "NetPulse/Common.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace NetPulse
{

// Forward declaration
class EtwNetworkMonitor;

/**
 * Information about a single network connection
 */
struct ConnectionInfo
{
    DWORD processId;
    std::wstring localAddress;
    USHORT localPort;
    std::wstring remoteAddress;
    USHORT remotePort;
    std::wstring protocol; // "TCP" or "UDP"
    std::wstring state;    // Connection state for TCP
};

/**
 * Aggregated network usage per application
 */
struct AppNetworkUsage
{
    DWORD processId;
    std::wstring processName;
    std::wstring processPath;
    HICON processIcon;
    int tcpConnections;
    int udpConnections;
    
    // For future ETW integration
    uint64_t bytesSent;
    uint64_t bytesReceived;
    
    AppNetworkUsage()
        : processId(0)
        , processIcon(nullptr)
        , tcpConnections(0)
        , udpConnections(0)
        , bytesSent(0)
        , bytesReceived(0)
    {}
};

/**
 * PerAppMonitor - Monitors network usage per application
 * 
 * Phase 3a: Uses IP Helper API (GetExtendedTcpTable/GetExtendedUdpTable)
 * Phase 3b: Will add ETW for real-time traffic monitoring
 */
class PerAppMonitor
{
public:
    PerAppMonitor();
    ~PerAppMonitor();

    /**
     * Initialize the monitor
     */
    bool Initialize();

    /**
     * Shutdown and cleanup
     */
    void Shutdown();

    /**
     * Refresh the list of connections and aggregate by process
     */
    void Refresh();

    /**
     * Enable/disable ETW traffic monitoring
     * @return true if ETW was started successfully
     */
    bool EnableEtw(bool enable);

    /**
     * Check if ETW monitoring is enabled
     */
    bool IsEtwEnabled() const { return m_etwEnabled; }

    /**
     * Get all connections
     */
    const std::vector<ConnectionInfo>& GetConnections() const { return m_connections; }

    /**
     * Get aggregated usage per app
     */
    const std::vector<AppNetworkUsage>& GetAppUsage() const { return m_appUsage; }

    /**
     * Get process name by PID
     */
    std::wstring GetProcessName(DWORD pid) const;

    /**
     * Get process path by PID
     */
    std::wstring GetProcessPath(DWORD pid) const;

    /**
     * Get process icon by PID
     */
    HICON GetProcessIcon(DWORD pid) const;

private:
    void EnumerateTcpConnections();
    void EnumerateUdpConnections();
    void AggregateByProcess();
    static std::wstring IpAddressToString(DWORD ip);
    static std::wstring GetTcpStateString(DWORD state);

    bool m_initialized;
    bool m_etwEnabled;
    std::unique_ptr<EtwNetworkMonitor> m_pEtwMonitor;
    std::vector<ConnectionInfo> m_connections;
    std::vector<AppNetworkUsage> m_appUsage;
    
    // Cache for process names/paths
    mutable std::map<DWORD, std::wstring> m_processNameCache;
    mutable std::map<DWORD, std::wstring> m_processPathCache;
    mutable std::map<DWORD, HICON> m_processIconCache;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_PERAPP_MONITOR_H
