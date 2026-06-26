#include "NetPulse/Common.h"
#include "NetPulse/NetworkCalculator.h"
#include "TestUtils.h"

#include <thread>
#include <chrono>

using namespace NetPulse;

namespace NetPulseTests
{

void RunNetworkCalculatorTests()
{
    LogTestMessage(L"=== NetworkCalculator tests ===");

    NetworkCalculator calc;
    NetworkStats stats;

    // First update initializes stats without computing speed
    bool okInit = calc.UpdateStats(stats, 100000ULL, 50000ULL);
    AssertTrue(okInit, L"NetworkCalculator first UpdateStats returns true");
    AssertTrue(stats.currentDownloadSpeed == 0.0 && stats.currentUploadSpeed == 0.0,
               L"NetworkCalculator initial speeds are zero");

    // Wait a bit to ensure timeElapsed >= 0.1s
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    bool okUpdate = calc.UpdateStats(stats, 101000ULL, 50500ULL);
    AssertTrue(okUpdate, L"NetworkCalculator second UpdateStats returns true");
    AssertTrue(stats.currentDownloadSpeed > 0.0 && stats.currentUploadSpeed > 0.0,
               L"NetworkCalculator computes positive speeds");
    AssertTrue(stats.peakDownloadSpeed >= stats.currentDownloadSpeed &&
               stats.peakUploadSpeed >= stats.currentUploadSpeed,
               L"NetworkCalculator peak speeds >= current speeds");

    // Aggregate calculation
    NetworkStats s1 = stats;
    s1.isPhysicalHardware = true;
    NetworkStats s2 = stats;
    s2.isPhysicalHardware = true;
    s2.currentDownloadSpeed *= 2.0;
    s2.currentUploadSpeed *= 2.0;

    std::vector<NetworkStats> list;
    list.push_back(s1);
    list.push_back(s2);

    NetworkStats agg = calc.CalculateAggregate(list);
    AssertTrue(agg.currentDownloadSpeed == s1.currentDownloadSpeed + s2.currentDownloadSpeed &&
               agg.currentUploadSpeed == s1.currentUploadSpeed + s2.currentUploadSpeed,
               L"NetworkCalculator aggregate sums speeds");

    NetworkStats emptyAgg = NetworkCalculator::CalculateAggregate(std::vector<NetworkStats>());
    AssertTrue(emptyAgg.currentDownloadSpeed == 0.0 && emptyAgg.currentUploadSpeed == 0.0,
               L"NetworkCalculator aggregate empty list has zero speeds");

    NetworkStats active;
    active.isActive = true;
    active.currentDownloadSpeed = 10.0;
    active.currentUploadSpeed = 5.0;
    active.peakDownloadSpeed = 10.0;
    active.peakUploadSpeed = 5.0;

    NetworkStats inactive = active;
    inactive.isActive = false;

    std::vector<NetworkStats> mixed;
    mixed.push_back(active);
    mixed.push_back(inactive);

    NetworkStats mixedAgg = NetworkCalculator::CalculateAggregate(mixed);
    AssertTrue(mixedAgg.currentDownloadSpeed == 10.0 && mixedAgg.currentUploadSpeed == 5.0,
               L"NetworkCalculator aggregate ignores inactive interfaces");

    // Physical-only aggregate: virtual interfaces are excluded when physical exist
    NetworkStats physical;
    physical.isActive = true;
    physical.isPhysicalHardware = true;
    physical.currentDownloadSpeed = 100.0;
    physical.currentUploadSpeed = 50.0;

    NetworkStats virtualIface = physical;
    virtualIface.isPhysicalHardware = false;
    virtualIface.currentDownloadSpeed = 200.0;
    virtualIface.currentUploadSpeed = 100.0;

    std::vector<NetworkStats> physVirtMixed;
    physVirtMixed.push_back(physical);
    physVirtMixed.push_back(virtualIface);

    NetworkStats physOnlyAgg = NetworkCalculator::CalculateAggregate(physVirtMixed);
    AssertTrue(physOnlyAgg.currentDownloadSpeed == 100.0 && physOnlyAgg.currentUploadSpeed == 50.0,
               L"NetworkCalculator aggregate excludes virtual when physical present");

    // Fallback: no physical -> aggregate all active
    NetworkStats virtualOnly1;
    virtualOnly1.isActive = true;
    virtualOnly1.isPhysicalHardware = false;
    virtualOnly1.currentDownloadSpeed = 30.0;
    virtualOnly1.currentUploadSpeed = 15.0;

    NetworkStats virtualOnly2 = virtualOnly1;
    virtualOnly2.currentDownloadSpeed = 70.0;
    virtualOnly2.currentUploadSpeed = 35.0;

    std::vector<NetworkStats> allVirtual;
    allVirtual.push_back(virtualOnly1);
    allVirtual.push_back(virtualOnly2);

    NetworkStats fallbackAgg = NetworkCalculator::CalculateAggregate(allVirtual);
    AssertTrue(fallbackAgg.currentDownloadSpeed == 100.0 && fallbackAgg.currentUploadSpeed == 50.0,
               L"NetworkCalculator aggregate falls back to all active when no physical present");

    NetworkStats rollover;
    NetworkCalculator::UpdateStats(rollover, 1000ULL, 500ULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    bool rolloverOk = NetworkCalculator::UpdateStats(rollover, 200ULL, 100ULL);
    AssertTrue(rolloverOk, L"NetworkCalculator handles counter rollover");
    AssertTrue(rollover.currentDownloadSpeed >= 0.0 && rollover.currentUploadSpeed >= 0.0,
               L"NetworkCalculator rollover speeds are non-negative");

    NetworkStats tooFast;
    NetworkCalculator::UpdateStats(tooFast, 1000ULL, 500ULL);
    bool tooSoon = NetworkCalculator::UpdateStats(tooFast, 1100ULL, 550ULL);
    AssertTrue(!tooSoon, L"NetworkCalculator rejects updates with elapsed < 0.1s");

    NetworkStats peakStats;
    NetworkCalculator::UpdateStats(peakStats, 1000ULL, 500ULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    NetworkCalculator::UpdateStats(peakStats, 2000ULL, 1000ULL);
    double firstPeakDown = peakStats.peakDownloadSpeed;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    NetworkCalculator::UpdateStats(peakStats, 2100ULL, 1050ULL);
    AssertTrue(peakStats.peakDownloadSpeed >= firstPeakDown,
               L"NetworkCalculator peak download speed is monotonic");

    NetworkStats resetStats;
    NetworkCalculator::UpdateStats(resetStats, 1000ULL, 500ULL);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    NetworkCalculator::UpdateStats(resetStats, 2000ULL, 1000ULL);
    NetworkCalculator::ResetStats(resetStats);
    AssertTrue(resetStats.currentDownloadSpeed == 0.0 && resetStats.currentUploadSpeed == 0.0,
               L"NetworkCalculator ResetStats clears current speeds");
}

} // namespace NetPulseTests
