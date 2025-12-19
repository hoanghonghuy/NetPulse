#include "NetPulse/DataUsageMonitor.h"
#include "NetPulse/Utils.h"
#include <algorithm>

namespace NetPulse
{

DataUsageMonitor::DataUsageMonitor()
    : m_quotaBytes(0)
    , m_currentUsageBytes(0)
    , m_pendingAlertPercentage(-1)
    , m_thresholds({80, 100})  // Default thresholds: 80% and 100%
{
}

DataUsageMonitor::~DataUsageMonitor()
{
}

void DataUsageMonitor::SetQuota(uint64_t bytesPerMonth)
{
    m_quotaBytes = bytesPerMonth;
    LogDebug(L"DataUsageMonitor::SetQuota: " + std::to_wstring(bytesPerMonth) + L" bytes");
}

void DataUsageMonitor::SetAlertThresholds(const std::vector<int>& thresholds)
{
    m_thresholds = thresholds;
    // Sort thresholds in ascending order
    std::sort(m_thresholds.begin(), m_thresholds.end());
}

void DataUsageMonitor::ResetAlerts()
{
    m_alertedThresholds.clear();
    m_pendingAlertPercentage = -1;
    LogDebug(L"DataUsageMonitor::ResetAlerts: Cleared alerted thresholds");
}

bool DataUsageMonitor::Update(uint64_t currentUsageBytes)
{
    if (!IsEnabled())
    {
        return false;
    }

    m_currentUsageBytes = currentUsageBytes;
    int currentPercentage = GetUsagePercentage();

    // Check each threshold
    for (int threshold : m_thresholds)
    {
        if (currentPercentage >= threshold)
        {
            // Check if we've already alerted for this threshold
            if (m_alertedThresholds.find(threshold) == m_alertedThresholds.end())
            {
                // New threshold crossed
                m_alertedThresholds.insert(threshold);
                m_pendingAlertPercentage = threshold;
                LogDebug(L"DataUsageMonitor::Update: Threshold " + std::to_wstring(threshold) + 
                         L"% crossed (usage: " + std::to_wstring(currentPercentage) + L"%)");
                return true;
            }
        }
    }

    return false;
}

bool DataUsageMonitor::ShouldAlert(int& outPercentage)
{
    if (m_pendingAlertPercentage > 0)
    {
        outPercentage = m_pendingAlertPercentage;
        m_pendingAlertPercentage = -1;  // Clear pending alert
        return true;
    }
    return false;
}

int DataUsageMonitor::GetUsagePercentage() const
{
    if (m_quotaBytes == 0)
    {
        return 0;
    }
    return static_cast<int>((m_currentUsageBytes * 100) / m_quotaBytes);
}

} // namespace NetPulse
