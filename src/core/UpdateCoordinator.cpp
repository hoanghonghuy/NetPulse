#include "NetworkMonitor/UpdateCoordinator.h"
#include "NetworkMonitor/TrayIcon.h"
#include "NetworkMonitor/TaskbarOverlay.h"
#include "NetworkMonitor/PingMonitor.h"
#include "NetworkMonitor/Utils.h"
#include "../../resources/resource.h"

namespace NetworkMonitor
{

UpdateCoordinator::UpdateCoordinator()
    : m_pConfig(nullptr)
    , m_pNetworkMonitor(nullptr)
    , m_pTrayIcon(nullptr)
    , m_pOverlay(nullptr)
    , m_pPingMonitor(nullptr)
    , m_prevTotalBytesDown(0)
    , m_prevTotalBytesUp(0)
    , m_prevTotalsValid(false)
    , m_wasConnected(false)
{
}

UpdateCoordinator::~UpdateCoordinator()
{
}

void UpdateCoordinator::Initialize(
    const AppConfig* config,
    INetworkStatsProvider* networkMonitor,
    TrayIcon* trayIcon,
    TaskbarOverlay* overlay,
    PingMonitor* pingMonitor)
{
    m_pConfig = config;
    m_pNetworkMonitor = networkMonitor;
    m_pTrayIcon = trayIcon;
    m_pOverlay = overlay;
    m_pPingMonitor = pingMonitor;
}

void UpdateCoordinator::SetLogHistoryCallback(LogHistoryCallback callback)
{
    m_logHistoryCallback = callback;
}

void UpdateCoordinator::SetConnectionStatusCallback(ConnectionStatusCallback callback)
{
    m_connectionStatusCallback = callback;
}

void UpdateCoordinator::OnNetworkUpdateTick()
{
    if (!m_pNetworkMonitor)
    {
        return;
    }

    // Update network statistics
    m_pNetworkMonitor->Update();

    NetworkStats stats = GetCurrentStats();

    if (m_pConfig && m_pConfig->enableLogging)
    {
        // History logging: record per-interval usage
        unsigned long long totalDown = static_cast<unsigned long long>(stats.bytesReceived);
        unsigned long long totalUp = static_cast<unsigned long long>(stats.bytesSent);

        if (!m_prevTotalsValid)
        {
            m_prevTotalBytesDown = totalDown;
            m_prevTotalBytesUp = totalUp;
            m_prevTotalsValid = true;
        }
        else
        {
            unsigned long long deltaDown = 0;
            unsigned long long deltaUp = 0;

            if (totalDown >= m_prevTotalBytesDown)
            {
                deltaDown = totalDown - m_prevTotalBytesDown;
            }

            if (totalUp >= m_prevTotalBytesUp)
            {
                deltaUp = totalUp - m_prevTotalBytesUp;
            }

            if ((deltaDown > 0 || deltaUp > 0) && m_logHistoryCallback)
            {
                m_logHistoryCallback(deltaDown, deltaUp);
            }

            m_prevTotalBytesDown = totalDown;
            m_prevTotalBytesUp = totalUp;
        }
    }

    // Update tray icon
    UpdateTrayIcon(stats);

    // Update taskbar overlay
    UpdateTaskbarOverlay(stats);

    // Check connection status for notifications
    CheckConnectionStatus(stats.isActive);
}

void UpdateCoordinator::OnPingTick()
{
    if (!m_pPingMonitor)
    {
        return;
    }

    m_pPingMonitor->Update();

    // Update overlay with ping info
    if (m_pOverlay)
    {
        m_pOverlay->SetPingLatency(m_pPingMonitor->GetLatency());
    }
}

NetworkStats UpdateCoordinator::GetCurrentStats()
{
    NetworkStats stats;
    if (!m_pNetworkMonitor || !m_pConfig)
    {
        return stats;
    }

    bool useSpecificInterface = !m_pConfig->selectedInterface.empty();
    if (useSpecificInterface)
    {
        NetworkStats selectedStats;
        if (m_pNetworkMonitor->GetInterfaceStats(m_pConfig->selectedInterface, selectedStats))
        {
            stats = selectedStats;
        }
        else
        {
            stats = m_pNetworkMonitor->GetAggregatedStats();
        }
    }
    else
    {
        stats = m_pNetworkMonitor->GetAggregatedStats();
    }

    return stats;
}

void UpdateCoordinator::UpdateTrayIcon(const NetworkStats& stats)
{
    if (m_pTrayIcon && m_pConfig)
    {
        m_pTrayIcon->UpdateTooltip(stats, m_pConfig->displayUnit);
        m_pTrayIcon->UpdateIcon(stats.currentDownloadSpeed, stats.currentUploadSpeed);
    }
}

void UpdateCoordinator::UpdateTaskbarOverlay(const NetworkStats& stats)
{
    if (m_pOverlay && m_pConfig && m_pOverlay->IsVisible())
    {
        m_pOverlay->UpdateSpeed(
            stats.currentDownloadSpeed,
            stats.currentUploadSpeed,
            m_pConfig->displayUnit);
    }
}

void UpdateCoordinator::CheckConnectionStatus(bool hasActiveInterface)
{
    if (!m_pConfig || !m_pConfig->enableConnectionNotification)
    {
        m_wasConnected = hasActiveInterface;
        return;
    }

    if (m_wasConnected && !hasActiveInterface)
    {
        // Disconnected
        if (m_pTrayIcon)
        {
            std::wstring title = LoadStringResource(IDS_NOTIFICATION_DISCONNECTED_TITLE);
            std::wstring msg = LoadStringResource(IDS_NOTIFICATION_DISCONNECTED_MSG);
            if (title.empty()) title = L"Network Disconnected";
            if (msg.empty()) msg = L"No active network connection";
            m_pTrayIcon->ShowBalloonNotification(title, msg);
        }
        LogDebug(L"UpdateCoordinator::CheckConnectionStatus: Network disconnected");
    }
    else if (!m_wasConnected && hasActiveInterface)
    {
        // Reconnected
        if (m_pTrayIcon)
        {
            std::wstring title = LoadStringResource(IDS_NOTIFICATION_CONNECTED_TITLE);
            std::wstring msg = LoadStringResource(IDS_NOTIFICATION_CONNECTED_MSG);
            if (title.empty()) title = L"Network Connected";
            if (msg.empty()) msg = L"Network connection restored";
            m_pTrayIcon->ShowBalloonNotification(title, msg);
        }
        LogDebug(L"UpdateCoordinator::CheckConnectionStatus: Network connected");
    }

    m_wasConnected = hasActiveInterface;
}

} // namespace NetworkMonitor
