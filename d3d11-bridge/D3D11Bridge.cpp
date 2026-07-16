// d3d11-bridge/D3D11Bridge.cpp
// Creates the real D3D11 device + swap chain bound to the UWP CoreWindow's HWND.
//
// On real Windows / Xbox builds, <d3d11.h> and <dxgi.h> are pulled in via the
// UWP SDK. On Linux syntax-check builds (XWR_SYNTAX_CHECK defined), those
// headers don't exist; we stub the real API calls and return fake pointers so
// the rest of the file compiles cleanly against winheaders/Windows.h.

#include "UwpSdkIncludes.h"


#include "D3D11Bridge.h"

// Real builds link against d3d11.lib / dxgi.lib and have these headers.
// Syntax-check builds have only the stub Windows.h, so skip them.
#ifndef XWR_SYNTAX_CHECK
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#endif

namespace xwr {

// FAILED/SUCCEEDED are defined by the real SDK but not by the stub Windows.h.
#ifndef FAILED
#define FAILED(hr) ((HRESULT)(hr) < 0)
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#endif

// UWP on Xbox Series X|S hard-caps D3D at feature level 11_0.
static const D3D_FEATURE_LEVEL XWR_MAX_FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0;

// D3D11_SDK_VERSION is 7 in modern Windows SDKs. The stub Windows.h doesn't
// define it, so define it locally if missing.
#ifndef D3D11_SDK_VERSION
#define D3D11_SDK_VERSION 7
#endif

// D3D11_CREATE_DEVICE_FLAG values used by Initialize().
#ifndef D3D11_CREATE_DEVICE_SINGLETHREADED
#define D3D11_CREATE_DEVICE_SINGLETHREADED 0x1
#endif
#ifndef D3D11_CREATE_DEVICE_BGRA_SUPPORT
#define D3D11_CREATE_DEVICE_BGRA_SUPPORT 0x20
#endif
#ifndef D3D11_CREATE_DEVICE_DEBUG
#define D3D11_CREATE_DEVICE_DEBUG 0x2
#endif

D3D11Bridge::D3D11Bridge() = default;

D3D11Bridge& D3D11Bridge::Instance() {
    static D3D11Bridge inst;
    return inst;
}

bool D3D11Bridge::Initialize(HWND hwnd) {
    if (!hwnd) {
        return false;
    }
    if (m_device && m_context && m_swapChain) {
        return true;  // already initialized
    }

    // Build the swap-chain descriptor. UWP requires DXGI_SWAP_EFFECT_FLIP_DISCARD
    // (or FLIP_SEQUENTIAL) and Windowed=TRUE; the OS scales to full screen.
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = 0;   // 0 = match window
    scd.BufferDesc.Height = 0;
    scd.BufferDesc.Format = (DXGI_FORMAT)DXGI_FORMAT_B8G8R8A8_UNORM;  // 87
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.ScanlineOrdering = (DXGI_MODE_SCANLINE_ORDER)0;  // DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED
    scd.BufferDesc.Scaling = (DXGI_MODE_SCALING)0;                  // DXGI_MODE_SCALING_UNSPECIFIED
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 0x20
    scd.OutputWindow = hwnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;     // 4
    scd.Flags = 0;  // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH = 0 (none)

    // Single feature level list — clamped to 11_0 max.
    D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL obtainedLevel = D3D_FEATURE_LEVEL_11_0;

    UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;  // required for 2D interop
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#ifndef XWR_SYNTAX_CHECK
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain*      swap    = nullptr;

    HRESULT hr = ::D3D11CreateDeviceAndSwapChain(
        /*pAdapter*/          nullptr,
        /*DriverType*/        D3D_DRIVER_TYPE_HARDWARE,
        /*Software*/          nullptr,
        /*Flags*/             createFlags,
        /*pFeatureLevels*/    requestedLevels,
        /*FeatureLevels*/     1,
        /*SDKVersion*/        D3D11_SDK_VERSION,
        /*pSwapChainDesc*/    &scd,
        /*ppSwapChain*/       &swap,
        /*ppDevice*/          &device,
        /*pFeatureLevel*/     &obtainedLevel,
        /*ppImmediateContext*/ &context);

    if (FAILED(hr) || !device || !context || !swap) {
        // Defensive: release anything we did get.
        if (swap)    { swap->Release(); }
        if (context) { context->Release(); }
        if (device)  { device->Release(); }
        return false;
    }

    m_device    = (void*)device;
    m_context   = (void*)context;
    m_swapChain = (void*)swap;
    return true;
#else
    // Syntax-check stub: can't call real D3D11 (no <d3d11.h>). Pretend success
    // with sentinel pointers so downstream GetDevice() callers compile.
    (void)requestedLevels;
    (void)obtainedLevel;
    (void)createFlags;
    m_device    = (void*)0x1;
    m_context   = (void*)0x2;
    m_swapChain = (void*)0x3;
    return true;
#endif
}

void* D3D11Bridge::GetDevice() const {
    return m_device;
}

void* D3D11Bridge::GetImmediateContext() const {
    return m_context;
}

void* D3D11Bridge::GetSwapChain() const {
    return m_swapChain;
}

void D3D11Bridge::Present(int syncInterval, int flags) {
#ifndef XWR_SYNTAX_CHECK
    if (m_swapChain) {
        // IDXGISwapChain::Present — syncInterval=1 locks to vsync, 0 tears.
        // DXGI_PRESENT_TEST (0x1) doesn't actually flip.
        ((IDXGISwapChain*)m_swapChain)->Present((UINT)syncInterval, (UINT)flags);
    }
#else
    (void)syncInterval;
    (void)flags;
#endif
}

void D3D11Bridge::Shutdown() {
#ifndef XWR_SYNTAX_CHECK
    if (m_swapChain) {
        // Set fullscreen state to false before release to avoid leaks.
        ((IDXGISwapChain*)m_swapChain)->SetFullscreenState(FALSE, nullptr);
        ((IDXGISwapChain*)m_swapChain)->Release();
    }
    if (m_context) {
        // ClearState before Release to flush any bound resources.
        ((ID3D11DeviceContext*)m_context)->ClearState();
        ((ID3D11DeviceContext*)m_context)->Flush();
        ((ID3D11DeviceContext*)m_context)->Release();
    }
    if (m_device) {
        ((ID3D11Device*)m_device)->Release();
    }
#endif
    m_device    = nullptr;
    m_context   = nullptr;
    m_swapChain = nullptr;
}

}  // namespace xwr
