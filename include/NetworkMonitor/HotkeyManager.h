#ifndef NETWORK_MONITOR_HOTKEY_MANAGER_H
#define NETWORK_MONITOR_HOTKEY_MANAGER_H

#include <windows.h>
#include <functional>
#include <vector>

namespace NetworkMonitor
{

/**
 * HotkeyManager - Manages global hotkey registration and callbacks
 * Extracted from Application class for SRP compliance
 */
class HotkeyManager
{
public:
    HotkeyManager();
    ~HotkeyManager();

    /**
     * Initialize the hotkey manager
     * @param hwnd Window handle to receive hotkey messages
     */
    void Initialize(HWND hwnd);

    /**
     * Register a global hotkey
     * @param id Unique hotkey identifier
     * @param modifiers Modifier keys (MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN)
     * @param key Virtual key code
     * @return true if successful, false otherwise
     */
    bool RegisterHotkey(int id, UINT modifiers, UINT key);

    /**
     * Unregister all registered hotkeys
     */
    void UnregisterAll();

    /**
     * Set callback for hotkey events
     * @param callback Function called when a hotkey is pressed
     */
    void SetCallback(std::function<void(int)> callback);

    /**
     * Handle hotkey message (call from WM_HOTKEY handler)
     * @param hotkeyId The hotkey ID from the message
     */
    void OnHotkey(int hotkeyId);

private:
    HWND m_hwnd;
    std::vector<int> m_registeredIds;
    std::function<void(int)> m_callback;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_HOTKEY_MANAGER_H
