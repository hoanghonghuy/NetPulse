#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/HistoryLogger.h"
#include "TestUtils.h"

#include <chrono>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

using namespace NetPulse;

namespace NetPulseTests
{

static bool ReadExportFileHeader(const std::wstring& filePath, std::string& headerOut)
{
    std::ifstream file{std::filesystem::path(filePath), std::ios::binary};
    if (!file.is_open())
    {
        return false;
    }

    unsigned char bom[3] = {};
    file.read(reinterpret_cast<char*>(bom), 3);
    if (bom[0] != 0xEF || bom[1] != 0xBB || bom[2] != 0xBF)
    {
        file.seekg(0);
    }

    std::getline(file, headerOut);
    return !headerOut.empty();
}

void RunHistoryLoggerTests()
{
    LogTestMessage(L"=== HistoryLogger tests ===");

    HistoryLogger& logger = HistoryLogger::Instance();
    const std::wstring ifaceName = L"TestIface";
    const std::wstring otherIface = L"OtherIface";

    bool cleared = logger.DeleteAll();
    AssertTrue(cleared, L"HistoryLogger.DeleteAll succeeds");

    logger.AppendSample(ifaceName, 1000ULL, 500ULL);
    logger.AppendSample(ifaceName, 4000ULL, 1500ULL);

    unsigned long long totalDownToday = 0;
    unsigned long long totalUpToday = 0;
    bool okToday = logger.GetTotalsToday(totalDownToday, totalUpToday, &ifaceName);
    AssertTrue(okToday, L"HistoryLogger.GetTotalsToday returns true");
    AssertTrue(totalDownToday == 5000ULL && totalUpToday == 2000ULL,
               L"HistoryLogger totals today match inserted bytes");

    unsigned long long totalDownMonth = 0;
    unsigned long long totalUpMonth = 0;
    bool okMonth = logger.GetTotalsThisMonth(totalDownMonth, totalUpMonth, &ifaceName);
    AssertTrue(okMonth, L"HistoryLogger.GetTotalsThisMonth returns true");
    AssertTrue(totalDownMonth == 5000ULL && totalUpMonth == 2000ULL,
               L"HistoryLogger totals this month match inserted bytes");

    cleared = logger.DeleteAll();
    AssertTrue(cleared, L"HistoryLogger.DeleteAll before filter tests");

    logger.AppendSample(ifaceName, 1000ULL, 200ULL);
    logger.AppendSample(otherIface, 3000ULL, 400ULL);

    okToday = logger.GetTotalsToday(totalDownToday, totalUpToday, &ifaceName);
    AssertTrue(okToday, L"HistoryLogger.GetTotalsToday with filter returns true");
    AssertTrue(totalDownToday == 1000ULL && totalUpToday == 200ULL,
               L"HistoryLogger.GetTotalsToday filters by interface");

    okToday = logger.GetTotalsToday(totalDownToday, totalUpToday, &otherIface);
    AssertTrue(okToday, L"HistoryLogger.GetTotalsToday other interface returns true");
    AssertTrue(totalDownToday == 3000ULL && totalUpToday == 400ULL,
               L"HistoryLogger.GetTotalsToday returns other interface totals");

    uint64_t monthTotal = logger.GetThisMonthTotalBytes(&ifaceName);
    AssertTrue(monthTotal == 1200ULL, L"HistoryLogger.GetThisMonthTotalBytes filters by interface");

    cleared = logger.DeleteAll();
    logger.AppendSample(ifaceName, 100ULL, 10ULL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    logger.AppendSample(ifaceName, 200ULL, 20ULL);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    logger.AppendSample(ifaceName, 300ULL, 30ULL);

    std::vector<HistorySample> samples;
    bool okRecent = logger.GetRecentSamples(2, samples, &ifaceName, false);
    AssertTrue(okRecent, L"HistoryLogger.GetRecentSamples limit returns true");
    AssertTrue(samples.size() == 2, L"HistoryLogger.GetRecentSamples respects limit");
    AssertTrue(samples[0].bytesDown == 300ULL && samples[1].bytesDown == 200ULL,
               L"HistoryLogger.GetRecentSamples returns newest-first ordering");
    AssertTrue(samples[0].timestamp >= samples[1].timestamp,
               L"HistoryLogger.GetRecentSamples timestamps are descending");

    cleared = logger.DeleteAll();
    AssertTrue(cleared, L"HistoryLogger.DeleteAll before trim tests");

    logger.AppendSample(ifaceName, 2000ULL, 1000ULL);
    samples.clear();
    okRecent = logger.GetRecentSamples(10, samples, &ifaceName, false);
    AssertTrue(okRecent, L"HistoryLogger.GetRecentSamples before trim returns true");
    AssertTrue(!samples.empty(), L"HistoryLogger.GetRecentSamples before trim has data");

    bool trimmed0 = logger.TrimToRecentDays(0);
    AssertTrue(trimmed0, L"HistoryLogger.TrimToRecentDays(0) returns true");

    samples.clear();
    okRecent = logger.GetRecentSamples(10, samples, &ifaceName, false);
    AssertTrue(okRecent, L"HistoryLogger.GetRecentSamples after Trim(0) returns true");
    AssertTrue(samples.empty(), L"HistoryLogger.DeleteAll via Trim(0) cleared history");

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

    cleared = logger.DeleteAll();
    logger.AppendSample(ifaceName, 111ULL, 222ULL);

    const std::wstring exportPath = GetTestSandboxDir() + L"\\history_export.csv";
    bool exported = logger.ExportToCSV(exportPath);
    AssertTrue(exported, L"HistoryLogger.ExportToCSV succeeds");

    std::string header;
    AssertTrue(ReadExportFileHeader(exportPath, header),
               L"HistoryLogger.ExportToCSV creates readable file");
    AssertTrue(header.find("Timestamp,DateTime,Interface,BytesDown,BytesUp") != std::string::npos,
               L"HistoryLogger.ExportToCSV writes expected header");

    cleared = logger.DeleteAll();
    const std::wstring trickyIface = L"NIC \"main\", backup";
    logger.AppendSample(trickyIface, 555ULL, 666ULL);

    const std::wstring trickyExportPath = GetTestSandboxDir() + L"\\history_export_tricky.csv";
    exported = logger.ExportToCSV(trickyExportPath, &trickyIface);
    AssertTrue(exported, L"HistoryLogger.ExportToCSV with tricky interface succeeds");

    std::ifstream trickyFile{std::filesystem::path(trickyExportPath), std::ios::binary};
    AssertTrue(trickyFile.is_open(), L"HistoryLogger.ExportToCSV tricky file opens");
    trickyFile.seekg(3);
    std::string trickyContent((std::istreambuf_iterator<char>(trickyFile)),
                              std::istreambuf_iterator<char>());
    AssertTrue(trickyContent.find("\"NIC \"\"main\"\", backup\"") != std::string::npos,
               L"HistoryLogger.ExportToCSV escapes comma and quotes in interface");

    cleared = logger.DeleteAll();
    constexpr int kThreadCount = 4;
    std::vector<std::thread> workers;
    for (int i = 0; i < kThreadCount; ++i)
    {
        workers.emplace_back([&logger]()
        {
            for (int j = 0; j < 25; ++j)
            {
                logger.AppendSample(L"ConcurrentIface", 10ULL, 5ULL);
                std::vector<HistorySample> localSamples;
                logger.GetRecentSamples(5, localSamples, nullptr, false);
            }
        });
    }
    for (auto& worker : workers)
    {
        worker.join();
    }

    const std::wstring concurrentIface = L"ConcurrentIface";
    samples.clear();
    okRecent = logger.GetRecentSamples(10, samples, &concurrentIface, false);
    AssertTrue(okRecent && !samples.empty(),
               L"HistoryLogger concurrent append/query does not corrupt data");

    // Chart query and extended tests
    cleared = logger.DeleteAll();
    AssertTrue(cleared, L"HistoryLogger.DeleteAll before chart tests");

    std::time_t now = std::time(nullptr);
    std::tm localTime = {};
    AssertTrue(localtime_s(&localTime, &now) == 0, L"localtime_s succeeds");
    int currentYear = localTime.tm_year + 1900;
    int currentMonth = localTime.tm_mon + 1;
    int currentDay = localTime.tm_mday;

    logger.AppendSample(ifaceName, 1200ULL, 800ULL);

    std::vector<DailyUsage> dailyData;
    bool okDaily = logger.GetDailyUsage(currentYear, currentMonth, dailyData);
    AssertTrue(okDaily, L"HistoryLogger.GetDailyUsage succeeds");
    AssertTrue(dailyData.size() == 1, L"HistoryLogger.GetDailyUsage returns 1 day");
    AssertTrue(dailyData[0].day == currentDay, L"HistoryLogger.GetDailyUsage returns correct day");
    AssertTrue(dailyData[0].bytesDown == 1200ULL && dailyData[0].bytesUp == 800ULL, L"HistoryLogger.GetDailyUsage returns correct bytes");

    std::vector<MonthlyUsage> monthlyData;
    bool okMonthly = logger.GetMonthlyUsage(currentYear, monthlyData);
    AssertTrue(okMonthly, L"HistoryLogger.GetMonthlyUsage succeeds");
    AssertTrue(monthlyData.size() == 1, L"HistoryLogger.GetMonthlyUsage returns 1 month");
    AssertTrue(monthlyData[0].month == currentMonth, L"HistoryLogger.GetMonthlyUsage returns correct month");
    AssertTrue(monthlyData[0].bytesDown == 1200ULL && monthlyData[0].bytesUp == 800ULL, L"HistoryLogger.GetMonthlyUsage returns correct bytes");

    // Empty month/year queries
    dailyData.clear();
    okDaily = logger.GetDailyUsage(1999, 12, dailyData);
    AssertTrue(okDaily, L"HistoryLogger.GetDailyUsage for empty month succeeds");
    AssertTrue(dailyData.empty(), L"HistoryLogger.GetDailyUsage for empty month returns no data");

    monthlyData.clear();
    okMonthly = logger.GetMonthlyUsage(1999, monthlyData);
    AssertTrue(okMonthly, L"HistoryLogger.GetMonthlyUsage for empty year succeeds");
    AssertTrue(monthlyData.empty(), L"HistoryLogger.GetMonthlyUsage for empty year returns no data");

    // Invalid parameters (mktime fails)
    dailyData.clear();
    bool failDaily = logger.GetDailyUsage(999999, 1, dailyData);
    AssertTrue(!failDaily, L"HistoryLogger.GetDailyUsage with invalid year fails");

    monthlyData.clear();
    bool failMonthly = logger.GetMonthlyUsage(999999, monthlyData);
    AssertTrue(!failMonthly, L"HistoryLogger.GetMonthlyUsage with invalid year fails");

    // Test GetRecentSamples onlyToday=true
    cleared = logger.DeleteAll();
    logger.AppendSample(ifaceName, 500ULL, 250ULL);
    std::vector<HistorySample> recentSamples;
    bool okRecentToday = logger.GetRecentSamples(10, recentSamples, &ifaceName, true);
    AssertTrue(okRecentToday, L"HistoryLogger.GetRecentSamples with onlyToday=true succeeds");
    AssertTrue(recentSamples.size() == 1, L"HistoryLogger.GetRecentSamples with onlyToday=true returns current sample");
}

} // namespace NetPulseTests
