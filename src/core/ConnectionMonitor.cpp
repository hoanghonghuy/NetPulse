#include "NetPulse/ConnectionMonitor.h"
#include "NetPulse/Utils.h"
#include <ws2tcpip.h>
#include <psapi.h>

#pragma comment(lib, "Ws2_32.lib")

namespace NetPulse
{

ConnectionMonitor::ConnectionMonitor()
{
}

ConnectionMonitor::~ConnectionMonitor()
{
    Stop();
}

bool ConnectionMonitor::Start()
{
    if (m_running)
    {
        return true;
    }
    
    m_stopRequested = false;
    m_running = true;
    
    // Take initial snapshot
    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_previousSnapshot = SnapshotConnections();
    }
    
    // Start monitor thread
    m_monitorThread = std::thread(&ConnectionMonitor::MonitorThreadProc, this);
    
    LogDebug(L"ConnectionMonitor::Start: Connection monitoring started");
    return true;
}

void ConnectionMonitor::Stop()
{
    if (!m_running)
    {
        return;
    }
    
    m_stopRequested = true;
    
    if (m_monitorThread.joinable())
    {
        m_monitorThread.join();
    }
    
    m_running = false;
    LogDebug(L"ConnectionMonitor::Stop: Connection monitoring stopped");
}

std::vector<NetConnectionInfo> ConnectionMonitor::GetActiveConnections() const
{
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    return std::vector<NetConnectionInfo>(m_previousSnapshot.begin(), m_previousSnapshot.end());
}

std::vector<NetConnectionInfo> ConnectionMonitor::GetRecentConnections(size_t maxCount) const
{
    std::lock_guard<std::mutex> lock(m_recentMutex);
    
    size_t count = (std::min)(maxCount, m_recentConnections.size());
    if (count == 0)
    {
        return {};
    }
    
    // Return most recent first
    auto startIt = m_recentConnections.end() - static_cast<ptrdiff_t>(count);
    return std::vector<NetConnectionInfo>(startIt, m_recentConnections.end());
}

void ConnectionMonitor::MonitorThreadProc()
{
    while (!m_stopRequested)
    {
        // Take new snapshot
        auto currentSnapshot = SnapshotConnections();
        
        // Find new connections (in current but not in previous)
        std::vector<NetConnectionInfo> newConnections;
        {
            std::lock_guard<std::mutex> lock(m_snapshotMutex);
            for (const auto& conn : currentSnapshot)
            {
                if (m_previousSnapshot.find(conn) == m_previousSnapshot.end())
                {
                    newConnections.push_back(conn);
                }
            }
            m_previousSnapshot = std::move(currentSnapshot);
        }
        
        // Process new connections
        if (!newConnections.empty())
        {
            std::lock_guard<std::mutex> lock(m_recentMutex);
            for (auto& conn : newConnections)
            {
                conn.timestamp = time(nullptr);
                
                // Add to recent buffer
                m_recentConnections.push_back(conn);
                
                // Trim if too large
                if (m_recentConnections.size() > MAX_RECENT_CONNECTIONS)
                {
                    m_recentConnections.erase(m_recentConnections.begin());
                }
                
                // Invoke callback
                if (m_callback)
                {
                    m_callback(conn);
                }
            }
        }
        
        // Sleep before next poll
        Sleep(POLL_INTERVAL_MS);
    }
}

std::set<NetConnectionInfo> ConnectionMonitor::SnapshotConnections()
{
    std::set<NetConnectionInfo> result;
    
    // Get TCP connections
    auto tcpConns = GetTcpConnections();
    for (auto& conn : tcpConns)
    {
        result.insert(std::move(conn));
    }
    
    // Get UDP connections
    auto udpConns = GetUdpConnections();
    for (auto& conn : udpConns)
    {
        result.insert(std::move(conn));
    }
    
    return result;
}

std::vector<NetConnectionInfo> ConnectionMonitor::GetTcpConnections()
{
    std::vector<NetConnectionInfo> result;
    
    // Get IPv4 TCP table
    DWORD size = 0;
    GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    
    if (size > 0)
    {
        std::vector<BYTE> buffer(size);
        if (GetExtendedTcpTable(buffer.data(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
        {
            auto* table = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
            for (DWORD i = 0; i < table->dwNumEntries; i++)
            {
                const auto& row = table->table[i];
                
                // Skip listening sockets for now (focus on established)
                if (row.dwState == MIB_TCP_STATE_LISTEN)
                {
                    continue;
                }
                
                NetConnectionInfo info;
                info.processId = row.dwOwningPid;
                info.processName = GetProcessName(row.dwOwningPid);
                info.localAddress = IpToString(row.dwLocalAddr);
                info.localPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
                info.remoteAddress = IpToString(row.dwRemoteAddr);
                info.remotePort = ntohs(static_cast<USHORT>(row.dwRemotePort));
                info.protocol = L"TCP";
                info.state = TcpStateToString(row.dwState);
                
                result.push_back(std::move(info));
            }
        }
    }
    
    return result;
}

std::vector<NetConnectionInfo> ConnectionMonitor::GetUdpConnections()
{
    std::vector<NetConnectionInfo> result;
    
    // Get IPv4 UDP table
    DWORD size = 0;
    GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    
    if (size > 0)
    {
        std::vector<BYTE> buffer(size);
        if (GetExtendedUdpTable(buffer.data(), &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
        {
            auto* table = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(buffer.data());
            for (DWORD i = 0; i < table->dwNumEntries; i++)
            {
                const auto& row = table->table[i];
                
                NetConnectionInfo info;
                info.processId = row.dwOwningPid;
                info.processName = GetProcessName(row.dwOwningPid);
                info.localAddress = IpToString(row.dwLocalAddr);
                info.localPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
                info.remoteAddress = L"*";
                info.remotePort = 0;
                info.protocol = L"UDP";
                info.state = L"";
                
                result.push_back(std::move(info));
            }
        }
    }
    
    return result;
}

std::wstring ConnectionMonitor::GetProcessName(DWORD pid)
{
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_processNameCache.find(pid);
        if (it != m_processNameCache.end())
        {
            return it->second;
        }
    }
    
    std::wstring name = L"<unknown>";
    
    if (pid == 0)
    {
        name = L"System Idle";
    }
    else if (pid == 4)
    {
        name = L"System";
    }
    else
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess)
        {
            wchar_t buffer[MAX_PATH] = {0};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size))
            {
                std::wstring path = buffer;
                size_t pos = path.find_last_of(L"\\/");
                if (pos != std::wstring::npos)
                {
                    name = path.substr(pos + 1);
                }
                else
                {
                    name = path;
                }
            }
            CloseHandle(hProcess);
        }
    }
    
    // Cache it
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_processNameCache[pid] = name;
    }
    
    return name;
}

std::wstring ConnectionMonitor::IpToString(DWORD ip)
{
    wchar_t buffer[INET_ADDRSTRLEN] = {0};
    IN_ADDR addr;
    addr.S_un.S_addr = ip;
    InetNtopW(AF_INET, &addr, buffer, INET_ADDRSTRLEN);
    return buffer;
}

std::wstring ConnectionMonitor::TcpStateToString(DWORD state)
{
    switch (state)
    {
        case MIB_TCP_STATE_CLOSED:      return L"CLOSED";
        case MIB_TCP_STATE_LISTEN:      return L"LISTEN";
        case MIB_TCP_STATE_SYN_SENT:    return L"SYN_SENT";
        case MIB_TCP_STATE_SYN_RCVD:    return L"SYN_RCVD";
        case MIB_TCP_STATE_ESTAB:       return L"ESTABLISHED";
        case MIB_TCP_STATE_FIN_WAIT1:   return L"FIN_WAIT1";
        case MIB_TCP_STATE_FIN_WAIT2:   return L"FIN_WAIT2";
        case MIB_TCP_STATE_CLOSE_WAIT:  return L"CLOSE_WAIT";
        case MIB_TCP_STATE_CLOSING:     return L"CLOSING";
        case MIB_TCP_STATE_LAST_ACK:    return L"LAST_ACK";
        case MIB_TCP_STATE_TIME_WAIT:   return L"TIME_WAIT";
        case MIB_TCP_STATE_DELETE_TCB:  return L"DELETE_TCB";
        default:                        return L"UNKNOWN";
    }
}

} // namespace NetPulse
