#include "NetPulse/NetworkMonitor.h"
#include "NetPulse/Utils.h"

namespace NetPulse
{

NetworkMonitorClass::NetworkMonitorClass()
    : m_isRunning(false)
    , m_initialized(false)
{
}

NetworkMonitorClass::~NetworkMonitorClass()
{
    Stop();
}

bool NetworkMonitorClass::Start()
{
    if (m_isRunning)
    {
        return true;
    }

    // Initialize by querying interfaces once
    if (!QueryNetworkInterfaces())
    {
        LogError(L"NetworkMonitorClass::Start: initial QueryNetworkInterfaces failed");
        return false;
    }

    m_isRunning = true;
    m_initialized = true;
    return true;
}

void NetworkMonitorClass::Stop()
{
    m_isRunning = false;
}

std::vector<NetworkStats> NetworkMonitorClass::GetAllStats()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<NetworkStats> result;
    result.reserve(m_statsMap.size());

    for (const auto& pair : m_statsMap)
    {
        if (pair.second.isActive)
        {
            result.push_back(pair.second);
        }
    }

    return result;
}

NetworkStats NetworkMonitorClass::GetAggregatedStats()
{
    std::vector<NetworkStats> allStats = GetAllStats();
    return m_calculator.CalculateAggregate(allStats);
}

bool NetworkMonitorClass::GetInterfaceStats(const std::wstring& interfaceName, NetworkStats& stats)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_statsMap.find(interfaceName);
    if (it != m_statsMap.end())
    {
        stats = it->second;
        return true;
    }

    return false;
}

bool NetworkMonitorClass::Update()
{
    if (!m_isRunning)
    {
        return false;
    }

    return QueryNetworkInterfaces();
}

bool NetworkMonitorClass::QueryNetworkInterfaces()
{
    PMIB_IF_TABLE2 pIfTable = nullptr;

    // Get interface table
    DWORD result = GetIfTable2(&pIfTable);
    if (result != NO_ERROR)
    {
        LogError(L"NetworkMonitorClass::QueryNetworkInterfaces: GetIfTable2 failed, error=" + std::to_wstring(result));
        return false;
    }

    try
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Mark all existing entries as potentially inactive
        for (auto& pair : m_statsMap)
        {
            pair.second.isActive = false;
        }

        // Process each interface
        for (ULONG i = 0; i < pIfTable->NumEntries; i++)
        {
            MIB_IF_ROW2* pIfRow = &pIfTable->Table[i];

            // Skip interfaces we don't want to monitor
            if (!ShouldMonitorInterface(pIfRow))
            {
                continue;
            }

            // Get interface name
            std::wstring interfaceName = pIfRow->Alias;

            // Get or create stats entry
            NetworkStats& stats = m_statsMap[interfaceName];

            // Update basic info
            if (stats.interfaceName.empty())
            {
                stats.interfaceName = interfaceName;
                stats.interfaceDesc = pIfRow->Description;
            }

            // Update statistics using calculator
            m_calculator.UpdateStats(stats, pIfRow->InOctets, pIfRow->OutOctets);

            // Mark whether this is a physical hardware interface (for aggregate dedup)
            stats.isPhysicalHardware = IsPhysicalHardwareInterface(pIfRow);
        }

        // Remove inactive interfaces (disconnected)
        for (auto it = m_statsMap.begin(); it != m_statsMap.end();)
        {
            if (!it->second.isActive)
            {
                it = m_statsMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    catch (...)
    {
        FreeMibTable(pIfTable);
        LogError(L"NetworkMonitorClass::QueryNetworkInterfaces: exception while processing interface table");
        return false;
    }

    FreeMibTable(pIfTable);
    return true;
}

bool NetworkMonitorClass::ShouldMonitorInterface(const MIB_IF_ROW2* ifRow)
{
    if (ifRow == nullptr)
    {
        return false;
    }

    // Only monitor interfaces that are UP
    if (ifRow->OperStatus != IfOperStatusUp)
    {
        return false;
    }

    // Skip filter/intermediate driver adapters - not real traffic sources
    if (ifRow->InterfaceAndOperStatusFlags.FilterInterface)
    {
        return false;
    }

    // Skip loopback interfaces
    if (ifRow->Type == IF_TYPE_SOFTWARE_LOOPBACK)
    {
        return false;
    }

    // Only monitor common interface types
    // Ethernet, Wi-Fi, etc.
    if (ifRow->Type != IF_TYPE_ETHERNET_CSMACD &&
        ifRow->Type != IF_TYPE_IEEE80211 &&
        ifRow->Type != IF_TYPE_PPP)
    {
        return false;
    }

    return true;
}

bool NetworkMonitorClass::IsPhysicalHardwareInterface(const MIB_IF_ROW2* ifRow)
{
    if (ifRow == nullptr)
    {
        return false;
    }
    // Must be a real hardware adapter with a physical connector
    return ifRow->InterfaceAndOperStatusFlags.HardwareInterface &&
           ifRow->InterfaceAndOperStatusFlags.ConnectorPresent;
}

} // namespace NetPulse
