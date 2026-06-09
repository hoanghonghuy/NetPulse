#include "NetPulse/UpdateChecker.h"
#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include <shellapi.h>
#include <winhttp.h>
#include <thread>
#include <vector>
#include <sstream>
#include <regex>

namespace NetPulse
{

void UpdateChecker::CheckForUpdates(HWND hParent, bool silent)
{
    std::thread([hParent, silent]()
    {
        std::wstring latestVersion;
        std::wstring downloadUrl;

        // Visual feedback for manual check
        // Note: For a truly polished experience, we might want a "Checking..." dialog,
        // but for now we just show results when ready.

        bool success = PerformCheck(latestVersion, downloadUrl);

        // Switch back to UI thread flow essentially by using MessageBox which is modal 
        // but here we are in a thread. 
        // IMPORTANT: MessageBox should be called on the UI thread or it blocks this thread.
        // Since this thread effectively just waits for the check, it's fine to block it with MessageBox.
        // However, we should ensure the parent window handle is valid.

        if (success)
        {
            // Clean up version strings for comparison (remove 'v' prefix if present)
            std::wstring cleanLatest = latestVersion;
            if (!cleanLatest.empty() && cleanLatest[0] == L'v') cleanLatest = cleanLatest.substr(1);

            std::wstring cleanCurrent = APP_VERSION;
            if (!cleanCurrent.empty() && cleanCurrent[0] == L'v') cleanCurrent = cleanCurrent.substr(1);

            if (CompareVersions(cleanLatest, cleanCurrent) > 0)
            {
                // Update Available
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
                // Up to date
                std::wstringstream msg;
                msg << L"You are using the latest version of NetPulse.\n"
                    << L"Version: " << APP_VERSION;
                ShowDarkMessageBox(hParent, msg.str(), L"Check for Updates", MB_OK | MB_ICONINFORMATION, true);
            }
        }
        else if (!silent)
        {
            // Error
            ShowDarkMessageBox(hParent, L"Failed to check for updates.\nPlease check your internet connection.", L"Error", MB_OK | MB_ICONERROR, true);
        }
        
    }).detach();
}

bool UpdateChecker::PerformCheck(std::wstring& outLatestVersion, std::wstring& outUrl)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    bool result = false;

    // 1. Initialize WinHTTP
    hSession = WinHttpOpen(L"NetPulse Update Checker",  
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, 
                           WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession)
    {
        // 2. Connect to GitHub API
        hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    }

    if (hConnect)
    {
        // 3. Create Request
        std::wstring releasePath = L"/repos/";
        releasePath += APP_GITHUB_REPO;
        releasePath += L"/releases/latest";
        hRequest = WinHttpOpenRequest(hConnect, L"GET", releasePath.c_str(),
                                      NULL, WINHTTP_NO_REFERER, 
                                      WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                      WINHTTP_FLAG_SECURE);
    }

    if (hRequest)
    {
        // 4. Send Request
        bool bResults = WinHttpSendRequest(hRequest,
                                           WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                           WINHTTP_NO_REQUEST_DATA, 0, 
                                           0, 0);

        if (bResults)
        {
            bResults = WinHttpReceiveResponse(hRequest, NULL);
        }

        if (bResults)
        {
            // 5. Read Data
            std::string responseData;
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;

            do
            {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

                if (dwSize == 0) break;

                std::vector<char> buffer(dwSize + 1); // +1 for null terminator safety
                if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
                {
                    responseData.append(buffer.data(), dwDownloaded);
                }
            } while (dwSize > 0);

            // 6. Simple JSON Parsing (String Search)
            // Look for "tag_name": "vX.X.X"
            std::regex tagRegex("\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
            std::smatch m;
            if (std::regex_search(responseData, m, tagRegex))
            {
                std::string tagUtf8 = m[1].str();
                
                // Look for html_url to release page
                std::regex urlRegex("\"html_url\"\\s*:\\s*\"([^\"]+)\""); 
                // Note: The first html_url in the payload is usually the release page
                if (std::regex_search(responseData, m, urlRegex))
                {
                   std::string urlUtf8 = m[1].str();
                   
                   // Convert to wstring
                   int size_needed = MultiByteToWideChar(CP_UTF8, 0, tagUtf8.c_str(), (int)tagUtf8.length(), NULL, 0);
                   std::wstring tagW(size_needed, 0);
                   MultiByteToWideChar(CP_UTF8, 0, tagUtf8.c_str(), (int)tagUtf8.length(), &tagW[0], size_needed);
                   outLatestVersion = tagW;

                   size_needed = MultiByteToWideChar(CP_UTF8, 0, urlUtf8.c_str(), (int)urlUtf8.length(), NULL, 0);
                   std::wstring urlW(size_needed, 0);
                   MultiByteToWideChar(CP_UTF8, 0, urlUtf8.c_str(), (int)urlUtf8.length(), &urlW[0], size_needed);
                   outUrl = urlW;

                   result = true;
                }
            }
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return result;
}

int UpdateChecker::CompareVersions(const std::wstring& v1, const std::wstring& v2)
{
    // Split by '.'
    auto split = [](const std::wstring& s) {
        std::vector<int> parts;
        std::wstringstream ss(s);
        std::wstring item;
        while (std::getline(ss, item, L'.')) {
            try {
                parts.push_back(std::stoi(item));
            } catch (...) {
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

    if (i < parts1.size()) return 1; // v1 is longer (e.g. 1.0.1 > 1.0)
    if (i < parts2.size()) return -1; // v2 is longer

    return 0;
}

} // namespace NetPulse
