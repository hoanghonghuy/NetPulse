#ifndef NETPULSE_WIN_HTTP_CLIENT_H
#define NETPULSE_WIN_HTTP_CLIENT_H

#include "NetPulse/Interfaces/IHttpClient.h"

namespace NetPulse
{

class WinHttpClient : public IHttpClient
{
public:
    explicit WinHttpClient(const wchar_t* userAgent = L"NetPulse/1.0");

    bool HttpGet(const std::wstring& host,
                 const std::wstring& path,
                 std::string& outBody) override;

    bool HttpDownload(const std::wstring& host,
                      const std::wstring& path,
                      double& speedMbps,
                      std::function<void(size_t bytesReceived)> progressCallback,
                      std::atomic<bool>* cancelFlag) override;

    bool HttpUpload(const std::wstring& host,
                    const std::wstring& path,
                    size_t dataSize,
                    double& speedMbps,
                    std::function<void(size_t bytesSent)> progressCallback,
                    std::atomic<bool>* cancelFlag) override;

private:
    std::wstring m_userAgent;
};

} // namespace NetPulse

#endif // NETPULSE_WIN_HTTP_CLIENT_H
