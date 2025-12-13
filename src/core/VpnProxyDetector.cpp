// ============================================================================
// File: VpnProxyDetector.cpp
// Description: VPN and Proxy detection implementation
// Author: NetworkMonitor Project
// ============================================================================

#include "NetworkMonitor/VpnProxyDetector.h"
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <vector>
#include <string>

namespace NetworkMonitor
{

// VPN adapter keywords for detection (case-insensitive)
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

VpnProxyDetector::VpnProxyDetector()
    : m_initialized(false)
    , m_isVpnActive(false)
    , m_isProxyActive(false)
    , m_lastIPUpdateTime(0)
    , m_ipUpdateIntervalMs(DEFAULT_IP_UPDATE_INTERVAL_MS)
{
}

VpnProxyDetector::~VpnProxyDetector()
{
    Cleanup();
}

bool VpnProxyDetector::Initialize()
{
    if (m_initialized)
        return true;

    m_initialized = true;
    
    // Perform initial detection
    Update();
    
    return true;
}

void VpnProxyDetector::Cleanup()
{
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
        return;

    // Detect VPN (fast, can be called frequently)
    DetectVpnAdapters();
    
    // Detect proxy settings (fast)
    DetectProxySettings();
    
    // Fetch public IP (rate limited)
    DWORD currentTime = GetTickCount();
    if (m_lastIPUpdateTime == 0 || 
        (currentTime - m_lastIPUpdateTime) >= m_ipUpdateIntervalMs)
    {
        std::wstring newIP = FetchPublicIP();
        if (!newIP.empty())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_publicIP = newIP;
        }
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
    std::wstring newIP = FetchPublicIP();
    if (!newIP.empty())
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_publicIP = newIP;
    }
    m_lastIPUpdateTime = GetTickCount();
}

bool VpnProxyDetector::DetectVpnAdapters()
{
    // Get required buffer size
    ULONG bufferSize = 0;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | 
                  GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    
    if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, nullptr, &bufferSize) != ERROR_BUFFER_OVERFLOW)
    {
        m_isVpnActive = false;
        return false;
    }

    // Allocate buffer
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    // Get adapter addresses
    ULONG result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addresses, &bufferSize);
    if (result != NO_ERROR)
    {
        m_isVpnActive = false;
        return false;
    }

    // Scan adapters for VPN
    bool vpnFound = false;
    std::wstring vpnName;

    for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter != nullptr; adapter = adapter->Next)
    {
        // Skip disabled adapters
        if (adapter->OperStatus != IfOperStatusUp)
            continue;

        std::wstring description = adapter->Description ? adapter->Description : L"";
        std::wstring friendlyName = adapter->FriendlyName ? adapter->FriendlyName : L"";

        // Check adapter type
        bool isVpnType = false;
        switch (adapter->IfType)
        {
            case IF_TYPE_PPP:           // Point-to-point protocol
            case IF_TYPE_TUNNEL:        // Tunnel interface
            case IF_TYPE_PROP_VIRTUAL:  // Proprietary virtual interface
                isVpnType = true;
                break;
            default:
                break;
        }

        // Check description/name for VPN keywords
        if (IsVpnAdapter(adapter->IfType, description) || 
            IsVpnAdapter(adapter->IfType, friendlyName) ||
            isVpnType)
        {
            vpnFound = true;
            vpnName = friendlyName.empty() ? description : friendlyName;
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

bool VpnProxyDetector::IsVpnAdapter(DWORD adapterType, const std::wstring& description) const
{
    if (description.empty())
        return false;

    // Convert to lowercase for case-insensitive comparison
    std::wstring descLower = description;
    std::transform(descLower.begin(), descLower.end(), descLower.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });

    // Check for VPN keywords
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

bool VpnProxyDetector::DetectProxySettings()
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = {};
    
    if (!WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
    {
        m_isProxyActive = false;
        return false;
    }

    bool proxyEnabled = false;

    // Check if manual proxy is configured
    if (proxyConfig.lpszProxy != nullptr && proxyConfig.lpszProxy[0] != L'\0')
    {
        proxyEnabled = true;
    }

    // Check if auto-config URL is set
    if (proxyConfig.lpszAutoConfigUrl != nullptr && proxyConfig.lpszAutoConfigUrl[0] != L'\0')
    {
        proxyEnabled = true;
    }

    // Check if auto-detect is enabled (WPAD)
    if (proxyConfig.fAutoDetect)
    {
        proxyEnabled = true;
    }

    // Cleanup allocated strings
    if (proxyConfig.lpszAutoConfigUrl)
        GlobalFree(proxyConfig.lpszAutoConfigUrl);
    if (proxyConfig.lpszProxy)
        GlobalFree(proxyConfig.lpszProxy);
    if (proxyConfig.lpszProxyBypass)
        GlobalFree(proxyConfig.lpszProxyBypass);

    m_isProxyActive = proxyEnabled;
    return proxyEnabled;
}

std::wstring VpnProxyDetector::FetchPublicIP()
{
    std::wstring ip;
    
    // Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(
        L"NetworkMonitor/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
        return ip;

    // Set timeouts
    WinHttpSetTimeouts(hSession, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS, HTTP_TIMEOUT_MS);

    // Connect to api.ipify.org
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"api.ipify.org",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return ip;
    }

    // Create request
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        L"/",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return ip;
    }

    // Send request
    BOOL result = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (result)
    {
        result = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (result)
    {
        // Read response
        DWORD bytesAvailable = 0;
        DWORD bytesRead = 0;
        std::string response;

        do
        {
            bytesAvailable = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
                break;

            if (bytesAvailable == 0)
                break;

            std::vector<char> buffer(bytesAvailable + 1, 0);
            if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
                break;

            response.append(buffer.data(), bytesRead);
            
            // Sanity check: IP should be short
            if (response.size() > 45)  // Max IPv6 length
                break;

        } while (bytesAvailable > 0);

        // Convert to wide string
        if (!response.empty())
        {
            // Trim whitespace
            while (!response.empty() && (response.back() == '\n' || 
                   response.back() == '\r' || response.back() == ' '))
            {
                response.pop_back();
            }
            
            ip = std::wstring(response.begin(), response.end());
        }
    }

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ip;
}

} // namespace NetworkMonitor
