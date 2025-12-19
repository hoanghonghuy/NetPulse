#ifndef NETWORK_MONITOR_NETWORKMONITOR_H
#define NETWORK_MONITOR_NETWORKMONITOR_H

#include "NetPulse/Common.h"
#include "NetPulse/NetworkCalculator.h"
#include "NetPulse/Interfaces/INetworkStatsProvider.h"
#include <windows.h>
#include <netioapi.h>
#include <iphlpapi.h>
#include <vector>
#include <map>
#include <mutex>

#pragma comment(lib, "Iphlpapi.lib")

namespace NetPulse
{

class NetworkMonitorClass : public INetworkStatsProvider
{
public:
    NetworkMonitorClass();
    ~NetworkMonitorClass() override;

    /**
     * Start monitoring network interfaces
     * @return true if started successfully, false otherwise
     */
    bool Start();

    /**
     * Stop monitoring network interfaces
     */
    void Stop();

    /**
     * Check if monitoring is running
     * @return true if running, false otherwise
     */
    bool IsRunning() const override { return m_isRunning; }

    /**
     * Get statistics for all active network interfaces
     * @return Vector of network statistics
     */
    std::vector<NetworkStats> GetAllStats() override;

    /**
     * Get aggregated statistics from all interfaces
     * @return Aggregated network stats
     */
    NetworkStats GetAggregatedStats() override;

    /**
     * Get statistics for a specific interface
     * @param interfaceName Name of the interface
     * @param stats Output network stats
     * @return true if interface found, false otherwise
     */
    bool GetInterfaceStats(const std::wstring& interfaceName, NetworkStats& stats) override;

    /**
     * Update network statistics (call periodically)
     * @return true if update successful, false otherwise
     */
    bool Update() override;

private:
    /**
     * Query network interfaces and collect data
     * @return true if successful, false otherwise
     */
    bool QueryNetworkInterfaces();

    /**
     * Check if interface should be monitored
     * @param ifRow Interface row data
     * @return true if should monitor, false otherwise
     */
    static bool ShouldMonitorInterface(const MIB_IF_ROW2* ifRow);

private:
    NetworkCalculator m_calculator;                    // Calculator for network statistics
    std::map<std::wstring, NetworkStats> m_statsMap;   // Map of interface name to stats
    std::mutex m_mutex;                                // Mutex for thread-safe access
    bool m_isRunning;                                  // Is monitoring running?
    bool m_initialized;                                // Is initialized?
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_NETWORKMONITOR_H
