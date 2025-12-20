#ifndef NETWORK_MONITOR_UPDATE_CHECKER_H
#define NETWORK_MONITOR_UPDATE_CHECKER_H

#include "NetPulse/Common.h"
#include <string>
#include <functional>

namespace NetPulse
{

class UpdateChecker
{
public:
    // Callback for update check result.
    // hasUpdate: true if newer version found.
    // latestVersion: the tag name of the new version (e.g. "v2.3.0").
    // downloadUrl: URL to the release page.
    using UpdateCheckCallback = std::function<void(bool hasUpdate, const std::wstring& latestVersion, const std::wstring& downloadUrl)>;

    // Start a check in a background thread.
    // If silent is true, only report if there IS an update (good for auto-check).
    // If silent is false, report "You are up to date" or errors as well (user triggered).
    static void CheckForUpdates(HWND hParent, bool silent);

private:
    // Helper to perform the actual HTTP request/parsing.
    // Returns true if successful.
    static bool PerformCheck(std::wstring& outLatestVersion, std::wstring& outUrl);

    // Compare two version strings (e.g. "2.2.0" and "2.3.0").
    // Returns: 1 if v1 > v2, -1 if v1 < v2, 0 if equal.
    static int CompareVersions(const std::wstring& v1, const std::wstring& v2);
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_UPDATE_CHECKER_H
