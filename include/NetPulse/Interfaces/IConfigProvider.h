#ifndef NETWORK_MONITOR_ICONFIG_PROVIDER_H
#define NETWORK_MONITOR_ICONFIG_PROVIDER_H

#include "NetPulse/Common.h"

namespace NetPulse
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
    virtual bool SetAutoStart(bool enable, bool asAdmin = false) = 0;

    /**
     * Check if running in portable mode
     * @return true if portable mode is enabled and INI file exists, false otherwise
     */
    virtual bool IsPortableMode() const { return false; }

    /**
     * Check if portable config file exists (regardless of whether portable mode is enabled)
     * @return true if netpulse.ini exists, false otherwise
     */
    virtual bool HasPortableConfigFile() const { return false; }

    /**
     * Enable/disable portable mode preference
     * @param enable true to enable portable mode, false to use Registry
     * @return true if successful, false otherwise
     */
    virtual bool SetPortableMode(bool /* enable */) { return false; }

    /**
     * Enable portable mode by creating the INI file and migrating current settings
     * @param currentConfig Current configuration to export
     * @return true if successful, false otherwise
     */
    virtual bool EnablePortableMode(const AppConfig& /* currentConfig */) { return false; }

    /**
     * Get the path to the portable config file
     * @return Full path to the INI file
     */
    virtual std::wstring GetPortableFilePath() const { return L""; }
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_ICONFIG_PROVIDER_H
