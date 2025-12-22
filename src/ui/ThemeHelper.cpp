#include "NetPulse/ThemeHelper.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace NetPulse
{

// Define palettes statically
static const ThemeColors s_darkColors = {
    RGB(30, 30, 30),      // background
    RGB(80, 80, 80),      // border
    RGB(255, 255, 255),   // textPrimary
    RGB(200, 200, 200),   // textSecondary
    RGB(128, 128, 128),   // textDisabled
    RGB(0, 200, 255),     // download
    RGB(0, 60, 80),       // downloadFill
    RGB(0, 220, 100),     // upload
    RGB(255, 180, 50),    // cpu
    RGB(200, 100, 255),   // ram
    RGB(0, 220, 100),     // pingLow
    RGB(255, 200, 50),    // pingMed
    RGB(255, 80, 80),     // pingHigh
    RGB(128, 128, 128),   // pingNone
    RGB(0, 220, 100),     // vpnOn
    RGB(255, 200, 50),    // vpnProxy
    RGB(128, 128, 128),   // vpnOff

    // Charts
    RGB(30, 30, 30),      // chartBackground
    RGB(60, 60, 60),      // chartGrid
    RGB(80, 80, 80),      // chartBorder
    RGB(230, 230, 230),   // chartText
    RGB(0, 150, 255),     // chartDownload
    RGB(0, 200, 100),     // chartUpload

    // Dialogs
    RGB(32, 34, 37),      // dialogBackground
    RGB(43, 45, 49),      // dialogPanel
    RGB(58, 60, 67),      // dialogBorder
    RGB(242, 243, 245),   // dialogText
    RGB(160, 160, 160),   // dialogTextDisabled
    
    // Lists (Mapped to Dialog Panel/Text for backward compatibility)
    RGB(43, 45, 49),      // listBackground (dialogPanel)
    RGB(242, 243, 245),   // listText (dialogText)
    RGB(58, 60, 67),      // listSelectedBackground (dialogBorder)
    RGB(242, 243, 245),   // listSelectedText
    RGB(32, 34, 37),      // listHeaderBackground (dialogBackground)
    RGB(242, 243, 245),   // listHeaderText

    // Tabs
    RGB(32, 34, 37),      // tabBackground (dialogBackground)
    RGB(43, 45, 49),      // tabSelectedBackground (dialogPanel)
    RGB(160, 160, 160),   // tabText
    RGB(242, 243, 245),   // tabSelectedText

    // Controls
    RGB(25, 25, 25),      // inputBackground
    RGB(40, 40, 40),      // buttonBackground
    RGB(50, 50, 50),      // buttonPressed
    RGB(90, 90, 90),      // buttonBorder

    // Overlay
    RGB(0, 255, 255),     // overlayDownload
    RGB(0, 255, 0)        // overlayUpload
};

static const ThemeColors s_lightColors = {
    RGB(245, 245, 245),   // background
    RGB(180, 180, 180),   // border
    RGB(0, 0, 0),         // textPrimary
    RGB(80, 80, 80),      // textSecondary
    RGB(150, 150, 150),   // textDisabled
    RGB(0, 120, 180),     // download
    RGB(200, 235, 250),   // downloadFill
    RGB(0, 150, 60),      // upload
    RGB(200, 120, 0),     // cpu
    RGB(140, 60, 180),    // ram
    RGB(0, 150, 60),      // pingLow
    RGB(200, 140, 0),     // pingMed
    RGB(200, 40, 40),     // pingHigh
    RGB(150, 150, 150),   // pingNone
    RGB(0, 150, 60),      // vpnOn
    RGB(200, 140, 0),     // vpnProxy
    RGB(150, 150, 150),   // vpnOff

    // Charts
    RGB(255, 255, 255),   // chartBackground
    RGB(220, 220, 220),   // chartGrid
    RGB(180, 180, 180),   // chartBorder
    RGB(30, 30, 30),      // chartText
    RGB(0, 120, 215),     // chartDownload
    RGB(40, 167, 69),     // chartUpload

    // Dialogs
    RGB(255, 255, 255),   // dialogBackground
    RGB(255, 255, 255),   // dialogPanel
    RGB(180, 180, 180),   // dialogBorder
    RGB(0, 0, 0),         // dialogText
    RGB(128, 128, 128),   // dialogTextDisabled

    // Lists
    RGB(255, 255, 255),   // listBackground
    RGB(0, 0, 0),         // listText
    RGB(204, 232, 255),   // listSelectedBackground (System highlightish)
    RGB(0, 0, 0),         // listSelectedText
    RGB(240, 240, 240),   // listHeaderBackground
    RGB(0, 0, 0),         // listHeaderText

    // Tabs
    RGB(240, 240, 240),   // tabBackground
    RGB(255, 255, 255),   // tabSelectedBackground
    RGB(80, 80, 80),      // tabText
    RGB(0, 0, 0),         // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(240, 240, 240),   // buttonBackground
    RGB(200, 200, 200),   // buttonPressed
    RGB(160, 160, 160),   // buttonBorder

    // Overlay
    RGB(0, 120, 215),     // overlayDownload
    RGB(0, 150, 60)       // overlayUpload
};

// ============================================================================
// NEW THEME PRESETS
// ============================================================================

// 1. Dracula
static const ThemeColors s_draculaColors = {
    RGB(40, 42, 54),      // background (#282a36)
    RGB(98, 114, 164),    // border (#6272a4)
    RGB(248, 248, 242),   // textPrimary (#f8f8f2)
    RGB(98, 114, 164),    // textSecondary (#6272a4)
    RGB(68, 71, 90),      // textDisabled
    RGB(139, 233, 253),   // download (Cyan #8be9fd)
    RGB(40, 42, 54),      // downloadFill
    RGB(80, 250, 123),    // upload (Green #50fa7b)
    RGB(255, 184, 108),   // cpu (Orange #ffb86c)
    RGB(189, 147, 249),   // ram (Purple #bd93f9)
    RGB(80, 250, 123),    // pingLow
    RGB(255, 184, 108),   // pingMed
    RGB(255, 85, 85),     // pingHigh (#ff5555)
    RGB(98, 114, 164),    // pingNone
    RGB(80, 250, 123),    // vpnOn
    RGB(241, 250, 140),   // vpnProxy (Yellow #f1fa8c)
    RGB(98, 114, 164),    // vpnOff

    // Charts
    RGB(40, 42, 54),      // chartBackground
    RGB(68, 71, 90),      // chartGrid
    RGB(98, 114, 164),    // chartBorder
    RGB(248, 248, 242),   // chartText
    RGB(139, 233, 253),   // chartDownload
    RGB(80, 250, 123),    // chartUpload

    // Dialogs
    RGB(40, 42, 54),      // dialogBackground
    RGB(68, 71, 90),      // dialogPanel
    RGB(98, 114, 164),    // dialogBorder
    RGB(248, 248, 242),   // dialogText
    RGB(98, 114, 164),    // dialogTextDisabled

    // Lists
    RGB(68, 71, 90),      // listBackground (dialogPanel)
    RGB(248, 248, 242),   // listText
    RGB(98, 114, 164),    // listSelectedBackground
    RGB(248, 248, 242),   // listSelectedText
    RGB(40, 42, 54),      // listHeaderBackground
    RGB(248, 248, 242),   // listHeaderText
    
    // Tabs
    RGB(40, 42, 54),      // tabBackground
    RGB(68, 71, 90),      // tabSelectedBackground
    RGB(98, 114, 164),    // tabText
    RGB(248, 248, 242),   // tabSelectedText

    // Controls
    RGB(68, 71, 90),      // inputBackground
    RGB(98, 114, 164),    // buttonBackground
    RGB(139, 233, 253),   // buttonPressed
    RGB(189, 147, 249),   // buttonBorder

    // Overlay
    RGB(139, 233, 253),   // overlayDownload
    RGB(80, 250, 123)     // overlayUpload
};

// 2. Cyberpunk / Neon City
static const ThemeColors s_cyberpunkColors = {
    RGB(10, 10, 20),      // background (Deep Blue/Black)
    RGB(0, 243, 255),     // border (Neon Cyan)
    RGB(0, 243, 255),     // textPrimary (Cyan)
    RGB(255, 0, 255),     // textSecondary (Magenta)
    RGB(80, 80, 80),      // textDisabled
    RGB(0, 243, 255),     // download (Cyan)
    RGB(0, 50, 60),       // downloadFill
    RGB(255, 0, 255),     // upload (Magenta)
    RGB(255, 230, 0),     // cpu (Yellow)
    RGB(180, 0, 255),     // ram (Purple)
    RGB(0, 255, 100),     // pingLow
    RGB(255, 230, 0),     // pingMed
    RGB(255, 0, 60),      // pingHigh (Red)
    RGB(80, 80, 80),      // pingNone
    RGB(0, 255, 100),     // vpnOn
    RGB(255, 230, 0),     // vpnProxy
    RGB(80, 80, 80),      // vpnOff

    // Charts
    RGB(5, 5, 15),        // chartBackground
    RGB(40, 0, 60),       // chartGrid (Dark Purple)
    RGB(0, 243, 255),     // chartBorder
    RGB(0, 243, 255),     // chartText
    RGB(0, 243, 255),     // chartDownload
    RGB(255, 0, 255),     // chartUpload

    // Dialogs
    RGB(10, 10, 20),      // dialogBackground
    RGB(20, 20, 40),      // dialogPanel
    RGB(0, 243, 255),     // dialogBorder
    RGB(0, 243, 255),     // dialogText (Cyan as primary text)
    RGB(100, 0, 100),     // dialogTextDisabled

    // Lists
    RGB(20, 20, 40),      // listBackground
    RGB(0, 243, 255),     // listText
    RGB(0, 50, 60),       // listSelectedBackground
    RGB(255, 255, 255),   // listSelectedText
    RGB(10, 10, 20),      // listHeaderBackground
    RGB(255, 0, 255),     // listHeaderText

    // Tabs
    RGB(10, 10, 20),      // tabBackground
    RGB(20, 20, 40),      // tabSelectedBackground
    RGB(80, 80, 80),      // tabText
    RGB(0, 243, 255),     // tabSelectedText

    // Controls
    RGB(0, 0, 0),         // inputBackground
    RGB(40, 0, 60),       // buttonBackground
    RGB(255, 0, 255),     // buttonPressed
    RGB(0, 243, 255),     // buttonBorder

    // Overlay
    RGB(0, 243, 255),     // overlayDownload
    RGB(255, 0, 255)      // overlayUpload
};

// 3. Nord
static const ThemeColors s_nordColors = {
    RGB(46, 52, 64),      // background (Polar Night #2e3440)
    RGB(76, 86, 106),     // border (#4c566a)
    RGB(236, 239, 244),   // textPrimary (Snow Storm #eceff4)
    RGB(216, 222, 233),   // textSecondary (#d8dee9)
    RGB(76, 86, 106),     // textDisabled
    RGB(136, 192, 208),   // download (Frost #88c0d0)
    RGB(46, 52, 64),      // downloadFill
    RGB(163, 190, 140),   // upload (Aurora Green #a3be8c)
    RGB(208, 135, 112),   // cpu (Aurora Orange #d08770)
    RGB(180, 142, 173),   // ram (Aurora Purple #b48ead)
    RGB(163, 190, 140),   // pingLow
    RGB(235, 203, 139),   // pingMed (Yellow #ebcb8b)
    RGB(191, 97, 106),    // pingHigh (Red #bf616a)
    RGB(76, 86, 106),     // pingNone
    RGB(163, 190, 140),   // vpnOn
    RGB(94, 129, 172),    // vpnProxy (Frost Blue #5e81ac)
    RGB(76, 86, 106),     // vpnOff

    // Charts
    RGB(46, 52, 64),      // chartBackground
    RGB(59, 66, 82),      // chartGrid (#3b4252)
    RGB(76, 86, 106),     // chartBorder
    RGB(236, 239, 244),   // chartText
    RGB(136, 192, 208),   // chartDownload
    RGB(163, 190, 140),   // chartUpload

    // Dialogs
    RGB(46, 52, 64),      // dialogBackground
    RGB(59, 66, 82),      // dialogPanel
    RGB(129, 161, 193),   // dialogBorder (#81a1c1)
    RGB(236, 239, 244),   // dialogText
    RGB(76, 86, 106),     // dialogTextDisabled

    // Lists
    RGB(59, 66, 82),      // listBackground
    RGB(236, 239, 244),   // listText
    RGB(76, 86, 106),     // listSelectedBackground
    RGB(236, 239, 244),   // listSelectedText
    RGB(46, 52, 64),      // listHeaderBackground
    RGB(236, 239, 244),   // listHeaderText
    
    // Tabs
    RGB(46, 52, 64),      // tabBackground
    RGB(59, 66, 82),      // tabSelectedBackground
    RGB(129, 161, 193),   // tabText
    RGB(236, 239, 244),   // tabSelectedText

    // Controls
    RGB(59, 66, 82),      // inputBackground
    RGB(67, 76, 94),      // buttonBackground (#434c5e)
    RGB(94, 129, 172),    // buttonPressed
    RGB(129, 161, 193),   // buttonBorder

    // Overlay
    RGB(136, 192, 208),   // overlayDownload
    RGB(163, 190, 140)    // overlayUpload
};

// 4. Forest / Matcha
static const ThemeColors s_forestColors = {
    RGB(30, 45, 36),      // background (Deep Green)
    RGB(70, 90, 75),      // border
    RGB(232, 245, 233),   // textPrimary (Light Green/White)
    RGB(165, 214, 167),   // textSecondary
    RGB(70, 90, 75),      // textDisabled
    RGB(105, 240, 174),   // download (Bright Green)
    RGB(30, 45, 36),      // downloadFill
    RGB(255, 204, 128),   // upload (Orange - contrast)
    RGB(200, 230, 201),   // cpu
    RGB(129, 199, 132),   // ram
    RGB(0, 200, 83),      // pingLow
    RGB(255, 179, 0),     // pingMed
    RGB(213, 0, 0),       // pingHigh
    RGB(70, 90, 75),      // pingNone
    RGB(0, 200, 83),      // vpnOn
    RGB(255, 179, 0),     // vpnProxy
    RGB(70, 90, 75),      // vpnOff

    // Charts
    RGB(27, 40, 32),      // chartBackground
    RGB(50, 70, 60),      // chartGrid
    RGB(70, 90, 75),      // chartBorder
    RGB(232, 245, 233),   // chartText
    RGB(105, 240, 174),   // chartDownload
    RGB(255, 204, 128),   // chartUpload

    // Dialogs
    RGB(30, 45, 36),      // dialogBackground
    RGB(40, 60, 50),      // dialogPanel
    RGB(70, 90, 75),      // dialogBorder
    RGB(232, 245, 233),   // dialogText
    RGB(100, 120, 110),   // dialogTextDisabled

    // Lists
    RGB(40, 60, 50),      // listBackground
    RGB(232, 245, 233),   // listText
    RGB(50, 75, 60),      // listSelectedBackground
    RGB(255, 255, 255),   // listSelectedText
    RGB(30, 45, 36),      // listHeaderBackground
    RGB(232, 245, 233),   // listHeaderText
    
    // Tabs
    RGB(30, 45, 36),      // tabBackground
    RGB(40, 60, 50),      // tabSelectedBackground
    RGB(165, 214, 167),   // tabText
    RGB(255, 255, 255),   // tabSelectedText

    // Controls
    RGB(40, 60, 50),      // inputBackground
    RGB(50, 75, 60),      // buttonBackground
    RGB(105, 240, 174),   // buttonPressed
    RGB(165, 214, 167),   // buttonBorder

    // Overlay
    RGB(105, 240, 174),   // overlayDownload
    RGB(255, 204, 128)    // overlayUpload
};

// 5. OLED Black
static const ThemeColors s_oledColors = {
    RGB(0, 0, 0),         // background (Pure Black)
    RGB(50, 50, 50),      // border (Dark Grey)
    RGB(255, 255, 255),   // textPrimary (White)
    RGB(180, 180, 180),   // textSecondary
    RGB(80, 80, 80),      // textDisabled
    RGB(255, 255, 255),   // download (White for max contrast)
    RGB(0, 0, 0),         // downloadFill
    RGB(180, 180, 180),   // upload (Light Grey)
    RGB(150, 150, 150),   // cpu
    RGB(120, 120, 120),   // ram
    RGB(0, 255, 0),       // pingLow (Green)
    RGB(255, 255, 0),     // pingMed (Yellow)
    RGB(255, 0, 0),       // pingHigh (Red)
    RGB(80, 80, 80),      // pingNone
    RGB(0, 255, 0),       // vpnOn
    RGB(0, 255, 255),     // vpnProxy
    RGB(80, 80, 80),      // vpnOff

    // Charts
    RGB(0, 0, 0),         // chartBackground
    RGB(40, 40, 40),      // chartGrid
    RGB(80, 80, 80),      // chartBorder
    RGB(255, 255, 255),   // chartText
    RGB(255, 255, 255),   // chartDownload
    RGB(180, 180, 180),   // chartUpload

    // Dialogs
    RGB(0, 0, 0),         // dialogBackground
    RGB(20, 20, 20),      // dialogPanel
    RGB(80, 80, 80),      // dialogBorder
    RGB(255, 255, 255),   // dialogText
    RGB(100, 100, 100),   // dialogTextDisabled

    // Lists
    RGB(20, 20, 20),      // listBackground
    RGB(255, 255, 255),   // listText
    RGB(40, 40, 40),      // listSelectedBackground
    RGB(255, 255, 255),   // listSelectedText
    RGB(0, 0, 0),         // listHeaderBackground
    RGB(255, 255, 255),   // listHeaderText
    
    // Tabs
    RGB(0, 0, 0),         // tabBackground
    RGB(20, 20, 20),      // tabSelectedBackground
    RGB(128, 128, 128),   // tabText
    RGB(255, 255, 255),   // tabSelectedText

    // Controls
    RGB(10, 10, 10),      // inputBackground
    RGB(30, 30, 30),      // buttonBackground
    RGB(255, 255, 255),   // buttonPressed
    RGB(128, 128, 128),   // buttonBorder

    // Overlay
    RGB(255, 255, 255),   // overlayDownload
    RGB(180, 180, 180)    // overlayUpload
};

// 6. Solarized Light
static const ThemeColors s_solarizedLightColors = {
    RGB(253, 246, 227),   // background (Base3)
    RGB(147, 161, 161),   // border (Base1)
    RGB(101, 123, 131),   // textPrimary (Base00)
    RGB(147, 161, 161),   // textSecondary (Base1)
    RGB(181, 137, 0),     // textDisabled (Yellow)
    RGB(38, 139, 210),    // download (Blue)
    RGB(238, 232, 213),   // downloadFill (Base2)
    RGB(133, 153, 0),     // upload (Green)
    RGB(203, 75, 22),     // cpu (Orange)
    RGB(108, 113, 196),   // ram (Violet)
    RGB(133, 153, 0),     // pingLow
    RGB(181, 137, 0),     // pingMed
    RGB(220, 50, 47),     // pingHigh (Red)
    RGB(147, 161, 161),   // pingNone
    RGB(38, 139, 210),    // vpnOn
    RGB(181, 137, 0),     // vpnProxy
    RGB(147, 161, 161),   // vpnOff

    // Charts
    RGB(253, 246, 227),   // chartBackground
    RGB(238, 232, 213),   // chartGrid
    RGB(147, 161, 161),   // chartBorder
    RGB(101, 123, 131),   // chartText
    RGB(38, 139, 210),    // chartDownload
    RGB(133, 153, 0),     // chartUpload

    // Dialogs
    RGB(253, 246, 227),   // dialogBackground
    RGB(238, 232, 213),   // dialogPanel (Base2)
    RGB(147, 161, 161),   // dialogBorder
    RGB(101, 123, 131),   // dialogText
    RGB(147, 161, 161),   // dialogTextDisabled

    // Lists
    RGB(253, 246, 227),   // listBackground
    RGB(101, 123, 131),   // listText
    RGB(238, 232, 213),   // listSelectedBackground
    RGB(101, 123, 131),   // listSelectedText
    RGB(238, 232, 213),   // listHeaderBackground
    RGB(101, 123, 131),   // listHeaderText
    
    // Tabs
    RGB(238, 232, 213),   // tabBackground
    RGB(253, 246, 227),   // tabSelectedBackground
    RGB(147, 161, 161),   // tabText
    RGB(101, 123, 131),   // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(238, 232, 213),   // buttonBackground
    RGB(147, 161, 161),   // buttonPressed
    RGB(101, 123, 131),   // buttonBorder

    // Overlay
    RGB(38, 139, 210),    // overlayDownload
    RGB(133, 153, 0)      // overlayUpload
};

// 7. Morning Mist (Cool Blue)
static const ThemeColors s_morningMistColors = {
    RGB(240, 248, 255),   // background (AliceBlue)
    RGB(176, 196, 222),   // border (LightSteelBlue)
    RGB(25, 25, 112),     // textPrimary (MidnightBlue)
    RGB(70, 130, 180),    // textSecondary (SteelBlue)
    RGB(176, 196, 222),   // textDisabled
    RGB(30, 144, 255),    // download (DodgerBlue)
    RGB(224, 255, 255),   // downloadFill (LightCyan)
    RGB(60, 179, 113),    // upload (MediumSeaGreen)
    RGB(255, 165, 0),     // cpu
    RGB(147, 112, 219),   // ram
    RGB(60, 179, 113),    // pingLow
    RGB(255, 165, 0),     // pingMed
    RGB(255, 69, 0),      // pingHigh
    RGB(192, 192, 192),   // pingNone
    RGB(30, 144, 255),    // vpnOn
    RGB(255, 165, 0),     // vpnProxy
    RGB(192, 192, 192),   // vpnOff

    // Charts
    RGB(240, 248, 255),   // chartBackground
    RGB(224, 255, 255),   // chartGrid
    RGB(176, 196, 222),   // chartBorder
    RGB(25, 25, 112),     // chartText
    RGB(30, 144, 255),    // chartDownload
    RGB(60, 179, 113),    // chartUpload

    // Dialogs
    RGB(240, 248, 255),   // dialogBackground
    RGB(230, 240, 250),   // dialogPanel
    RGB(176, 196, 222),   // dialogBorder
    RGB(25, 25, 112),     // dialogText
    RGB(160, 180, 200),   // dialogTextDisabled

    // Lists
    RGB(255, 255, 255),   // listBackground
    RGB(25, 25, 112),     // listText
    RGB(220, 235, 255),   // listSelectedBackground
    RGB(0, 0, 100),       // listSelectedText
    RGB(235, 245, 255),   // listHeaderBackground
    RGB(25, 25, 112),     // listHeaderText
    
    // Tabs
    RGB(235, 245, 255),   // tabBackground
    RGB(255, 255, 255),   // tabSelectedBackground
    RGB(100, 149, 237),   // tabText
    RGB(25, 25, 112),     // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(235, 245, 255),   // buttonBackground
    RGB(200, 220, 240),   // buttonPressed
    RGB(176, 196, 222),   // buttonBorder

    // Overlay
    RGB(30, 144, 255),    // overlayDownload
    RGB(60, 179, 113)     // overlayUpload
};

// 8. Soft Paper (Warm)
static const ThemeColors s_softPaperColors = {
    RGB(253, 245, 230),   // background (OldLace)
    RGB(210, 180, 140),   // border (Tan)
    RGB(62, 39, 35),      // textPrimary (Dark Brown)
    RGB(121, 85, 72),     // textSecondary
    RGB(188, 170, 164),   // textDisabled
    RGB(93, 64, 55),      // download (Brown)
    RGB(255, 248, 225),   // downloadFill
    RGB(56, 142, 60),     // upload (Green)
    RGB(230, 81, 0),      // cpu
    RGB(123, 31, 162),    // ram
    RGB(56, 142, 60),     // pingLow
    RGB(245, 124, 0),     // pingMed
    RGB(211, 47, 47),     // pingHigh
    RGB(161, 136, 127),   // pingNone
    RGB(93, 64, 55),      // vpnOn
    RGB(245, 124, 0),     // vpnProxy
    RGB(161, 136, 127),   // vpnOff

    // Charts
    RGB(253, 245, 230),   // chartBackground
    RGB(245, 222, 179),   // chartGrid (Wheat)
    RGB(210, 180, 140),   // chartBorder
    RGB(62, 39, 35),      // chartText
    RGB(109, 76, 65),     // chartDownload
    RGB(56, 142, 60),     // chartUpload

    // Dialogs
    RGB(253, 245, 230),   // dialogBackground
    RGB(250, 235, 215),   // dialogPanel (AntiqueWhite)
    RGB(210, 180, 140),   // dialogBorder
    RGB(62, 39, 35),      // dialogText
    RGB(141, 110, 99),    // dialogTextDisabled

    // Lists
    RGB(255, 250, 240),   // listBackground (FloralWhite)
    RGB(62, 39, 35),      // listText
    RGB(255, 228, 196),   // listSelectedBackground (Bisque)
    RGB(40, 20, 10),      // listSelectedText
    RGB(250, 235, 215),   // listHeaderBackground
    RGB(62, 39, 35),      // listHeaderText
    
    // Tabs
    RGB(250, 235, 215),   // tabBackground
    RGB(253, 245, 230),   // tabSelectedBackground
    RGB(141, 110, 99),    // tabText
    RGB(62, 39, 35),      // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(250, 235, 215),   // buttonBackground
    RGB(222, 184, 135),   // buttonPressed (Burlywood)
    RGB(210, 180, 140),   // buttonBorder

    // Overlay
    RGB(93, 64, 55),      // overlayDownload
    RGB(56, 142, 60)      // overlayUpload
};

// 9. Mint Fresh (Green)
static const ThemeColors s_mintFreshColors = {
    RGB(240, 255, 240),   // background (Honeydew)
    RGB(144, 238, 144),   // border (LightGreen)
    RGB(0, 100, 0),       // textPrimary (DarkGreen)
    RGB(46, 139, 87),     // textSecondary (SeaGreen)
    RGB(152, 251, 152),   // textDisabled
    RGB(0, 128, 128),     // download (Teal)
    RGB(224, 255, 255),   // downloadFill
    RGB(34, 139, 34),     // upload (ForestGreen)
    RGB(255, 140, 0),     // cpu
    RGB(106, 90, 205),    // ram
    RGB(34, 139, 34),     // pingLow
    RGB(218, 165, 32),    // pingMed
    RGB(205, 92, 92),     // pingHigh
    RGB(143, 188, 143),   // pingNone
    RGB(0, 128, 128),     // vpnOn
    RGB(218, 165, 32),    // vpnProxy
    RGB(143, 188, 143),   // vpnOff

    // Charts
    RGB(240, 255, 240),   // chartBackground
    RGB(193, 255, 193),   // chartGrid
    RGB(144, 238, 144),   // chartBorder
    RGB(0, 100, 0),       // chartText
    RGB(0, 128, 128),     // chartDownload
    RGB(34, 139, 34),     // chartUpload

    // Dialogs
    RGB(240, 255, 240),   // dialogBackground
    RGB(220, 250, 220),   // dialogPanel
    RGB(144, 238, 144),   // dialogBorder
    RGB(0, 80, 0),        // dialogText
    RGB(120, 180, 120),   // dialogTextDisabled

    // Lists
    RGB(250, 255, 250),   // listBackground
    RGB(0, 80, 0),        // listText
    RGB(200, 255, 200),   // listSelectedBackground
    RGB(0, 50, 0),        // listSelectedText
    RGB(230, 255, 230),   // listHeaderBackground
    RGB(0, 80, 0),        // listHeaderText
    
    // Tabs
    RGB(230, 255, 230),   // tabBackground
    RGB(240, 255, 240),   // tabSelectedBackground
    RGB(80, 160, 80),     // tabText
    RGB(0, 80, 0),        // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(230, 255, 230),   // buttonBackground
    RGB(152, 251, 152),   // buttonPressed
    RGB(144, 238, 144),   // buttonBorder

    // Overlay
    RGB(0, 128, 128),     // overlayDownload
    RGB(34, 139, 34)      // overlayUpload
};

// 10. Lavender (Purple)
static const ThemeColors s_lavenderColors = {
    RGB(245, 245, 255),   // background (GhostWhite/Lavenderish)
    RGB(200, 190, 230),   // border
    RGB(75, 0, 130),      // textPrimary (Indigo)
    RGB(106, 90, 205),    // textSecondary (SlateBlue)
    RGB(200, 190, 230),   // textDisabled
    RGB(65, 105, 225),    // download (RoyalBlue)
    RGB(230, 230, 250),   // downloadFill
    RGB(50, 205, 50),     // upload (LimeGreen)
    RGB(255, 69, 0),      // cpu
    RGB(138, 43, 226),    // ram (BlueViolet)
    RGB(50, 205, 50),     // pingLow
    RGB(255, 140, 0),     // pingMed
    RGB(220, 20, 60),     // pingHigh
    RGB(192, 192, 192),   // pingNone
    RGB(65, 105, 225),    // vpnOn
    RGB(255, 140, 0),     // vpnProxy
    RGB(192, 192, 192),   // vpnOff

    // Charts
    RGB(245, 245, 255),   // chartBackground
    RGB(230, 230, 250),   // chartGrid
    RGB(200, 190, 230),   // chartBorder
    RGB(75, 0, 130),      // chartText
    RGB(65, 105, 225),    // chartDownload
    RGB(50, 205, 50),     // chartUpload

    // Dialogs
    RGB(245, 245, 255),   // dialogBackground
    RGB(240, 240, 255),   // dialogPanel
    RGB(200, 190, 230),   // dialogBorder
    RGB(60, 0, 100),      // dialogText
    RGB(160, 140, 200),   // dialogTextDisabled

    // Lists
    RGB(250, 250, 255),   // listBackground
    RGB(60, 0, 100),      // listText
    RGB(230, 220, 255),   // listSelectedBackground
    RGB(30, 0, 60),       // listSelectedText
    RGB(220, 220, 250),   // listHeaderBackground
    RGB(60, 0, 100),      // listHeaderText
    
    // Tabs
    RGB(230, 230, 250),   // tabBackground
    RGB(245, 245, 255),   // tabSelectedBackground
    RGB(120, 100, 180),   // tabText
    RGB(60, 0, 100),      // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(230, 230, 250),   // buttonBackground
    RGB(216, 191, 216),   // buttonPressed (Thistle)
    RGB(200, 190, 230),   // buttonBorder

    // Overlay
    RGB(106, 90, 205),    // overlayDownload
    RGB(50, 205, 50)      // overlayUpload
};

// 11. Rose Pink (Light Pink)
static const ThemeColors s_rosePinkColors = {
    RGB(255, 245, 247),   // background (Lavender Blush)
    RGB(255, 182, 193),   // border (Light Pink)
    RGB(139, 69, 85),     // textPrimary (Dark Rose)
    RGB(199, 112, 135),   // textSecondary
    RGB(220, 180, 190),   // textDisabled
    RGB(255, 105, 180),   // download (Hot Pink)
    RGB(255, 228, 235),   // downloadFill
    RGB(50, 205, 50),     // upload (Lime Green - contrast)
    RGB(255, 127, 80),    // cpu (Coral)
    RGB(186, 85, 211),    // ram (Medium Orchid)
    RGB(50, 205, 50),     // pingLow
    RGB(255, 165, 0),     // pingMed
    RGB(220, 20, 60),     // pingHigh (Crimson)
    RGB(192, 192, 192),   // pingNone
    RGB(255, 105, 180),   // vpnOn
    RGB(255, 165, 0),     // vpnProxy
    RGB(192, 192, 192),   // vpnOff

    // Charts
    RGB(255, 245, 247),   // chartBackground
    RGB(255, 228, 235),   // chartGrid
    RGB(255, 182, 193),   // chartBorder
    RGB(139, 69, 85),     // chartText
    RGB(255, 105, 180),   // chartDownload
    RGB(50, 205, 50),     // chartUpload

    // Dialogs
    RGB(255, 245, 247),   // dialogBackground
    RGB(255, 240, 245),   // dialogPanel
    RGB(255, 182, 193),   // dialogBorder
    RGB(139, 69, 85),     // dialogText
    RGB(200, 150, 165),   // dialogTextDisabled

    // Lists
    RGB(255, 250, 252),   // listBackground
    RGB(139, 69, 85),     // listText
    RGB(255, 220, 230),   // listSelectedBackground
    RGB(100, 40, 60),     // listSelectedText
    RGB(255, 235, 240),   // listHeaderBackground
    RGB(139, 69, 85),     // listHeaderText
    
    // Tabs
    RGB(255, 235, 240),   // tabBackground
    RGB(255, 245, 247),   // tabSelectedBackground
    RGB(180, 120, 140),   // tabText
    RGB(139, 69, 85),     // tabSelectedText

    // Controls
    RGB(255, 255, 255),   // inputBackground
    RGB(255, 228, 235),   // buttonBackground
    RGB(255, 182, 193),   // buttonPressed
    RGB(255, 182, 193),   // buttonBorder

    // Overlay
    RGB(255, 105, 180),   // overlayDownload (Hot Pink)
    RGB(50, 205, 50)      // overlayUpload
};

// Undocumented definitions for dark mode support
enum class PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnAllowDarkModeForApp = bool(WINAPI*)(bool allow);
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND hwnd, bool allow);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();
using fnFlushMenuThemes = void(WINAPI*)();

// DWMWA_USE_IMMERSIVE_DARK_MODE values
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

// Static function pointers (initialized once)
static bool s_initialized = false;
static fnSetPreferredAppMode s_pSetPreferredAppMode = nullptr;
static fnAllowDarkModeForApp s_pAllowDarkModeForApp = nullptr;
static fnAllowDarkModeForWindow s_pAllowDarkModeForWindow = nullptr;
static fnRefreshImmersiveColorPolicyState s_pRefreshImmersiveColorPolicyState = nullptr;
static fnFlushMenuThemes s_pFlushMenuThemes = nullptr;

static void EnsureInitialized()
{
    if (s_initialized)
    {
        return;
    }

    HMODULE hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxTheme)
    {
        // Ordinal 135 is SetPreferredAppMode (Win10 1903+)
        s_pSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135)));
        
        // Ordinal 132 is AllowDarkModeForApp (Win10 1809)
        s_pAllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(132)));

        // Ordinal 133 is AllowDarkModeForWindow (Win10 1809+)
        s_pAllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133)));

        // Ordinal 104 is RefreshImmersiveColorPolicyState
        s_pRefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(104)));

        // Ordinal 136 is FlushMenuThemes
        s_pFlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(136)));
    }
    s_initialized = true;
}

void ThemeHelper::Initialize()
{
    EnsureInitialized();
}

static ThemeMode s_currentTheme = ThemeMode::SystemDefault; // Default

const ThemeColors& ThemeHelper::GetColors(bool dark)
{
    // If requesting dark colors and we are in a dark preset, return that preset
    if (dark)
    {
        // If current theme is one of the dark presets, return it
        if (s_currentTheme == ThemeMode::Dracula ||
            s_currentTheme == ThemeMode::Cyberpunk ||
            s_currentTheme == ThemeMode::Nord ||
            s_currentTheme == ThemeMode::Forest ||
            s_currentTheme == ThemeMode::OLED ||
            s_currentTheme == ThemeMode::Dark)
        {
            return GetColors(s_currentTheme);
        }
        // Fallback or SystemDefault resolving to dark
        return GetColors(ThemeMode::Dark);
    }
    else
    {
        // If current theme is one of the light presets, return it
        if (s_currentTheme == ThemeMode::SolarizedLight ||
            s_currentTheme == ThemeMode::MorningMist ||
            s_currentTheme == ThemeMode::SoftPaper ||
            s_currentTheme == ThemeMode::MintFresh ||
            s_currentTheme == ThemeMode::Lavender ||
            s_currentTheme == ThemeMode::RosePink ||
            s_currentTheme == ThemeMode::Light)
        {
            return GetColors(s_currentTheme);
        }
        return GetColors(ThemeMode::Light);
    }
}

void ThemeHelper::SetCurrentTheme(ThemeMode mode)
{
    s_currentTheme = mode;
}

ThemeMode ThemeHelper::GetCurrentTheme()
{
    return s_currentTheme;
}

const ThemeColors& ThemeHelper::GetColors(ThemeMode mode)
{
    switch (mode)
    {
    case ThemeMode::Light:
        return s_lightColors;
    case ThemeMode::Dracula:
        return s_draculaColors;
    case ThemeMode::Cyberpunk:
        return s_cyberpunkColors;
    case ThemeMode::Nord:
        return s_nordColors;
    case ThemeMode::Forest:
        return s_forestColors;
    case ThemeMode::OLED:
        return s_oledColors;
    case ThemeMode::SolarizedLight:
        return s_solarizedLightColors;
    case ThemeMode::MorningMist:
        return s_morningMistColors;
    case ThemeMode::SoftPaper:
        return s_softPaperColors;
    case ThemeMode::MintFresh:
        return s_mintFreshColors;
    case ThemeMode::Lavender:
        return s_lavenderColors;
    case ThemeMode::RosePink:
        return s_rosePinkColors;
    case ThemeMode::Dark:
    case ThemeMode::SystemDefault:
    default:
        // For SystemDefault within this context, we usually rely on current state,
        // but here we just return base dark if strictly asking for palette.
        // However, caller usually resolves System -> Light/Dark before calling this
        // if they care. But if we pass SystemDefault here, let's assume it resolved to Dark
        // or check logic? Actually checking IsSystemInDarkMode() here might be better
        // if we want true dynamic, but static return is safer.
        // Let's just return Dark as default.
        return s_darkColors;
    }
}

void ThemeHelper::AllowDarkModeForApp(bool enable)
{
    EnsureInitialized();

    if (s_pSetPreferredAppMode)
    {
        s_pSetPreferredAppMode(enable ? PreferredAppMode::ForceDark : PreferredAppMode::ForceLight);
    }
    else if (s_pAllowDarkModeForApp)
    {
        s_pAllowDarkModeForApp(enable);
    }

    if (s_pRefreshImmersiveColorPolicyState)
    {
        s_pRefreshImmersiveColorPolicyState();
    }

    if (s_pFlushMenuThemes)
    {
        s_pFlushMenuThemes();
    }
}

void ThemeHelper::AllowDarkModeForWindow(HWND hwnd, bool enable)
{
    if (!hwnd)
    {
        return;
    }

    EnsureInitialized();

    if (s_pAllowDarkModeForWindow)
    {
        s_pAllowDarkModeForWindow(hwnd, enable);
    }
}

void ThemeHelper::ApplyDarkThemeToControl(HWND hwnd, bool enable)
{
    if (!hwnd)
    {
        return;
    }

    // First allow dark mode for the window
    AllowDarkModeForWindow(hwnd, enable);

    // Check if this is a ComboBox - needs special handling for dropdown list
    wchar_t className[64] = {0};
    GetClassNameW(hwnd, className, 64);
    bool isComboBox = (_wcsicmp(className, L"ComboBox") == 0);

    if (isComboBox && enable)
    {
        // Apply CFD theme to the ComboBox itself for dark appearance
        SetWindowTheme(hwnd, L"CFD", NULL);

        // Get the dropdown list handle and apply dark theme to it
        COMBOBOXINFO cbi = {0};
        cbi.cbSize = sizeof(COMBOBOXINFO);
        if (GetComboBoxInfo(hwnd, &cbi))
        {
            if (cbi.hwndList)
            {
                AllowDarkModeForWindow(cbi.hwndList, true);
                SetWindowTheme(cbi.hwndList, L"DarkMode_Explorer", NULL);
            }
        }
    }
    else if (enable)
    {
        // Use DarkMode_Explorer for other controls (ListView, etc.)
        SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
    }
    else
    {
        // Reset to default theme
        SetWindowTheme(hwnd, L"Explorer", NULL);
    }

    // Force redraw
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ThemeHelper::ApplyDarkTitleBar(HWND hwnd, bool enable)
{
    if (!hwnd) return;

    // IMPORTANT: Must enable dark mode for window BEFORE DwmSetWindowAttribute
    AllowDarkModeForWindow(hwnd, enable);
    
    BOOL value = enable ? TRUE : FALSE;
    
    // Try the modern attribute first (Windows 11, Win 10 20H1+)
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    
    if (FAILED(hr))
    {
        // Fallback to the older undocumented attribute (Windows 10 1809-1909)
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &value, sizeof(value));
    }

    // Force a repaint of the non-client area (title bar)
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool ThemeHelper::IsSystemInDarkMode()
{
    // Check registry for AppsUseLightTheme
    // 0 = Dark, 1 = Light
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS)
    {
        return false; // Default to light if key missing
    }

    DWORD useLightTheme = 1;
    DWORD dataSize = sizeof(useLightTheme);
    DWORD type = REG_DWORD;

    result = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, &type, reinterpret_cast<LPBYTE>(&useLightTheme), &dataSize);
    
    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS)
    {
        return (useLightTheme == 0);
    }

    return false;
}

} // namespace NetPulse
