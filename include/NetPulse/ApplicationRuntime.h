#ifndef NETPULSE_APPLICATION_RUNTIME_H
#define NETPULSE_APPLICATION_RUNTIME_H

#include <string>

namespace NetPulse
{

class Application;

struct ApplicationRuntimeOptions
{
    bool testMode = false;
    std::wstring sandboxDir;
    std::wstring testScenario;
    unsigned int timeoutMs = 30000;
    bool autoExit = false;
};

class ApplicationRuntime
{
public:
    static void ParseCommandLine();
    static const ApplicationRuntimeOptions& Options();
    static bool IsTestMode();

    static void ApplySandboxEnvironment();
    static int RunTestScenario(Application& app);

    static const wchar_t* GetMutexName();

private:
    static ApplicationRuntimeOptions s_options;
    static bool s_parsed;
};

} // namespace NetPulse

#endif // NETPULSE_APPLICATION_RUNTIME_H
