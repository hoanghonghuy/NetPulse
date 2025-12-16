#define WIN32_LEAN_AND_MEAN
#include "NetPulse/HotkeyManager.h"
#include "NetPulse/Utils.h"

namespace NetPulse
{

HotkeyManager::HotkeyManager()
    : m_hwnd(nullptr)
{
}

HotkeyManager::~HotkeyManager()
{
    UnregisterAll();
}

void HotkeyManager::Initialize(HWND hwnd)
{
    m_hwnd = hwnd;
}

bool HotkeyManager::RegisterHotkey(int id, UINT modifiers, UINT key)
{
    if (!m_hwnd)
    {
        LogDebug(L"HotkeyManager::RegisterHotkey: No window handle");
        return false;
    }

    // Add MOD_NOREPEAT to prevent repeated hotkey messages
    UINT finalModifiers = modifiers | MOD_NOREPEAT;

    if (!RegisterHotKey(m_hwnd, id, finalModifiers, key))
    {
        LogDebug(L"HotkeyManager::RegisterHotkey: Failed to register hotkey " + std::to_wstring(id));
        return false;
    }

    m_registeredIds.push_back(id);
    LogDebug(L"HotkeyManager::RegisterHotkey: Registered hotkey " + std::to_wstring(id));
    return true;
}

void HotkeyManager::UnregisterAll()
{
    if (!m_hwnd)
    {
        return;
    }

    for (int id : m_registeredIds)
    {
        UnregisterHotKey(m_hwnd, id);
    }

    if (!m_registeredIds.empty())
    {
        LogDebug(L"HotkeyManager::UnregisterAll: Unregistered " + 
                 std::to_wstring(m_registeredIds.size()) + L" hotkeys");
    }

    m_registeredIds.clear();
}

void HotkeyManager::SetCallback(std::function<void(int)> callback)
{
    m_callback = callback;
}

void HotkeyManager::OnHotkey(int hotkeyId)
{
    if (m_callback)
    {
        m_callback(hotkeyId);
    }
}

} // namespace NetPulse
