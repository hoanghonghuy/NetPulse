#include "NetPulse/SpeedTestHistory.h"
#include "TestUtils.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>

using namespace NetPulse;

namespace NetPulseTests
{

static std::wstring GetSandboxHistoryPath()
{
    const wchar_t* testDataDir = _wgetenv(L"NETPULSE_TEST_DATA_DIR");
    if (!testDataDir || testDataDir[0] == L'\0')
    {
        return L"";
    }
    return std::wstring(testDataDir) + L"\\speed_test_history.json";
}

void RunSpeedTestHistoryPersistenceTests()
{
    LogTestMessage(L"=== SpeedTestHistory persistence tests ===");

    const std::wstring historyPath = GetSandboxHistoryPath();
    AssertTrue(!historyPath.empty(), L"SpeedTestHistory sandbox path is available");

    {
        SpeedTestHistory history;
        history.ClearHistory();

        SpeedTestResult first;
        first.downloadMbps = 42.0;
        first.uploadMbps = 21.0;
        first.pingMs = 15;
        first.timestamp = 1700000000;
        first.serverName = L"ServerA";
        first.success = true;
        history.AddResult(first);

        SpeedTestResult second;
        second.downloadMbps = 99.0;
        second.uploadMbps = 44.0;
        second.pingMs = 25;
        second.timestamp = 1700000100;
        second.serverName = L"ServerB";
        second.success = true;
        history.AddResult(second);

        auto inMemory = history.GetHistory();
        AssertTrue(inMemory.size() == 2, L"SpeedTestHistory keeps two entries in memory");
        AssertTrue(inMemory[0].downloadMbps == 99.0, L"SpeedTestHistory newest entry is first in memory");
    }

    {
        SpeedTestHistory reloaded;
        auto loaded = reloaded.GetHistory();
        AssertTrue(loaded.size() == 2, L"SpeedTestHistory reloads persisted entries");
        AssertTrue(loaded[0].downloadMbps == 99.0, L"SpeedTestHistory persisted newest-first order");
        AssertTrue(loaded[1].downloadMbps == 42.0, L"SpeedTestHistory persisted older entry");
        AssertTrue(loaded[0].serverName == L"ServerB", L"SpeedTestHistory persisted server name");
    }

    {
        std::ofstream corrupt{std::filesystem::path(historyPath)};
        corrupt << "[{\"download\":not_a_number,\"upload\":1,\"ping\":1,\"server\":\"X\",\"timestamp\":1,\"success\":true},"
                   "{\"download\":12.5,\"upload\":6.25,\"ping\":9,\"server\":\"OK\",\"timestamp\":2,\"success\":true}]";
        corrupt.close();

        SpeedTestHistory recovered;
        auto entries = recovered.GetHistory();
        AssertTrue(entries.size() == 1, L"SpeedTestHistory skips corrupt JSON entries");
        AssertTrue(entries[0].downloadMbps == 12.5, L"SpeedTestHistory keeps valid JSON entries");
    }

    {
        std::ofstream emptyFile{std::filesystem::path(historyPath), std::ios::trunc};
        emptyFile.close();

        SpeedTestHistory emptyHistory;
        AssertTrue(emptyHistory.GetHistory().empty(), L"SpeedTestHistory handles empty file");

        emptyHistory.ClearHistory();
        AssertTrue(std::filesystem::exists(historyPath), L"SpeedTestHistory ClearHistory writes file");
    }

    {
        SpeedTestHistory unicodeHistory;
        unicodeHistory.ClearHistory();

        SpeedTestResult unicodeResult;
        unicodeResult.downloadMbps = 12.5;
        unicodeResult.uploadMbps = 6.25;
        unicodeResult.pingMs = 9;
        unicodeResult.timestamp = 1700000200;
        unicodeResult.serverName = L"Server-Tokyo";
        unicodeResult.success = true;
        unicodeHistory.AddResult(unicodeResult);
    }

    {
        SpeedTestHistory unicodeReloaded;
        auto entries = unicodeReloaded.GetHistory();
        AssertTrue(!entries.empty(), L"SpeedTestHistory persists unicode server name");
        AssertTrue(entries[0].serverName == L"Server-Tokyo",
                   L"SpeedTestHistory reloads unicode server name");
    }
}

} // namespace NetPulseTests
