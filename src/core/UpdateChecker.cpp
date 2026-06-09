#include "NetPulse/UpdateChecker.h"
#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/Interfaces/IHttpClient.h"
#include "NetPulse/WinHttpClient.h"
#include <shellapi.h>
#include <thread>
#include <sstream>
#include <regex>
#include <vector>

namespace NetPulse
{

void UpdateChecker::CheckForUpdates(HWND hParent, bool silent)
{
    std::thread([hParent, silent]()
    {
        std::wstring latestVersion;
        std::wstring downloadUrl;

        bool success = PerformCheck(latestVersion, downloadUrl);

        if (success)
        {
            std::wstring cleanLatest = StripVersionPrefix(latestVersion);
            std::wstring cleanCurrent = StripVersionPrefix(APP_VERSION);

            if (CompareVersions(cleanLatest, cleanCurrent) > 0)
            {
                std::wstringstream msg;
                msg << L"A new version of NetPulse is available!\n\n"
                    << L"Current Version: " << APP_VERSION << L"\n"
                    << L"Latest Version: " << latestVersion << L"\n\n"
                    << L"Do you want to visit the download page?";

                int result = ShowDarkMessageBox(hParent, msg.str(), L"Update Available", MB_YESNO | MB_ICONINFORMATION, true);
                if (result == IDYES)
                {
                    ShellExecuteW(nullptr, L"open", downloadUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
            }
            else if (!silent)
            {
                std::wstringstream msg;
                msg << L"You are using the latest version of NetPulse.\n"
                    << L"Version: " << APP_VERSION;
                ShowDarkMessageBox(hParent, msg.str(), L"Check for Updates", MB_OK | MB_ICONINFORMATION, true);
            }
        }
        else if (!silent)
        {
            ShowDarkMessageBox(hParent, L"Failed to check for updates.\nPlease check your internet connection.", L"Error", MB_OK | MB_ICONERROR, true);
        }
    }).detach();
}

std::wstring UpdateChecker::GetGitHubReleaseApiPath()
{
    std::wstring releasePath = L"/repos/";
    releasePath += APP_GITHUB_REPO;
    releasePath += L"/releases/latest";
    return releasePath;
}

std::wstring UpdateChecker::StripVersionPrefix(const std::wstring& version)
{
    if (!version.empty() && version[0] == L'v')
    {
        return version.substr(1);
    }
    return version;
}

bool UpdateChecker::ParseGitHubReleaseResponse(const std::string& responseData,
                                               std::wstring& outLatestVersion,
                                               std::wstring& outUrl)
{
    outLatestVersion.clear();
    outUrl.clear();

    std::regex tagRegex("\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (!std::regex_search(responseData, match, tagRegex))
    {
        return false;
    }

    std::string tagUtf8 = match[1].str();

    std::regex urlRegex("\"html_url\"\\s*:\\s*\"([^\"]+)\"");
    if (!std::regex_search(responseData, match, urlRegex))
    {
        return false;
    }

    std::string urlUtf8 = match[1].str();

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, tagUtf8.c_str(), static_cast<int>(tagUtf8.length()), nullptr, 0);
    if (sizeNeeded <= 0)
    {
        return false;
    }

    std::wstring tagW(static_cast<size_t>(sizeNeeded), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, tagUtf8.c_str(), static_cast<int>(tagUtf8.length()), &tagW[0], sizeNeeded);
    outLatestVersion = tagW;

    sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, urlUtf8.c_str(), static_cast<int>(urlUtf8.length()), nullptr, 0);
    if (sizeNeeded <= 0)
    {
        return false;
    }

    std::wstring urlW(static_cast<size_t>(sizeNeeded), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, urlUtf8.c_str(), static_cast<int>(urlUtf8.length()), &urlW[0], sizeNeeded);
    outUrl = urlW;

    return true;
}

bool UpdateChecker::PerformCheck(std::wstring& outLatestVersion,
                                 std::wstring& outUrl,
                                 IHttpClient* httpClient)
{
    std::unique_ptr<WinHttpClient> defaultClient;
    IHttpClient* client = httpClient;
    if (!client)
    {
        defaultClient = std::make_unique<WinHttpClient>(L"NetPulse Update Checker");
        client = defaultClient.get();
    }

    std::string responseData;
    if (!client->HttpGet(L"api.github.com", GetGitHubReleaseApiPath(), responseData))
    {
        return false;
    }

    return ParseGitHubReleaseResponse(responseData, outLatestVersion, outUrl);
}

int UpdateChecker::CompareVersions(const std::wstring& v1, const std::wstring& v2)
{
    auto split = [](const std::wstring& s) {
        std::vector<int> parts;
        std::wstringstream ss(s);
        std::wstring item;
        while (std::getline(ss, item, L'.'))
        {
            try
            {
                parts.push_back(std::stoi(item));
            }
            catch (...)
            {
                parts.push_back(0);
            }
        }
        return parts;
    };

    std::vector<int> parts1 = split(v1);
    std::vector<int> parts2 = split(v2);

    size_t i = 0;
    while (i < parts1.size() && i < parts2.size())
    {
        if (parts1[i] > parts2[i]) return 1;
        if (parts1[i] < parts2[i]) return -1;
        i++;
    }

    if (i < parts1.size()) return 1;
    if (i < parts2.size()) return -1;

    return 0;
}

} // namespace NetPulse
