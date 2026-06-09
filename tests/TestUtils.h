#pragma once

#include <string>

namespace NetPulseTests
{

void EnableTestSandbox();
void LogTestMessage(const wchar_t* message);
void AssertTrue(bool condition, const wchar_t* testName);
int GetFailureCount();
void ResetFailureCount();

} // namespace NetPulseTests
