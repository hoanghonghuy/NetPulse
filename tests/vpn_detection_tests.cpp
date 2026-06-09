#include "TestUtils.h"
#include "test_fakes/FakeHttpClient.h"
#include "NetPulse/VpnProxyDetector.h"
#include <memory>
#include <thread>
#include <chrono>

namespace NetPulseTests
{

using namespace NetPulse;

struct VpnProxyDetectorTestFriend
{
    static std::wstring FetchPublicIPSync(VpnProxyDetector& detector)
    {
        return detector.FetchPublicIP();
    }

    static void StorePublicIP(VpnProxyDetector& detector, const std::wstring& ip)
    {
        std::lock_guard<std::mutex> lock(detector.m_mutex);
        detector.m_publicIP = ip;
    }
};

void TestVpnDetectorInitialization()
{
    LogTestMessage(L"  Running TestVpnDetectorInitialization...");

    VpnProxyDetector detector;
    AssertTrue(detector.Initialize(), L"Initialize should return true");
    AssertTrue(detector.IsAvailable(), L"Detector should be available after init");

    detector.Cleanup();
    AssertTrue(!detector.IsAvailable(), L"Detector unavailable after cleanup");
}

void TestVpnAdapterKeywordHeuristic()
{
    LogTestMessage(L"  Running TestVpnAdapterKeywordHeuristic...");

    AssertTrue(VpnProxyDetector::IsVpnAdapter(0, L"WireGuard Tunnel"),
               L"WireGuard keyword matches VPN adapter");
    AssertTrue(VpnProxyDetector::IsVpnAdapter(0, L"TAP-Windows Adapter V9"),
               L"TAP-Windows keyword matches VPN adapter");
    AssertTrue(VpnProxyDetector::IsVpnAdapter(0, L"openvpn data channel"),
               L"Keyword match is case-insensitive");
    AssertTrue(!VpnProxyDetector::IsVpnAdapter(0, L"Intel(R) Ethernet"),
               L"Regular adapter description does not match VPN keywords");
}

void TestVpnAdapterTypeHeuristic()
{
    LogTestMessage(L"  Running TestVpnAdapterTypeHeuristic...");

    AssertTrue(VpnProxyDetector::IsVpnInterfaceType(IF_TYPE_TUNNEL),
               L"TUNNEL interface type is VPN-like");
    AssertTrue(VpnProxyDetector::IsVpnInterfaceType(IF_TYPE_PPP),
               L"PPP interface type is VPN-like");
    AssertTrue(!VpnProxyDetector::IsVpnInterfaceType(IF_TYPE_ETHERNET_CSMACD),
               L"Ethernet interface type is not VPN-like");
}

void TestVpnAdapterFakeListClassification()
{
    LogTestMessage(L"  Running TestVpnAdapterFakeListClassification...");

    VpnAdapterCandidate wireGuard{};
    wireGuard.ifType = IF_TYPE_ETHERNET_CSMACD;
    wireGuard.friendlyName = L"WireGuard Tunnel";
    wireGuard.isUp = true;
    AssertTrue(VpnProxyDetector::ClassifyAdapterAsVpn(wireGuard),
               L"Fake adapter list classifies WireGuard as VPN");

    VpnAdapterCandidate tunnel{};
    tunnel.ifType = IF_TYPE_TUNNEL;
    tunnel.friendlyName = L"Corporate Tunnel";
    tunnel.isUp = true;
    AssertTrue(VpnProxyDetector::ClassifyAdapterAsVpn(tunnel),
               L"Fake adapter list classifies tunnel interface as VPN");

    VpnAdapterCandidate disabled{};
    disabled.ifType = IF_TYPE_TUNNEL;
    disabled.friendlyName = L"WireGuard Tunnel";
    disabled.isUp = false;
    AssertTrue(!VpnProxyDetector::ClassifyAdapterAsVpn(disabled),
               L"Disabled adapter is not classified as active VPN");

    VpnAdapterCandidate ethernet{};
    ethernet.ifType = IF_TYPE_ETHERNET_CSMACD;
    ethernet.friendlyName = L"Realtek PCIe GbE";
    ethernet.isUp = true;
    AssertTrue(!VpnProxyDetector::ClassifyAdapterAsVpn(ethernet),
               L"Regular ethernet adapter is not classified as VPN");
}

void TestProxyConfigEvaluation()
{
    LogTestMessage(L"  Running TestProxyConfigEvaluation...");

    AssertTrue(!VpnProxyDetector::EvaluateProxyConfig(false, false, false),
               L"Proxy off when no manual/auto/detect flags");
    AssertTrue(VpnProxyDetector::EvaluateProxyConfig(true, false, false),
               L"Manual proxy flag enables proxy");
    AssertTrue(VpnProxyDetector::EvaluateProxyConfig(false, true, false),
               L"Auto-config URL enables proxy");
    AssertTrue(VpnProxyDetector::EvaluateProxyConfig(false, false, true),
               L"WPAD auto-detect enables proxy");
}

void TestPublicIPParseResponse()
{
    LogTestMessage(L"  Running TestPublicIPParseResponse...");

    AssertTrue(VpnProxyDetector::ParsePublicIPResponse("203.0.113.10\n") == L"203.0.113.10",
               L"ParsePublicIPResponse trims trailing newline");
    AssertTrue(VpnProxyDetector::ParsePublicIPResponse("").empty(),
               L"ParsePublicIPResponse rejects empty body");
    AssertTrue(VpnProxyDetector::ParsePublicIPResponse(std::string(50, 'x')).empty(),
               L"ParsePublicIPResponse rejects oversized body");
}

void TestPublicIPFetchWithFakeHttp()
{
    LogTestMessage(L"  Running TestPublicIPFetchWithFakeHttp...");

    auto fake = std::make_shared<FakeHttpClient>();
    fake->m_getBody = "198.51.100.42";

    VpnProxyDetector detector(fake);
    std::wstring ip = VpnProxyDetectorTestFriend::FetchPublicIPSync(detector);

    AssertTrue(ip == L"198.51.100.42", L"Fake HTTP client returns parsed public IP");
    AssertTrue(fake->m_getCallCount == 1, L"Public IP fetch issues one HTTP GET");
    AssertTrue(fake->m_lastHost == VpnProxyDetector::GetPublicIPApiHost(),
               L"Public IP fetch uses ipify host");
    AssertTrue(fake->m_lastPath == VpnProxyDetector::GetPublicIPApiPath(),
               L"Public IP fetch uses root path");
}

void TestPublicIPFetchHttpFail()
{
    LogTestMessage(L"  Running TestPublicIPFetchHttpFail...");

    auto fake = std::make_shared<FakeHttpClient>();
    fake->m_getSuccess = false;

    VpnProxyDetector detector(fake);
    std::wstring ip = VpnProxyDetectorTestFriend::FetchPublicIPSync(detector);

    AssertTrue(ip.empty(), L"HTTP fail returns empty public IP");
}

void TestVpnDetectorStatusSmoke()
{
    LogTestMessage(L"  Running TestVpnDetectorStatusSmoke...");

    VpnProxyDetector detector;
    detector.Initialize();
    detector.Update();

    bool vpnActive = detector.IsVpnActive();
    bool proxyActive = detector.IsProxyActive();
    LogTestMessage((L"    VPN Active: " + std::to_wstring(vpnActive)).c_str());
    LogTestMessage((L"    Proxy Active: " + std::to_wstring(proxyActive)).c_str());

    if (vpnActive)
    {
        AssertTrue(!detector.GetVpnAdapterName().empty(),
                   L"If VPN active, adapter name should not be empty");
    }

    detector.Cleanup();
}

void TestPublicIPFetchingNetworkSmoke()
{
    LogTestMessage(L"  Running TestPublicIPFetchingNetworkSmoke (optional network)...");

    VpnProxyDetector detector;
    detector.Initialize();
    detector.SetPublicIPUpdateInterval(0);
    detector.RefreshPublicIP();

    for (int i = 0; i < 50; ++i)
    {
        detector.Update();
        if (!detector.GetPublicIP().empty())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::wstring ip = detector.GetPublicIP();
    if (ip.empty())
    {
        LogTestMessage(L"    Public IP: [Empty/Failed] — skipped assertion (no network)");
    }
    else
    {
        LogTestMessage((L"    Public IP: " + ip).c_str());
    }

    detector.Cleanup();
}

void RunVpnProxyDetectorTests()
{
    LogTestMessage(L"=== VpnProxyDetector tests ===");

    TestVpnDetectorInitialization();
    TestVpnAdapterKeywordHeuristic();
    TestVpnAdapterTypeHeuristic();
    TestVpnAdapterFakeListClassification();
    TestProxyConfigEvaluation();
    TestPublicIPParseResponse();
    TestPublicIPFetchWithFakeHttp();
    TestPublicIPFetchHttpFail();
    TestVpnDetectorStatusSmoke();

    // Real network smoke — not required for PR pass
    // TestPublicIPFetchingNetworkSmoke();

    LogTestMessage(L"VpnProxyDetector tests completed.");
}

} // namespace NetPulseTests
