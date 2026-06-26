#pragma once

#include "NetPulse/Common.h"
#include <string>

struct HWND__;
typedef struct HWND__* HWND;
namespace NetPulseTests
{

void EnableTestSandbox();
std::wstring GetTestSandboxDir();
bool ClearTestRegistrySandbox();
bool PumpWinMessages(unsigned int timeoutMs);
HWND WaitForWindow(const std::wstring& className, unsigned int timeoutMs);
HWND FindProcessDialogHwnd(unsigned int timeoutMs);
void LogTestMessage(const wchar_t* message);
void AssertTrue(bool condition, const wchar_t* testName);
int GetFailureCount();
void ResetFailureCount();

NetPulse::NetworkStats MakeNetworkStats(const std::wstring& name,
                                       unsigned long long bytesDown,
                                       unsigned long long bytesUp,
                                       bool isActive = true,
                                       bool isPhysical = true);

} // namespace NetPulseTests
