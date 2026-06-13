#include "NetPulse/ApplicationRuntime.h"
#include "NetPulse/Application.h"
#include "NetPulse/Common.h"
#include "NetPulse/Utils.h"
#include "NetPulse/ConfigManager.h"
#include <shellapi.h>
#include <stdlib.h>

namespace NetPulse
{

ApplicationRuntimeOptions ApplicationRuntime::s_options = {};
bool ApplicationRuntime::s_parsed = false;

namespace
{

std::wstring GetSwitchValue(const std::wstring& commandLine, const std::wstring& switchName)
{
    const std::wstring token = switchName + L"=";
    size_t pos = commandLine.find(token);
    if (pos == std::wstring::npos)
    {
        return std::wstring();
    }

    pos += token.size();
    size_t end = commandLine.find(L' ', pos);
    if (end == std::wstring::npos)
    {
        return commandLine.substr(pos);
    }
    return commandLine.substr(pos, end - pos);
}

bool HasSwitch(const std::wstring& commandLine, const std::wstring& switchName)
{
    return commandLine.find(switchName) != std::wstring::npos;
}

} // namespace

void ApplicationRuntime::ParseCommandLine()
{
    if (s_parsed)
    {
        return;
    }

    s_parsed = true;

    const wchar_t* testModeEnv = _wgetenv(L"NETPULSE_TEST_MODE");
    if (testModeEnv && testModeEnv[0] == L'1')
    {
        s_options.testMode = true;
    }

    const wchar_t* scenarioEnv = _wgetenv(L"NETPULSE_TEST_SCENARIO");
    if (scenarioEnv && scenarioEnv[0] != L'\0')
    {
        s_options.testScenario = scenarioEnv;
    }

    const wchar_t* timeoutEnv = _wgetenv(L"NETPULSE_TEST_TIMEOUT_MS");
    if (timeoutEnv && timeoutEnv[0] != L'\0')
    {
        s_options.timeoutMs = static_cast<unsigned int>(_wtoi(timeoutEnv));
    }

    std::wstring commandLine = GetCommandLineW();
    if (commandLine.empty())
    {
        return;
    }

    if (HasSwitch(commandLine, L"--test-mode"))
    {
        s_options.testMode = true;
    }

    if (HasSwitch(commandLine, L"--test-auto-exit"))
    {
        s_options.autoExit = true;
    }

    std::wstring sandboxDir = GetSwitchValue(commandLine, L"--sandbox-dir");
    if (!sandboxDir.empty())
    {
        s_options.sandboxDir = sandboxDir;
    }

    std::wstring scenario = GetSwitchValue(commandLine, L"--test-scenario");
    if (!scenario.empty())
    {
        s_options.testScenario = scenario;
    }

    std::wstring timeout = GetSwitchValue(commandLine, L"--test-timeout-ms");
    if (!timeout.empty())
    {
        s_options.timeoutMs = static_cast<unsigned int>(_wtoi(timeout.c_str()));
    }
}

const ApplicationRuntimeOptions& ApplicationRuntime::Options()
{
    return s_options;
}

bool ApplicationRuntime::IsTestMode()
{
    if (!s_parsed)
    {
        ParseCommandLine();
    }

    if (s_options.testMode)
    {
        return true;
    }

    const wchar_t* testModeEnv = _wgetenv(L"NETPULSE_TEST_MODE");
    return testModeEnv && testModeEnv[0] == L'1';
}

void ApplicationRuntime::ApplySandboxEnvironment()
{
    if (!s_parsed)
    {
        ParseCommandLine();
    }

    if (s_options.testMode)
    {
        _wputenv_s(L"NETPULSE_TEST_MODE", L"1");
    }

    if (!s_options.sandboxDir.empty())
    {
        CreateDirectoryW(s_options.sandboxDir.c_str(), nullptr);
        _wputenv_s(L"NETPULSE_TEST_DATA_DIR", s_options.sandboxDir.c_str());
    }

    const wchar_t* registryPath = _wgetenv(L"NETPULSE_TEST_REGISTRY_PATH");
    if (!registryPath || registryPath[0] == L'\0')
    {
        _wputenv_s(L"NETPULSE_TEST_REGISTRY_PATH", L"Software\\NetworkMonitor\\Test");
    }
}

const wchar_t* ApplicationRuntime::GetMutexName()
{
    if (!IsTestMode())
    {
        return APP_MUTEX_NAME;
    }

    static std::wstring testMutexName;
    testMutexName = APP_MUTEX_NAME_TEST;
    testMutexName += L"_";
    testMutexName += std::to_wstring(GetCurrentProcessId());
    return testMutexName.c_str();
}

int ApplicationRuntime::RunTestScenario(Application& app)
{
    if (s_options.testScenario == L"launch-exit")
    {
        LogDebug(L"ApplicationRuntime: scenario launch-exit completed");
        return 0;
    }

    if (s_options.testScenario == L"config-default")
    {
        const AppConfig& config = app.GetConfig();
        if (config.displayUnit != SpeedUnit::KiloBytesPerSecond)
        {
            LogError(L"ApplicationRuntime: expected default display unit KiloBytesPerSecond");
            return 2;
        }
        LogDebug(L"ApplicationRuntime: scenario config-default completed");
        return 0;
    }

    LogError(L"ApplicationRuntime: unknown test scenario");
    return 3;
}

} // namespace NetPulse
