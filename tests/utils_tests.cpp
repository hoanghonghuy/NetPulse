#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "TestUtils.h"

using namespace NetPulse;

namespace NetPulseTests
{

void RunUtilsTests()
{
    LogTestMessage(L"=== Utils tests ===");

    // FormatBytes tests
    AssertTrue(FormatBytes(500ULL) == L"500 B", L"FormatBytes 500 B");
    AssertTrue(FormatBytes(1024ULL) == L"1.00 KB", L"FormatBytes 1 KB");
    AssertTrue(FormatBytes(1024ULL * 1024ULL) == L"1.00 MB", L"FormatBytes 1 MB");

    // FormatSpeed tests for BytesPerSecond (auto-scaling)
    std::wstring s1 = FormatSpeed(512.0, SpeedUnit::BytesPerSecond);
    AssertTrue(s1 == L"512.00 B/s", L"FormatSpeed 512 B/s");

    std::wstring s2 = FormatSpeed(1024.0, SpeedUnit::BytesPerSecond);
    AssertTrue(s2 == L"1.00 KB/s", L"FormatSpeed 1 KB/s");

    std::wstring s3 = FormatSpeed(1024.0 * 1024.0, SpeedUnit::BytesPerSecond);
    AssertTrue(s3 == L"1.00 MB/s", L"FormatSpeed 1 MB/s");

    // ConvertSpeed tests removed as function is deprecated
}

} // namespace NetPulseTests
