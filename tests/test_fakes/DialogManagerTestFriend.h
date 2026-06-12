#pragma once

#include "NetPulse/DialogManager.h"

namespace NetPulseTests
{

struct DialogManagerTestFriend
{
    static void SetDashboardHandle(NetPulse::DialogManager& manager, HWND hwnd)
    {
        manager.m_hDashboardDialog = hwnd;
    }

    static void SetConnectionLogHandle(NetPulse::DialogManager& manager, HWND hwnd)
    {
        manager.m_hConnectionLogDialog = hwnd;
    }

    static void SetPerAppHandle(NetPulse::DialogManager& manager, HWND hwnd)
    {
        manager.m_hPerAppDialog = hwnd;
    }

    static void SetHistoryHandle(NetPulse::DialogManager& manager, HWND hwnd)
    {
        manager.m_hHistoryDialog = hwnd;
    }

    static HWND GetDashboardHandle(const NetPulse::DialogManager& manager)
    {
        return manager.m_hDashboardDialog;
    }

    static HWND GetSpeedTestHandle(const NetPulse::DialogManager& manager)
    {
        return manager.m_hSpeedTestDialog;
    }

    static void ApplySettings(NetPulse::DialogManager& manager, NetPulse::AppConfig& oldConfig)
    {
        manager.HandleSettingsApplied(oldConfig);
    }
};

} // namespace NetPulseTests
