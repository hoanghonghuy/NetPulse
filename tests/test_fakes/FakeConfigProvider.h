#pragma once

#include "NetPulse/Interfaces/IConfigProvider.h"

namespace NetPulseTests
{

class FakeConfigProvider : public NetPulse::IConfigProvider
{
public:
    bool m_autoStartEnabled = false;
    bool m_lastSetAutoStartValue = false;
    bool m_lastSetAutoStartAsAdmin = false;
    int m_setAutoStartCallCount = 0;

    bool LoadConfig(NetPulse::AppConfig& /*config*/) override
    {
        return true;
    }

    bool SaveConfig(const NetPulse::AppConfig& /*config*/) override
    {
        return true;
    }

    bool IsAutoStartEnabled() override
    {
        return m_autoStartEnabled;
    }

    bool SetAutoStart(bool enable, bool asAdmin = false) override
    {
        ++m_setAutoStartCallCount;
        m_lastSetAutoStartValue = enable;
        m_lastSetAutoStartAsAdmin = asAdmin;
        m_autoStartEnabled = enable;
        return true;
    }
};

} // namespace NetPulseTests
