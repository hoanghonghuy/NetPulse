#ifndef NETWORK_MONITOR_DATA_USAGE_MONITOR_H
#define NETWORK_MONITOR_DATA_USAGE_MONITOR_H

#include "NetPulse/Common.h"
#include <vector>
#include <set>

namespace NetPulse
{

/**
 * DataUsageMonitor - Monitors data usage against configurable quotas
 * 
 * This class tracks cumulative network usage and triggers alerts
 * when usage exceeds configured threshold percentages.
 */
class DataUsageMonitor
{
public:
    DataUsageMonitor();
    ~DataUsageMonitor();

    /**
     * Set the monthly data quota in bytes
     * @param bytesPerMonth Zero to disable monitoring
     */
    void SetQuota(uint64_t bytesPerMonth);

    /**
     * Get the current quota setting
     */
    uint64_t GetQuota() const { return m_quotaBytes; }

    /**
     * Set alert thresholds (percentages of quota)
     * @param thresholds List of percentages, e.g., {80, 100}
     */
    void SetAlertThresholds(const std::vector<int>& thresholds);

    /**
     * Reset tracked alerts (call at start of new billing cycle)
     */
    void ResetAlerts();

    /**
     * Update with current month's usage
     * @param currentUsageBytes Total bytes used this month
     * @return True if a new threshold was crossed
     */
    bool Update(uint64_t currentUsageBytes);

    /**
     * Check if an alert should be shown
     * @param outPercentage The threshold percentage that was crossed
     * @return True if alert should be shown
     */
    bool ShouldAlert(int& outPercentage);

    /**
     * Get current usage as a percentage of quota
     */
    int GetUsagePercentage() const;

    /**
     * Check if monitoring is enabled (quota > 0)
     */
    bool IsEnabled() const { return m_quotaBytes > 0; }

private:
    uint64_t m_quotaBytes;
    uint64_t m_currentUsageBytes;
    std::vector<int> m_thresholds;
    std::set<int> m_alertedThresholds;  // Thresholds already alerted this cycle
    int m_pendingAlertPercentage;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_DATA_USAGE_MONITOR_H
