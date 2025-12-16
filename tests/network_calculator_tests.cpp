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
    NetworkStats s2 = stats;
    s2.currentDownloadSpeed *= 2.0;
    s2.currentUploadSpeed *= 2.0;

    std::vector<NetworkStats> list;
    list.push_back(s1);
    list.push_back(s2);

    NetworkStats agg = calc.CalculateAggregate(list);
    AssertTrue(agg.currentDownloadSpeed == s1.currentDownloadSpeed + s2.currentDownloadSpeed &&
               agg.currentUploadSpeed == s1.currentUploadSpeed + s2.currentUploadSpeed,
               L"NetworkCalculator aggregate sums speeds");
}

} // namespace NetPulseTests
