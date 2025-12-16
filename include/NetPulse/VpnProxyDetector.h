#ifndef NETWORK_MONITOR_VPN_PROXY_DETECTOR_H
#define NETWORK_MONITOR_VPN_PROXY_DETECTOR_H

#include "NetPulse/Common.h"
#include "NetPulse/Interfaces/IVpnProxyProvider.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <winhttp.h>
#include <string>
#include <atomic>
#include <mutex>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "winhttp.lib")

namespace NetPulse
{

/**
 * VpnProxyDetector - Detects VPN connections and proxy settings
 * 
 * Features:
 * - Detects VPN by scanning network adapters for VPN-type interfaces
 * - Detects system proxy settings using WinHTTP
 * - Fetches public IP address from external API
 */
class VpnProxyDetector : public IVpnProxyProvider
{
public:
    VpnProxyDetector();
    ~VpnProxyDetector() override;

    /**
     * Initialize the detector
     * @return true if initialized successfully
     */
    bool Initialize();

    /**
     * Cleanup resources
     */
    void Cleanup();

    /**
     * Update VPN/Proxy status and public IP
     * Call this periodically (e.g., every 5 minutes for IP, every 30s for VPN)
     */
    void Update() override;

    /**
     * Check if connected via VPN
     */
    bool IsVpnActive() const override { return m_isVpnActive; }

    /**
     * Check if proxy is enabled
     */
    bool IsProxyActive() const override { return m_isProxyActive; }

    /**
     * Get current public IP address
     */
    std::wstring GetPublicIP() const override;

    /**
     * Check if detector is available
     */
    bool IsAvailable() const override { return m_initialized; }

    /**
     * Get VPN adapter name if connected
     */
    std::wstring GetVpnAdapterName() const;

    /**
     * Force refresh public IP now (ignores rate limiting)
     */
    void RefreshPublicIP();

    /**
     * Set public IP update interval in milliseconds (default: 5 minutes)
     */
    void SetPublicIPUpdateInterval(UINT intervalMs) { m_ipUpdateIntervalMs = intervalMs; }

private:
    /**
     * Scan network adapters for VPN connections
     * @return true if VPN adapter found
     */
    bool DetectVpnAdapters();

    /**
     * Check system proxy settings
     * @return true if proxy is configured and enabled
     */
    bool DetectProxySettings();

    /**
     * Fetch public IP from external API
     * @return Public IP string, or empty if failed
     */
    std::wstring FetchPublicIP();

    /**
     * Check if adapter is a VPN adapter based on type and description
     */
    bool IsVpnAdapter(DWORD adapterType, const std::wstring& description) const;

private:
    bool m_initialized;
    std::atomic<bool> m_isVpnActive;
    std::atomic<bool> m_isProxyActive;
    std::wstring m_publicIP;
    std::wstring m_vpnAdapterName;
    mutable std::mutex m_mutex;
    
    // Rate limiting for public IP fetch
    DWORD m_lastIPUpdateTime;
    UINT m_ipUpdateIntervalMs;
    
    // VPN adapter keywords for detection
    static const wchar_t* VPN_KEYWORDS[];
    static const size_t VPN_KEYWORDS_COUNT;

    // Default IP update interval: 5 minutes
    static constexpr UINT DEFAULT_IP_UPDATE_INTERVAL_MS = 5 * 60 * 1000;
    
    // HTTP timeout for IP fetch
    static constexpr DWORD HTTP_TIMEOUT_MS = 5000;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_VPN_PROXY_DETECTOR_H
