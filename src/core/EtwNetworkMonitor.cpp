#include "NetPulse/EtwNetworkMonitor.h"
#include "NetPulse/Utils.h"
#include <tdh.h>
#include <in6addr.h>
#include <psapi.h>

#pragma comment(lib, "tdh.lib")

namespace NetPulse
{

// Static instance for callback
EtwNetworkMonitor* EtwNetworkMonitor::s_instance = nullptr;

// Microsoft-Windows-Kernel-Network provider GUID
// {7dd42a49-5329-4832-8dfd-43d979153a88}
static const GUID KernelNetworkProviderGuid = 
    { 0x7dd42a49, 0x5329, 0x4832, { 0x8d, 0xfd, 0x43, 0xd9, 0x79, 0x15, 0x3a, 0x88 } };

// Event IDs for TCP/IP events
static const USHORT EVENT_ID_TCP_SEND = 10;
static const USHORT EVENT_ID_TCP_RECV = 11;
static const USHORT EVENT_ID_UDP_SEND = 12;
static const USHORT EVENT_ID_UDP_RECV = 13;

EtwNetworkMonitor::EtwNetworkMonitor()
    : m_sessionHandle(0)
    , m_traceHandle(INVALID_PROCESSTRACE_HANDLE)
    , m_running(false)
    , m_stopRequested(false)
{
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
    ControlTraceW(0, SESSION_NAME, pSessionProps, EVENT_TRACE_CONTROL_STOP);
    
    // Reset properties after stop attempt
    ZeroMemory(pSessionProps, bufferSize);
    pSessionProps->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    pSessionProps->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pSessionProps->Wnode.ClientContext = 1;
    pSessionProps->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    pSessionProps->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    
    // Start the trace session
    ULONG status = StartTraceW(&m_sessionHandle, SESSION_NAME, pSessionProps);
    if (status != ERROR_SUCCESS)
    {
        LogError(L"EtwNetworkMonitor::Start: StartTraceW failed with error " + std::to_wstring(status));
        s_instance = nullptr;
        return false;
    }
    
    // Enable the kernel network provider
    status = EnableTraceEx2(
        m_sessionHandle,
        &KernelNetworkProviderGuid,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION,
        0,  // MatchAnyKeyword
        0,  // MatchAllKeyword
        0,  // Timeout
        nullptr
    );
    
    if (status != ERROR_SUCCESS)
    {
        LogError(L"EtwNetworkMonitor::Start: EnableTraceEx2 failed with error " + std::to_wstring(status));
        ControlTraceW(m_sessionHandle, nullptr, pSessionProps, EVENT_TRACE_CONTROL_STOP);
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
        
        ControlTraceW(m_sessionHandle, nullptr, pSessionProps, EVENT_TRACE_CONTROL_STOP);
        m_sessionHandle = 0;
    }
    
    // Close the trace handle (this will cause ProcessTrace to return)
    if (m_traceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        CloseTrace(m_traceHandle);
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
    EVENT_TRACE_LOGFILEW traceLogfile = {0};
    traceLogfile.LoggerName = const_cast<LPWSTR>(SESSION_NAME);
    traceLogfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    traceLogfile.EventRecordCallback = EventRecordCallback;
    
    m_traceHandle = OpenTraceW(&traceLogfile);
    if (m_traceHandle == INVALID_PROCESSTRACE_HANDLE)
    {
        LogError(L"EtwNetworkMonitor::ProcessThreadProc: OpenTraceW failed");
        return;
    }
    
    // This will block until the trace is closed
    ULONG status = ProcessTrace(&m_traceHandle, 1, nullptr, nullptr);
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

void EtwNetworkMonitor::ProcessEvent(PEVENT_RECORD pEventRecord)
{
    if (!pEventRecord)
    {
        return;
    }

    // Check if this is from the kernel network provider
    if (!IsEqualGUID(pEventRecord->EventHeader.ProviderId, KernelNetworkProviderGuid))
    {
        return;
    }
    
    USHORT eventId = pEventRecord->EventHeader.EventDescriptor.Id;
    DWORD pid = pEventRecord->EventHeader.ProcessId;
    
    // Get the size from the event data
    // The structure varies by event type, but typically the size is at offset 0
    if (pEventRecord->UserDataLength < sizeof(ULONG))
    {
        return;
    }
    
    ULONG size = *reinterpret_cast<PULONG>(pEventRecord->UserData);
    
    bool isSend = (eventId == EVENT_ID_TCP_SEND || eventId == EVENT_ID_UDP_SEND);
    bool isRecv = (eventId == EVENT_ID_TCP_RECV || eventId == EVENT_ID_UDP_RECV);
    
    if (!isSend && !isRecv)
    {
        return;
    }
    
    // Update statistics
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
        it->second.bytesSent += size;
    }
    else if (isRecv)
    {
        it->second.bytesReceived += size;
    }
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
