#include "NetPulse/ChartRenderer.h"
#include "NetPulse/Utils.h"
#include <algorithm>
#include <cmath>

namespace NetPulse
{

// Theme colors - dark
constexpr COLORREF DARK_BG = RGB(30, 30, 30);
constexpr COLORREF DARK_TEXT = RGB(230, 230, 230);
constexpr COLORREF DARK_GRID = RGB(60, 60, 60);
constexpr COLORREF DARK_DOWNLOAD = RGB(0, 150, 255);   // Blue
constexpr COLORREF DARK_UPLOAD = RGB(0, 200, 100);     // Green
constexpr COLORREF DARK_BORDER = RGB(80, 80, 80);

// Theme colors - light
constexpr COLORREF LIGHT_BG = RGB(255, 255, 255);
constexpr COLORREF LIGHT_TEXT = RGB(30, 30, 30);
constexpr COLORREF LIGHT_GRID = RGB(220, 220, 220);
constexpr COLORREF LIGHT_DOWNLOAD = RGB(0, 120, 215);  // Blue
constexpr COLORREF LIGHT_UPLOAD = RGB(40, 167, 69);    // Green
constexpr COLORREF LIGHT_BORDER = RGB(180, 180, 180);

COLORREF ChartRenderer::GetBackgroundColor(bool darkTheme)
{
    return darkTheme ? DARK_BG : LIGHT_BG;
}

COLORREF ChartRenderer::GetTextColor(bool darkTheme)
{
    return darkTheme ? DARK_TEXT : LIGHT_TEXT;
}

COLORREF ChartRenderer::GetGridColor(bool darkTheme)
{
    return darkTheme ? DARK_GRID : LIGHT_GRID;
}

COLORREF ChartRenderer::GetDownloadColor(bool darkTheme)
{
    return darkTheme ? DARK_DOWNLOAD : LIGHT_DOWNLOAD;
}

COLORREF ChartRenderer::GetUploadColor(bool darkTheme)
{
    return darkTheme ? DARK_UPLOAD : LIGHT_UPLOAD;
}

COLORREF ChartRenderer::GetBorderColor(bool darkTheme)
{
    return darkTheme ? DARK_BORDER : LIGHT_BORDER;
}

// FormatBytes removed - using NetPulse::Utils version

std::vector<ChartDataPoint> ChartRenderer::ConvertDailyUsage(const std::vector<DailyUsage>& daily)
{
    std::vector<ChartDataPoint> result;
    result.reserve(daily.size());
    
    for (const auto& d : daily)
    {
        ChartDataPoint point;
        point.label = std::to_wstring(d.day);
        point.valueDown = d.bytesDown;
        point.valueUp = d.bytesUp;
        result.push_back(point);
    }
    
    return result;
}

std::vector<ChartDataPoint> ChartRenderer::ConvertMonthlyUsage(const std::vector<MonthlyUsage>& monthly)
{
    static const wchar_t* monthNames[] = {
        L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
        L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };
    
    std::vector<ChartDataPoint> result;
    result.reserve(monthly.size());
    
    for (const auto& m : monthly)
    {
        ChartDataPoint point;
        if (m.month >= 1 && m.month <= 12)
        {
            point.label = monthNames[m.month - 1];
        }
        else
        {
            point.label = std::to_wstring(m.month);
        }
        point.valueDown = m.bytesDown;
        point.valueUp = m.bytesUp;
        result.push_back(point);
    }
    
    return result;
}

void ChartRenderer::DrawGridLines(HDC hdc, const RECT& chartArea, uint64_t maxValue,
                                   const ChartOptions& options)
{
    if (!options.showGridLines || maxValue == 0)
    {
        return;
    }

    HPEN hPen = CreatePen(PS_DOT, 1, GetGridColor(options.darkTheme));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // Draw 4 horizontal grid lines
    int chartHeight = chartArea.bottom - chartArea.top;
    for (int i = 1; i <= 4; i++)
    {
        int y = chartArea.bottom - (chartHeight * i / 5);
        MoveToEx(hdc, chartArea.left, y, nullptr);
        LineTo(hdc, chartArea.right, y);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void ChartRenderer::DrawYAxisLabels(HDC hdc, const RECT& chartArea, uint64_t maxValue,
                                    const ChartOptions& options)
{
    if (maxValue == 0)
    {
        return;
    }

    SetTextColor(hdc, GetTextColor(options.darkTheme));
    SetBkMode(hdc, TRANSPARENT);

    // Draw 5 labels (0, 25%, 50%, 75%, 100%)
    int chartHeight = chartArea.bottom - chartArea.top;
    for (int i = 0; i <= 4; i++)
    {
        int y = chartArea.bottom - (chartHeight * i / 4);
        uint64_t value = (maxValue * i) / 4;
        std::wstring label = FormatBytes(value);

        RECT labelRect = { chartArea.left - options.axisPadding, y - 8, chartArea.left - 5, y + 8 };
        DrawTextW(hdc, label.c_str(), -1, &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
}

void ChartRenderer::DrawLegend(HDC hdc, const RECT& rect, const ChartOptions& options)
{
    if (!options.showLegend)
    {
        return;
    }

    int legendY = rect.top + 5;
    int legendX = rect.right - 150;

    SetTextColor(hdc, GetTextColor(options.darkTheme));
    SetBkMode(hdc, TRANSPARENT);

    // Download legend
    HBRUSH hDownBrush = CreateSolidBrush(GetDownloadColor(options.darkTheme));
    RECT downRect = { legendX, legendY, legendX + 12, legendY + 12 };
    FillRect(hdc, &downRect, hDownBrush);
    DeleteObject(hDownBrush);

    RECT downTextRect = { legendX + 16, legendY, legendX + 70, legendY + 14 };
    DrawTextW(hdc, L"Down", -1, &downTextRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Upload legend
    legendX += 65;
    HBRUSH hUpBrush = CreateSolidBrush(GetUploadColor(options.darkTheme));
    RECT upRect = { legendX, legendY, legendX + 12, legendY + 12 };
    FillRect(hdc, &upRect, hUpBrush);
    DeleteObject(hUpBrush);

    RECT upTextRect = { legendX + 16, legendY, legendX + 60, legendY + 14 };
    DrawTextW(hdc, L"Up", -1, &upTextRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

void ChartRenderer::DrawBarChart(
    HDC hdc,
    const RECT& rect,
    const std::vector<ChartDataPoint>& data,
    const ChartOptions& options)
{
    if (data.empty())
    {
        // Draw "No data" message
        SetTextColor(hdc, GetTextColor(options.darkTheme));
        SetBkMode(hdc, TRANSPARENT);
        RECT msgRect = rect;
        DrawTextW(hdc, L"No data available", -1, &msgRect, 
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    // Calculate chart area (excluding padding)
    RECT chartArea = rect;
    chartArea.left += options.axisPadding;
    chartArea.top += options.topPadding;
    chartArea.bottom -= options.bottomPadding;
    chartArea.right -= 10;  // Small right padding

    int chartWidth = chartArea.right - chartArea.left;
    int chartHeight = chartArea.bottom - chartArea.top;

    if (chartWidth <= 0 || chartHeight <= 0)
    {
        return;
    }

    // Find max value for scaling
    uint64_t maxValue = 0;
    for (const auto& point : data)
    {
        uint64_t total = point.valueDown + point.valueUp;
        if (total > maxValue)
        {
            maxValue = total;
        }
    }

    // Add 10% headroom to max value
    maxValue = static_cast<uint64_t>(maxValue * 1.1);
    if (maxValue == 0)
    {
        maxValue = 1024;  // Minimum 1 KB
    }

    // Draw grid lines
    DrawGridLines(hdc, chartArea, maxValue, options);

    // Draw Y-axis labels
    DrawYAxisLabels(hdc, chartArea, maxValue, options);

    // Draw legend
    DrawLegend(hdc, rect, options);

    // Calculate bar width
    size_t numBars = data.size();
    int totalSpacing = static_cast<int>(options.barSpacing * (numBars + 1));
    int barWidth = (chartWidth - totalSpacing) / static_cast<int>(numBars);
    if (barWidth < 4)
    {
        barWidth = 4;
    }

    // Draw bars
    HBRUSH hDownBrush = CreateSolidBrush(GetDownloadColor(options.darkTheme));
    HBRUSH hUpBrush = CreateSolidBrush(GetUploadColor(options.darkTheme));

    SetTextColor(hdc, GetTextColor(options.darkTheme));
    SetBkMode(hdc, TRANSPARENT);

    int x = chartArea.left + options.barSpacing;
    
    for (size_t i = 0; i < numBars; i++)
    {
        const auto& point = data[i];
        
        // Calculate bar heights
        int downHeight = static_cast<int>((static_cast<double>(point.valueDown) / maxValue) * chartHeight);
        int upHeight = static_cast<int>((static_cast<double>(point.valueUp) / maxValue) * chartHeight);

        // Ensure visible bar for non-zero values (min 1px)
        if (point.valueDown > 0 && downHeight < 1) downHeight = 1;
        if (point.valueUp > 0 && upHeight < 1) upHeight = 1;

        // Draw download bar (bottom)
        if (downHeight > 0)
        {
            RECT downBar = { x, chartArea.bottom - downHeight - upHeight, 
                            x + barWidth, chartArea.bottom - upHeight };
            FillRect(hdc, &downBar, hDownBrush);
        }

        // Draw upload bar (on top of download)
        if (upHeight > 0)
        {
            RECT upBar = { x, chartArea.bottom - upHeight, 
                          x + barWidth, chartArea.bottom };
            FillRect(hdc, &upBar, hUpBrush);
        }

        // Draw X-axis label (optimize for dense charts)
        bool drawLabel = true;
        if (numBars > 20)
        {
            // For many bars, show only every 5th label, plus the last one
            if (i % 5 != 0 && i != numBars - 1)
            {
                drawLabel = false;
            }
        }

        if (drawLabel)
        {
            RECT labelRect = { x - 10, chartArea.bottom + 2, 
                              x + barWidth + 10, rect.bottom };
            DrawTextW(hdc, point.label.c_str(), -1, &labelRect, 
                      DT_CENTER | DT_TOP | DT_SINGLELINE);
        }

        x += barWidth + options.barSpacing;
    }

    DeleteObject(hDownBrush);
    DeleteObject(hUpBrush);

    // Draw X-axis line
    HPEN hAxisPen = CreatePen(PS_SOLID, 1, GetBorderColor(options.darkTheme));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hAxisPen);
    MoveToEx(hdc, chartArea.left, chartArea.bottom, nullptr);
    LineTo(hdc, chartArea.right, chartArea.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hAxisPen);
}

int ChartRenderer::HitTestBar(const RECT& chartRect, int mouseX, int dataCount, const ChartOptions& options)
{
    if (dataCount <= 0)
    {
        return -1;
    }

    // Calculate chart area (same logic as DrawBarChart)
    RECT chartArea = chartRect;
    chartArea.left += options.axisPadding;
    chartArea.top += options.topPadding;
    chartArea.bottom -= options.bottomPadding;
    chartArea.right -= 10;

    int chartWidth = chartArea.right - chartArea.left;
    if (chartWidth <= 0)
    {
        return -1;
    }

    // Calculate bar width (same logic as DrawBarChart)
    int totalSpacing = options.barSpacing * (dataCount + 1);
    int barWidth = (chartWidth - totalSpacing) / dataCount;
    if (barWidth < 4)
    {
        barWidth = 4;
    }

    // Check if mouseX is within chart area
    if (mouseX < chartArea.left || mouseX > chartArea.right)
    {
        return -1;
    }

    // Find which bar the mouse is over
    int x = chartArea.left + options.barSpacing;
    for (int i = 0; i < dataCount; i++)
    {
        if (mouseX >= x && mouseX < x + barWidth)
        {
            return i;
        }
        x += barWidth + options.barSpacing;
    }

    return -1;
}

} // namespace NetPulse
