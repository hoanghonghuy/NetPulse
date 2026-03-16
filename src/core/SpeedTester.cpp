#include "NetPulse/SpeedTester.h"

// NOMINMAX to prevent Windows min/max macro conflicts with std::min/max
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <winhttp.h>
#include <chrono>
#include <vector>
#include <random>
#include <string>
#include <algorithm>
#include "NetPulse/Utils.h"
#include "../../resources/resource.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

namespace NetPulse
{

// Helper: Convert UTF-8 std::string to std::wstring using Windows API
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

SpeedTester::SpeedTester()
{
}

SpeedTester::~SpeedTester()
{
    CancelTest();
    if (m_testThread.joinable())
    {
        m_testThread.join();
    }
}

void SpeedTester::StartTest(std::function<void(int progress, const std::wstring& status)> progressCallback)
{
    if (m_running.load())
    {
        return; // Already running
    }
    
    // Wait for any previous thread to finish
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
    
    // Initialize Winsock once for the entire test
    WSADATA wsaData;
    bool wsaInit = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    
    try
    {
        // Use do-while(false) for structured cancellation flow
        do
        {
            // Phase 1: Ping (0-10%)
            if (progressCallback) progressCallback(0, L"Measuring latency...");
            
            if (m_cancelled.load())
            {
                result.success = false;
                result.errorMessage = L"Test cancelled";
                break;
            }
            
            result.pingMs = MeasurePing(L"speed.cloudflare.com");
            if (progressCallback) progressCallback(10, L"Latency: " + std::to_wstring(result.pingMs) + L" ms");
            
            // Phase 2: Download (10-60%)
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
            
            // Phase 3: Upload (60-100%)
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
    
    // Cleanup Winsock
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
    // Use TCP connection timing instead of ICMP (avoids complex IcmpSendEcho API)
    // Note: WSAStartup/WSACleanup is managed by RunTest()
    
    ADDRINFOW hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    PADDRINFOW result = nullptr;
    if (GetAddrInfoW(host.c_str(), L"443", &hints, &result) != 0)
    {
        WSACleanup();
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
        
        // Measure TCP connect time
        auto startTime = std::chrono::high_resolution_clock::now();
        
        int connectResult = connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen));
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        closesocket(sock);
        
        if (connectResult == 0)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            totalMs += static_cast<int>(elapsed);
            successCount++;
        }
    }
    
    FreeAddrInfoW(result);
    
    return successCount > 0 ? (totalMs / successCount) : -1;
}

double SpeedTester::MeasureDownloadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback)
{
    double speedMbps = 0.0;
    
    // Use Cloudflare speed test endpoint
    std::wstring host = L"speed.cloudflare.com";
    std::wstring path = L"/__down?bytes=" + std::to_wstring(DOWNLOAD_SIZE);
    
    auto lastUpdate = std::chrono::steady_clock::now();
    size_t lastBytes = 0;
    
    bool success = HttpDownload(host, path, DOWNLOAD_SIZE, speedMbps, 
        [&](size_t bytesReceived) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count();
            
            if (elapsed >= 200) // Update every 200ms
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
    
    // Use Cloudflare speed test endpoint for upload
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
    (void)expectedBytes; // Unused parameter
    HINTERNET hSession = WinHttpOpen(L"NetPulse/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    
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
    
    // Send request
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Read data and measure time
    auto startTime = std::chrono::high_resolution_clock::now();
    size_t totalBytesRead = 0;
    DWORD bytesAvailable = 0;
    
    std::vector<BYTE> buffer(65536); // 64KB buffer
    
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
    {
        if (m_cancelled.load()) break;
        
        DWORD bytesToRead = (std::min)(bytesAvailable, static_cast<DWORD>(buffer.size()));
        DWORD bytesRead = 0;
        
        if (WinHttpReadData(hRequest, buffer.data(), bytesToRead, &bytesRead))
        {
            totalBytesRead += bytesRead;
            if (progressCallback) progressCallback(totalBytesRead);
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
    
    return !m_cancelled.load() && totalBytesRead > 0;
}

bool SpeedTester::HttpUpload(const std::wstring& host, const std::wstring& path,
                              size_t dataSize, double& speedMbps,
                              std::function<void(size_t bytesSent)> progressCallback)
{
    HINTERNET hSession = WinHttpOpen(L"NetPulse/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    
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
    
    // Generate random data for upload
    std::vector<BYTE> uploadData(dataSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < dataSize; i++)
    {
        uploadData[i] = static_cast<BYTE>(dis(gen));
    }
    
    // Set content type
    const wchar_t* headers = L"Content-Type: application/octet-stream\r\n";
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Send request with data
    if (!WinHttpSendRequest(hRequest, headers, static_cast<DWORD>(-1),
                            uploadData.data(), static_cast<DWORD>(dataSize),
                            static_cast<DWORD>(dataSize), 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Progress callback (approximate - WinHTTP doesn't provide byte-level progress)
    if (progressCallback) progressCallback(dataSize);
    
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
    
    return !m_cancelled.load();
}

} // namespace NetPulse
