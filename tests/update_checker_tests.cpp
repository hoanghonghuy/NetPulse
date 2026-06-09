#include "TestUtils.h"
#include "test_fakes/FakeHttpClient.h"
#include "NetPulse/UpdateChecker.h"
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

void RunUpdateCheckerTests()
{
    LogTestMessage(L"=== UpdateChecker tests ===");

    TestUpdateCheckerParseValidJson();
    TestUpdateCheckerStripVersionPrefix();
    TestUpdateCheckerCompareVersions();
    TestUpdateCheckerGitHubRepoPath();
    TestUpdateCheckerPerformCheckSuccess();
    TestUpdateCheckerPerformCheckHttpFail();
    TestUpdateCheckerPerformCheckInvalidJson();

    LogTestMessage(L"UpdateChecker tests completed.");
}

} // namespace NetPulseTests
