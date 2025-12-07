#ifndef NETWORK_MONITOR_LANGUAGEMANAGER_H
#define NETWORK_MONITOR_LANGUAGEMANAGER_H

#include "NetworkMonitor/Common.h"

namespace NetworkMonitor
{

/**
 * LanguageManager - Manages application language/i18n settings
 * 
 * This class is responsible for applying language settings to the application,
 * following the Single Responsibility Principle.
 */
class LanguageManager
{
public:
    LanguageManager();
    ~LanguageManager();

    /**
     * Apply language from configuration
     * @param language The language to apply
     */
    void ApplyLanguage(AppLanguage language);

    /**
     * Get the Windows LANGID for a given AppLanguage
     * @param language The application language enum
     * @return The corresponding LANGID
     */
    static LANGID GetLangIdForLanguage(AppLanguage language);
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_LANGUAGEMANAGER_H
