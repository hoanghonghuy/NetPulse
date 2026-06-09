#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
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
}

} // namespace NetPulseTests
