#include "NetPulse/WinHttpClient.h"
#include "NetPulse/Common.h"

#include <winhttp.h>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

namespace NetPulse
{

WinHttpClient::WinHttpClient(const wchar_t* userAgent)
    : m_userAgent(userAgent ? userAgent : L"NetPulse/1.0")
{
}

bool WinHttpClient::HttpGet(const std::wstring& host,
                            const std::wstring& path,
                            std::string& outBody)
{
    outBody.clear();

    HINTERNET hSession = WinHttpOpen(m_userAgent.c_str(),
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                         INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        && WinHttpReceiveResponse(hRequest, nullptr))
    {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
            {
                break;
            }
            if (dwSize == 0)
            {
                break;
            }

            std::vector<char> buffer(dwSize + 1);
            if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
            {
                outBody.append(buffer.data(), dwDownloaded);
            }
        } while (dwSize > 0);

        success = !outBody.empty();
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

bool WinHttpClient::HttpDownload(const std::wstring& host,
                                   const std::wstring& path,
                                   double& speedMbps,
                                   std::function<void(size_t bytesReceived)> progressCallback,
                                   std::atomic<bool>* cancelFlag)
{
    speedMbps = 0.0;

    HINTERNET hSession = WinHttpOpen(m_userAgent.c_str(),
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                         INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    size_t totalBytesRead = 0;
    DWORD bytesAvailable = 0;
    std::vector<BYTE> buffer(65536);

    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
    {
        if (cancelFlag && cancelFlag->load())
        {
            break;
        }

        DWORD bytesToRead = (std::min)(bytesAvailable, static_cast<DWORD>(buffer.size()));
        DWORD bytesRead = 0;

        if (WinHttpReadData(hRequest, buffer.data(), bytesToRead, &bytesRead))
        {
            totalBytesRead += bytesRead;
            if (progressCallback)
            {
                progressCallback(totalBytesRead);
            }
        }
        else
        {
            break;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double durationSec = std::chrono::duration<double>(endTime - startTime).count();

    if (durationSec > 0 && totalBytesRead > 0)
    {
        speedMbps = (totalBytesRead * 8.0) / (durationSec * 1000000.0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return !(cancelFlag && cancelFlag->load()) && totalBytesRead > 0;
}

bool WinHttpClient::HttpUpload(const std::wstring& host,
                               const std::wstring& path,
                               size_t dataSize,
                               double& speedMbps,
                               std::function<void(size_t bytesSent)> progressCallback,
                               std::atomic<bool>* cancelFlag)
{
    speedMbps = 0.0;

    if (cancelFlag && cancelFlag->load())
    {
        return false;
    }

    HINTERNET hSession = WinHttpOpen(m_userAgent.c_str(),
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                         INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                            nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::vector<BYTE> uploadData(dataSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < dataSize; i++)
    {
        uploadData[i] = static_cast<BYTE>(dis(gen));
    }

    const wchar_t* headers = L"Content-Type: application/octet-stream\r\n";
    auto startTime = std::chrono::high_resolution_clock::now();

    if (!WinHttpSendRequest(hRequest, headers, static_cast<DWORD>(-1),
                            uploadData.data(), static_cast<DWORD>(dataSize),
                            static_cast<DWORD>(dataSize), 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (progressCallback)
    {
        progressCallback(dataSize);
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double durationSec = std::chrono::duration<double>(endTime - startTime).count();

    if (durationSec > 0)
    {
        speedMbps = (dataSize * 8.0) / (durationSec * 1000000.0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return !(cancelFlag && cancelFlag->load());
}

} // namespace NetPulse
