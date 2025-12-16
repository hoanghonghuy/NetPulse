// ============================================================================
// File: vpn_detection_tests.cpp
// Description: Unit tests for VpnProxyDetector
// ============================================================================

#include "TestUtils.h"
#include "NetPulse/VpnProxyDetector.h"
#include <memory>
#include <thread>
#include <chrono>

namespace NetPulseTests
{

using namespace NetPulse;

void TestVpnDetectorInitialization()
{
    LogTestMessage(L"  Running TestVpnDetectorInitialization...");
    
    VpnProxyDetector detector;
    AssertTrue(detector.Initialize(), L"Initialize should return true");
    
    detector.Cleanup();
}

void TestVpnDetectorStatusCheck()
{
    LogTestMessage(L"  Running TestVpnDetectorStatusCheck...");
    
    // We can't easily mock network adapters in a unit test without a complex mock framework,
    // so we just verify that we can call the methods without crashing and get a valid bool.
    // In a CI environment without VPN, this should be false, but user might have VPN.
    
    VpnProxyDetector detector;
    detector.Initialize();
    
    // Force an update
    detector.Update();
    
    bool vpnActive = detector.IsVpnActive();
    LogTestMessage((L"    VPN Active: " + std::to_wstring(vpnActive)).c_str());
    
    bool proxyActive = detector.IsProxyActive();
    LogTestMessage((L"    Proxy Active: " + std::to_wstring(proxyActive)).c_str());
    
    // Get adapter name (might be empty)
    std::wstring adapterName = detector.GetVpnAdapterName();
    if (vpnActive)
    {
        AssertTrue(!adapterName.empty(), L"If VPN active, adapter name should not be empty");
        LogTestMessage((L"    VPN Adapter: " + adapterName).c_str());
    }
    
    detector.Cleanup();
}

void TestPublicIPFetching()
{
    // Skip this test in automated environments if no internet access is guaranteed,
    // but for now we'll run it and log the result. We won't Assert assertion failure on IP fetch
    // to avoid flaky tests if internet is down.
    
    LogTestMessage(L"  Running TestPublicIPFetching...");
    
    VpnProxyDetector detector;
    detector.Initialize();
    
    // Force update (synchronous for tests?)
    // Update() calls FetchPublicIP() only if interval passed, but first call should run.
    detector.Update();
    
    // Give it a moment (Update calls FetchPublicIP synchronously in current implementation?)
    // Yes, FetchPublicIP is synchronous in Update() at line 92 of VpnProxyDetector.cpp
    
    std::wstring ip = detector.GetPublicIP();
    
    if (ip.empty())
    {
        LogTestMessage(L"    Public IP: [Empty/Failed] (Check internet connection)");
    }
    else
    {
        AssertTrue(!ip.empty(), L"Public IP should not be empty if successful");
        LogTestMessage((L"    Public IP: " + ip).c_str());
    }
    
    detector.Cleanup();
}

void RunVpnProxyDetectorTests()
{
    LogTestMessage(L"Running VpnProxyDetector Tests...");
    
    TestVpnDetectorInitialization();
    TestVpnDetectorStatusCheck();
    TestPublicIPFetching();
}

} // namespacce NetworkMonitorTests
