#ifndef NETWORK_MONITOR_UPDATECOORDINATOR_H
#define NETWORK_MONITOR_UPDATECOORDINATOR_H

#include "NetworkMonitor/Common.h"
#include "NetworkMonitor/Interfaces/INetworkStatsProvider.h"
#include "NetworkMonitor/DataUsageMonitor.h"
#include <functional>
#include <memory>

namespace NetworkMonitor
{

class TrayIcon;
class TaskbarOverlay;
class PingMonitor;

/**
 * UpdateCoordinator - Manages network update cycles and logging
 * 
 * This class is responsible for coordinating periodic updates of network
 * statistics, ping monitoring, and history logging, following the
 * Single Responsibility Principle.
 */
class UpdateCoordinator
{
public:
    // Callback for logging history samples
    using LogHistoryCallback = std::function<void(unsigned long long bytesDown, unsigned long long bytesUp)>;
    // Callback for connection status changes
    using ConnectionStatusCallback = std::function<void(bool isConnected)>;
    // Callback for data usage alerts
    using DataUsageAlertCallback = std::function<void(int thresholdPercent, int currentPercent)>;

    UpdateCoordinator();
    ~UpdateCoordinator();

    /**
     * Initialize the coordinator with required dependencies
     */
    void Initialize(
        const AppConfig* config,
        INetworkStatsProvider* networkMonitor,
        TrayIcon* trayIcon,
        TaskbarOverlay* overlay,
        PingMonitor* pingMonitor
    );

    /**
     * Set callback for history logging
     */
    void SetLogHistoryCallback(LogHistoryCallback callback);

    /**
     * Set callback for connection status changes
     */
    void SetConnectionStatusCallback(ConnectionStatusCallback callback);

    /**
     * Set callback for data usage alerts
     */
    void SetDataUsageAlertCallback(DataUsageAlertCallback callback);

    /**
     * Get current month's total usage in bytes (for UI display)
     */
    uint64_t GetCurrentMonthUsage() const { return m_currentMonthUsageBytes; }

    /**
     * Called on network update timer tick
     */
    void OnNetworkUpdateTick();

    /**
     * Called on ping timer tick
     */
    void OnPingTick();

    /**
     * Apply overlay style from current config (call after settings change)
     */
    void ApplyOverlayStyleFromConfig();

private:
    /**
     * Get current network stats based on config (single interface or aggregated)
     */
    NetworkStats GetCurrentStats();

    /**
     * Update tray icon with current stats
     */
    void UpdateTrayIcon(const NetworkStats& stats);

    /**
     * Update taskbar overlay with current stats
     */
    void UpdateTaskbarOverlay(const NetworkStats& stats);

    /**
     * Check and report connection status changes
     */
    void CheckConnectionStatus(bool hasActiveInterface);

    const AppConfig* m_pConfig;
    INetworkStatsProvider* m_pNetworkMonitor;
    TrayIcon* m_pTrayIcon;
    TaskbarOverlay* m_pOverlay;
    PingMonitor* m_pPingMonitor;

    // Previous totals for logging per-interval usage
    unsigned long long m_prevTotalBytesDown;
    unsigned long long m_prevTotalBytesUp;
    bool m_prevTotalsValid;

    // Connection state tracking
    bool m_wasConnected;

    // Callbacks
    LogHistoryCallback m_logHistoryCallback;
    ConnectionStatusCallback m_connectionStatusCallback;
    DataUsageAlertCallback m_dataUsageAlertCallback;

    // Data usage monitoring
    std::unique_ptr<DataUsageMonitor> m_pDataUsageMonitor;
    uint64_t m_currentMonthUsageBytes;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_UPDATECOORDINATOR_H
