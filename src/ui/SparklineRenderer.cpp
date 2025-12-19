#include "NetPulse/SparklineRenderer.h"
#include <algorithm>
#include <cmath>

namespace NetPulse
{

SparklineRenderer::SparklineRenderer(size_t maxPoints)
    : m_maxPoints(maxPoints)
    , m_minValue(0.0)
    , m_maxValue(0.0)
{
}

void SparklineRenderer::AddDataPoint(double value)
{
    // Add new point
    m_dataPoints.push_back(value);
    
    // Remove oldest if over limit
    while (m_dataPoints.size() > m_maxPoints)
    {
        m_dataPoints.pop_front();
    }
    
    // Update min/max
    if (m_dataPoints.size() == 1)
    {
        m_minValue = value;
        m_maxValue = value;
    }
    else
    {
        if (value < m_minValue) m_minValue = value;
        if (value > m_maxValue) m_maxValue = value;
        
        // Recalculate if we removed points that might have been min/max
        RecalculateMinMax();
    }
}

void SparklineRenderer::Clear()
{
    m_dataPoints.clear();
    m_minValue = 0.0;
    m_maxValue = 0.0;
}

double SparklineRenderer::GetLastValue() const
{
    if (m_dataPoints.empty())
    {
        return 0.0;
    }
    return m_dataPoints.back();
}

void SparklineRenderer::SetMaxPoints(size_t maxPoints)
{
    m_maxPoints = maxPoints;
    while (m_dataPoints.size() > m_maxPoints)
    {
        m_dataPoints.pop_front();
    }
    RecalculateMinMax();
}

void SparklineRenderer::RecalculateMinMax()
{
    if (m_dataPoints.empty())
    {
        m_minValue = 0.0;
        m_maxValue = 0.0;
        return;
    }
    
    m_minValue = m_dataPoints[0];
    m_maxValue = m_dataPoints[0];
    
    for (const auto& val : m_dataPoints)
    {
        if (val < m_minValue) m_minValue = val;
        if (val > m_maxValue) m_maxValue = val;
    }
}

void SparklineRenderer::Render(HDC hdc, const RECT& bounds, COLORREF lineColor, COLORREF fillColor) const
{
    if (m_dataPoints.size() < 2)
    {
        return; // Need at least 2 points to draw a line
    }
    
    int width = bounds.right - bounds.left;
    int height = bounds.bottom - bounds.top;
    
    if (width <= 0 || height <= 0)
    {
        return;
    }
    
    // Calculate value range with some padding
    double range = m_maxValue - m_minValue;
    if (range < 0.001) range = 1.0; // Avoid division by zero
    
    // Calculate point positions
    size_t numPoints = m_dataPoints.size();
    double xStep = static_cast<double>(width) / (numPoints - 1);
    
    // Create points array for polyline
    std::vector<POINT> points(numPoints);
    
    for (size_t i = 0; i < numPoints; ++i)
    {
        double normalizedValue = (m_dataPoints[i] - m_minValue) / range;
        
        points[i].x = bounds.left + static_cast<LONG>(i * xStep);
        points[i].y = bounds.bottom - static_cast<LONG>(normalizedValue * height);
        
        // Clamp Y within bounds
        if (points[i].y < bounds.top) points[i].y = bounds.top;
        if (points[i].y > bounds.bottom) points[i].y = bounds.bottom;
    }
    
    // Draw fill under the line if requested
    if (fillColor != 0)
    {
        // Create polygon with bottom edge
        std::vector<POINT> fillPoints(numPoints + 2);
        for (size_t i = 0; i < numPoints; ++i)
        {
            fillPoints[i] = points[i];
        }
        fillPoints[numPoints] = { bounds.right, bounds.bottom };
        fillPoints[numPoints + 1] = { bounds.left, bounds.bottom };
        
        HBRUSH hFillBrush = CreateSolidBrush(fillColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFillBrush);
        HPEN hNullPen = (HPEN)GetStockObject(NULL_PEN);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hNullPen);
        
        Polygon(hdc, fillPoints.data(), static_cast<int>(fillPoints.size()));
        
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hFillBrush);
    }
    
    // Draw the line
    HPEN hLinePen = CreatePen(PS_SOLID, 1, lineColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hLinePen);
    
    Polyline(hdc, points.data(), static_cast<int>(points.size()));
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hLinePen);
}

} // namespace NetPulse
