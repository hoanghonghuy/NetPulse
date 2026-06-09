#pragma once

#include "NetPulse/Interfaces/IHttpClient.h"
#include <chrono>
#include <thread>

namespace NetPulseTests
{

class FakeHttpClient : public NetPulse::IHttpClient
{
public:
    bool m_getSuccess = true;
    std::string m_getBody;

    bool m_downloadSuccess = true;
    bool m_uploadSuccess = true;
    int m_getCallCount = 0;
    int m_downloadCallCount = 0;
    int m_uploadCallCount = 0;
    std::wstring m_lastHost;
    std::wstring m_lastPath;
    std::chrono::milliseconds m_downloadBlockMs{200};
    std::chrono::milliseconds m_uploadBlockMs{200};

    bool HttpGet(const std::wstring& host,
                 const std::wstring& path,
                 std::string& outBody) override
    {
        ++m_getCallCount;
        m_lastHost = host;
        m_lastPath = path;
        if (!m_getSuccess)
        {
            outBody.clear();
            return false;
        }
        outBody = m_getBody;
        return true;
    }

    bool HttpDownload(const std::wstring& host,
                      const std::wstring& path,
                      double& speedMbps,
                      std::function<void(size_t bytesReceived)> progressCallback,
                      std::atomic<bool>* cancelFlag) override
    {
        ++m_downloadCallCount;
        m_lastHost = host;
        m_lastPath = path;

        const auto step = std::chrono::milliseconds(25);
        auto elapsed = std::chrono::milliseconds(0);
        size_t bytes = 0;

        while (elapsed < m_downloadBlockMs)
        {
            if (cancelFlag && cancelFlag->load())
            {
                speedMbps = 0.0;
                return false;
            }

            std::this_thread::sleep_for(step);
            elapsed += step;
            bytes += 4096;
            if (progressCallback)
            {
                progressCallback(bytes);
            }
        }

        if (!m_downloadSuccess || (cancelFlag && cancelFlag->load()))
        {
            speedMbps = 0.0;
            return false;
        }

        speedMbps = 12.5;
        return true;
    }

    bool HttpUpload(const std::wstring& host,
                    const std::wstring& path,
                    size_t dataSize,
                    double& speedMbps,
                    std::function<void(size_t bytesSent)> progressCallback,
                    std::atomic<bool>* cancelFlag) override
    {
        ++m_uploadCallCount;
        m_lastHost = host;
        m_lastPath = path;

        if (cancelFlag && cancelFlag->load())
        {
            speedMbps = 0.0;
            return false;
        }

        const auto step = std::chrono::milliseconds(25);
        auto elapsed = std::chrono::milliseconds(0);

        while (elapsed < m_uploadBlockMs)
        {
            if (cancelFlag && cancelFlag->load())
            {
                speedMbps = 0.0;
                return false;
            }

            std::this_thread::sleep_for(step);
            elapsed += step;
        }

        if (progressCallback)
        {
            progressCallback(dataSize);
        }

        if (!m_uploadSuccess || (cancelFlag && cancelFlag->load()))
        {
            speedMbps = 0.0;
            return false;
        }

        speedMbps = 8.0;
        return true;
    }
};

} // namespace NetPulseTests
