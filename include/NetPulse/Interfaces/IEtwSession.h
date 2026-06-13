#ifndef NETWORK_MONITOR_IETW_SESSION_H
#define NETWORK_MONITOR_IETW_SESSION_H

#include <windows.h>
#include <evntrace.h>
#include <string>

namespace NetPulse
{

class IEtwSession
{
public:
    virtual ~IEtwSession() = default;

    virtual ULONG Start(TRACEHANDLE& sessionHandle, const std::wstring& sessionName, PEVENT_TRACE_PROPERTIES properties) = 0;
    virtual ULONG Stop(TRACEHANDLE sessionHandle, const std::wstring& sessionName, PEVENT_TRACE_PROPERTIES properties) = 0;
    virtual ULONG EnableProvider(TRACEHANDLE sessionHandle, const GUID* providerGuid, UCHAR level, ULONGLONG matchAnyKeyword, ULONGLONG matchAllKeyword) = 0;
    
    virtual TRACEHANDLE Open(EVENT_TRACE_LOGFILEW* logfile) = 0;
    virtual ULONG Process(TRACEHANDLE* traceHandles, ULONG count) = 0;
    virtual ULONG Close(TRACEHANDLE traceHandle) = 0;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_IETW_SESSION_H
