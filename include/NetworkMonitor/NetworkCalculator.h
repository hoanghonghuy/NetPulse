#ifndef NETWORK_MONITOR_NETWORKCALCULATOR_H
#define NETWORK_MONITOR_NETWORKCALCULATOR_H

#include "NetworkMonitor/Common.h"
#include <vector>

namespace NetworkMonitor
{

class NetworkCalculator
{
public:
    NetworkCalculator();
    ~NetworkCalculator();

    /**
     * Update network statistics with new data
     * @param stats Current network stats to update
     * @param currentBytesIn Current total bytes received
     * @param currentBytesOut Current total bytes sent
     * @return true if update successful, false otherwise
     */
    bool UpdateStats(NetworkStats& stats, ULONG64 currentBytesIn, ULONG64 currentBytesOut);

    /**
     * Calculate aggregate statistics from multiple interfaces
     * @param statsList List of network stats from all interfaces
     * @return Aggregated network stats
     */
    NetworkStats CalculateAggregate(const std::vector<NetworkStats>& statsList);

    /**
     * Reset statistics for a network interface
     * @param stats Network stats to reset
     */
    void ResetStats(NetworkStats& stats);

private:
    /**
     * Calculate speed from byte delta and time interval
     * @param byteDelta Number of bytes transferred
     * @param timeIntervalSeconds Time interval in seconds
     * @return Speed in bytes per second
     */
    double CalculateSpeed(ULONG64 byteDelta, double timeIntervalSeconds);
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_NETWORKCALCULATOR_H
