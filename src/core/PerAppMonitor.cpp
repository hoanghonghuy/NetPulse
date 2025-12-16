#include "NetPulse/PerAppMonitor.h"
#include "NetPulse/EtwNetworkMonitor.h"
#include "NetPulse/Utils.h"
#include <iphlpapi.h>
#include <psapi.h>
#include <shellapi.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

namespace NetPulse
{

PerAppMonitor::PerAppMonitor()
    : m_initialized(false)
    , m_etwEnabled(false)
{
}

PerAppMonitor::~PerAppMonitor()
{
    Shutdown();
}

bool PerAppMonitor::Initialize()
{
    if (m_initialized)
    {
        return true;
    }
    
    m_initialized = true;
    LogDebug(L"PerAppMonitor::Initialize: Initialized successfully");
    return true;
}

void PerAppMonitor::Shutdown()
{
    // Stop ETW if running
    if (m_pEtwMonitor)
    {
        m_pEtwMonitor->Stop();
        m_pEtwMonitor.reset();
    }
    m_etwEnabled = false;
    
    // Clear caches and free icons
    for (auto& pair : m_processIconCache)
    {
        if (pair.second)
        {
            DestroyIcon(pair.second);
        }
    }
    m_processIconCache.clear();
    m_processNameCache.clear();
    m_processPathCache.clear();
    m_connections.clear();
    m_appUsage.clear();
    m_initialized = false;
}

bool PerAppMonitor::EnableEtw(bool enable)
{
    if (enable == m_etwEnabled)
    {
        return true;
    }
    
    if (enable)
    {
        if (!m_pEtwMonitor)
        {
            m_pEtwMonitor = std::make_unique<EtwNetworkMonitor>();
        }
        
        if (m_pEtwMonitor->Start())
        {
            m_etwEnabled = true;
            LogDebug(L"PerAppMonitor::EnableEtw: ETW monitoring enabled");
            return true;
        }
        else
        {
            LogError(L"PerAppMonitor::EnableEtw: Failed to start ETW");
            return false;
        }
    }
    else
    {
        if (m_pEtwMonitor)
        {
            m_pEtwMonitor->Stop();
        }
        m_etwEnabled = false;
        LogDebug(L"PerAppMonitor::EnableEtw: ETW monitoring disabled");
        return true;
    }
}

void PerAppMonitor::Refresh()
{
    m_connections.clear();
    m_appUsage.clear();
    
    EnumerateTcpConnections();
    EnumerateUdpConnections();
    AggregateByProcess();
}

void PerAppMonitor::EnumerateTcpConnections()
{
    // First call to get required buffer size
    DWORD size = 0;
    DWORD result = GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    
    if (result != ERROR_INSUFFICIENT_BUFFER)
    {
        return;
    }
    
    std::vector<BYTE> buffer(size);
    PMIB_TCPTABLE_OWNER_PID pTcpTable = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
    
    result = GetExtendedTcpTable(pTcpTable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (result != NO_ERROR)
    {
        return;
    }
    
    for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++)
    {
        const MIB_TCPROW_OWNER_PID& row = pTcpTable->table[i];
        
        // Skip listening sockets in some cases (optional)
        // if (row.dwState == MIB_TCP_STATE_LISTEN) continue;
        
        ConnectionInfo conn;
        conn.processId = row.dwOwningPid;
        conn.localAddress = IpAddressToString(row.dwLocalAddr);
        conn.localPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
        conn.remoteAddress = IpAddressToString(row.dwRemoteAddr);
        conn.remotePort = ntohs(static_cast<USHORT>(row.dwRemotePort));
        conn.protocol = L"TCP";
        conn.state = GetTcpStateString(row.dwState);
        
        m_connections.push_back(conn);
    }
}

void PerAppMonitor::EnumerateUdpConnections()
{
    // First call to get required buffer size
    DWORD size = 0;
    DWORD result = GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    
    if (result != ERROR_INSUFFICIENT_BUFFER)
    {
        return;
    }
    
    std::vector<BYTE> buffer(size);
    PMIB_UDPTABLE_OWNER_PID pUdpTable = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(buffer.data());
    
    result = GetExtendedUdpTable(pUdpTable, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (result != NO_ERROR)
    {
        return;
    }
    
    for (DWORD i = 0; i < pUdpTable->dwNumEntries; i++)
    {
        const MIB_UDPROW_OWNER_PID& row = pUdpTable->table[i];
        
        ConnectionInfo conn;
        conn.processId = row.dwOwningPid;
        conn.localAddress = IpAddressToString(row.dwLocalAddr);
        conn.localPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
        conn.remoteAddress = L"*";
        conn.remotePort = 0;
        conn.protocol = L"UDP";
        conn.state = L"";
        
        m_connections.push_back(conn);
    }
}

void PerAppMonitor::AggregateByProcess()
{
    std::map<DWORD, AppNetworkUsage> usageMap;
    
    for (const auto& conn : m_connections)
    {
        auto& usage = usageMap[conn.processId];
        usage.processId = conn.processId;
        
        if (conn.protocol == L"TCP")
        {
            usage.tcpConnections++;
        }
        else if (conn.protocol == L"UDP")
        {
            usage.udpConnections++;
        }
    }
    
    // Fill in process details and ETW traffic stats
    std::map<DWORD, ProcessTrafficStats> etwStats;
    if (m_etwEnabled && m_pEtwMonitor)
    {
        etwStats = m_pEtwMonitor->GetAllStats();
    }
    
    for (auto& pair : usageMap)
    {
        pair.second.processName = GetProcessName(pair.first);
        pair.second.processPath = GetProcessPath(pair.first);
        pair.second.processIcon = GetProcessIcon(pair.first);
        
        // Add ETW traffic stats if available
        auto etwIt = etwStats.find(pair.first);
        if (etwIt != etwStats.end())
        {
            pair.second.bytesSent = etwIt->second.bytesSent.load();
            pair.second.bytesReceived = etwIt->second.bytesReceived.load();
        }
        
        m_appUsage.push_back(pair.second);
    }
    
    // Also add processes that have ETW stats but no active connections
    if (m_etwEnabled && m_pEtwMonitor)
    {
        for (const auto& etwPair : etwStats)
        {
            if (usageMap.find(etwPair.first) == usageMap.end())
            {
                AppNetworkUsage usage;
                usage.processId = etwPair.first;
                usage.processName = GetProcessName(etwPair.first);
                usage.processPath = GetProcessPath(etwPair.first);
                usage.processIcon = GetProcessIcon(etwPair.first);
                usage.bytesSent = etwPair.second.bytesSent.load();
                usage.bytesReceived = etwPair.second.bytesReceived.load();
                m_appUsage.push_back(usage);
            }
        }
    }
    
    // Sort by total traffic (or connections if no traffic)
    std::sort(m_appUsage.begin(), m_appUsage.end(), 
        [](const AppNetworkUsage& a, const AppNetworkUsage& b) {
            uint64_t totalA = a.bytesSent + a.bytesReceived;
            uint64_t totalB = b.bytesSent + b.bytesReceived;
            if (totalA != totalB)
            {
                return totalA > totalB;
            }
            return (a.tcpConnections + a.udpConnections) > (b.tcpConnections + b.udpConnections);
        });
}

std::wstring PerAppMonitor::IpAddressToString(DWORD ip) const
{
    if (ip == 0)
    {
        return L"0.0.0.0";
    }
    
    IN_ADDR addr;
    addr.S_un.S_addr = ip;
    
    wchar_t buffer[16];
    swprintf_s(buffer, L"%u.%u.%u.%u",
        addr.S_un.S_un_b.s_b1,
        addr.S_un.S_un_b.s_b2,
        addr.S_un.S_un_b.s_b3,
        addr.S_un.S_un_b.s_b4);
    
    return buffer;
}

std::wstring PerAppMonitor::Ip6AddressToString(const UCHAR* ip) const
{
    wchar_t buffer[64];
    swprintf_s(buffer, L"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
        ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7],
        ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14], ip[15]);
    return buffer;
}

std::wstring PerAppMonitor::GetTcpStateString(DWORD state) const
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

std::wstring PerAppMonitor::GetProcessName(DWORD pid) const
{
    // Check cache first
    auto it = m_processNameCache.find(pid);
    if (it != m_processNameCache.end())
    {
        return it->second;
    }
    
    std::wstring name = L"<unknown>";
    
    if (pid == 0)
    {
        name = L"System Idle Process";
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
                // Extract filename from path
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
    
    m_processNameCache[pid] = name;
    return name;
}

std::wstring PerAppMonitor::GetProcessPath(DWORD pid) const
{
    // Check cache first
    auto it = m_processPathCache.find(pid);
    if (it != m_processPathCache.end())
    {
        return it->second;
    }
    
    std::wstring path;
    
    if (pid == 0 || pid == 4)
    {
        path = L"";
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
                path = buffer;
            }
            CloseHandle(hProcess);
        }
    }
    
    m_processPathCache[pid] = path;
    return path;
}

HICON PerAppMonitor::GetProcessIcon(DWORD pid) const
{
    // Check cache first
    auto it = m_processIconCache.find(pid);
    if (it != m_processIconCache.end())
    {
        return it->second;
    }
    
    HICON icon = nullptr;
    std::wstring path = GetProcessPath(pid);
    
    if (!path.empty())
    {
        // Extract icon from executable
        icon = ExtractIconW(GetModuleHandle(nullptr), path.c_str(), 0);
        if (icon == reinterpret_cast<HICON>(1) || icon == nullptr)
        {
            icon = nullptr;
        }
    }
    
    m_processIconCache[pid] = icon;
    return icon;
}

} // namespace NetPulse
