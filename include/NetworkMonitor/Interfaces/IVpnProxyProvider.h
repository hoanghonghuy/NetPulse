#ifndef NETWORK_MONITOR_IVPN_PROXY_PROVIDER_H
#define NETWORK_MONITOR_IVPN_PROXY_PROVIDER_H

#include <string>

namespace NetworkMonitor
{

/**
 * Interface for VPN/Proxy detection provider
 * Allows dependency injection and testing with mock implementations
 */
class IVpnProxyProvider
{
public:
    virtual ~IVpnProxyProvider() = default;

    /**
     * Check if connected via VPN
     * @return true if VPN is active, false otherwise
     */
    virtual bool IsVpnActive() const = 0;

    /**
     * Check if proxy is enabled
     * @return true if proxy is configured and active, false otherwise
     */
    virtual bool IsProxyActive() const = 0;

    /**
     * Get current public IP address
     * @return Public IP string, or empty if unavailable
     */
    virtual std::wstring GetPublicIP() const = 0;

    /**
     * Update VPN/Proxy status and public IP (call periodically)
     */
    virtual void Update() = 0;

    /**
     * Check if VPN/Proxy detection is available
     * @return true if available, false otherwise
     */
    virtual bool IsAvailable() const = 0;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IVPN_PROXY_PROVIDER_H
