#ifndef NETWORK_MONITOR_IICMP_PROVIDER_H
#define NETWORK_MONITOR_IICMP_PROVIDER_H

#include <windows.h>
#include <string>

namespace NetPulse
{

class IIcmpProvider
{
public:
    virtual ~IIcmpProvider() = default;

    virtual HANDLE CreateIcmpHandle() = 0;
    virtual void CloseIcmpHandle(HANDLE h) = 0;
    
    virtual DWORD SendEcho(
        HANDLE h,
        ULONG targetIP,
        LPVOID replyBuffer,
        DWORD replySize,
        DWORD timeout
    ) = 0;

    virtual bool ResolveTarget(const std::wstring& target, ULONG& ipOut) = 0;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_IICMP_PROVIDER_H
