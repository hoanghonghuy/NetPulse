#include "NetPulse/NetworkCalculator.h"
#include "NetPulse/Utils.h"
#include "../../resources/resource.h"
#include <algorithm>

namespace NetPulse
{

NetworkCalculator::NetworkCalculator()
{
}

NetworkCalculator::~NetworkCalculator()
{
}

bool NetworkCalculator::UpdateStats(NetworkStats& stats, ULONG64 currentBytesIn, ULONG64 currentBytesOut)
{
    DWORD currentTime = GetTickCount();

    // First time initialization
    if (stats.lastUpdateTime == 0)
    {
        stats.bytesReceived = currentBytesIn;
        stats.bytesSent = currentBytesOut;
        stats.prevBytesReceived = currentBytesIn;
        stats.prevBytesSent = currentBytesOut;
        stats.lastUpdateTime = currentTime;
        stats.currentDownloadSpeed = 0.0;
        stats.currentUploadSpeed = 0.0;
        stats.isActive = true;
        return true;
    }

    // Calculate time elapsed since last update
    double timeElapsed = GetElapsedSeconds(stats.lastUpdateTime, currentTime);

    // Guard against very small time intervals (prevent division by very small numbers)
    if (timeElapsed < 0.1)
    {
        return false;
    }

    // Update byte counters
    stats.prevBytesReceived = stats.bytesReceived;
    stats.prevBytesSent = stats.bytesSent;
    stats.bytesReceived = currentBytesIn;
    stats.bytesSent = currentBytesOut;

    // Calculate byte deltas (handle potential counter wraparound)
    ULONG64 deltaIn = 0;
    ULONG64 deltaOut = 0;

    if (currentBytesIn >= stats.prevBytesReceived)
    {
        deltaIn = currentBytesIn - stats.prevBytesReceived;
    }
    else
    {
        // Counter wraparound occurred (very rare, but possible)
        deltaIn = (MAXULONG64 - stats.prevBytesReceived) + currentBytesIn + 1;
    }

    if (currentBytesOut >= stats.prevBytesSent)
    {
        deltaOut = currentBytesOut - stats.prevBytesSent;
    }
    else
    {
        // Counter wraparound occurred
        deltaOut = (MAXULONG64 - stats.prevBytesSent) + currentBytesOut + 1;
    }

    // Calculate current speeds
    stats.currentDownloadSpeed = CalculateSpeed(deltaIn, timeElapsed);
    stats.currentUploadSpeed = CalculateSpeed(deltaOut, timeElapsed);

    // Update peak speeds
    if (stats.currentDownloadSpeed > stats.peakDownloadSpeed)
    {
        stats.peakDownloadSpeed = stats.currentDownloadSpeed;
    }

    if (stats.currentUploadSpeed > stats.peakUploadSpeed)
    {
        stats.peakUploadSpeed = stats.currentUploadSpeed;
    }

    // Update timestamp
    stats.lastUpdateTime = currentTime;
    stats.isActive = true;

    return true;
}

NetworkStats NetworkCalculator::CalculateAggregate(const std::vector<NetworkStats>& statsList)
{
    NetworkStats aggregate;
    aggregate.interfaceName = LoadStringResource(IDS_ALL_INTERFACES);
    if (aggregate.interfaceName.empty())
    {
        aggregate.interfaceName = L"All Interfaces";
    }

    aggregate.interfaceDesc = LoadStringResource(IDS_AGGREGATED_STATS);
    if (aggregate.interfaceDesc.empty())
    {
        aggregate.interfaceDesc = L"Aggregated Statistics";
    }

    if (statsList.empty())
    {
        return aggregate;
    }

    // Sum up all statistics
    for (const auto& stats : statsList)
    {
        if (!stats.isActive)
            continue;

        aggregate.bytesReceived += stats.bytesReceived;
        aggregate.bytesSent += stats.bytesSent;
        aggregate.currentDownloadSpeed += stats.currentDownloadSpeed;
        aggregate.currentUploadSpeed += stats.currentUploadSpeed;
        
        // Use maximum peak values
        aggregate.peakDownloadSpeed = (std::max)(aggregate.peakDownloadSpeed, stats.peakDownloadSpeed);
        aggregate.peakUploadSpeed = (std::max)(aggregate.peakUploadSpeed, stats.peakUploadSpeed);
    }

    aggregate.isActive = true;
    aggregate.lastUpdateTime = GetTickCount();

    return aggregate;
}

void NetworkCalculator::ResetStats(NetworkStats& stats)
{
    stats.prevBytesReceived = stats.bytesReceived;
    stats.prevBytesSent = stats.bytesSent;
    stats.currentDownloadSpeed = 0.0;
    stats.currentUploadSpeed = 0.0;
    stats.peakDownloadSpeed = 0.0;
    stats.peakUploadSpeed = 0.0;
    stats.lastUpdateTime = GetTickCount();
}

double NetworkCalculator::CalculateSpeed(ULONG64 byteDelta, double timeIntervalSeconds)
{
    if (timeIntervalSeconds <= 0.0)
    {
        return 0.0;
    }

    return static_cast<double>(byteDelta) / timeIntervalSeconds;
}

} // namespace NetPulse
