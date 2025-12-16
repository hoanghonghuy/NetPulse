#include "NetPulse/LanguageManager.h"
#include "NetPulse/Utils.h"
#include <string>

namespace NetPulse
{

LanguageManager::LanguageManager()
{
}

LanguageManager::~LanguageManager()
{
}

LANGID LanguageManager::GetLangIdForLanguage(AppLanguage language)
{
    switch (language)
    {
    case AppLanguage::English:
        return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    case AppLanguage::Vietnamese:
        return MAKELANGID(LANG_VIETNAMESE, SUBLANG_VIETNAMESE_VIETNAM);

    case AppLanguage::Japanese:
        return MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN);

    case AppLanguage::Korean:
        return MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);

    case AppLanguage::ChineseSimplified:
        return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

    case AppLanguage::SystemDefault:
    default:
        return GetUserDefaultUILanguage();
    }
}

void LanguageManager::ApplyLanguage(AppLanguage language)
{
    LANGID langId = GetLangIdForLanguage(language);
    if (langId != 0)
    {
        // Debug logging
        const wchar_t* langName = L"Unknown";
        switch (language)
        {
        case AppLanguage::English: langName = L"English"; break;
        case AppLanguage::Vietnamese: langName = L"Vietnamese"; break;
        case AppLanguage::Japanese: langName = L"Japanese"; break;
        case AppLanguage::Korean: langName = L"Korean"; break;
        case AppLanguage::ChineseSimplified: langName = L"Chinese Simplified"; break;
        case AppLanguage::SystemDefault: langName = L"System Default"; break;
        }
        
        std::wstring logMsg = L"LanguageManager::ApplyLanguage: Applying ";
        logMsg += langName;
        logMsg += L" (LANGID: 0x";
        wchar_t hexBuf[16];
        swprintf_s(hexBuf, L"%04X", langId);
        logMsg += hexBuf;
        logMsg += L")";
        LogDebug(logMsg);
        
        // Set thread UI language for resource loading
        SetThreadUILanguage(langId);
        
        // CRITICAL: Also set thread locale to persist the change
        // This affects number/date formatting AND resource loading
        LCID lcid = MAKELCID(langId, SORT_DEFAULT);
        SetThreadLocale(lcid);
        
        // Force locale for all future threads
        wchar_t langIdStr[8] = { 0 };
        swprintf_s(langIdStr, 8, L"%04X", langId);
        // Buffer is initialized to 0, so swprintf_s adds one null, and the next byte is already 0, forming a multi-string.
        SetThreadPreferredUILanguages(MUI_LANGUAGE_ID, langIdStr, nullptr);
    }
}

} // namespace NetPulse
