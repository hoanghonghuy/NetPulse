#ifndef NETWORK_MONITOR_CONFIGMANAGER_H
#define NETWORK_MONITOR_CONFIGMANAGER_H

#include "NetPulse/Common.h"
#include "NetPulse/Interfaces/IConfigProvider.h"
#include <windows.h>

namespace NetPulse {

class ConfigManager : public IConfigProvider {
public:
  ConfigManager();
  ~ConfigManager() override;

  /**
   * Load configuration from registry or portable file
   * @param config Output configuration
   * @return true if successful, false otherwise
   */
  bool LoadConfig(AppConfig &config) override;

  /**
   * Save configuration to registry or portable file
   * @param config Configuration to save
   * @return true if successful, false otherwise
   */
  bool SaveConfig(const AppConfig &config) override;

  /**
   * Enable/disable auto-start with Windows
   * @param enable true to enable, false to disable
   * @return true if successful, false otherwise
   */
  bool SetAutoStart(bool enable, bool asAdmin = false) override;

  /**
   * Check if auto-start is enabled
   * @return true if enabled, false otherwise
   */
  bool IsAutoStartEnabled() override;

  /**
   * Check if running in portable mode
   * @return true if portable mode is enabled and INI file exists, false otherwise
   */
  bool IsPortableMode() const override { return m_isPortable; }

  /**
   * Check if portable config file exists (regardless of whether portable mode is enabled)
   * @return true if netpulse.ini exists, false otherwise
   */
  bool HasPortableConfigFile() const override { return m_portableFileExists; }

  /**
   * Enable/disable portable mode preference (stored in Registry)
   * @param enable true to enable portable mode, false to use Registry
   * @return true if successful, false otherwise
   */
  bool SetPortableMode(bool enable) override;

  /**
   * Enable portable mode by creating the INI file and migrating current settings
   * @param currentConfig Current configuration to export
   * @return true if successful, false otherwise
   */
  bool EnablePortableMode(const AppConfig& currentConfig) override;

  /**
   * Get the path to the portable config file
   * @return Full path to the INI file
   */
  std::wstring GetPortableFilePath() const override { return m_portableFilePath; }

private:
  /**
   * Initialize portable mode detection
   */
  void DetectPortableMode();

  /**
   * Load configuration from INI file
   * @param config Output configuration
   * @return true if successful, false otherwise
   */
  bool LoadConfigFromFile(AppConfig& config);

  /**
   * Save configuration to INI file
   * @param config Configuration to save
   * @return true if successful, false otherwise
   */
  bool SaveConfigToFile(const AppConfig& config);

  /**
   * Open or create registry key for application settings
   * @param hKey Output key handle
   * @return true if successful, false otherwise
   */
  bool OpenSettingsKey(HKEY &hKey);

  /**
   * Read DWORD value from registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param defaultValue Default value if not found
   * @return Value read from registry or default
   */
  DWORD ReadDWORD(HKEY hKey, const wchar_t *valueName, DWORD defaultValue);

  /**
   * Write DWORD value to registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param value Value to write
   * @return true if successful, false otherwise
   */
  bool WriteDWORD(HKEY hKey, const wchar_t *valueName, DWORD value);

  /**
   * Read string value from registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param defaultValue Default value if not found
   * @return Value read from registry or default
   */
  std::wstring ReadString(HKEY hKey, const wchar_t *valueName,
                          const std::wstring &defaultValue);

  /**
   * Write string value to registry
   * @param hKey Registry key handle
   * @param valueName Value name
   * @param value Value to write
   * @return true if successful, false otherwise
   */
  bool WriteString(HKEY hKey, const wchar_t *valueName,
                   const std::wstring &value);

  // INI file helpers
  DWORD ReadIniDWORD(const wchar_t* section, const wchar_t* key, DWORD defaultValue);
  std::wstring ReadIniString(const wchar_t* section, const wchar_t* key, const std::wstring& defaultValue);
  bool WriteIniDWORD(const wchar_t* section, const wchar_t* key, DWORD value);
  bool WriteIniString(const wchar_t* section, const wchar_t* key, const std::wstring& value);

private:
  // FIX: Thêm const vào đây
  static constexpr const wchar_t *REGISTRY_PATH = L"Software\\NetworkMonitor";
  static constexpr const wchar_t *AUTOSTART_PATH =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  static constexpr const wchar_t *PORTABLE_FILENAME = L"netpulse.ini";
  static constexpr const wchar_t *INI_SECTION = L"Settings";

  bool m_isPortable = false;           // Currently using portable mode (file exists AND enabled)
  bool m_portableFileExists = false;   // INI file exists on disk
  std::wstring m_portableFilePath;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_CONFIGMANAGER_H

