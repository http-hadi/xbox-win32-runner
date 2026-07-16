// d3d11-bridge/D3D11Bridge.h
// Owns the real Xbox D3D11 device + swap chain and exposes the raw
// ID3D11Device* / ID3D11DeviceContext* / IDXGISwapChain* pointers to game code
// via the PE loader's IAT.
//
// Lifecycle:
//   1) UWP shell creates the CoreWindow and obtains its HWND via interop.
//   2) Shell calls D3D11Bridge::Instance().Initialize(hwnd).
//   3) Shim_D3D11CreateDevice / CreateDeviceAndSwapChain hand out
//      D3D11Bridge::Instance().GetDevice() etc. instead of creating a new
//      device, so the game sees the same device the shell uses to Present().
//   4) UWP shell's render loop calls Present() every frame.
//   5) On suspend / shutdown, call Shutdown().
//
// Feature level is hard-clamped to D3D_FEATURE_LEVEL_11_0 (UWP hard cap on
// Xbox Series X|S Developer Mode).

#pragma once
#include <Windows.h>

namespace xwr {

class D3D11Bridge {
public:
    static D3D11Bridge& Instance();

    // Initialize the real D3D11 device bound to the UWP window's CoreWindow.
    // Clamps feature level to 11.0 (UWP hard cap).
    // Returns false if the device cannot be created.
    bool Initialize(HWND hwnd);

    // Returns the real ID3D11Device* for handing out to the game's IAT.
    void* GetDevice() const;
    void* GetImmediateContext() const;
    void* GetSwapChain() const;

    // Present the backbuffer to the screen.
    void Present(int syncInterval, int flags);

    void Shutdown();

private:
    D3D11Bridge();
    D3D11Bridge(const D3D11Bridge&) = delete;
    D3D11Bridge& operator=(const D3D11Bridge&) = delete;

    void* m_device = nullptr;     // ID3D11Device*
    void* m_context = nullptr;    // ID3D11DeviceContext*
    void* m_swapChain = nullptr;  // IDXGISwapChain*
};

}  // namespace xwr
