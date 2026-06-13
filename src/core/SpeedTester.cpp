#include "NetPulse/SpeedTester.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <string>
#include "NetPulse/Interfaces/IHttpClient.h"
#include "NetPulse/WinHttpClient.h"
#include "NetPulse/Utils.h"
#include "../../resources/resource.h"

#pragma comment(lib, "ws2_32.lib")

namespace NetPulse
{

static std::wstring Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty())
    {
        return std::wstring();
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                       static_cast<int>(utf8.size()), nullptr, 0);
    if (wideLen <= 0)
    {
        return std::wstring();
    }

    std::wstring result(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                        static_cast<int>(utf8.size()), &result[0], wideLen);
    return result;
}

SpeedTester::SpeedTester(std::shared_ptr<IHttpClient> httpClient)
    : m_httpClient(std::move(httpClient))
{
}

SpeedTester::~SpeedTester()
{
    CancelTest();
    m_resultCallback = nullptr;
    if (m_testThread.joinable())
    {
        m_testThread.join();
    }
}

IHttpClient& SpeedTester::GetHttpClient()
{
    if (m_httpClient)
    {
        return *m_httpClient;
    }

    if (!m_defaultHttpClient)
    {
        m_defaultHttpClient = std::make_unique<WinHttpClient>(L"NetPulse/1.0");
    }

    return *m_defaultHttpClient;
}

void SpeedTester::StartTest(std::function<void(int progress, const std::wstring& status)> progressCallback)
{
    if (m_running.load())
    {
        return;
    }
    
    if (m_testThread.joinable())
    {
        m_testThread.join();
    }
    
    m_cancelled.store(false);
    m_running.store(true);
    
    m_testThread = std::thread([this, progressCallback]() {
        RunTest(progressCallback);
    });
}

void SpeedTester::CancelTest()
{
    m_cancelled.store(true);
}

bool SpeedTester::IsRunning() const
{
    return m_running.load();
}

SpeedTestResult SpeedTester::GetLastResult() const
{
    std::lock_guard<std::mutex> lock(m_resultMutex);
    return m_lastResult;
}

void SpeedTester::SetResultCallback(std::function<void(const SpeedTestResult&)> callback)
{
    m_resultCallback = callback;
}

void SpeedTester::RunTest(std::function<void(int progress, const std::wstring& status)> progressCallback)
{
    SpeedTestResult result;
    result.timestamp = std::time(nullptr);
    result.serverName = L"Cloudflare";
    
    WSADATA wsaData;
    bool wsaInit = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    
    try
    {
        do
        {
            if (progressCallback) progressCallback(0, L"Measuring latency...");
            
            if (m_cancelled.load())
            {
                result.success = false;
                result.errorMessage = L"Test cancelled";
                break;
            }
            
            result.pingMs = MeasurePing(L"speed.cloudflare.com");
            if (progressCallback) progressCallback(10, L"Latency: " + std::to_wstring(result.pingMs) + L" ms");
            
            if (m_cancelled.load())
            {
                result.success = false;
                result.errorMessage = L"Test cancelled";
                break;
            }
            
            std::wstring downloadStatus = LoadStringResource(IDS_SPEED_TEST_DOWNLOAD);
            if (downloadStatus.empty()) downloadStatus = L"Testing download speed...";
            if (progressCallback) progressCallback(10, downloadStatus);
            
            result.downloadMbps = MeasureDownloadSpeed([&progressCallback, downloadStatus](int p, const std::wstring& s) {
                if (progressCallback) progressCallback(10 + (p * 50 / 100), s);
            });

            if (m_cancelled.load())
            {
                result.success = false;
                result.errorMessage = L"Test cancelled";
                break;
            }
            
            std::wstring uploadStatus = LoadStringResource(IDS_SPEED_TEST_UPLOAD);
            if (uploadStatus.empty()) uploadStatus = L"Testing upload speed...";
            if (progressCallback) progressCallback(60, uploadStatus);
            
            result.uploadMbps = MeasureUploadSpeed([&progressCallback, uploadStatus](int p, const std::wstring& s) {
                if (progressCallback) progressCallback(60 + (p * 40 / 100), s);
            });

            if (m_cancelled.load())
            {
                result.success = false;
                result.errorMessage = L"Test cancelled";
                break;
            }
            
            result.success = true;
            std::wstring completeStatus = LoadStringResource(IDS_SPEED_TEST_COMPLETE);
            if (completeStatus.empty()) completeStatus = L"Test complete";
            if (progressCallback) progressCallback(100, completeStatus);
        } while (false);
    }
    catch (const std::exception& e)
    {
        result.success = false;
        result.errorMessage = Utf8ToWide(e.what());
    }
    catch (...)
    {
        result.success = false;
        result.errorMessage = L"Unknown error occurred";
    }
    
    if (wsaInit)
    {
        WSACleanup();
    }
    
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);
        m_lastResult = result;
    }
    
    m_running.store(false);
    
    if (m_resultCallback)
    {
        m_resultCallback(result);
    }
}

int SpeedTester::MeasurePing(const std::wstring& host)
{
    ADDRINFOW hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    PADDRINFOW addrResult = nullptr;
    if (GetAddrInfoW(host.c_str(), L"443", &hints, &addrResult) != 0)
    {
        return -1;
    }
    
    int totalMs = 0;
    int successCount = 0;
    
    for (int i = 0; i < PING_COUNT && !m_cancelled.load(); i++)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
        {
            continue;
        }
        
        // Set to non-blocking mode to avoid OS-level connection hang (default 21 seconds on Windows)
        unsigned long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        int connectResult = connect(sock, addrResult->ai_addr, static_cast<int>(addrResult->ai_addrlen));
        
        if (connectResult == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS)
            {
                fd_set writefds;
                FD_ZERO(&writefds);
                FD_SET(sock, &writefds);
                
                fd_set errfds;
                FD_ZERO(&errfds);
                FD_SET(sock, &errfds);
                
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 200000; // 200ms timeout
                
                int selectResult = select(0, nullptr, &writefds, &errfds, &tv);
                if (selectResult > 0 && FD_ISSET(sock, &writefds) && !FD_ISSET(sock, &errfds))
                {
                    connectResult = 0; // Success
                }
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        closesocket(sock);
        
        if (connectResult == 0)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            totalMs += static_cast<int>(elapsed);
            successCount++;
        }
    }
    
    FreeAddrInfoW(addrResult);
    
    return successCount > 0 ? (totalMs / successCount) : -1;
}

double SpeedTester::MeasureDownloadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback)
{
    double speedMbps = 0.0;
    
    std::wstring host = L"speed.cloudflare.com";
    std::wstring path = L"/__down?bytes=" + std::to_wstring(DOWNLOAD_SIZE);
    
    auto lastUpdate = std::chrono::steady_clock::now();
    size_t lastBytes = 0;
    
    bool success = HttpDownload(host, path, DOWNLOAD_SIZE, speedMbps, 
        [&](size_t bytesReceived) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count();
            
            if (elapsed >= 200)
            {
                int progress = static_cast<int>((bytesReceived * 100) / DOWNLOAD_SIZE);
                double instantSpeed = ((bytesReceived - lastBytes) * 8.0) / (elapsed / 1000.0) / 1000000.0;
                
                wchar_t status[64];
                swprintf_s(status, L"Download: %.1f Mbps", instantSpeed);
                if (progressCallback) progressCallback(progress, status);
                
                lastUpdate = now;
                lastBytes = bytesReceived;
            }
        });
    
    if (!success)
    {
        return 0.0;
    }
    
    return speedMbps;
}

double SpeedTester::MeasureUploadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback)
{
    double speedMbps = 0.0;
    
    std::wstring host = L"speed.cloudflare.com";
    std::wstring path = L"/__up";
    
    auto lastUpdate = std::chrono::steady_clock::now();
    size_t lastBytes = 0;
    
    bool success = HttpUpload(host, path, UPLOAD_SIZE, speedMbps,
        [&](size_t bytesSent) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count();
            
            if (elapsed >= 200)
            {
                int progress = static_cast<int>((bytesSent * 100) / UPLOAD_SIZE);
                double instantSpeed = ((bytesSent - lastBytes) * 8.0) / (elapsed / 1000.0) / 1000000.0;
                
                wchar_t status[64];
                swprintf_s(status, L"Upload: %.1f Mbps", instantSpeed);
                if (progressCallback) progressCallback(progress, status);
                
                lastUpdate = now;
                lastBytes = bytesSent;
            }
        });
    
    if (!success)
    {
        return 0.0;
    }
    
    return speedMbps;
}

bool SpeedTester::HttpDownload(const std::wstring& host, const std::wstring& path,
                                size_t expectedBytes, double& speedMbps,
                                std::function<void(size_t bytesReceived)> progressCallback)
{
    (void)expectedBytes;
    return GetHttpClient().HttpDownload(host, path, speedMbps, progressCallback, &m_cancelled);
}

bool SpeedTester::HttpUpload(const std::wstring& host, const std::wstring& path,
                              size_t dataSize, double& speedMbps,
                              std::function<void(size_t bytesSent)> progressCallback)
{
    return GetHttpClient().HttpUpload(host, path, dataSize, speedMbps, progressCallback, &m_cancelled);
}

} // namespace NetPulse
