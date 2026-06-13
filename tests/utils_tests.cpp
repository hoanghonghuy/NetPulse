#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/ThemeHelper.h"
#include "TestUtils.h"
#include "../../resources/resource.h"

using namespace NetPulse;



namespace NetPulseTests
{

void RunUtilsTests()
{
    LogTestMessage(L"=== Utils tests ===");

    AssertTrue(FormatBytes(500ULL) == L"500 B", L"FormatBytes 500 B");
    AssertTrue(FormatBytes(1024ULL) == L"1.00 KB", L"FormatBytes 1 KB");
    AssertTrue(FormatBytes(1024ULL * 1024ULL) == L"1.00 MB", L"FormatBytes 1 MB");
    AssertTrue(FormatBytes(1024ULL * 1024ULL * 1024ULL) == L"1.00 GB", L"FormatBytes 1 GB");
    AssertTrue(FormatBytes(1024ULL * 1024ULL * 1024ULL * 1024ULL) == L"1.00 TB", L"FormatBytes 1 TB");

    AssertTrue(FormatSpeed(512.0, SpeedUnit::BytesPerSecond) == L"512.00 B/s",
               L"FormatSpeed 512 B/s");
    AssertTrue(FormatSpeed(1024.0, SpeedUnit::BytesPerSecond) == L"1.00 KB/s",
               L"FormatSpeed 1 KB/s");
    AssertTrue(FormatSpeed(1024.0 * 1024.0, SpeedUnit::BytesPerSecond) == L"1.00 MB/s",
               L"FormatSpeed 1 MB/s");

    AssertTrue(FormatSpeed(1024.0, SpeedUnit::KiloBytesPerSecond) == L"1.00 KB/s",
               L"FormatSpeed KB/s unit");
    AssertTrue(FormatSpeed(1024.0 * 1024.0, SpeedUnit::MegaBytesPerSecond) == L"1.00 MB/s",
               L"FormatSpeed MB/s unit");
    AssertTrue(FormatSpeed(125000.0, SpeedUnit::MegaBitsPerSecond) == L"1.00 Mbps",
               L"FormatSpeed Mbps unit");

    AssertTrue(GetElapsedSeconds(1000, 2500) == 1.5, L"GetElapsedSeconds normal ordering");
    AssertTrue(GetElapsedSeconds(ULONG_MAX - 500, 1000) > 0.0,
               L"GetElapsedSeconds handles tick wraparound");

    std::wstring allIfaces = LoadStringResource(IDS_ALL_INTERFACES);
    AssertTrue(!allIfaces.empty(), L"LoadStringResource returns embedded string");

    std::wstring missing = LoadStringResource(999999);
    AssertTrue(missing.empty(), L"LoadStringResource missing id returns empty");

    // Test SetDebugLoggingEnabled, LogDebug, LogError không crash
    SetDebugLoggingEnabled(true);
    LogDebug(L"test debug message enabled");
    LogError(L"test error message");
    SetDebugLoggingEnabled(false);
    LogDebug(L"test debug message disabled");
    AssertTrue(true, L"SetDebugLoggingEnabled and log functions do not crash");

    // Test GetLastErrorString
    SetLastError(ERROR_ACCESS_DENIED);
    std::wstring errStr = GetLastErrorString();
    AssertTrue(!errStr.empty() && errStr != L"No error", L"GetLastErrorString returns non-empty message for ERROR_ACCESS_DENIED");
    SetLastError(0);
    AssertTrue(GetLastErrorString() == L"No error", L"GetLastErrorString returns No error when error code is 0");

    // Test FormatSpeed overflows to GB/s
    AssertTrue(FormatSpeed(1024.0 * 1024.0 * 1024.0 * 2.5, SpeedUnit::BytesPerSecond) == L"2.50 GB/s",
               L"FormatSpeed overflow to GB/s (BytesPerSecond)");
    AssertTrue(FormatSpeed(1024.0 * 1024.0 * 1024.0 * 3.5, SpeedUnit::KiloBytesPerSecond) == L"3.50 GB/s",
               L"FormatSpeed overflow to GB/s (KiloBytesPerSecond)");
    AssertTrue(FormatSpeed(1024.0 * 1024.0 * 1024.0 * 4.5, SpeedUnit::MegaBytesPerSecond) == L"4.50 GB/s",
               L"FormatSpeed overflow to GB/s (MegaBytesPerSecond)");

    // Test IsDarkThemeEnabled and IsCustomThemeEnabled
    AppConfig themeConfig;

    // Light themes
    themeConfig.themeMode = ThemeMode::Light;
    AssertTrue(!IsDarkThemeEnabled(themeConfig), L"IsDarkThemeEnabled(Light) is false");
    AssertTrue(!IsCustomThemeEnabled(themeConfig), L"IsCustomThemeEnabled(Light) is false");

    themeConfig.themeMode = ThemeMode::SolarizedLight;
    AssertTrue(!IsDarkThemeEnabled(themeConfig), L"IsDarkThemeEnabled(SolarizedLight) is false");
    AssertTrue(IsCustomThemeEnabled(themeConfig), L"IsCustomThemeEnabled(SolarizedLight) is true");

    // Dark themes
    themeConfig.themeMode = ThemeMode::Dark;
    AssertTrue(IsDarkThemeEnabled(themeConfig), L"IsDarkThemeEnabled(Dark) is true");
    AssertTrue(IsCustomThemeEnabled(themeConfig), L"IsCustomThemeEnabled(Dark) is true");

    themeConfig.themeMode = ThemeMode::Dracula;
    AssertTrue(IsDarkThemeEnabled(themeConfig), L"IsDarkThemeEnabled(Dracula) is true");
    AssertTrue(IsCustomThemeEnabled(themeConfig), L"IsCustomThemeEnabled(Dracula) is true");

    // System default theme
    themeConfig.themeMode = ThemeMode::SystemDefault;
    bool systemDark = ThemeHelper::IsSystemInDarkMode();
    AssertTrue(IsDarkThemeEnabled(themeConfig) == systemDark, L"IsDarkThemeEnabled(SystemDefault) matches system dark mode");
    AssertTrue(IsCustomThemeEnabled(themeConfig) == systemDark, L"IsCustomThemeEnabled(SystemDefault) matches system dark mode");

    // Test ShowErrorMessage (modal with AutoCloser)
    {
        g_messageBoxHook = [](HWND, const wchar_t*, const wchar_t*, UINT, bool) -> int { return IDOK; };
        ShowErrorMessage(L"Test Error Message", L"Test Error Title");
        g_messageBoxHook = nullptr;
        AssertTrue(true, L"ShowErrorMessage executed without blocking");
    }
}

} // namespace NetPulseTests
