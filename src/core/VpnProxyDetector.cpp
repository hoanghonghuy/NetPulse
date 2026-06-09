#include "NetPulse/VpnProxyDetector.h"
#include "NetPulse/Interfaces/IHttpClient.h"
#include "NetPulse/WinHttpClient.h"
#include <winhttp.h>

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif
#include <algorithm>
#include <cwctype>
#include <vector>
#include <string>

namespace NetPulse
{

const wchar_t* VpnProxyDetector::VPN_KEYWORDS[] = {
    L"VPN",
    L"TAP-Windows",
    L"TAP",
    L"TUN",
    L"WireGuard",
    L"OpenVPN",
    L"NordVPN",
    L"ExpressVPN",
    L"Surfshark",
    L"ProtonVPN",
    L"Cisco AnyConnect",
    L"Fortinet",
    L"Pulse Secure",
    L"GlobalProtect",
    L"Juniper",
    L"SoftEther",
    L"Hamachi",
    L"ZeroTier",
    L"Tailscale",
    L"Wintun"
};
const size_t VpnProxyDetector::VPN_KEYWORDS_COUNT = sizeof(VPN_KEYWORDS) / sizeof(VPN_KEYWORDS[0]);

VpnProxyDetector::VpnProxyDetector(std::shared_ptr<IHttpClient> httpClient)
    : m_httpClient(std::move(httpClient))
    , m_initialized(false)
    , m_isVpnActive(false)
    , m_isProxyActive(false)
    , m_lastIPUpdateTime(0)
    , m_ipUpdateIntervalMs(DEFAULT_IP_UPDATE_INTERVAL_MS)
    , m_ipFetchInProgress(false)
{
}

VpnProxyDetector::~VpnProxyDetector()
{
    Cleanup();
}

IHttpClient& VpnProxyDetector::GetHttpClient()
{
    if (m_httpClient)
    {
        return *m_httpClient;
    }

    if (!m_defaultHttpClient)
    {
        m_defaultHttpClient = std::make_unique<WinHttpClient>(L"NetPulse/1.0");
    }

    return *m_defaultHttpClient;
}

bool VpnProxyDetector::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    m_initialized = true;

    const wchar_t* testMode = _wgetenv(L"NETPULSE_TEST_MODE");
    if (!testMode || testMode[0] != L'1')
    {
        Update();
    }

    return true;
}

void VpnProxyDetector::Cleanup()
{
    if (m_ipFetchFuture.valid())
    {
        m_ipFetchFuture.wait();
    }
    m_ipFetchInProgress = false;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
    m_isVpnActive = false;
    m_isProxyActive = false;
    m_publicIP.clear();
    m_vpnAdapterName.clear();
}

void VpnProxyDetector::Update()
{
    if (!m_initialized)
    {
        return;
    }

    DetectVpnAdapters();
    DetectProxySettings();
    CheckAsyncIPResult();

    ULONGLONG currentTime = GetTickCount64();
    if (m_lastIPUpdateTime == 0 ||
        (currentTime - m_lastIPUpdateTime) >= m_ipUpdateIntervalMs)
    {
        StartAsyncIPFetch();
        m_lastIPUpdateTime = currentTime;
    }
}

std::wstring VpnProxyDetector::GetPublicIP() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_publicIP;
}

std::wstring VpnProxyDetector::GetVpnAdapterName() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_vpnAdapterName;
}

void VpnProxyDetector::RefreshPublicIP()
{
    StartAsyncIPFetch();
    m_lastIPUpdateTime = GetTickCount64();
}

void VpnProxyDetector::StartAsyncIPFetch()
{
    if (m_ipFetchInProgress)
    {
        return;
    }

    m_ipFetchInProgress = true;
    m_ipFetchFuture = std::async(std::launch::async, [this]() {
        return FetchPublicIP();
    });
}

void VpnProxyDetector::CheckAsyncIPResult()
{
    if (!m_ipFetchInProgress)
    {
        return;
    }

    if (m_ipFetchFuture.valid() &&
        m_ipFetchFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
    {
        std::wstring newIP = m_ipFetchFuture.get();
        if (!newIP.empty())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_publicIP = newIP;
        }
        m_ipFetchInProgress = false;
    }
}

bool VpnProxyDetector::IsVpnInterfaceType(DWORD adapterType)
{
    switch (adapterType)
    {
        case IF_TYPE_PPP:
        case IF_TYPE_TUNNEL:
        case IF_TYPE_PROP_VIRTUAL:
            return true;
        default:
            return false;
    }
}

bool VpnProxyDetector::ClassifyAdapterAsVpn(const VpnAdapterCandidate& candidate)
{
    if (!candidate.isUp)
    {
        return false;
    }

    return IsVpnAdapter(candidate.ifType, candidate.description) ||
           IsVpnAdapter(candidate.ifType, candidate.friendlyName) ||
           IsVpnInterfaceType(candidate.ifType);
}

bool VpnProxyDetector::DetectVpnAdapters()
{
    ULONG bufferSize = 0;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST |
                  GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &bufferSize) != ERROR_BUFFER_OVERFLOW)
    {
        m_isVpnActive = false;
        return false;
    }

    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    ULONG result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addresses, &bufferSize);
    if (result != NO_ERROR)
    {
        m_isVpnActive = false;
        return false;
    }

    bool vpnFound = false;
    std::wstring vpnName;

    for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter != nullptr; adapter = adapter->Next)
    {
        VpnAdapterCandidate candidate;
        candidate.ifType = adapter->IfType;
        candidate.description = adapter->Description ? adapter->Description : L"";
        candidate.friendlyName = adapter->FriendlyName ? adapter->FriendlyName : L"";
        candidate.isUp = adapter->OperStatus == IfOperStatusUp;

        if (ClassifyAdapterAsVpn(candidate))
        {
            vpnFound = true;
            vpnName = candidate.friendlyName.empty() ? candidate.description : candidate.friendlyName;
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_vpnAdapterName = vpnName;
    }
    m_isVpnActive = vpnFound;
    return vpnFound;
}

bool VpnProxyDetector::IsVpnAdapter(DWORD /*adapterType*/, const std::wstring& description)
{
    if (description.empty())
    {
        return false;
    }

    std::wstring descLower = description;
    std::transform(descLower.begin(), descLower.end(), descLower.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });

    for (size_t i = 0; i < VPN_KEYWORDS_COUNT; ++i)
    {
        std::wstring keyword = VPN_KEYWORDS[i];
        std::transform(keyword.begin(), keyword.end(), keyword.begin(),
            [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });

        if (descLower.find(keyword) != std::wstring::npos)
        {
            return true;
        }
    }

    return false;
}

bool VpnProxyDetector::EvaluateProxyConfig(bool hasManualProxy,
                                           bool hasAutoConfigUrl,
                                           bool autoDetect)
{
    return hasManualProxy || hasAutoConfigUrl || autoDetect;
}

bool VpnProxyDetector::DetectProxySettings()
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = {};

    if (!WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
    {
        m_isProxyActive = false;
        return false;
    }

    bool hasManualProxy = proxyConfig.lpszProxy != nullptr && proxyConfig.lpszProxy[0] != L'\0';
    bool hasAutoConfigUrl = proxyConfig.lpszAutoConfigUrl != nullptr && proxyConfig.lpszAutoConfigUrl[0] != L'\0';
    bool autoDetect = proxyConfig.fAutoDetect != FALSE;

    bool proxyEnabled = EvaluateProxyConfig(hasManualProxy, hasAutoConfigUrl, autoDetect);

    if (proxyConfig.lpszAutoConfigUrl)
    {
        GlobalFree(proxyConfig.lpszAutoConfigUrl);
    }
    if (proxyConfig.lpszProxy)
    {
        GlobalFree(proxyConfig.lpszProxy);
    }
    if (proxyConfig.lpszProxyBypass)
    {
        GlobalFree(proxyConfig.lpszProxyBypass);
    }

    m_isProxyActive = proxyEnabled;
    return proxyEnabled;
}

std::wstring VpnProxyDetector::GetPublicIPApiHost()
{
    return L"api.ipify.org";
}

std::wstring VpnProxyDetector::GetPublicIPApiPath()
{
    return L"/";
}

std::wstring VpnProxyDetector::ParsePublicIPResponse(const std::string& response)
{
    std::string trimmed = response;
    while (!trimmed.empty() &&
           (trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == ' '))
    {
        trimmed.pop_back();
    }

    if (trimmed.empty() || trimmed.size() > 45)
    {
        return std::wstring();
    }

    return std::wstring(trimmed.begin(), trimmed.end());
}

std::wstring VpnProxyDetector::FetchPublicIP()
{
    std::string response;
    if (!GetHttpClient().HttpGet(GetPublicIPApiHost(), GetPublicIPApiPath(), response))
    {
        return std::wstring();
    }

    return ParsePublicIPResponse(response);
}

} // namespace NetPulse
