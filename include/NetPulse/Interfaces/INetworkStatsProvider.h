#ifndef NETWORK_MONITOR_INETWORK_STATS_PROVIDER_H
#define NETWORK_MONITOR_INETWORK_STATS_PROVIDER_H

#include "NetPulse/Common.h"
#include <vector>
#include <string>

namespace NetPulse
{

/**
 * Interface for network statistics provider
 * Allows dependency injection and testing with mock implementations
 */
class INetworkStatsProvider
{
public:
    virtual ~INetworkStatsProvider() = default;

    /**
     * Get statistics for all active network interfaces
     * @return Vector of network statistics
     */
    virtual std::vector<NetworkStats> GetAllStats() = 0;

    /**
     * Get aggregated statistics from all interfaces
     * @return Aggregated network stats
     */
    virtual NetworkStats GetAggregatedStats() = 0;

    /**
     * Get statistics for a specific interface
     * @param interfaceName Name of the interface
     * @param stats Output network stats
     * @return true if interface found, false otherwise
     */
    virtual bool GetInterfaceStats(const std::wstring& interfaceName, NetworkStats& stats) = 0;

    /**
     * Update network statistics (call periodically)
     * @return true if update successful, false otherwise
     */
    virtual bool Update() = 0;

    /**
     * Check if monitoring is running
     * @return true if running, false otherwise
     */
    virtual bool IsRunning() const = 0;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_INETWORK_STATS_PROVIDER_H
