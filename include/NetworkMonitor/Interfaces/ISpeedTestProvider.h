#pragma once

#include <string>
#include <functional>
#include <ctime>

namespace NetworkMonitor
{

/**
 * @brief Result of a speed test
 */
struct SpeedTestResult
{
    double downloadMbps = 0.0;   // Download speed in Mbps
    double uploadMbps = 0.0;     // Upload speed in Mbps
    int pingMs = 0;              // Ping to test server in ms
    std::wstring serverName;     // Name of test server used
    std::time_t timestamp = 0;   // When test was performed
    bool success = false;        // Whether test completed successfully
    std::wstring errorMessage;   // Error message if failed
};

/**
 * @brief Interface for speed test providers
 */
class ISpeedTestProvider
{
public:
    virtual ~ISpeedTestProvider() = default;
    
    /**
     * @brief Start a speed test
     * @param progressCallback Called with progress percentage (0-100)
     */
    virtual void StartTest(std::function<void(int progress, const std::wstring& status)> progressCallback) = 0;
    
    /**
     * @brief Cancel an ongoing test
     */
    virtual void CancelTest() = 0;
    
    /**
     * @brief Check if a test is currently running
     */
    virtual bool IsRunning() const = 0;
    
    /**
     * @brief Get the result of the last test
     */
    virtual SpeedTestResult GetLastResult() const = 0;
    
    /**
     * @brief Set callback for when test completes
     */
    virtual void SetResultCallback(std::function<void(const SpeedTestResult&)> callback) = 0;
};

} // namespace NetworkMonitor
