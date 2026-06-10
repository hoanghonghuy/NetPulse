#include "NetPulse/EtwNetworkMonitor.h"
#include "NetPulse/Utils.h"
#include <tdh.h>
#include <in6addr.h>
#include <psapi.h>

#pragma comment(lib, "tdh.lib")

namespace NetPulse
{

class WinEtwSession : public IEtwSession
{
public:
    ULONG Start(TRACEHANDLE& sessionHandle, const std::wstring& sessionName, PEVENT_TRACE_PROPERTIES properties) override
    {
        return ::StartTraceW(&sessionHandle, sessionName.c_str(), properties);
    }

    ULONG Stop(TRACEHANDLE sessionHandle, const std::wstring& sessionName, PEVENT_TRACE_PROPERTIES properties) override
    {
        return ::ControlTraceW(sessionHandle, sessionName.empty() ? nullptr : sessionName.c_str(), properties, EVENT_TRACE_CONTROL_STOP);
    }

    ULONG EnableProvider(TRACEHANDLE sessionHandle, const GUID* providerGuid, UCHAR level, ULONGLONG matchAnyKeyword, ULONGLONG matchAllKeyword) override
    {
        return ::EnableTraceEx2(
            sessionHandle,
            providerGuid,
            EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            level,
            matchAnyKeyword,
            matchAllKeyword,
            0,
            nullptr
        );
    }

    TRACEHANDLE Open(EVENT_TRACE_LOGFILEW* logfile) override
    {
        return ::OpenTraceW(logfile);
    }

    ULONG Process(TRACEHANDLE* traceHandles, ULONG count) override
    {
        return ::ProcessTrace(traceHandles, count, nullptr, nullptr);
    }

    ULONG Close(TRACEHANDLE traceHandle) override
    {
        return ::CloseTrace(traceHandle);
    }
};

// Static instance for callback
EtwNetworkMonitor* EtwNetworkMonitor::s_instance = nullptr;

// Microsoft-Windows-Kernel-Network provider GUID
// {7dd42a49-5329-4832-8dfd-43d979153a88}
static const GUID KernelNetworkProviderGuid = 
    { 0x7dd42a49, 0x5329, 0x4832, { 0x8d, 0xfd, 0x43, 0xd9, 0x79, 0x15, 0x3a, 0x88 } };

EtwNetworkMonitor::EtwNetworkMonitor(IEtwSession* etwSession)
    : m_defaultEtwSession(nullptr)
    , m_pEtwSession(etwSession)
    , m_sessionHandle(0)
    , m_traceHandle(INVALID_PROCESSTRACE_HANDLE)
    , m_running(false)
    , m_stopRequested(false)
{
    if (!m_pEtwSession)
    {
        m_defaultEtwSession = std::make_unique<WinEtwSession>();
        m_pEtwSession = m_defaultEtwSession.get();
    }
}

EtwNetworkMonitor::~EtwNetworkMonitor()
{
    Stop();
}

bool EtwNetworkMonitor::Start()
{
    if (m_running)
    {
        return true;
    }

    m_stopRequested = false;

    // Calculate buffer size for session properties
    size_t sessionNameLen = (wcslen(SESSION_NAME) + 1) * sizeof(wchar_t);
    size_t bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sessionNameLen;
    
    std::vector<BYTE> buffer(bufferSize, 0);
    PEVENT_TRACE_PROPERTIES pSessionProps = reinterpret_cast<PEVENT_TRACE_PROPERTIES>(buffer.data());
    
    pSessionProps->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    pSessionProps->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pSessionProps->Wnode.ClientContext = 1; // QPC clock resolution
    pSessionProps->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    pSessionProps->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    
    // Try to stop any existing session first
    m_pEtwSession->Stop(0, SESSION_NAME, pSessionProps);
    
    // Reset properties after stop attempt
    ZeroMemory(pSessionProps, bufferSize);
    pSessionProps->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    pSessionProps->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pSessionProps->Wnode.ClientContext = 1;
    pSessionProps->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    pSessionProps->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    
    // Start the trace session
    ULONG status = m_pEtwSession->Start(m_sessionHandle, SESSION_NAME, pSessionProps);
    if (status != ERROR_SUCCESS)
    {
        LogError(L"EtwNetworkMonitor::Start: StartTraceW failed with error " + std::to_wstring(status));
        s_instance = nullptr;
        return false;
    }
    
    // Enable the kernel network provider
    status = m_pEtwSession->EnableProvider(
        m_sessionHandle,
        &KernelNetworkProviderGuid,
        TRACE_LEVEL_INFORMATION,
        0,  // MatchAnyKeyword
        0   // MatchAllKeyword
    );
    
    if (status != ERROR_SUCCESS)
    {
        LogError(L"EtwNetworkMonitor::Start: EnableTraceEx2 failed with error " + std::to_wstring(status));
        m_pEtwSession->Stop(m_sessionHandle, L"", pSessionProps);
        m_sessionHandle = 0;
        s_instance = nullptr;
        return false;
    }
    
    s_instance = this;

    // Start processing thread
    m_running = true;
    m_processThread = std::thread(&EtwNetworkMonitor::ProcessThreadProc, this);

    LogDebug(L"EtwNetworkMonitor::Start: ETW session started successfully");
    return true;
}

void EtwNetworkMonitor::Stop()
{
    if (!m_running)
    {
        return;
    }
    
    m_stopRequested = true;
    
    // Stop the trace session
    if (m_sessionHandle != 0)
    {
        size_t sessionNameLen = (wcslen(SESSION_NAME) + 1) * sizeof(wchar_t);
        size_t bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sessionNameLen;
        std::vector<BYTE> buffer(bufferSize, 0);
        PEVENT_TRACE_PROPERTIES pSessionProps = reinterpret_cast<PEVENT_TRACE_PROPERTIES>(buffer.data());
        pSessionProps->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
        pSessionProps->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        
        m_pEtwSession->Stop(m_sessionHandle, L"", pSessionProps);
        m_sessionHandle = 0;
    }
    
    // Close the trace handle (this will cause ProcessTrace to return)
    if (m_traceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        m_pEtwSession->Close(m_traceHandle);
        m_traceHandle = INVALID_PROCESSTRACE_HANDLE;
    }
    
    // Wait for processing thread to finish
    if (m_processThread.joinable())
    {
        m_processThread.join();
    }
    
    m_running = false;
    s_instance = nullptr;
    
    LogDebug(L"EtwNetworkMonitor::Stop: ETW session stopped");
}

void EtwNetworkMonitor::ProcessThreadProc()
{
    EVENT_TRACE_LOGFILEW traceLogfile = {};
    traceLogfile.LoggerName = const_cast<LPWSTR>(SESSION_NAME);
    traceLogfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    traceLogfile.EventRecordCallback = EventRecordCallback;
    
    m_traceHandle = m_pEtwSession->Open(&traceLogfile);
    if (m_traceHandle == INVALID_PROCESSTRACE_HANDLE)
    {
        LogError(L"EtwNetworkMonitor::ProcessThreadProc: OpenTraceW failed");
        return;
    }
    
    // This will block until the trace is closed
    ULONG status = m_pEtwSession->Process(&m_traceHandle, 1);
    if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
    {
        LogError(L"EtwNetworkMonitor::ProcessThreadProc: ProcessTrace failed with error " + std::to_wstring(status));
    }
}

void WINAPI EtwNetworkMonitor::EventRecordCallback(PEVENT_RECORD pEventRecord)
{
    if (s_instance && !s_instance->m_stopRequested)
    {
        s_instance->ProcessEvent(pEventRecord);
    }
}

void EtwNetworkMonitor::ApplyTrafficEventForTest(USHORT eventId, DWORD pid, ULONG byteCount)
{
    RecordTrafficEvent(eventId, pid, byteCount);
}

void EtwNetworkMonitor::RecordTrafficEvent(USHORT eventId, DWORD pid, ULONG byteCount)
{
    bool isSend = (eventId == EVENT_ID_TCP_SEND || eventId == EVENT_ID_UDP_SEND);
    bool isRecv = (eventId == EVENT_ID_TCP_RECV || eventId == EVENT_ID_UDP_RECV);

    if (!isSend && !isRecv)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_statsMutex);

    auto it = m_processStats.find(pid);
    if (it == m_processStats.end())
    {
        ProcessTrafficStats stats;
        stats.processId = pid;
        stats.processName = GetProcessName(pid);
        m_processStats[pid] = stats;
        it = m_processStats.find(pid);
    }

    if (isSend)
    {
        it->second.bytesSent += byteCount;
    }
    else if (isRecv)
    {
        it->second.bytesReceived += byteCount;
    }
}

void EtwNetworkMonitor::ProcessEvent(PEVENT_RECORD pEventRecord)
{
    if (!pEventRecord)
    {
        return;
    }

    if (!IsEqualGUID(pEventRecord->EventHeader.ProviderId, KernelNetworkProviderGuid))
    {
        return;
    }

    if (pEventRecord->UserDataLength < sizeof(ULONG))
    {
        return;
    }

    ULONG size = *reinterpret_cast<PULONG>(pEventRecord->UserData);
    RecordTrafficEvent(pEventRecord->EventHeader.EventDescriptor.Id,
                       pEventRecord->EventHeader.ProcessId,
                       size);
}

ProcessTrafficStats EtwNetworkMonitor::GetProcessStats(DWORD pid) const
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    auto it = m_processStats.find(pid);
    if (it != m_processStats.end())
    {
        return it->second;
    }
    
    return ProcessTrafficStats();
}

std::map<DWORD, ProcessTrafficStats> EtwNetworkMonitor::GetAllStats() const
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_processStats;
}

void EtwNetworkMonitor::ResetStats()
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_processStats.clear();
}

std::wstring EtwNetworkMonitor::GetProcessName(DWORD pid) const
{
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(m_nameCacheMutex);
        auto it = m_processNameCache.find(pid);
        if (it != m_processNameCache.end())
        {
            return it->second;
        }
    }
    
    std::wstring name = L"<unknown>";
    
    if (pid == 0)
    {
        name = L"System Idle Process";
    }
    else if (pid == 4)
    {
        name = L"System";
    }
    else
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess)
        {
            wchar_t buffer[MAX_PATH] = {0};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size))
            {
                std::wstring path = buffer;
                size_t pos = path.find_last_of(L"\\/");
                if (pos != std::wstring::npos)
                {
                    name = path.substr(pos + 1);
                }
                else
                {
                    name = path;
                }
            }
            CloseHandle(hProcess);
        }
    }
    
    // Cache the result
    {
        std::lock_guard<std::mutex> lock(m_nameCacheMutex);
        m_processNameCache[pid] = name;
    }
    
    return name;
}

} // namespace NetPulse
