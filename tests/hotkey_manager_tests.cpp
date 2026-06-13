#include "NetPulse/Common.h"
#include "NetPulse/HotkeyManager.h"
#include "TestUtils.h"

using namespace NetPulse;

namespace NetPulseTests
{

void RunHotkeyManagerTests()
{
    LogTestMessage(L"=== HotkeyManager tests ===");

    // Test 1: Default initialization
    {
        HotkeyManager manager;
        // RegisterHotkey without HWND should return false
        AssertTrue(!manager.RegisterHotkey(1, MOD_CONTROL, 'K'), 
                   L"RegisterHotkey with null HWND returns false");
        
        // No crash when calling UnregisterAll with null HWND
        manager.UnregisterAll();
    }

    // Test 2: Callback execution
    {
        HotkeyManager manager;
        int triggeredId = 0;
        manager.SetCallback([&triggeredId](int id) {
            triggeredId = id;
        });

        manager.OnHotkey(42);
        AssertTrue(triggeredId == 42, L"OnHotkey triggers callback with correct ID");
    }

    // Test 3: Test with message-only window
    {
        HWND hwnd = CreateWindowExW(
            0,
            L"Static",
            L"HotkeyTestWindow",
            0,
            0, 0, 0, 0,
            HWND_MESSAGE,
            nullptr,
            nullptr,
            nullptr
        );

        if (hwnd)
        {
            HotkeyManager manager;
            manager.Initialize(hwnd);

            // Register a key combination (Ctrl + Alt + Shift + P)
            UINT modifiers = MOD_CONTROL | MOD_ALT | MOD_SHIFT;
            bool registered = manager.RegisterHotkey(101, modifiers, 'P');
            
            if (registered)
            {
                LogTestMessage(L"  Hotkey registered successfully on test window");
                manager.UnregisterAll();
            }
            else
            {
                LogTestMessage(L"  Hotkey registration skipped/failed (expected if combination in use or non-interactive context)");
            }

            DestroyWindow(hwnd);
        }
        else
        {
            LogTestMessage(L"  Skipping message-only window test: CreateWindowExW failed");
        }
    }
    
    LogTestMessage(L"HotkeyManager tests completed.");
}

} // namespace NetPulseTests
