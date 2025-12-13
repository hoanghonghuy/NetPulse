#pragma once

#include "NetworkMonitor/Interfaces/ISpeedTestProvider.h"
#include <atomic>
#include <thread>
#include <mutex>

namespace NetworkMonitor
{

/**
 * @brief Measures network bandwidth using HTTP download/upload tests
 * 
 * Uses Cloudflare CDN for download tests and httpbin.org for upload tests.
 */
class SpeedTester : public ISpeedTestProvider
{
public:
    SpeedTester();
    ~SpeedTester() override;
    
    // ISpeedTestProvider implementation
    void StartTest(std::function<void(int progress, const std::wstring& status)> progressCallback) override;
    void CancelTest() override;
    bool IsRunning() const override;
    SpeedTestResult GetLastResult() const override;
    void SetResultCallback(std::function<void(const SpeedTestResult&)> callback) override;
    
private:
    // Test execution
    void RunTest(std::function<void(int progress, const std::wstring& status)> progressCallback);
    
    // Individual test phases
    int MeasurePing(const std::wstring& host);
    double MeasureDownloadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback);
    double MeasureUploadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback);
    
    // HTTP helpers using WinHTTP
    bool HttpDownload(const std::wstring& host, const std::wstring& path, 
                      size_t expectedBytes, double& speedMbps,
                      std::function<void(size_t bytesReceived)> progressCallback);
    bool HttpUpload(const std::wstring& host, const std::wstring& path,
                    size_t dataSize, double& speedMbps,
                    std::function<void(size_t bytesSent)> progressCallback);
    
    // State
    std::thread m_testThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancelled{false};
    
    mutable std::mutex m_resultMutex;
    SpeedTestResult m_lastResult;
    std::function<void(const SpeedTestResult&)> m_resultCallback;
    
    // Test parameters
    static constexpr size_t DOWNLOAD_SIZE = 10 * 1024 * 1024; // 10 MB
    static constexpr size_t UPLOAD_SIZE = 5 * 1024 * 1024;    // 5 MB
    static constexpr int PING_COUNT = 3;                       // Average of 3 pings
};

} // namespace NetworkMonitor
