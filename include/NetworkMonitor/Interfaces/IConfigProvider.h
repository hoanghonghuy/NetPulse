#ifndef NETWORK_MONITOR_ICONFIG_PROVIDER_H
#define NETWORK_MONITOR_ICONFIG_PROVIDER_H

#include "NetworkMonitor/Common.h"

namespace NetworkMonitor
{

/**
 * Interface for configuration provider
 * Allows dependency injection and testing with mock implementations
 */
class IConfigProvider
{
public:
    virtual ~IConfigProvider() = default;

    /**
     * Load configuration
     * @param config Output configuration
     * @return true if successful, false otherwise
     */
    virtual bool LoadConfig(AppConfig& config) = 0;

    /**
     * Save configuration
     * @param config Configuration to save
     * @return true if successful, false otherwise
     */
    virtual bool SaveConfig(const AppConfig& config) = 0;

    /**
     * Check if auto-start is enabled
     * @return true if enabled, false otherwise
     */
    virtual bool IsAutoStartEnabled() = 0;

    /**
     * Enable/disable auto-start with Windows
     * @param enable true to enable, false to disable
     * @return true if successful, false otherwise
     */
    virtual bool SetAutoStart(bool enable) = 0;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_ICONFIG_PROVIDER_H
