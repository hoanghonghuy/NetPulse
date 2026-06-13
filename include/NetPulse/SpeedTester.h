#ifndef NETPULSE_SPEED_TESTER_H
#define NETPULSE_SPEED_TESTER_H

#include "NetPulse/Interfaces/ISpeedTestProvider.h"
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>

namespace NetPulse
{

class IHttpClient;

/**
 * @brief Measures network bandwidth using HTTP download/upload tests
 * 
 * Uses Cloudflare CDN for download and upload tests.
 */
class SpeedTester : public ISpeedTestProvider
{
public:
    explicit SpeedTester(std::shared_ptr<IHttpClient> httpClient = nullptr);
    ~SpeedTester() override;
    
    void StartTest(std::function<void(int progress, const std::wstring& status)> progressCallback) override;
    void CancelTest() final;
    bool IsRunning() const override;
    SpeedTestResult GetLastResult() const override;
    void SetResultCallback(std::function<void(const SpeedTestResult&)> callback) override;
    void ClearResultCallback() { m_resultCallback = nullptr; }
    
public:
    void RunTest(std::function<void(int progress, const std::wstring& status)> progressCallback);
    
    int MeasurePing(const std::wstring& host);
    double MeasureDownloadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback);
    double MeasureUploadSpeed(std::function<void(int progress, const std::wstring& status)> progressCallback);
    
private:
    IHttpClient& GetHttpClient();

    bool HttpDownload(const std::wstring& host, const std::wstring& path, 
                      size_t expectedBytes, double& speedMbps,
                      std::function<void(size_t bytesReceived)> progressCallback);
    bool HttpUpload(const std::wstring& host, const std::wstring& path,
                    size_t dataSize, double& speedMbps,
                    std::function<void(size_t bytesSent)> progressCallback);
    
    std::shared_ptr<IHttpClient> m_httpClient;
    std::unique_ptr<class WinHttpClient> m_defaultHttpClient;
    
    std::thread m_testThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancelled{false};
    
    mutable std::mutex m_resultMutex;
    SpeedTestResult m_lastResult;
    std::function<void(const SpeedTestResult&)> m_resultCallback;
    
    static constexpr size_t DOWNLOAD_SIZE = 10 * 1024 * 1024;
    static constexpr size_t UPLOAD_SIZE = 5 * 1024 * 1024;
    static constexpr int PING_COUNT = 3;
};

} // namespace NetPulse

#endif // NETPULSE_SPEED_TESTER_H
