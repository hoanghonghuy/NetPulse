#ifndef NETWORK_MONITOR_VPN_PROXY_DETECTOR_H
#define NETWORK_MONITOR_VPN_PROXY_DETECTOR_H

#include "NetPulse/Common.h"
#include "NetPulse/Interfaces/IVpnProxyProvider.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <future>

#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace NetPulseTests
{
struct VpnProxyDetectorTestFriend;
}

namespace NetPulse
{

class IHttpClient;

struct VpnAdapterCandidate
{
    DWORD ifType = 0;
    std::wstring description;
    std::wstring friendlyName;
    bool isUp = true;
};

/**
 * VpnProxyDetector - Detects VPN connections and proxy settings
 */
class VpnProxyDetector : public IVpnProxyProvider
{
public:
    explicit VpnProxyDetector(std::shared_ptr<IHttpClient> httpClient = nullptr);
    ~VpnProxyDetector() override;

    bool Initialize();
    void Cleanup();
    void Update() override;

    bool IsVpnActive() const override { return m_isVpnActive; }
    bool IsProxyActive() const override { return m_isProxyActive; }
    std::wstring GetPublicIP() const override;
    bool IsAvailable() const override { return m_initialized; }

    std::wstring GetVpnAdapterName() const;
    void RefreshPublicIP();
    void SetPublicIPUpdateInterval(UINT intervalMs) { m_ipUpdateIntervalMs = intervalMs; }

    static bool IsVpnInterfaceType(DWORD adapterType);
    static bool IsVpnAdapter(DWORD adapterType, const std::wstring& description);
    static bool ClassifyAdapterAsVpn(const VpnAdapterCandidate& candidate);
    static bool EvaluateProxyConfig(bool hasManualProxy,
                                    bool hasAutoConfigUrl,
                                    bool autoDetect);
    static std::wstring ParsePublicIPResponse(const std::string& response);
    static std::wstring GetPublicIPApiHost();
    static std::wstring GetPublicIPApiPath();

private:
    friend struct NetPulseTests::VpnProxyDetectorTestFriend;

    bool DetectVpnAdapters();
    bool DetectProxySettings();
    std::wstring FetchPublicIP();
    IHttpClient& GetHttpClient();

    void StartAsyncIPFetch();
    void CheckAsyncIPResult();

    std::shared_ptr<IHttpClient> m_httpClient;
    std::unique_ptr<class WinHttpClient> m_defaultHttpClient;

    bool m_initialized;
    std::atomic<bool> m_isVpnActive;
    std::atomic<bool> m_isProxyActive;
    std::wstring m_publicIP;
    std::wstring m_vpnAdapterName;
    mutable std::mutex m_mutex;

    ULONGLONG m_lastIPUpdateTime;
    UINT m_ipUpdateIntervalMs;

    static const wchar_t* VPN_KEYWORDS[];
    static const size_t VPN_KEYWORDS_COUNT;

    static constexpr UINT DEFAULT_IP_UPDATE_INTERVAL_MS = 5 * 60 * 1000;
    static constexpr DWORD HTTP_TIMEOUT_MS = 5000;

    std::future<std::wstring> m_ipFetchFuture;
    std::atomic<bool> m_ipFetchInProgress;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_VPN_PROXY_DETECTOR_H
