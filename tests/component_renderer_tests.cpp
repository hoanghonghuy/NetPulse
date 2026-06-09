#include "TestUtils.h"
#include "NetPulse/SparklineRenderer.h"
#include "NetPulse/ChartRenderer.h"
#include "NetPulse/HistoryLogger.h"

using namespace NetPulse;

namespace NetPulseTests
{

void TestSparklineRendererRetention()
{
    LogTestMessage(L"  Running TestSparklineRendererRetention...");

    SparklineRenderer renderer(5);
    for (int i = 1; i <= 8; ++i)
    {
        renderer.AddDataPoint(static_cast<double>(i));
    }

    AssertTrue(renderer.GetPointCount() == 5, L"SparklineRenderer retains max points");
    AssertTrue(renderer.GetLastValue() == 8.0, L"SparklineRenderer keeps newest value");
    AssertTrue(renderer.GetMaxValue() >= renderer.GetMinValue(),
               L"SparklineRenderer min/max invariant");

    renderer.Clear();
    AssertTrue(renderer.GetPointCount() == 0, L"SparklineRenderer clear removes points");
}

void TestChartRendererConvertDailyUsage()
{
    LogTestMessage(L"  Running TestChartRendererConvertDailyUsage...");

    DailyUsage day1{};
    day1.day = 3;
    day1.bytesDown = 1000;
    day1.bytesUp = 500;

    DailyUsage day2{};
    day2.day = 4;
    day2.bytesDown = 2000;
    day2.bytesUp = 700;

    auto points = ChartRenderer::ConvertDailyUsage({day1, day2});
    AssertTrue(points.size() == 2, L"ChartRenderer converts daily usage count");
    AssertTrue(points[0].valueDown == 1000ULL && points[0].valueUp == 500ULL,
               L"ChartRenderer preserves daily byte values");
}

void TestChartRendererHitTestBar()
{
    LogTestMessage(L"  Running TestChartRendererHitTestBar...");

    RECT chartRect{0, 0, 220, 120};
    ChartOptions options;
    const int dataCount = 4;

    int hit = ChartRenderer::HitTestBar(chartRect, 50, dataCount, options);
    AssertTrue(hit >= 0 && hit < dataCount, L"ChartRenderer hit test finds bar inside chart area");

    int miss = ChartRenderer::HitTestBar(chartRect, -5, dataCount, options);
    AssertTrue(miss == -1, L"ChartRenderer hit test misses outside chart");
}

void RunComponentRendererTests()
{
    LogTestMessage(L"=== Component renderer tests ===");

    TestSparklineRendererRetention();
    TestChartRendererConvertDailyUsage();
    TestChartRendererHitTestBar();

    LogTestMessage(L"Component renderer tests completed.");
}

} // namespace NetPulseTests
