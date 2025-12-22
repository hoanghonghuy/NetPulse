#ifndef NETWORK_MONITOR_THEME_COLORS_H
#define NETWORK_MONITOR_THEME_COLORS_H

#include "NetPulse/Common.h"
#include <vector>

namespace NetPulse
{

/**
 * Structure defining a color palette for the application.
 * Allows easy switching between Light/Dark or custom themes.
 */
struct ThemeColors
{
    // Window Base
    COLORREF background;
    COLORREF border;
    
    // Text
    COLORREF textPrimary;   // Main text
    COLORREF textSecondary; // Labels, dates, less important info
    COLORREF textDisabled;  // Placeholder or disabled state

    // Network Graph / Stats
    COLORREF download;
    COLORREF downloadFill;  // For sparkline fill
    COLORREF upload;
    
    // System Stats
    COLORREF cpu;
    COLORREF ram;

    // Latency (Ping)
    COLORREF pingLow;       // Good (<50ms)
    COLORREF pingMed;       // Warning (<100ms)
    COLORREF pingHigh;      // Bad (>=100ms)
    COLORREF pingNone;      // No connection / Timeout

    // VPN / Proxy Status
    COLORREF vpnOn;
    COLORREF vpnProxy;      // Proxy active
    COLORREF vpnOff;

    // Charts
    COLORREF chartBackground;
    COLORREF chartGrid;
    COLORREF chartBorder;   // Access/Border
    COLORREF chartText;
    COLORREF chartDownload; // Can differ from main if needed
    COLORREF chartUpload;

    // Dialogs (Specific "Professional Dark" palette)
    COLORREF dialogBackground;
    COLORREF dialogPanel;      // For lists/groups
    COLORREF dialogBorder;
    COLORREF dialogText;
    COLORREF dialogTextDisabled;
    
    // Lists & Trees
    COLORREF listBackground;
    COLORREF listText;
    COLORREF listSelectedBackground;
    COLORREF listSelectedText;
    COLORREF listHeaderBackground;
    COLORREF listHeaderText;

    // Tabs
    COLORREF tabBackground;
    COLORREF tabSelectedBackground;
    COLORREF tabText;
    COLORREF tabSelectedText;
    
    // Controls
    COLORREF inputBackground;
    COLORREF buttonBackground;
    COLORREF buttonPressed;
    COLORREF buttonBorder;

    // Taskbar Overlay (High contrast)
    COLORREF overlayDownload;
    COLORREF overlayUpload;
};

} // namespace NetPulse

#endif // NETWORK_MONITOR_THEME_COLORS_H
