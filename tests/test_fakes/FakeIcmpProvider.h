#pragma once

#include "NetPulse/Interfaces/IIcmpProvider.h"
#include <iphlpapi.h>
#include <icmpapi.h>

namespace NetPulseTests
{

class FakeIcmpProvider : public NetPulse::IIcmpProvider
{
public:
    HANDLE m_mockHandle;
    DWORD m_sendEchoResult;
    ULONG m_echoReplyStatus;
    ULONG m_echoReplyRTT;
    
    int m_createHandleCalls;
    int m_closeHandleCalls;
    int m_sendEchoCalls;
    int m_resolveCalls;

    bool m_resolveSuccess;
    ULONG m_resolvedIP;

    FakeIcmpProvider()
        : m_mockHandle(reinterpret_cast<HANDLE>(0x1234))
        , m_sendEchoResult(1)
        , m_echoReplyStatus(0UL) // 0 = IP_SUCCESS
        , m_echoReplyRTT(15)
        , m_createHandleCalls(0)
        , m_closeHandleCalls(0)
        , m_sendEchoCalls(0)
        , m_resolveCalls(0)
        , m_resolveSuccess(true)
        , m_resolvedIP(0x08080808) // 8.8.8.8
    {
    }

    HANDLE CreateIcmpHandle() override
    {
        ++m_createHandleCalls;
        return m_mockHandle;
    }

    void CloseIcmpHandle(HANDLE h) override
    {
        (void)h;
        ++m_closeHandleCalls;
    }

    DWORD SendEcho(HANDLE h, ULONG targetIP, LPVOID replyBuffer, DWORD replySize, DWORD timeout) override
    {
        (void)h;
        (void)targetIP;
        (void)timeout;
        ++m_sendEchoCalls;

        if (m_sendEchoResult > 0 && replyBuffer && replySize >= (sizeof(ICMP_ECHO_REPLY) + 8))
        {
            PICMP_ECHO_REPLY pReply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer);
            pReply->Status = m_echoReplyStatus;
            pReply->RoundTripTime = m_echoReplyRTT;
        }

        return m_sendEchoResult;
    }

    bool ResolveTarget(const std::wstring& target, ULONG& ipOut) override
    {
        ++m_resolveCalls;
        if (target == L"fail.com")
        {
            ipOut = 0;
            return false;
        }
        
        if (m_resolveSuccess)
        {
            ipOut = m_resolvedIP;
            return true;
        }
        
        ipOut = 0;
        return false;
    }
};

} // namespace NetPulseTests
