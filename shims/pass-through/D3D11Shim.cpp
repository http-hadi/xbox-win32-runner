// shims/pass-through/D3D11Shim.cpp
// Pass-through D3D11 shim. Clamps requested feature levels to a max of
// D3D_FEATURE_LEVEL_11_0 (the UWP hard cap on Xbox Series X/S Developer
// Mode) before delegating to the real D3D11CreateDevice / CreateDeviceAndSwapChain.

#include "UwpSdkIncludes.h"


#include <vector>

#include "../ShimRegistry.h"

#ifndef D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS
#define D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS 0x8
#endif

namespace xwr {

// Re-derive the enum values we need at runtime, since the Windows.h stub
// declares D3D_FEATURE_LEVEL as a plain int typedef.
static const int XWR_D3D_FL_9_1  = 0x9100;
static const int XWR_D3D_FL_9_2  = 0x9200;
static const int XWR_D3D_FL_9_3  = 0x9300;
static const int XWR_D3D_FL_10_0 = 0xA000;
static const int XWR_D3D_FL_10_1 = 0xA100;
static const int XWR_D3D_FL_11_0 = 0xB000;
static const int XWR_D3D_FL_11_1 = 0xB100;

// Clamp pFeatureLevels down to those <= D3D_FEATURE_LEVEL_11_0.
// UWP on Xbox Series X|S hard-caps at 11_0.
static void ClampFeatureLevels(const D3D_FEATURE_LEVEL* pIn, UINT Count,
                                std::vector<D3D_FEATURE_LEVEL>* pOut) {
    if (!pIn || Count == 0) {
        // No list passed: real D3D11CreateDevice will use its own default
        // (which caps at 11_0 on UWP anyway).
        return;
    }
    for (UINT i = 0; i < Count; ++i) {
        int fl = (int)pIn[i];
        if (fl <= XWR_D3D_FL_11_0) pOut->push_back(pIn[i]);
    }
    if (pOut->empty()) {
        // Caller asked for 11_1 only; clamp down to 11_0.
        pOut->push_back((D3D_FEATURE_LEVEL)XWR_D3D_FL_11_0);
    }
}

extern "C" HRESULT __stdcall Shim_D3D11CreateDevice(IDXGIAdapter* pAdapter,
                                                     D3D_DRIVER_TYPE DriverType,
                                                     HMODULE Software,
                                                     UINT Flags,
                                                     const D3D_FEATURE_LEVEL* pFeatureLevels,
                                                     UINT FeatureLevels,
                                                     UINT SDKVersion,
                                                     ID3D11Device** ppDevice,
                                                     D3D_FEATURE_LEVEL* pFeatureLevel,
                                                     ID3D11DeviceContext** ppImmediateContext) {
    std::vector<D3D_FEATURE_LEVEL> clamped;
    ClampFeatureLevels(pFeatureLevels, FeatureLevels, &clamped);
    const D3D_FEATURE_LEVEL* pFL = clamped.empty() ? nullptr : clamped.data();
    UINT nFL = clamped.empty() ? 0 : (UINT)clamped.size();
    return ::D3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFL, nFL, SDKVersion,
                                ppDevice, pFeatureLevel, ppImmediateContext);
}

extern "C" HRESULT __stdcall Shim_D3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter,
                                                                  D3D_DRIVER_TYPE DriverType,
                                                                  HMODULE Software,
                                                                  UINT Flags,
                                                                  const D3D_FEATURE_LEVEL* pFeatureLevels,
                                                                  UINT FeatureLevels,
                                                                  UINT SDKVersion,
                                                                  const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
                                                                  IDXGISwapChain** ppSwapChain,
                                                                  ID3D11Device** ppDevice,
                                                                  D3D_FEATURE_LEVEL* pFeatureLevel,
                                                                  ID3D11DeviceContext** ppImmediateContext) {
    std::vector<D3D_FEATURE_LEVEL> clamped;
    ClampFeatureLevels(pFeatureLevels, FeatureLevels, &clamped);
    const D3D_FEATURE_LEVEL* pFL = clamped.empty() ? nullptr : clamped.data();
    UINT nFL = clamped.empty() ? 0 : (UINT)clamped.size();
    return ::D3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFL, nFL,
                                            SDKVersion, pSwapChainDesc, ppSwapChain,
                                            ppDevice, pFeatureLevel, ppImmediateContext);
}

}  // namespace xwr

// ===========================================================================
// Registration — under both "d3d11" and "d3d11.dll".
// ===========================================================================
REGISTER_SHIM("d3d11", "D3D11CreateDevice", (FARPROC)&xwr::Shim_D3D11CreateDevice);
REGISTER_SHIM("d3d11", "D3D11CreateDeviceAndSwapChain", (FARPROC)&xwr::Shim_D3D11CreateDeviceAndSwapChain);
REGISTER_SHIM("d3d11.dll", "D3D11CreateDevice", (FARPROC)&xwr::Shim_D3D11CreateDevice);
REGISTER_SHIM("d3d11.dll", "D3D11CreateDeviceAndSwapChain", (FARPROC)&xwr::Shim_D3D11CreateDeviceAndSwapChain);
