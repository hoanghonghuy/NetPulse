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

void TestChartRendererGdiPainting()
{
    LogTestMessage(L"  Running TestChartRendererGdiPainting (Virtual Memory DC)...");

    // 1. Create a Memory Device Context for virtual rendering
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, 400, 300);
    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hBitmap));

    RECT rect = {0, 0, 400, 300};
    ChartOptions options;

    // Test Case A: Empty data points (should draw "No data available")
    std::vector<ChartDataPoint> emptyData;
    ChartRenderer::DrawBarChart(hdcMem, rect, emptyData, options);

    // Test Case B: Normal data points with light theme
    std::vector<ChartDataPoint> normalData;
    normalData.push_back({L"1", 100, 50});
    normalData.push_back({L"2", 200, 100});
    normalData.push_back({L"3", 0, 0}); // 0 values to test min height logic
    options.darkTheme = false;
    options.showGridLines = true;
    options.showLegend = true;
    ChartRenderer::DrawBarChart(hdcMem, rect, normalData, options);

    // Test Case C: Dense data points (>20 bars to test X-axis label skip logic) with dark theme
    std::vector<ChartDataPoint> denseData;
    for (int i = 0; i < 25; ++i)
    {
        denseData.push_back({std::to_wstring(i), 1000ULL * i, 500ULL * i});
    }
    options.darkTheme = true;
    options.showGridLines = true;
    options.showLegend = true;
    ChartRenderer::DrawBarChart(hdcMem, rect, denseData, options);

    // Test Case D: Invalid/zero bounds (should return early without crash)
    RECT invalidRect = {0, 0, 0, 0};
    ChartRenderer::DrawBarChart(hdcMem, invalidRect, normalData, options);

    // Test Case E: Monthly usage conversion and render
    std::vector<MonthlyUsage> monthly;
    monthly.push_back({1, 5000, 1000}); // Jan
    monthly.push_back({13, 3000, 2000}); // Unknown/invalid month number
    auto convertedMonthly = ChartRenderer::ConvertMonthlyUsage(monthly);
    ChartRenderer::DrawBarChart(hdcMem, rect, convertedMonthly, options);

    // 2. Cleanup GDI resources
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    AssertTrue(true, L"ChartRenderer GDI painting executed without crash");
}

void TestSparklineRendererGdiPainting()
{
    LogTestMessage(L"  Running TestSparklineRendererGdiPainting (Virtual Memory DC)...");

    // 1. Create a Memory Device Context for virtual rendering
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, 150, 50);
    HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hBitmap));

    RECT bounds = {0, 0, 150, 50};
    SparklineRenderer renderer(10);

    // Test Case A: Empty/Insufficient points (should return early)
    renderer.Render(hdcMem, bounds, RGB(255, 0, 0), 0);
    renderer.AddDataPoint(10.0);
    renderer.Render(hdcMem, bounds, RGB(255, 0, 0), 0); // Still 1 point, should return early

    // Test Case B: Render 2+ points (should draw line)
    renderer.AddDataPoint(20.0);
    renderer.AddDataPoint(15.0);
    renderer.Render(hdcMem, bounds, RGB(255, 0, 0), 0);

    // Test Case C: Render with fill color
    renderer.Render(hdcMem, bounds, RGB(255, 0, 0), RGB(0, 255, 0));

    // Test Case D: Invalid bounds (should return early without crash)
    RECT invalidBounds = {0, 0, 0, 0};
    renderer.Render(hdcMem, invalidBounds, RGB(255, 0, 0), RGB(0, 255, 0));

    // Test Case E: SetMaxPoints triggers recalculation and trim
    renderer.SetMaxPoints(2);
    AssertTrue(renderer.GetPointCount() == 2, L"SparklineRenderer correctly trimmed data to 2 points");

    // 2. Cleanup GDI resources
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    AssertTrue(true, L"SparklineRenderer GDI painting executed without crash");
}

void RunComponentRendererTests()
{
    LogTestMessage(L"=== Component renderer tests ===");

    TestSparklineRendererRetention();
    TestChartRendererConvertDailyUsage();
    TestChartRendererHitTestBar();
    TestChartRendererGdiPainting();
    TestSparklineRendererGdiPainting();

    LogTestMessage(L"Component renderer tests completed.");
}

} // namespace NetPulseTests
