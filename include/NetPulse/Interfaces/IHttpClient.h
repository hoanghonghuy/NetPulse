#ifndef NETPULSE_IHTTP_CLIENT_H
#define NETPULSE_IHTTP_CLIENT_H

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>

namespace NetPulse
{

class IHttpClient
{
public:
    virtual ~IHttpClient() = default;

    virtual bool HttpGet(const std::wstring& host,
                         const std::wstring& path,
                         std::string& outBody) = 0;

    virtual bool HttpDownload(const std::wstring& host,
                              const std::wstring& path,
                              double& speedMbps,
                              std::function<void(size_t bytesReceived)> progressCallback,
                              std::atomic<bool>* cancelFlag) = 0;

    virtual bool HttpUpload(const std::wstring& host,
                            const std::wstring& path,
                            size_t dataSize,
                            double& speedMbps,
                            std::function<void(size_t bytesSent)> progressCallback,
                            std::atomic<bool>* cancelFlag) = 0;
};

} // namespace NetPulse

#endif // NETPULSE_IHTTP_CLIENT_H
