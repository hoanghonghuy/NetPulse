// ImGuiApp.h - Dear ImGui Direct3D 11 Host Window
#ifndef NETWORK_MONITOR_IMGUI_APP_H
#define NETWORK_MONITOR_IMGUI_APP_H

#include <d3d11.h>
#include <windows.h>
#include <functional>

namespace NetworkMonitor
{

class ImGuiApp
{
public:
    ImGuiApp();
    ~ImGuiApp();

    // Initialize the application window and DirectX 11
    bool Initialize(HINSTANCE hInstance, int nCmdShow);

    // Main loop - call UI callback every frame
    void Run(std::function<void()> uiCallback);

    // Main loop with exit control - callback returns false to exit
    void Run(std::function<bool()> uiCallbackWithExit);

    // Shutdown and cleanup
    void Shutdown();

    // Getters
    HWND GetHWND() const { return m_hWnd; }
    ID3D11Device* GetDevice() const { return m_pd3dDevice; }
    ID3D11DeviceContext* GetDeviceContext() const { return m_pd3dDeviceContext; }

    // Minimize to tray support
    void SetMinimizeCallback(std::function<void()> callback) { m_minimizeCallback = callback; }
    void SetHotkeyCallback(std::function<void()> callback) { m_hotkeyCallback = callback; }
    void RestoreWindow() { ::ShowWindow(m_hWnd, SW_RESTORE); ::SetForegroundWindow(m_hWnd); }
    void HideWindow() { ::ShowWindow(m_hWnd, SW_HIDE); }
    bool IsWindowVisible() const { return ::IsWindowVisible(m_hWnd); }

    // Global hotkey registration
    bool RegisterGlobalHotkey(UINT modifiers, UINT vk);  // e.g., MOD_WIN | MOD_SHIFT, 'N'
    void UnregisterGlobalHotkey();

private:
    // Window procedure
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // DirectX helpers
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void RunWithExitCheck(std::function<bool()> uiCallbackWithExit);

    // Member variables
    HWND m_hWnd;
    WNDCLASSEXW m_wc;

    // DirectX 11 objects
    ID3D11Device* m_pd3dDevice;
    ID3D11DeviceContext* m_pd3dDeviceContext;
    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_mainRenderTargetView;

    // State
    bool m_running;
    std::function<void()> m_minimizeCallback;
    std::function<void()> m_hotkeyCallback;
    int m_hotkeyId;
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_IMGUI_APP_H
