#pragma once

#include "NetPulse/Interfaces/IEtwSession.h"

namespace NetPulseTests
{

class FakeEtwSession : public NetPulse::IEtwSession
{
public:
    ULONG m_startResult;
    ULONG m_stopResult;
    ULONG m_enableResult;
    TRACEHANDLE m_openResult;
    ULONG m_processResult;
    ULONG m_closeResult;

    int m_startCalls;
    int m_stopCalls;
    int m_enableCalls;
    int m_openCalls;
    int m_processCalls;
    int m_closeCalls;

    FakeEtwSession()
        : m_startResult(ERROR_SUCCESS)
        , m_stopResult(ERROR_SUCCESS)
        , m_enableResult(ERROR_SUCCESS)
        , m_openResult(static_cast<TRACEHANDLE>(0x5678))
        , m_processResult(ERROR_SUCCESS)
        , m_closeResult(ERROR_SUCCESS)
        , m_startCalls(0)
        , m_stopCalls(0)
        , m_enableCalls(0)
        , m_openCalls(0)
        , m_processCalls(0)
        , m_closeCalls(0)
    {
    }

    ULONG Start(TRACEHANDLE& sessionHandle, const std::wstring& sessionName, PEVENT_TRACE_PROPERTIES properties) override
    {
        (void)sessionName;
        (void)properties;
        ++m_startCalls;
        if (m_startResult == ERROR_SUCCESS)
        {
            sessionHandle = static_cast<TRACEHANDLE>(0x9999);
        }
        return m_startResult;
    }

    ULONG Stop(TRACEHANDLE sessionHandle, const std::wstring& sessionName, PEVENT_TRACE_PROPERTIES properties) override
    {
        (void)sessionHandle;
        (void)sessionName;
        (void)properties;
        ++m_stopCalls;
        return m_stopResult;
    }

    ULONG EnableProvider(TRACEHANDLE sessionHandle, const GUID* providerGuid, UCHAR level, ULONGLONG matchAnyKeyword, ULONGLONG matchAllKeyword) override
    {
        (void)sessionHandle;
        (void)providerGuid;
        (void)level;
        (void)matchAnyKeyword;
        (void)matchAllKeyword;
        ++m_enableCalls;
        return m_enableResult;
    }

    TRACEHANDLE Open(EVENT_TRACE_LOGFILEW* logfile) override
    {
        (void)logfile;
        ++m_openCalls;
        return m_openResult;
    }

    ULONG Process(TRACEHANDLE* traceHandles, ULONG count) override
    {
        (void)traceHandles;
        (void)count;
        ++m_processCalls;
        return m_processResult;
    }

    ULONG Close(TRACEHANDLE traceHandle) override
    {
        (void)traceHandle;
        ++m_closeCalls;
        return m_closeResult;
    }
};

} // namespace NetPulseTests
