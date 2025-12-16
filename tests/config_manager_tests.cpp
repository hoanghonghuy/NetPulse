#include "NetPulse/Common.h"
#include "NetPulse/ConfigManager.h"
#include "TestUtils.h"

using namespace NetPulse;

namespace NetPulseTests
{

void RunConfigManagerTests()
{
    LogTestMessage(L"=== ConfigManager tests ===");

    ConfigManager mgr;

    // Round-trip AppConfig through registry and restore original
    AppConfig original;
    bool loadedOriginal = mgr.LoadConfig(original);
    AssertTrue(loadedOriginal, L"ConfigManager.LoadConfig(original) returns true");

    AppConfig modified = original;

    // Flip a few fields so that we can verify persistence
    modified.updateInterval = (original.updateInterval == UPDATE_INTERVAL_FAST)
        ? UPDATE_INTERVAL_NORMAL
        : UPDATE_INTERVAL_FAST;

    modified.displayUnit = (original.displayUnit == SpeedUnit::KiloBytesPerSecond)
        ? SpeedUnit::MegaBytesPerSecond
        : SpeedUnit::KiloBytesPerSecond;

    modified.debugLogging = !original.debugLogging;
    modified.darkTheme = !original.darkTheme;
    modified.selectedInterface = L"TestInterface";

    bool saved = mgr.SaveConfig(modified);
    AssertTrue(saved, L"ConfigManager.SaveConfig(modified) returns true");

    AppConfig reloaded;
    bool loaded = mgr.LoadConfig(reloaded);
    AssertTrue(loaded, L"ConfigManager.LoadConfig(reloaded) returns true");

    AssertTrue(reloaded.updateInterval == modified.updateInterval,
               L"ConfigManager round-trip updateInterval");
    AssertTrue(reloaded.displayUnit == modified.displayUnit,
               L"ConfigManager round-trip displayUnit");
    AssertTrue(reloaded.debugLogging == modified.debugLogging,
               L"ConfigManager round-trip debugLogging");
    AssertTrue(reloaded.darkTheme == modified.darkTheme,
               L"ConfigManager round-trip darkTheme");
    AssertTrue(reloaded.selectedInterface == modified.selectedInterface,
               L"ConfigManager round-trip selectedInterface");

    // Restore original configuration to avoid persisting test values
    bool restored = mgr.SaveConfig(original);
    AssertTrue(restored, L"ConfigManager.SaveConfig(original) restore returns true");

    // Auto-start toggle round-trip
    bool autoStartOriginal = mgr.IsAutoStartEnabled();

    bool setOpposite = mgr.SetAutoStart(!autoStartOriginal);
    AssertTrue(setOpposite, L"ConfigManager.SetAutoStart(toggle) returns true");

    bool autoStartAfter = mgr.IsAutoStartEnabled();
    AssertTrue(autoStartAfter == !autoStartOriginal,
               L"ConfigManager.IsAutoStartEnabled reflects toggled value");

    bool setBack = mgr.SetAutoStart(autoStartOriginal);
    AssertTrue(setBack, L"ConfigManager.SetAutoStart(restore) returns true");
}

} // namespace NetPulseTests
