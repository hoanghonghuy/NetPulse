#ifndef NETPULSE_CHART_RENDERER_H
#define NETPULSE_CHART_RENDERER_H

#include "NetPulse/Common.h"
#include "NetPulse/HistoryLogger.h"
#include <windows.h>
#include <vector>
#include <string>

namespace NetPulse
{

// Chart drawing options
struct ChartOptions
{
    bool darkTheme = false;         // Use dark theme colors
    bool showGridLines = true;      // Show horizontal grid lines
    bool showLegend = true;         // Show download/upload legend
    int barSpacing = 2;             // Pixels between bars
    int axisPadding = 40;           // Padding for Y-axis labels
    int bottomPadding = 25;         // Padding for X-axis labels
    int topPadding = 20;            // Top padding
};

// Chart data point for generic bar chart
struct ChartDataPoint
{
    std::wstring label;             // X-axis label (e.g., "15" for day 15, or "Dec")
    uint64_t valueDown = 0;         // Download bytes
    uint64_t valueUp = 0;           // Upload bytes
};

/**
 * ChartRenderer - Static utility class for rendering charts using GDI
 */
class ChartRenderer
{
public:
    /**
     * Draw a bar chart showing download/upload data
     * @param hdc Device context to draw on
     * @param rect Drawing area rectangle
     * @param data Vector of data points
     * @param options Chart drawing options
     */
    static void DrawBarChart(
        HDC hdc,
        const RECT& rect,
        const std::vector<ChartDataPoint>& data,
        const ChartOptions& options);

    /**
     * Helper: Convert DailyUsage vector to ChartDataPoint vector
     */
    static std::vector<ChartDataPoint> ConvertDailyUsage(const std::vector<DailyUsage>& daily);

    /**
     * Helper: Convert MonthlyUsage vector to ChartDataPoint vector
     */
    static std::vector<ChartDataPoint> ConvertMonthlyUsage(const std::vector<MonthlyUsage>& monthly);

    /**
     * Hit test: Returns index of bar at given point, or -1 if none
     * @param chartRect The chart drawing rectangle
     * @param mouseX Mouse X position relative to chart
     * @param dataCount Number of data bars
     * @param options Chart drawing options
     * @return Index of bar (0-based) or -1 if not over a bar
     */
    static int HitTestBar(const RECT& chartRect, int mouseX, int dataCount, const ChartOptions& options);

private:
    // Theme colors
    static COLORREF GetTextColor(bool darkTheme);
    static COLORREF GetGridColor(bool darkTheme);
    static COLORREF GetDownloadColor(bool darkTheme);
    static COLORREF GetUploadColor(bool darkTheme);
    static COLORREF GetBorderColor(bool darkTheme);

    // Helper methods
    static void DrawGridLines(HDC hdc, const RECT& chartArea, uint64_t maxValue, 
                              const ChartOptions& options);
    static void DrawYAxisLabels(HDC hdc, const RECT& chartArea, uint64_t maxValue,
                                const ChartOptions& options);
    static void DrawLegend(HDC hdc, const RECT& rect, const ChartOptions& options);
};

} // namespace NetPulse

#endif // NETPULSE_CHART_RENDERER_H
