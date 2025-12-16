#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/HistoryLogger.h"
#include "TestUtils.h"

#include <vector>

using namespace NetPulse;

namespace NetPulseTests
{

void RunHistoryLoggerTests()
{
    LogTestMessage(L"=== HistoryLogger tests ===");

    HistoryLogger& logger = HistoryLogger::Instance();
    const std::wstring ifaceName = L"TestIface";

    // Phase A: totals today & this month
    bool cleared = logger.DeleteAll();
    AssertTrue(cleared, L"HistoryLogger.DeleteAll succeeds");

    logger.AppendSample(ifaceName, 1000ULL, 500ULL);
    logger.AppendSample(ifaceName, 4000ULL, 1500ULL);

    unsigned long long totalDownToday = 0;
    unsigned long long totalUpToday = 0;
    bool okToday = logger.GetTotalsToday(totalDownToday, totalUpToday, &ifaceName);
    AssertTrue(okToday, L"HistoryLogger.GetTotalsToday returns true");
    AssertTrue(totalDownToday >= 5000ULL && totalUpToday >= 2000ULL,
               L"HistoryLogger totals today >= inserted bytes");

    unsigned long long totalDownMonth = 0;
    unsigned long long totalUpMonth = 0;
    bool okMonth = logger.GetTotalsThisMonth(totalDownMonth, totalUpMonth, &ifaceName);
    AssertTrue(okMonth, L"HistoryLogger.GetTotalsThisMonth returns true");
    AssertTrue(totalDownMonth >= 5000ULL && totalUpMonth >= 2000ULL,
               L"HistoryLogger totals this month >= inserted bytes");

    // Phase B: TrimToRecentDays behaviour
    cleared = logger.DeleteAll();
    AssertTrue(cleared, L"HistoryLogger.DeleteAll before trim tests");

    logger.AppendSample(ifaceName, 2000ULL, 1000ULL);
    std::vector<HistorySample> samples;
    bool okRecent = logger.GetRecentSamples(10, samples, &ifaceName, false);
    AssertTrue(okRecent, L"HistoryLogger.GetRecentSamples before trim returns true");
    AssertTrue(!samples.empty(), L"HistoryLogger.GetRecentSamples before trim has data");

    bool trimmed0 = logger.TrimToRecentDays(0);
    AssertTrue(trimmed0, L"HistoryLogger.TrimToRecentDays(0) returns true");

    samples.clear();
    okRecent = logger.GetRecentSamples(10, samples, &ifaceName, false);
    AssertTrue(okRecent, L"HistoryLogger.GetRecentSamples after Trim(0) returns true");
    AssertTrue(samples.empty(), L"HistoryLogger.DeleteAll via Trim(0) cleared history");

    // Add fresh samples and trim to 1-2 days (should keep recent data)
    logger.AppendSample(ifaceName, 3000ULL, 1500ULL);
    logger.AppendSample(ifaceName, 1000ULL, 500ULL);

    bool trimmed1 = logger.TrimToRecentDays(1);
    AssertTrue(trimmed1, L"HistoryLogger.TrimToRecentDays(1) returns true");

    samples.clear();
    okRecent = logger.GetRecentSamples(10, samples, &ifaceName, false);
    AssertTrue(okRecent, L"HistoryLogger.GetRecentSamples after Trim(1) returns true");
    AssertTrue(!samples.empty(), L"HistoryLogger.TrimToRecentDays(1) keeps recent data");

    bool trimmed2 = logger.TrimToRecentDays(2);
    AssertTrue(trimmed2, L"HistoryLogger.TrimToRecentDays(2) returns true");
}

} // namespace NetPulseTests
