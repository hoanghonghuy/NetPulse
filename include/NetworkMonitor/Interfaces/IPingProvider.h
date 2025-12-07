#ifndef NETWORK_MONITOR_IPING_PROVIDER_H
#define NETWORK_MONITOR_IPING_PROVIDER_H

#include <string>

namespace NetworkMonitor
{

/**
 * Interface for ping/latency provider
 * Allows dependency injection and testing with mock implementations
 */
class IPingProvider
{
public:
    virtual ~IPingProvider() = default;

    /**
     * Get last measured latency in milliseconds
     * @return Latency in ms, or -1 if unavailable/timeout
     */
    virtual int GetLatency() const = 0;

    /**
     * Check if ping is available
     * @return true if available, false otherwise
     */
    virtual bool IsAvailable() const = 0;

    /**
     * Perform ping and update latency (call from timer)
     */
    virtual void Update() = 0;

    /**
     * Set new target (will resolve on next Update)
     * @param target IP address or hostname
     */
    virtual void SetTarget(const std::wstring& target) = 0;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IPING_PROVIDER_H
