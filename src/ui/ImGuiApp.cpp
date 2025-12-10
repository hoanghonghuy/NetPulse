// ImGuiApp.cpp - Dear ImGui Direct3D 11 Host Window Implementation
#include "NetworkMonitor/ImGuiApp.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <string>

// Forward declaration of ImGui Win32 handler (from imgui_impl_win32.cpp)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace NetworkMonitor
{

// Static instance pointer for WndProc callback
static ImGuiApp* s_pInstance = nullptr;

ImGuiApp::ImGuiApp()
    : m_hWnd(nullptr)
    , m_wc{}
    , m_pd3dDevice(nullptr)
    , m_pd3dDeviceContext(nullptr)
    , m_pSwapChain(nullptr)
    , m_mainRenderTargetView(nullptr)
    , m_running(false)
    , m_hotkeyId(0)
{
    s_pInstance = this;
}

ImGuiApp::~ImGuiApp()
{
    Shutdown();
    s_pInstance = nullptr;
}

bool ImGuiApp::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    // Register window class
    m_wc.cbSize = sizeof(WNDCLASSEXW);
    m_wc.style = CS_CLASSDC;
    m_wc.lpfnWndProc = WndProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = hInstance;
    m_wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    m_wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    m_wc.hbrBackground = nullptr;
    m_wc.lpszMenuName = nullptr;
    m_wc.lpszClassName = L"ImGuiAppClass";
    m_wc.hIconSm = nullptr;
    RegisterClassExW(&m_wc);

    // Create window
    m_hWnd = CreateWindowExW(
        0,
        m_wc.lpszClassName,
        L"NetworkMonitor - Modern UI",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 720,
        nullptr, nullptr, hInstance, nullptr
    );

    // Initialize Direct3D
    if (!CreateDeviceD3D(m_hWnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
        return false;
    }

    // Show window
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // Note: Docking requires the docking branch of ImGui, which is not being used here.

    // Setup Dear ImGui style (Dark theme)
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_pd3dDevice, m_pd3dDeviceContext);

    m_running = true;
    return true;
}

void ImGuiApp::Run(std::function<void()> uiCallback)
{
    // Call the bool version with a wrapper
    std::function<bool()> wrapper = [uiCallback]() -> bool {
        if (uiCallback)
            uiCallback();
        return true; // Keep running
    };
    RunWithExitCheck(wrapper);
}

void ImGuiApp::Run(std::function<bool()> uiCallbackWithExit)
{
    RunWithExitCheck(uiCallbackWithExit);
}

void ImGuiApp::RunWithExitCheck(std::function<bool()> uiCallbackWithExit)
{
    ImVec4 clearColor = ImVec4(0.10f, 0.10f, 0.12f, 1.00f); // Dark background

    while (m_running)
    {
        // Process Win32 messages
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                m_running = false;
            }
        }

        if (!m_running)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Execute user UI callback (returns false to exit)
        if (uiCallbackWithExit)
        {
            if (!uiCallbackWithExit())
            {
                m_running = false;
                ImGui::Render();
                break;
            }
        }

        // Rendering
        ImGui::Render();
        const float clearColorRGBA[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
        m_pd3dDeviceContext->OMSetRenderTargets(1, &m_mainRenderTargetView, nullptr);
        m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView, clearColorRGBA);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        m_pSwapChain->Present(1, 0); // VSync
    }
}

void ImGuiApp::Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(m_hWnd);
    UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}

LRESULT CALLBACK ImGuiApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (s_pInstance && s_pInstance->m_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            s_pInstance->CleanupRenderTarget();
            s_pInstance->m_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            s_pInstance->CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        if ((wParam & 0xfff0) == SC_MINIMIZE)
        {
            // Minimize to tray instead of taskbar
            if (s_pInstance && s_pInstance->m_minimizeCallback)
            {
                s_pInstance->m_minimizeCallback();
            }
            ::ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_HOTKEY:
        if (s_pInstance && s_pInstance->m_hotkeyCallback)
        {
            s_pInstance->m_hotkeyCallback();
        }
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool ImGuiApp::CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &m_pSwapChain,
        &m_pd3dDevice,
        &featureLevel,
        &m_pd3dDeviceContext
    );

    if (FAILED(hr))
        return false;

    CreateRenderTarget();
    return true;
}

void ImGuiApp::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pd3dDeviceContext) { m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = nullptr; }
    if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
}

void ImGuiApp::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void ImGuiApp::CleanupRenderTarget()
{
    if (m_mainRenderTargetView)
    {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

bool ImGuiApp::RegisterGlobalHotkey(UINT modifiers, UINT vk)
{
    if (!m_hWnd)
        return false;

    // Use a static ID counter for unique hotkey IDs
    static int s_hotkeyCounter = 1;
    m_hotkeyId = s_hotkeyCounter++;

    return ::RegisterHotKey(m_hWnd, m_hotkeyId, modifiers, vk) != 0;
}

void ImGuiApp::UnregisterGlobalHotkey()
{
    if (m_hWnd && m_hotkeyId > 0)
    {
        ::UnregisterHotKey(m_hWnd, m_hotkeyId);
        m_hotkeyId = 0;
    }
}

} // namespace NetworkMonitor
