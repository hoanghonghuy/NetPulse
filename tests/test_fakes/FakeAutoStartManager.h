#pragma once

#include "NetPulse/Interfaces/IAutoStartManager.h"

namespace NetPulseTests
{

class FakeAutoStartManager : public NetPulse::IAutoStartManager
{
public:
    bool m_registryEnabled;
    bool m_scheduledTaskEnabled;

    int m_registryCalls;
    int m_scheduledTaskCalls;
    int m_isRegistryEnabledCalls;
    int m_isScheduledTaskEnabledCalls;

    bool m_configureRegistryResult;
    bool m_configureScheduledTaskResult;

    bool m_lastRegistryEnable;
    bool m_lastScheduledTaskEnable;
    std::wstring m_lastRegistryExePath;
    std::wstring m_lastScheduledTaskExePath;

    FakeAutoStartManager()
        : m_registryEnabled(false)
        , m_scheduledTaskEnabled(false)
        , m_registryCalls(0)
        , m_scheduledTaskCalls(0)
        , m_isRegistryEnabledCalls(0)
        , m_isScheduledTaskEnabledCalls(0)
        , m_configureRegistryResult(true)
        , m_configureScheduledTaskResult(true)
        , m_lastRegistryEnable(false)
        , m_lastScheduledTaskEnable(false)
    {
    }

    bool ConfigureRegistry(bool enable, const std::wstring& exePath) override
    {
        ++m_registryCalls;
        m_lastRegistryEnable = enable;
        m_lastRegistryExePath = exePath;
        if (m_configureRegistryResult)
        {
            m_registryEnabled = enable;
            return true;
        }
        return false;
    }

    bool ConfigureScheduledTask(bool enable, const std::wstring& exePath) override
    {
        ++m_scheduledTaskCalls;
        m_lastScheduledTaskEnable = enable;
        m_lastScheduledTaskExePath = exePath;
        if (m_configureScheduledTaskResult)
        {
            m_scheduledTaskEnabled = enable;
            return true;
        }
        return false;
    }

    bool IsRegistryEnabled() override
    {
        ++m_isRegistryEnabledCalls;
        return m_registryEnabled;
    }

    bool IsScheduledTaskEnabled() override
    {
        ++m_isScheduledTaskEnabledCalls;
        return m_scheduledTaskEnabled;
    }
};

} // namespace NetPulseTests
