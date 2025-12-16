#ifndef NETWORK_MONITOR_SYSTEM_MONITOR_H
#define NETWORK_MONITOR_SYSTEM_MONITOR_H

#include "NetPulse/Common.h"

namespace NetPulse
{

/**
 * SystemMonitor - Monitors CPU and RAM usage
 * 
 * Uses Windows PDH (Performance Data Helper) for CPU
 * and GlobalMemoryStatusEx for RAM.
 */
class SystemMonitor
{
public:
    SystemMonitor();
    ~SystemMonitor();

    /**
     * Initialize the monitor (must be called before Update)
     */
    bool Initialize();

    /**
     * Shutdown and clean up resources
     */
    void Shutdown();

    /**
     * Update CPU and RAM readings
     */
    void Update();

    /**
     * Get current CPU usage percentage (0-100)
     */
    double GetCPUPercent() const { return m_cpuPercent; }

    /**
     * Get current RAM usage percentage (0-100)
     */
    double GetRAMPercent() const { return m_ramPercent; }

    /**
     * Get total physical RAM in bytes
     */
    uint64_t GetTotalRAM() const { return m_totalRAM; }

    /**
     * Get used physical RAM in bytes
     */
    uint64_t GetUsedRAM() const { return m_usedRAM; }

private:
    bool m_initialized;
    
    // CPU monitoring using GetSystemTimes
    ULARGE_INTEGER m_prevIdleTime;
    ULARGE_INTEGER m_prevKernelTime;
    ULARGE_INTEGER m_prevUserTime;
    bool m_prevTimesValid;
    double m_cpuPercent;

    // RAM monitoring
    uint64_t m_totalRAM;
    uint64_t m_usedRAM;
    double m_ramPercent;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_SYSTEM_MONITOR_H
