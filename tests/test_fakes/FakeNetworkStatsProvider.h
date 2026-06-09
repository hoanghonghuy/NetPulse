#pragma once

#include "NetPulse/Interfaces/INetworkStatsProvider.h"

#include <map>
#include <vector>

namespace NetPulseTests
{

class FakeNetworkStatsProvider : public NetPulse::INetworkStatsProvider
{
public:
    NetPulse::NetworkStats m_aggregated;
    std::map<std::wstring, NetPulse::NetworkStats> m_interfaces;
    bool m_running = true;
    int m_updateCallCount = 0;

    std::vector<NetPulse::NetworkStats> GetAllStats() override
    {
        std::vector<NetPulse::NetworkStats> all;
        for (const auto& entry : m_interfaces)
        {
            all.push_back(entry.second);
        }
        if (all.empty() && m_aggregated.isActive)
        {
            all.push_back(m_aggregated);
        }
        return all;
    }

    NetPulse::NetworkStats GetAggregatedStats() override
    {
        return m_aggregated;
    }

    bool GetInterfaceStats(const std::wstring& interfaceName, NetPulse::NetworkStats& stats) override
    {
        auto it = m_interfaces.find(interfaceName);
        if (it == m_interfaces.end())
        {
            return false;
        }
        stats = it->second;
        return true;
    }

    bool Update() override
    {
        ++m_updateCallCount;
        return true;
    }

    bool IsRunning() const override
    {
        return m_running;
    }
};

} // namespace NetPulseTests
