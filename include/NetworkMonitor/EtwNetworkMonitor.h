#ifndef NETWORK_MONITOR_ETW_NETWORK_MONITOR_H
#define NETWORK_MONITOR_ETW_NETWORK_MONITOR_H

#include "NetworkMonitor/Common.h"
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

#pragma comment(lib, "advapi32.lib")

namespace NetworkMonitor
{

/**
 * Per-process traffic statistics
 */
struct ProcessTrafficStats
{
    DWORD processId;
    std::wstring processName;
    std::atomic<uint64_t> bytesSent;
    std::atomic<uint64_t> bytesReceived;
    
    ProcessTrafficStats() 
        : processId(0)
        , bytesSent(0)
        , bytesReceived(0)
    {}
    
    ProcessTrafficStats(const ProcessTrafficStats& other)
        : processId(other.processId)
        , processName(other.processName)
        , bytesSent(other.bytesSent.load())
        , bytesReceived(other.bytesReceived.load())
    {}

    ProcessTrafficStats& operator=(const ProcessTrafficStats& other)
    {
        if (this != &other)
        {
            processId = other.processId;
            processName = other.processName;
            bytesSent.store(other.bytesSent.load());
            bytesReceived.store(other.bytesReceived.load());
        }
        return *this;
    }
};

/**
 * EtwNetworkMonitor - Monitors per-process network traffic using ETW
 * 
 * Uses the Microsoft-Windows-Kernel-Network provider to capture
 * TcpIp send/receive events with process IDs and byte counts.
 */
class EtwNetworkMonitor
{
public:
    EtwNetworkMonitor();
    ~EtwNetworkMonitor();

    /**
     * Start the ETW trace session
     * @return true if successful
     */
    bool Start();

    /**
     * Stop the ETW trace session
     */
    void Stop();

    /**
     * Check if the monitor is running
     */
    bool IsRunning() const { return m_running; }

    /**
     * Get traffic stats for a specific process
     */
    ProcessTrafficStats GetProcessStats(DWORD pid) const;

    /**
     * Get all process traffic stats
     */
    std::map<DWORD, ProcessTrafficStats> GetAllStats() const;

    /**
     * Reset all statistics
     */
    void ResetStats();

    /**
     * Get process name by PID
     */
    std::wstring GetProcessName(DWORD pid) const;

private:
    static void WINAPI EventRecordCallback(PEVENT_RECORD pEventRecord);
    void ProcessEvent(PEVENT_RECORD pEventRecord);
    void ProcessThreadProc();

    // Session management
    TRACEHANDLE m_sessionHandle;
    TRACEHANDLE m_traceHandle;
    std::thread m_processThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_stopRequested;

    // Statistics storage
    mutable std::mutex m_statsMutex;
    std::map<DWORD, ProcessTrafficStats> m_processStats;
    
    // Process name cache
    mutable std::mutex m_nameCacheMutex;
    mutable std::map<DWORD, std::wstring> m_processNameCache;

    // Session name
    static constexpr wchar_t SESSION_NAME[] = L"NetworkMonitorEtwSession";
    
    // Instance pointer for static callback
    static EtwNetworkMonitor* s_instance;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_ETW_NETWORK_MONITOR_H
