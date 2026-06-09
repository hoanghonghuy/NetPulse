#ifndef NETWORK_MONITOR_UPDATE_CHECKER_H
#define NETWORK_MONITOR_UPDATE_CHECKER_H

#include "NetPulse/Common.h"
#include <string>
#include <functional>
#include <memory>

namespace NetPulseTests
{
struct UpdateCheckerTestFriend;
}

namespace NetPulse
{

class IHttpClient;

class UpdateChecker
{
public:
    using UpdateCheckCallback = std::function<void(bool hasUpdate, const std::wstring& latestVersion, const std::wstring& downloadUrl)>;

    static void CheckForUpdates(HWND hParent, bool silent);

    static std::wstring GetGitHubReleaseApiPath();
    static bool ParseGitHubReleaseResponse(const std::string& responseData,
                                           std::wstring& outLatestVersion,
                                           std::wstring& outUrl);
    static int CompareVersions(const std::wstring& v1, const std::wstring& v2);
    static std::wstring StripVersionPrefix(const std::wstring& version);

private:
    friend struct NetPulseTests::UpdateCheckerTestFriend;

    static bool PerformCheck(std::wstring& outLatestVersion,
                             std::wstring& outUrl,
                             IHttpClient* httpClient = nullptr);
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_UPDATE_CHECKER_H
