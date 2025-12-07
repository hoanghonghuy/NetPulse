#include "NetworkMonitor/LanguageManager.h"

namespace NetworkMonitor
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
        SetThreadUILanguage(langId);
    }
}

} // namespace NetworkMonitor
