#include "NetPulse/PingMonitor.h"
#include "NetPulse/Utils.h"
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace NetPulse
{

PingMonitor::PingMonitor()
    : m_hIcmp(INVALID_HANDLE_VALUE)
    , m_initialized(false)
    , m_latency(-1)
    , m_target(L"8.8.8.8")
    , m_targetIP(0)
{
}

PingMonitor::~PingMonitor()
{
    Cleanup();
}

bool PingMonitor::Initialize(const std::wstring& target)
{
    if (m_initialized)
    {
        return true;
    }

    m_target = target;
    if (!ResolveTarget())
    {
        LogError(L"PingMonitor::Initialize: Failed to resolve target");
        return false;
    }

    m_hIcmp = IcmpCreateFile();
    if (m_hIcmp == INVALID_HANDLE_VALUE)
    {
        LogError(L"PingMonitor::Initialize: IcmpCreateFile failed");
        return false;
    }

    m_initialized = true;
    LogDebug(L"PingMonitor::Initialize: success, target=" + m_target);
    return true;
}

void PingMonitor::Cleanup()
{
    if (m_hIcmp != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(m_hIcmp);
        m_hIcmp = INVALID_HANDLE_VALUE;
    }

    m_initialized = false;
    m_latency = -1;
    m_targetIP = 0;
}

void PingMonitor::SetTarget(const std::wstring& target)
{
    if (m_target != target)
    {
        m_target = target;
        ResolveTarget();
    }
}

bool PingMonitor::ResolveTarget()
{
    if (m_target.empty())
    {
        m_target = L"8.8.8.8";
    }

    // Convert wide string to narrow string for getaddrinfo
    int len = WideCharToMultiByte(CP_UTF8, 0, m_target.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
    {
        return false;
    }

    std::string narrowTarget(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, m_target.c_str(), -1, &narrowTarget[0], len, nullptr, nullptr);

    // Try to parse as IP address first
    struct in_addr addr;
    if (inet_pton(AF_INET, narrowTarget.c_str(), &addr) == 1)
    {
        m_targetIP = addr.s_addr;
        return true;
    }

    // Try DNS resolution
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    if (getaddrinfo(narrowTarget.c_str(), nullptr, &hints, &result) == 0 && result)
    {
        struct sockaddr_in* sockaddr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
        m_targetIP = sockaddr->sin_addr.s_addr;
        freeaddrinfo(result);
        return true;
    }

    m_targetIP = 0;
    return false;
}

void PingMonitor::Update()
{
    if (!m_initialized || m_hIcmp == INVALID_HANDLE_VALUE || m_targetIP == 0)
    {
        m_latency = -1;
        return;
    }

    // Prepare reply buffer
    char replyBuffer[sizeof(ICMP_ECHO_REPLY) + 8];
    DWORD replySize = sizeof(replyBuffer);

    // Send ICMP echo request (no data payload)
    DWORD result = IcmpSendEcho(
        m_hIcmp,
        m_targetIP,
        nullptr,    // No send data
        0,          // No send data size
        nullptr,    // No IP options
        replyBuffer,
        replySize,
        TIMEOUT_MS
    );

    if (result > 0)
    {
        PICMP_ECHO_REPLY pReply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer);
        if (pReply->Status == IP_SUCCESS)
        {
            m_latency = static_cast<int>(pReply->RoundTripTime);
        }
        else
        {
            m_latency = -1;
        }
    }
    else
    {
        m_latency = -1;
    }
}

} // namespace NetPulse
