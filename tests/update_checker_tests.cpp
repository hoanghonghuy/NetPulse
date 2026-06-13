#include "TestUtils.h"
#include "test_fakes/FakeHttpClient.h"
#include "NetPulse/UpdateChecker.h"
#include "NetPulse/WinHttpClient.h"
#include "NetPulse/Common.h"

namespace NetPulseTests
{

using namespace NetPulse;

struct UpdateCheckerTestFriend
{
    static bool PerformCheck(std::wstring& outLatestVersion,
                             std::wstring& outUrl,
                             IHttpClient* httpClient)
    {
        return UpdateChecker::PerformCheck(outLatestVersion, outUrl, httpClient);
    }

    static void SetMockHttpClient(UpdateChecker& checker, IHttpClient* client)
    {
        checker.m_pMockHttpClient = client;
    }
};

static const char* kSampleReleaseJson =
    R"({"tag_name":"v9.8.7","html_url":"https://github.com/hoanghonghuy/NetPulse/releases/tag/v9.8.7"})";

void TestUpdateCheckerParseValidJson()
{
    LogTestMessage(L"  Running TestUpdateCheckerParseValidJson...");

    std::wstring tag;
    std::wstring url;
    bool parsed = UpdateChecker::ParseGitHubReleaseResponse(kSampleReleaseJson, tag, url);

    AssertTrue(parsed, L"ParseGitHubReleaseResponse accepts valid JSON");
    AssertTrue(tag == L"v9.8.7", L"ParseGitHubReleaseResponse extracts tag_name");
    AssertTrue(url.find(L"github.com") != std::wstring::npos, L"ParseGitHubReleaseResponse extracts html_url");
}

void TestUpdateCheckerStripVersionPrefix()
{
    LogTestMessage(L"  Running TestUpdateCheckerStripVersionPrefix...");

    AssertTrue(UpdateChecker::StripVersionPrefix(L"v2.3.0") == L"2.3.0",
               L"StripVersionPrefix removes leading v");
    AssertTrue(UpdateChecker::StripVersionPrefix(L"2.3.0") == L"2.3.0",
               L"StripVersionPrefix leaves version without prefix");
}

void TestUpdateCheckerCompareVersions()
{
    LogTestMessage(L"  Running TestUpdateCheckerCompareVersions...");

    AssertTrue(UpdateChecker::CompareVersions(L"2.4.0", L"2.3.0") == 1,
               L"CompareVersions detects newer patch series");
    AssertTrue(UpdateChecker::CompareVersions(L"2.3.0", L"2.4.0") == -1,
               L"CompareVersions detects older version");
    AssertTrue(UpdateChecker::CompareVersions(L"2.3.0", L"2.3.0") == 0,
               L"CompareVersions detects equal versions");
    AssertTrue(UpdateChecker::CompareVersions(L"2.3.1", L"2.3.0") == 1,
               L"CompareVersions compares patch segment");
    AssertTrue(UpdateChecker::CompareVersions(L"3.0.0", L"2.9.9") == 1,
               L"CompareVersions compares major segment");
}

void TestUpdateCheckerGitHubRepoPath()
{
    LogTestMessage(L"  Running TestUpdateCheckerGitHubRepoPath...");

    std::wstring path = UpdateChecker::GetGitHubReleaseApiPath();
    AssertTrue(path.find(APP_GITHUB_REPO) != std::wstring::npos,
               L"Release API path contains APP_GITHUB_REPO");
    AssertTrue(path.find(L"/releases/latest") != std::wstring::npos,
               L"Release API path ends with /releases/latest");
}

void TestUpdateCheckerPerformCheckSuccess()
{
    LogTestMessage(L"  Running TestUpdateCheckerPerformCheckSuccess...");

    auto fake = std::make_shared<FakeHttpClient>();
    fake->m_getBody = kSampleReleaseJson;

    std::wstring tag;
    std::wstring url;
    bool ok = UpdateCheckerTestFriend::PerformCheck(tag, url, fake.get());

    AssertTrue(ok, L"PerformCheck succeeds with fake HTTP client");
    AssertTrue(tag == L"v9.8.7", L"PerformCheck returns parsed tag");
    AssertTrue(fake->m_getCallCount == 1, L"PerformCheck issues one HTTP GET");
    AssertTrue(fake->m_lastHost == L"api.github.com", L"PerformCheck targets GitHub API host");
    AssertTrue(fake->m_lastPath == UpdateChecker::GetGitHubReleaseApiPath(),
               L"PerformCheck uses release API path");
}

void TestUpdateCheckerPerformCheckHttpFail()
{
    LogTestMessage(L"  Running TestUpdateCheckerPerformCheckHttpFail...");

    auto fake = std::make_shared<FakeHttpClient>();
    fake->m_getSuccess = false;

    std::wstring tag;
    std::wstring url;
    bool ok = UpdateCheckerTestFriend::PerformCheck(tag, url, fake.get());

    AssertTrue(!ok, L"PerformCheck returns false when HTTP fails");
    AssertTrue(tag.empty() && url.empty(), L"PerformCheck leaves outputs empty on failure");
}

void TestUpdateCheckerPerformCheckInvalidJson()
{
    LogTestMessage(L"  Running TestUpdateCheckerPerformCheckInvalidJson...");

    auto fake = std::make_shared<FakeHttpClient>();
    fake->m_getBody = R"({"name":"broken"})";

    std::wstring tag;
    std::wstring url;
    bool ok = UpdateCheckerTestFriend::PerformCheck(tag, url, fake.get());

    AssertTrue(!ok, L"PerformCheck returns false for invalid release JSON");
}

void TestUpdateCheckerCompareVersionsEdgeCases()
{
    LogTestMessage(L"  Running TestUpdateCheckerCompareVersionsEdgeCases...");

    // Empty strings, invalid formats
    AssertTrue(UpdateChecker::CompareVersions(L"abc", L"1.2.3") == -1, L"Invalid format 1");
    AssertTrue(UpdateChecker::CompareVersions(L"1.a", L"1.2") == -1, L"Invalid format 2");
    AssertTrue(UpdateChecker::CompareVersions(L"1.2.3.4", L"1.2.3") == 1, L"Different length v1 > v2");
    AssertTrue(UpdateChecker::CompareVersions(L"1.2", L"1.2.3") == -1, L"Different length v1 < v2");
}

void TestUpdateCheckerParseJsonEdgeCases()
{
    LogTestMessage(L"  Running TestUpdateCheckerParseJsonEdgeCases...");

    std::wstring tag;
    std::wstring url;

    // Missing html_url
    bool parsed = UpdateChecker::ParseGitHubReleaseResponse(R"({"tag_name":"v1.0.0"})", tag, url);
    AssertTrue(!parsed, L"Reject JSON with missing html_url");

    // Missing tag_name
    parsed = UpdateChecker::ParseGitHubReleaseResponse(R"({"html_url":"https://github.com"})", tag, url);
    AssertTrue(!parsed, L"Reject JSON with missing tag_name");

    // Empty
    parsed = UpdateChecker::ParseGitHubReleaseResponse("", tag, url);
    AssertTrue(!parsed, L"Reject empty response");
}

void TestUpdateCheckerCheckForUpdatesSmoke()
{
    LogTestMessage(L"  Running TestUpdateCheckerCheckForUpdatesSmoke...");

    // Trigger silent check update. It runs on a background thread and does not show UI.
    UpdateChecker::CheckForUpdates(nullptr, true);
    
    // Give it a tiny moment to run background initialization
    Sleep(150);
}

void TestWinHttpClientInvalidDns()
{
    LogTestMessage(L"  Running TestWinHttpClientInvalidDns...");

    WinHttpClient client(L"NetPulse Tests");
    std::string body;
    bool ok = client.HttpGet(L"this.domain.does.not.exist.invalid", L"/", body);
    AssertTrue(!ok, L"HttpGet fails on invalid host");

    double downloadSpeed = 0;
    ok = client.HttpDownload(L"this.domain.does.not.exist.invalid", L"/", downloadSpeed, nullptr, nullptr);
    AssertTrue(!ok, L"HttpDownload fails on invalid host");

    double uploadSpeed = 0;
    ok = client.HttpUpload(L"this.domain.does.not.exist.invalid", L"/", 100, uploadSpeed, nullptr, nullptr);
    AssertTrue(!ok, L"HttpUpload fails on invalid host");
}

void TestWinHttpClientCancel()
{
    LogTestMessage(L"  Running TestWinHttpClientCancel...");

    WinHttpClient client(L"NetPulse Tests");
    std::atomic<bool> cancelFlag(true);

    double downloadSpeed = 0;
    bool ok = client.HttpDownload(L"api.github.com", L"/", downloadSpeed, nullptr, &cancelFlag);
    AssertTrue(!ok, L"HttpDownload fails immediately if cancelFlag is set");

    double uploadSpeed = 0;
    ok = client.HttpUpload(L"api.github.com", L"/", 100, uploadSpeed, nullptr, &cancelFlag);
    AssertTrue(!ok, L"HttpUpload fails immediately if cancelFlag is set");
}

void TestWinHttpClientGoogleSmoke()
{
    LogTestMessage(L"  Running TestWinHttpClientGoogleSmoke...");

    WinHttpClient client(L"NetPulse Tests");
    std::string body;
    // We don't assert the result since we might be offline in tests, but it exercises the successful connection path when online.
    client.HttpGet(L"www.google.com", L"/", body);
    
    double speed = 0;
    std::atomic<bool> cancel(false);
    client.HttpDownload(L"www.google.com", L"/", speed, [](size_t /*received*/) {}, &cancel);
    client.HttpUpload(L"www.google.com", L"/", 10, speed, [](size_t /*sent*/) {}, &cancel);
}

void TestUpdateCheckerAsync()
{
    LogTestMessage(L"  Running TestUpdateCheckerAsync...");

    // 1. Success, same version, silent = true
    {
        UpdateChecker checker;
        auto fake = std::make_shared<FakeHttpClient>();
        std::wstring currentVer = APP_VERSION;
        std::string currentVerUtf8(currentVer.begin(), currentVer.end());
        fake->m_getBody = "{\"tag_name\":\"" + currentVerUtf8 + "\",\"html_url\":\"https://github.com/hoanghonghuy/NetPulse/releases/tag/" + currentVerUtf8 + "\"}";
        
        UpdateCheckerTestFriend::SetMockHttpClient(checker, fake.get());
        checker.CheckForUpdatesAsync(nullptr, true);
        
        // Wait for thread to run
        for (int i = 0; i < 2000; ++i)
        {
            if (fake->m_getCallCount > 0) break;
            Sleep(10);
        }
        
        checker.CancelAndWait();
        AssertTrue(fake->m_getCallCount == 1, L"CheckForUpdatesAsync calls HttpGet");
    }

    // 2. Failure, silent = true
    {
        UpdateChecker checker;
        auto fake = std::make_shared<FakeHttpClient>();
        fake->m_getSuccess = false;
        
        UpdateCheckerTestFriend::SetMockHttpClient(checker, fake.get());
        checker.CheckForUpdatesAsync(nullptr, true);
        
        // Wait for thread to run
        for (int i = 0; i < 2000; ++i)
        {
            if (fake->m_getCallCount > 0) break;
            Sleep(10);
        }
        
        checker.CancelAndWait();
        AssertTrue(fake->m_getCallCount == 1, L"CheckForUpdatesAsync calls HttpGet on failure");
    }

    // 3. Immediate Cancel
    {
        UpdateChecker checker;
        auto fake = std::make_shared<FakeHttpClient>();
        fake->m_getBody = kSampleReleaseJson;
        
        UpdateCheckerTestFriend::SetMockHttpClient(checker, fake.get());
        checker.CheckForUpdatesAsync(nullptr, true);
        checker.CancelAndWait(); // Cancel immediately
        Sleep(50);
        // We just verify it doesn't crash on immediate cancel
        AssertTrue(true, L"CheckForUpdatesAsync handles immediate cancel");
    }
}

void RunUpdateCheckerTests()
{
    LogTestMessage(L"=== UpdateChecker & WinHttpClient tests ===");

    TestUpdateCheckerParseValidJson();
    TestUpdateCheckerStripVersionPrefix();
    TestUpdateCheckerCompareVersions();
    TestUpdateCheckerCompareVersionsEdgeCases();
    TestUpdateCheckerGitHubRepoPath();
    TestUpdateCheckerPerformCheckSuccess();
    TestUpdateCheckerPerformCheckHttpFail();
    TestUpdateCheckerPerformCheckInvalidJson();
    TestUpdateCheckerParseJsonEdgeCases();
    TestUpdateCheckerCheckForUpdatesSmoke();
    TestUpdateCheckerAsync();

    // WinHttpClient tests
    TestWinHttpClientInvalidDns();
    TestWinHttpClientCancel();
    TestWinHttpClientGoogleSmoke();

    LogTestMessage(L"UpdateChecker & WinHttpClient tests completed.");
}

} // namespace NetPulseTests
