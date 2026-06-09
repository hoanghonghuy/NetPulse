#include "NetPulse/SystemMonitor.h"
#include "NetPulse/Utils.h"

namespace NetPulse
{

SystemMonitor::SystemMonitor()
    : m_initialized(false)
    , m_prevTimesValid(false)
    , m_cpuPercent(0.0)
    , m_totalRAM(0)
    , m_usedRAM(0)
    , m_ramPercent(0.0)
{
    m_prevIdleTime.QuadPart = 0;
    m_prevKernelTime.QuadPart = 0;
    m_prevUserTime.QuadPart = 0;
}

SystemMonitor::~SystemMonitor()
{
    Shutdown();
}

bool SystemMonitor::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    // Get initial CPU times
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        m_prevIdleTime.LowPart = idleTime.dwLowDateTime;
        m_prevIdleTime.HighPart = idleTime.dwHighDateTime;
        m_prevKernelTime.LowPart = kernelTime.dwLowDateTime;
        m_prevKernelTime.HighPart = kernelTime.dwHighDateTime;
        m_prevUserTime.LowPart = userTime.dwLowDateTime;
        m_prevUserTime.HighPart = userTime.dwHighDateTime;
        m_prevTimesValid = true;
    }

    m_initialized = true;
    LogDebug(L"SystemMonitor::Initialize: Initialized successfully");
    return true;
}

void SystemMonitor::Shutdown()
{
    m_initialized = false;
    m_prevTimesValid = false;
}

void SystemMonitor::Update()
{
    if (!m_initialized)
    {
        Initialize();
    }

    // === Update CPU usage ===
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        ULARGE_INTEGER currIdleTime, currKernelTime, currUserTime;
        currIdleTime.LowPart = idleTime.dwLowDateTime;
        currIdleTime.HighPart = idleTime.dwHighDateTime;
        currKernelTime.LowPart = kernelTime.dwLowDateTime;
        currKernelTime.HighPart = kernelTime.dwHighDateTime;
        currUserTime.LowPart = userTime.dwLowDateTime;
        currUserTime.HighPart = userTime.dwHighDateTime;

        if (m_prevTimesValid)
        {
            ULONGLONG idleDiff = currIdleTime.QuadPart - m_prevIdleTime.QuadPart;
            ULONGLONG kernelDiff = currKernelTime.QuadPart - m_prevKernelTime.QuadPart;
            ULONGLONG userDiff = currUserTime.QuadPart - m_prevUserTime.QuadPart;

            // Kernel time includes idle time, so we need to subtract it
            ULONGLONG totalSystem = kernelDiff + userDiff;
            ULONGLONG totalActive = totalSystem - idleDiff;

            if (totalSystem > 0)
            {
                m_cpuPercent = (static_cast<double>(totalActive) / static_cast<double>(totalSystem)) * 100.0;
                if (m_cpuPercent < 0.0) m_cpuPercent = 0.0;
                if (m_cpuPercent > 100.0) m_cpuPercent = 100.0;
            }
        }

        m_prevIdleTime = currIdleTime;
        m_prevKernelTime = currKernelTime;
        m_prevUserTime = currUserTime;
        m_prevTimesValid = true;
    }

    // === Update RAM usage ===
    MEMORYSTATUSEX memInfo = {};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo))
    {
        m_totalRAM = memInfo.ullTotalPhys;
        m_usedRAM = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
        
        if (m_totalRAM > 0)
        {
            m_ramPercent = (static_cast<double>(m_usedRAM) / static_cast<double>(m_totalRAM)) * 100.0;
        }
    }
}

} // namespace NetPulse
