#ifndef NETWORK_MONITOR_IAUTOSTART_MANAGER_H
#define NETWORK_MONITOR_IAUTOSTART_MANAGER_H

#include <string>

namespace NetPulse
{

class IAutoStartManager
{
public:
    virtual ~IAutoStartManager() = default;

    virtual bool ConfigureRegistry(bool enable, const std::wstring& exePath) = 0;
    virtual bool ConfigureScheduledTask(bool enable, const std::wstring& exePath) = 0;
    virtual bool IsRegistryEnabled() = 0;
    virtual bool IsScheduledTaskEnabled() = 0;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_IAUTOSTART_MANAGER_H
