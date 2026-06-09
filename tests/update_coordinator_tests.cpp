#include "NetPulse/UpdateCoordinator.h"
#include "NetPulse/HistoryLogger.h"
#include "NetPulse/TaskbarOverlay.h"
#include "test_fakes/FakeNetworkStatsProvider.h"
#include "TestUtils.h"

#include <windows.h>
#include <ctime>

using namespace NetPulse;

namespace NetPulseTests
{

void RunUpdateCoordinatorTests()
{
    LogTestMessage(L"=== UpdateCoordinator tests ===");

    FakeNetworkStatsProvider provider;
    provider.m_aggregated.bytesReceived = 1000ULL;
    provider.m_aggregated.bytesSent = 500ULL;
    provider.m_aggregated.isActive = true;

    AppConfig config;
    config.enableLogging = true;
    config.enableDataUsageAlerts = false;
    config.enableConnectionNotification = false;
    config.selectedInterface.clear();

    UpdateCoordinator coordinator;
    coordinator.Initialize(&config, &provider, nullptr, nullptr, nullptr);

    int logCallbackCount = 0;
    unsigned long long lastDeltaDown = 0;
    unsigned long long lastDeltaUp = 0;
    std::wstring lastScope;

    coordinator.SetLogHistoryCallback(
        [&](unsigned long long bytesDown, unsigned long long bytesUp, const std::wstring& interfaceName)
        {
            ++logCallbackCount;
            lastDeltaDown = bytesDown;
            lastDeltaUp = bytesUp;
            lastScope = interfaceName;
        });

    coordinator.OnNetworkUpdateTick();
    AssertTrue(logCallbackCount == 0, L"UpdateCoordinator first tick establishes baseline only");
    AssertTrue(provider.m_updateCallCount == 1, L"UpdateCoordinator calls provider Update");

    provider.m_aggregated.bytesReceived = 2500ULL;
    provider.m_aggregated.bytesSent = 900ULL;
    coordinator.OnNetworkUpdateTick();
    AssertTrue(logCallbackCount == 1, L"UpdateCoordinator logs delta on second tick");
    AssertTrue(lastDeltaDown == 1500ULL && lastDeltaUp == 400ULL,
               L"UpdateCoordinator logs correct byte deltas");
    AssertTrue(!lastScope.empty(), L"UpdateCoordinator logs non-empty interface scope");

    NetworkStats current = coordinator.GetCurrentStats();
    AssertTrue(current.bytesReceived == 2500ULL && current.bytesSent == 900ULL,
               L"UpdateCoordinator GetCurrentStats returns aggregated stats");

    NetPulse::NetworkStats wifiStats;
    wifiStats.interfaceName = L"Wi-Fi";
    wifiStats.bytesReceived = 9000ULL;
    wifiStats.bytesSent = 3000ULL;
    wifiStats.isActive = true;
    provider.m_interfaces[L"Wi-Fi"] = wifiStats;
    config.selectedInterface = L"Wi-Fi";

    coordinator.OnNetworkUpdateTick();
    AssertTrue(logCallbackCount == 1, L"UpdateCoordinator resets baseline when logging scope changes");

    provider.m_interfaces[L"Wi-Fi"].bytesReceived = 10000ULL;
    provider.m_interfaces[L"Wi-Fi"].bytesSent = 3500ULL;
    coordinator.OnNetworkUpdateTick();
    AssertTrue(logCallbackCount == 2, L"UpdateCoordinator logs after interface scope baseline");
    AssertTrue(lastScope == L"Wi-Fi", L"UpdateCoordinator logs selected interface name");
    AssertTrue(lastDeltaDown == 1000ULL && lastDeltaUp == 500ULL,
               L"UpdateCoordinator logs per-interface deltas");

    current = coordinator.GetCurrentStats();
    AssertTrue(current.bytesReceived == 10000ULL, L"UpdateCoordinator GetCurrentStats uses selected interface");

    config.selectedInterface = L"MissingAdapter";
    coordinator.OnNetworkUpdateTick();
    current = coordinator.GetCurrentStats();
    AssertTrue(current.bytesReceived == provider.m_aggregated.bytesReceived,
               L"UpdateCoordinator falls back to aggregate when selected interface missing");

    config.selectedInterface.clear();
    coordinator.OnNetworkUpdateTick();
    provider.m_aggregated.bytesReceived = 6000ULL;
    provider.m_aggregated.bytesSent = 1000ULL;
    coordinator.OnNetworkUpdateTick();
    provider.m_aggregated.bytesReceived = 1000ULL;
    provider.m_aggregated.bytesSent = 200ULL;
    coordinator.OnNetworkUpdateTick();
    AssertTrue(logCallbackCount == 3,
               L"UpdateCoordinator counter reset does not append negative delta");

    FakeNetworkStatsProvider connectionProvider;
    connectionProvider.m_aggregated.isActive = true;

    AppConfig connectionConfig;
    connectionConfig.enableLogging = false;
    connectionConfig.enableDataUsageAlerts = false;
    connectionConfig.enableConnectionNotification = true;

    UpdateCoordinator connectionCoordinator;
    connectionCoordinator.Initialize(&connectionConfig, &connectionProvider, nullptr, nullptr, nullptr);

    int connectionCallbacks = 0;
    bool lastConnected = true;
    connectionCoordinator.SetConnectionStatusCallback(
        [&](bool isConnected)
        {
            ++connectionCallbacks;
            lastConnected = isConnected;
        });

    connectionCoordinator.OnNetworkUpdateTick();
    AssertTrue(connectionCallbacks == 0,
               L"UpdateCoordinator startup does not fire connection notification");

    connectionProvider.m_aggregated.isActive = false;
    connectionCoordinator.OnNetworkUpdateTick();
    AssertTrue(connectionCallbacks == 1 && !lastConnected,
               L"UpdateCoordinator disconnect notification callback");

    connectionProvider.m_aggregated.isActive = true;
    connectionCoordinator.OnNetworkUpdateTick();
    AssertTrue(connectionCallbacks == 2 && lastConnected,
               L"UpdateCoordinator reconnect notification callback");

    HistoryLogger::Instance().DeleteAll();
    HistoryLogger::Instance().AppendSample(L"All Interfaces", 800ULL, 0ULL);

    FakeNetworkStatsProvider usageProvider;
    usageProvider.m_aggregated.isActive = true;

    AppConfig usageConfig;
    usageConfig.enableLogging = false;
    usageConfig.enableDataUsageAlerts = true;
    usageConfig.enableConnectionNotification = false;
    usageConfig.dataQuotaGB = 1000.0 / (1024.0 * 1024.0 * 1024.0);
    usageConfig.dataAlertThreshold1 = 80;
    usageConfig.dataAlertThreshold2 = 100;

    UpdateCoordinator usageCoordinator;
    usageCoordinator.Initialize(&usageConfig, &usageProvider, nullptr, nullptr, nullptr);

    int alertCallbacks = 0;
    int lastAlertThreshold = 0;
    int lastAlertPercent = 0;
    usageCoordinator.SetDataUsageAlertCallback(
        [&](int thresholdPercent, int currentPercent)
        {
            ++alertCallbacks;
            lastAlertThreshold = thresholdPercent;
            lastAlertPercent = currentPercent;
        });

    usageCoordinator.OnNetworkUpdateTick();
    AssertTrue(alertCallbacks == 1 && lastAlertThreshold == 80,
               L"UpdateCoordinator data usage alert at 80%");
    AssertTrue(lastAlertPercent >= 80, L"UpdateCoordinator reports current usage percent");

    usageCoordinator.OnNetworkUpdateTick();
    AssertTrue(alertCallbacks == 1, L"UpdateCoordinator does not repeat 80% alert");

    HistoryLogger::Instance().DeleteAll();
    HistoryLogger::Instance().AppendSample(L"All Interfaces", 1000ULL, 0ULL);
    usageCoordinator.OnNetworkUpdateTick();
    AssertTrue(alertCallbacks == 2 && lastAlertThreshold == 100,
               L"UpdateCoordinator data usage alert at 100%");

    HistoryLogger::Instance().DeleteAll();
    HistoryLogger::Instance().AppendSample(L"All Interfaces", 800ULL, 0ULL);

    UpdateCoordinator monthResetCoordinator;
    monthResetCoordinator.Initialize(&usageConfig, &usageProvider, nullptr, nullptr, nullptr);

    int monthResetAlertCallbacks = 0;
    int monthResetAlertThreshold = 0;
    monthResetCoordinator.SetDataUsageAlertCallback(
        [&](int thresholdPercent, int /*currentPercent*/)
        {
            ++monthResetAlertCallbacks;
            monthResetAlertThreshold = thresholdPercent;
        });

    monthResetCoordinator.OnNetworkUpdateTick();
    AssertTrue(monthResetAlertCallbacks == 1 && monthResetAlertThreshold == 80,
               L"UpdateCoordinator month reset test establishes 80% alert");

    monthResetCoordinator.OnNetworkUpdateTick();
    AssertTrue(monthResetAlertCallbacks == 1,
               L"UpdateCoordinator month reset test does not repeat 80% alert");

    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    AssertTrue(localtime_s(&localTime, &now) == 0,
               L"UpdateCoordinator billing month test reads local time");
    int currentMonthKey = (localTime.tm_year + 1900) * 100 + (localTime.tm_mon + 1);
    int previousMonthKey = currentMonthKey - 1;
    if (localTime.tm_mon == 0)
    {
        previousMonthKey = (localTime.tm_year + 1900 - 1) * 100 + 12;
    }

    monthResetCoordinator.SetBillingMonthKeyForTest(previousMonthKey);
    monthResetCoordinator.OnNetworkUpdateTick();
    AssertTrue(monthResetAlertCallbacks == 2 && monthResetAlertThreshold == 80,
               L"UpdateCoordinator resets data usage alerts when billing month changes");

    monthResetCoordinator.OnNetworkUpdateTick();
    AssertTrue(monthResetAlertCallbacks == 2,
               L"UpdateCoordinator does not repeat alert again within same billing month");

    HistoryLogger::Instance().DeleteAll();
    HistoryLogger::Instance().AppendSample(L"Wi-Fi", 800ULL, 0ULL);
    HistoryLogger::Instance().AppendSample(L"Ethernet", 5000ULL, 0ULL);

    AppConfig ifaceUsageConfig = usageConfig;
    ifaceUsageConfig.selectedInterface = L"Wi-Fi";

    UpdateCoordinator ifaceUsageCoordinator;
    ifaceUsageCoordinator.Initialize(&ifaceUsageConfig, &usageProvider, nullptr, nullptr, nullptr);

    int ifaceAlertCallbacks = 0;
    int ifaceAlertThreshold = 0;
    ifaceUsageCoordinator.SetDataUsageAlertCallback(
        [&](int thresholdPercent, int /*currentPercent*/)
        {
            ++ifaceAlertCallbacks;
            ifaceAlertThreshold = thresholdPercent;
        });

    ifaceUsageCoordinator.OnNetworkUpdateTick();
    AssertTrue(ifaceAlertCallbacks == 1 && ifaceAlertThreshold == 80,
               L"UpdateCoordinator quota alert uses selected interface filter");

    TaskbarOverlay overlay;
    if (overlay.Initialize(GetModuleHandleW(nullptr)))
    {
        overlay.Show(true);

        FakeNetworkStatsProvider overlayProvider;
        overlayProvider.m_aggregated.isActive = true;
        overlayProvider.m_aggregated.currentDownloadSpeed = 2048.0;
        overlayProvider.m_aggregated.currentUploadSpeed = 1024.0;

        AppConfig overlayConfig;
        overlayConfig.enableLogging = false;
        overlayConfig.enableDataUsageAlerts = false;
        overlayConfig.displayUnit = SpeedUnit::KiloBytesPerSecond;

        UpdateCoordinator overlayCoordinator;
        overlayCoordinator.Initialize(&overlayConfig, &overlayProvider, nullptr, &overlay, nullptr);
        overlayCoordinator.OnNetworkUpdateTick();
        AssertTrue(overlay.IsUserWantsVisible(),
                   L"UpdateCoordinator overlay update path runs when user wants visible");
        overlay.Cleanup();
    }
}

} // namespace NetPulseTests
