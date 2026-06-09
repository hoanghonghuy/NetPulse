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
