#pragma once
#include <string>
#include <vector>
#include <functional>

namespace NetPulseTests
{

struct LoggedSample
{
    unsigned long long bytesDown;
    unsigned long long bytesUp;
    std::wstring interfaceName;
};

class FakeHistoryLoggerSink
{
public:
    std::vector<LoggedSample> m_samples;

    std::function<void(unsigned long long, unsigned long long, const std::wstring&)> GetCallback()
    {
        return [this](unsigned long long down, unsigned long long up, const std::wstring& iface) {
            m_samples.push_back({down, up, iface});
        };
    }

    void Clear()
    {
        m_samples.clear();
    }
};

} // namespace NetPulseTests
