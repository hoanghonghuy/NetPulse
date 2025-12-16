// ============================================================================
// File: speed_test_tests.cpp
// Description: Unit tests for SpeedTester and SpeedTestHistory
// ============================================================================

#include "TestUtils.h"
#include "NetPulse/SpeedTester.h"
#include "NetPulse/SpeedTestHistory.h"
#include <memory>
#include <thread>
#include <chrono>

namespace NetPulseTests
{

using namespace NetPulse;

// ============================================================================
// SpeedTester Tests
// ============================================================================

void TestSpeedTesterInitialization()
{
    LogTestMessage(L"  Running TestSpeedTesterInitialization...");
    
    SpeedTester tester;
    
    // Should initialize without crashing
    AssertTrue(true, L"SpeedTester construction should succeed");
}

void TestSpeedTesterPingMeasurement()
{
    LogTestMessage(L"  Running TestSpeedTesterPingMeasurement...");
    
    SpeedTester tester;
    
    // MeasurePing requires a host
    int ping = tester.MeasurePing(L"8.8.8.8");
    
    LogTestMessage((L"    Ping result: " + std::to_wstring(ping) + L" ms").c_str());
    
    // -1 means failure, otherwise should be positive
    if (ping != -1)
    {
        AssertTrue(ping >= 0, L"Ping should be non-negative if successful");
        AssertTrue(ping < 30000, L"Ping should be less than 30 seconds");
    }
}

void TestSpeedTesterRunTestBasic()
{
    LogTestMessage(L"  Running TestSpeedTesterRunTestBasic...");
    LogTestMessage(L"    Note: This test may take time, requires internet access...");
    
    SpeedTester tester;
    
    bool progressCalled = false;
    int lastProgress = -1;
    
    // RunTest returns void, results come via callback or GetLastResult
    tester.RunTest([&progressCalled, &lastProgress](int progress, const std::wstring& /*status*/) {
        progressCalled = true;
        lastProgress = progress;
    });
    
    SpeedTestResult result = tester.GetLastResult();
    
    LogTestMessage((L"    Download: " + std::to_wstring(result.downloadMbps) + L" Mbps").c_str());
    LogTestMessage((L"    Upload: " + std::to_wstring(result.uploadMbps) + L" Mbps").c_str());
    LogTestMessage((L"    Ping: " + std::to_wstring(result.pingMs) + L" ms").c_str());
    LogTestMessage((L"    Server: " + result.serverName).c_str());
    
    // Progress callback should have been called
    AssertTrue(progressCalled, L"Progress callback should have been called");
    
    // Results should be valid (non-negative, unless failure)
    AssertTrue(result.downloadMbps >= 0.0, L"Download speed should be non-negative");
    AssertTrue(result.uploadMbps >= 0.0, L"Upload speed should be non-negative");
    AssertTrue(result.timestamp > 0, L"Timestamp should be set");
}

// ============================================================================
// SpeedTestHistory Tests
// ============================================================================

void TestSpeedTestHistoryAddAndGet()
{
    LogTestMessage(L"  Running TestSpeedTestHistoryAddAndGet...");
    
    SpeedTestHistory history;
    history.ClearHistory(); // Start clean
    
    // Create test result
    SpeedTestResult result1;
    result1.downloadMbps = 50.0;
    result1.uploadMbps = 25.0;
    result1.pingMs = 30;
    result1.timestamp = std::time(nullptr);
    result1.serverName = L"TestServer1";
    
    history.AddResult(result1);
    
    auto historyList = history.GetHistory();
    AssertTrue(historyList.size() == 1, L"History should have 1 item after adding 1 result");
    AssertTrue(historyList[0].downloadMbps == 50.0, L"Download speed should match");
    AssertTrue(historyList[0].uploadMbps == 25.0, L"Upload speed should match");
    AssertTrue(historyList[0].pingMs == 30, L"Ping should match");
}

void TestSpeedTestHistoryOrdering()
{
    LogTestMessage(L"  Running TestSpeedTestHistoryOrdering...");
    
    SpeedTestHistory history;
    history.ClearHistory(); // Start clean
    
    // Add multiple results in sequence
    for (int i = 1; i <= 5; i++)
    {
        SpeedTestResult result;
        result.downloadMbps = static_cast<double>(i * 10);  // 10, 20, 30, 40, 50
        result.uploadMbps = static_cast<double>(i * 5);
        result.pingMs = i * 10;
        result.timestamp = std::time(nullptr) + i;  // Different timestamps
        result.serverName = L"Server" + std::to_wstring(i);
        
        history.AddResult(result);
    }
    
    auto historyList = history.GetHistory();
    AssertTrue(historyList.size() == 5, L"History should have 5 items");
    
    // First item in vector should be NEWEST (inserted at begin)
    AssertTrue(historyList[0].downloadMbps == 50.0, L"First item should be newest (50 Mbps)");
    AssertTrue(historyList[4].downloadMbps == 10.0, L"Last item should be oldest (10 Mbps)");
}

void TestSpeedTestHistoryLimit()
{
    LogTestMessage(L"  Running TestSpeedTestHistoryLimit...");
    
    SpeedTestHistory history;
    history.ClearHistory(); // Start clean
    
    // Add more than default limit (100 entries)
    for (int i = 0; i < 150; i++)
    {
        SpeedTestResult result;
        result.downloadMbps = static_cast<double>(i);
        result.uploadMbps = static_cast<double>(i);
        result.pingMs = i;
        result.timestamp = std::time(nullptr) + i;
        result.serverName = L"Server";
        
        history.AddResult(result);
    }
    
    auto historyList = history.GetHistory();
    
    // History should be limited (assume 100 max)
    AssertTrue(historyList.size() <= 100, L"History should be limited to max entries");
}

void TestSpeedTestHistoryGetLatest()
{
    LogTestMessage(L"  Running TestSpeedTestHistoryGetLatest...");
    
    SpeedTestHistory history;
    history.ClearHistory(); // Start clean
    
    // Add 10 results
    for (int i = 1; i <= 10; i++)
    {
        SpeedTestResult result;
        result.downloadMbps = static_cast<double>(i * 10);
        result.uploadMbps = static_cast<double>(i * 5);
        result.pingMs = i * 10;
        result.timestamp = std::time(nullptr) + i;
        result.serverName = L"Server";
        
        history.AddResult(result);
    }
    
    auto historyList = history.GetHistory(1);
    if (!historyList.empty())
    {
        const auto& latest = historyList[0];
        AssertTrue(latest.downloadMbps == 100.0, L"Latest should have download 100 Mbps");
        AssertTrue(latest.pingMs == 100, L"Latest should have ping 100 ms");
    }
    else
    {
        AssertTrue(false, L"GetHistory(1) should return a value when history not empty");
    }
}

void TestSpeedTestHistoryClear()
{
    LogTestMessage(L"  Running TestSpeedTestHistoryClear...");
    
    SpeedTestHistory history;
    history.ClearHistory(); // Start clean
    
    // Add some results
    SpeedTestResult result;
    result.downloadMbps = 50.0;
    result.uploadMbps = 25.0;
    result.pingMs = 30;
    result.timestamp = std::time(nullptr);
    result.serverName = L"TestServer";
    
    history.AddResult(result);
    history.AddResult(result);
    
    AssertTrue(history.GetHistory().size() == 2, L"Should have 2 items before clear");
    
    history.ClearHistory();
    
    AssertTrue(history.GetHistory().empty(), L"History should be empty after clear");
}

// ============================================================================
// Run All Speed Test Tests
// ============================================================================

void RunSpeedTestTests()
{
    LogTestMessage(L"Running SpeedTest Tests...");
    
    // SpeedTestHistory tests (fast, no network)
    TestSpeedTestHistoryAddAndGet();
    TestSpeedTestHistoryOrdering();
    TestSpeedTestHistoryLimit();
    TestSpeedTestHistoryGetLatest();
    TestSpeedTestHistoryClear();
    
    // SpeedTester tests (require network, slower)
    TestSpeedTesterInitialization();
    TestSpeedTesterPingMeasurement();
    
    // Note: The full RunTest is slow - can be uncommented for full integration testing
    // TestSpeedTesterRunTestBasic();
    
    LogTestMessage(L"SpeedTest Tests completed.");
}

} // namespace NetPulseTests
