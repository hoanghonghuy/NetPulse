#ifndef NETWORK_MONITOR_SPARKLINE_RENDERER_H
#define NETWORK_MONITOR_SPARKLINE_RENDERER_H

#include "NetPulse/Common.h"
#include <vector>
#include <deque>

namespace NetPulse
{

/**
 * SparklineRenderer - Renders mini line charts for speed visualization
 * 
 * Features:
 * - Circular buffer for efficient data point storage
 * - Configurable max points and auto-scaling
 * - GDI-based rendering for Win32 compatibility
 */
class SparklineRenderer
{
public:
    SparklineRenderer(size_t maxPoints = 30);
    ~SparklineRenderer() = default;

    /**
     * Add a new data point to the sparkline
     * Old points are automatically removed when buffer is full
     */
    void AddDataPoint(double value);

    /**
     * Clear all data points
     */
    void Clear();

    /**
     * Render the sparkline to a device context
     * @param hdc Device context to render to
     * @param bounds Rectangle defining the render area
     * @param lineColor Color of the sparkline
     * @param fillColor Optional fill color under the line (0 = no fill)
     */
    void Render(HDC hdc, const RECT& bounds, COLORREF lineColor, COLORREF fillColor = 0);

    /**
     * Get the number of data points currently stored
     */
    size_t GetPointCount() const { return m_dataPoints.size(); }

    /**
     * Get the maximum value in the current dataset
     */
    double GetMaxValue() const { return m_maxValue; }

    /**
     * Get the minimum value in the current dataset  
     */
    double GetMinValue() const { return m_minValue; }

    /**
     * Get the most recent data point value
     */
    double GetLastValue() const;

    /**
     * Set the maximum number of data points to store
     */
    void SetMaxPoints(size_t maxPoints);

private:
    void RecalculateMinMax();

    std::deque<double> m_dataPoints;
    size_t m_maxPoints;
    double m_minValue;
    double m_maxValue;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_SPARKLINE_RENDERER_H
