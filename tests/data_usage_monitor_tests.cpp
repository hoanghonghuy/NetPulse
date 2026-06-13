#include "NetPulse/DataUsageMonitor.h"
#include "TestUtils.h"

using namespace NetPulse;

namespace NetPulseTests
{

static const uint64_t kTestQuota = 1000ULL;

void RunDataUsageMonitorTests()
{
    LogTestMessage(L"=== DataUsageMonitor tests ===");

    DataUsageMonitor monitor;

    monitor.SetQuota(0);
    AssertTrue(!monitor.IsEnabled(), L"DataUsageMonitor disabled when quota is zero");
    AssertTrue(monitor.GetUsagePercentage() == 0, L"DataUsageMonitor percentage is zero when disabled");
    AssertTrue(!monitor.Update(500ULL), L"DataUsageMonitor Update returns false when disabled");

    monitor.SetQuota(kTestQuota);
    monitor.SetAlertThresholds({80, 100});
    AssertTrue(monitor.IsEnabled(), L"DataUsageMonitor enabled when quota > 0");

    AssertTrue(!monitor.Update(799ULL), L"DataUsageMonitor no alert below threshold");
    int threshold = 0;
    AssertTrue(!monitor.ShouldAlert(threshold), L"DataUsageMonitor ShouldAlert false below threshold");

    AssertTrue(monitor.Update(800ULL), L"DataUsageMonitor Update true at 80%");
    AssertTrue(monitor.ShouldAlert(threshold) && threshold == 80,
               L"DataUsageMonitor alerts once at 80%");
    AssertTrue(!monitor.ShouldAlert(threshold), L"DataUsageMonitor ShouldAlert clears pending alert");

    AssertTrue(!monitor.Update(800ULL), L"DataUsageMonitor does not re-alert at same threshold");
    AssertTrue(monitor.GetUsagePercentage() == 80, L"DataUsageMonitor reports 80% usage");

    AssertTrue(monitor.Update(1000ULL), L"DataUsageMonitor Update true at 100%");
    AssertTrue(monitor.ShouldAlert(threshold) && threshold == 100,
               L"DataUsageMonitor alerts at 100% after 80%");

    monitor.ResetAlerts();
    AssertTrue(monitor.Update(800ULL), L"DataUsageMonitor can alert again after ResetAlerts");
    AssertTrue(monitor.ShouldAlert(threshold) && threshold == 80,
               L"DataUsageMonitor re-alerts at 80% after reset");

    DataUsageMonitor sortedMonitor;
    sortedMonitor.SetQuota(kTestQuota);
    sortedMonitor.SetAlertThresholds({100, 80, 80});
    AssertTrue(sortedMonitor.Update(800ULL), L"DataUsageMonitor sorts thresholds and alerts at 80%");
    AssertTrue(sortedMonitor.ShouldAlert(threshold) && threshold == 80,
               L"DataUsageMonitor sorted thresholds alert at lowest crossed value");
    AssertTrue(!sortedMonitor.Update(800ULL), L"DataUsageMonitor duplicate thresholds alert only once");
}

} // namespace NetPulseTests
