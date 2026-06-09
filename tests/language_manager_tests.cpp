#include "NetPulse/LanguageManager.h"
#include "NetPulse/Utils.h"
#include "TestUtils.h"
#include "../../resources/resource.h"

using namespace NetPulse;

namespace NetPulseTests
{

void RunLanguageManagerTests()
{
    LogTestMessage(L"=== LanguageManager tests ===");

    LANGID english = LanguageManager::GetLangIdForLanguage(AppLanguage::English);
    LANGID vietnamese = LanguageManager::GetLangIdForLanguage(AppLanguage::Vietnamese);
    LANGID japanese = LanguageManager::GetLangIdForLanguage(AppLanguage::Japanese);
    LANGID korean = LanguageManager::GetLangIdForLanguage(AppLanguage::Korean);
    LANGID chinese = LanguageManager::GetLangIdForLanguage(AppLanguage::ChineseSimplified);
    LANGID systemDefault = LanguageManager::GetLangIdForLanguage(AppLanguage::SystemDefault);

    AssertTrue(english != 0, L"LanguageManager English LANGID is non-zero");
    AssertTrue(vietnamese != 0, L"LanguageManager Vietnamese LANGID is non-zero");
    AssertTrue(japanese != 0, L"LanguageManager Japanese LANGID is non-zero");
    AssertTrue(korean != 0, L"LanguageManager Korean LANGID is non-zero");
    AssertTrue(chinese != 0, L"LanguageManager Chinese LANGID is non-zero");
    AssertTrue(systemDefault != 0, L"LanguageManager SystemDefault LANGID is non-zero");

    AssertTrue(PRIMARYLANGID(english) == LANG_ENGLISH,
               L"LanguageManager English primary language id");
    AssertTrue(PRIMARYLANGID(vietnamese) == LANG_VIETNAMESE,
               L"LanguageManager Vietnamese primary language id");

    LANGID userDefault = GetUserDefaultUILanguage();
    AssertTrue(systemDefault == userDefault,
               L"LanguageManager SystemDefault matches user default UI language");

    LANGID invalidLanguage = LanguageManager::GetLangIdForLanguage(static_cast<AppLanguage>(999));
    AssertTrue(invalidLanguage == userDefault,
               L"LanguageManager invalid language falls back to user default");

    LanguageManager::ApplyLanguage(AppLanguage::English);
    std::wstring englishString = LoadStringResource(IDS_ALL_INTERFACES);
    AssertTrue(!englishString.empty(),
               L"LanguageManager ApplyLanguage English loads embedded resource string");

    LanguageManager::ApplyLanguage(AppLanguage::SystemDefault);
    AssertTrue(true, L"LanguageManager ApplyLanguage SystemDefault does not crash");

    const AppLanguage locales[] = {
        AppLanguage::English,
        AppLanguage::Vietnamese,
        AppLanguage::Japanese,
        AppLanguage::Korean,
        AppLanguage::ChineseSimplified
    };

    for (AppLanguage locale : locales)
    {
        LanguageManager::ApplyLanguage(locale);
        std::wstring appTitle = LoadStringResource(IDS_APP_TITLE);
        std::wstring dashboard = LoadStringResource(IDS_MENU_DASHBOARD);
        AssertTrue(!appTitle.empty(),
                   L"LanguageManager locale loads IDS_APP_TITLE");
        AssertTrue(!dashboard.empty(),
                   L"LanguageManager locale loads IDS_MENU_DASHBOARD");
    }

    LanguageManager::ApplyLanguage(AppLanguage::Vietnamese);
    std::wstring vietnameseDashboard = LoadStringResource(IDS_DASHBOARD_TITLE);
    const std::wstring expectedDashboard = L"B\u1ea3ng theo d\u00f5i l\u01b0u l\u01b0\u1ee3ng";
    AssertTrue(vietnameseDashboard == expectedDashboard,
               L"LanguageManager Vietnamese dashboard title matches embedded translation");
    AssertTrue(vietnameseDashboard.find(L"\u00e1\u00ba\u00a3") == std::wstring::npos,
               L"LanguageManager Vietnamese dashboard title avoids UTF-8-as-wide corruption");
}

} // namespace NetPulseTests
