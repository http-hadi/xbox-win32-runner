// shims/pass-through/D3D12Shim.cpp
// Pass-through D3D12 shim. Clamps requested feature level to 11_0 (the UWP
// hard cap on Xbox Developer Mode) before delegating to the real
// D3D12CreateDevice. D3D12 on Xbox is presented via the DX12-on-FL11 path
// in UWP, so we just clamp and forward.

#include "UwpSdkIncludes.h"


#include "../ShimRegistry.h"

// Forward-declare the real D3D12CreateDevice (declared in d3d12.h on Windows;
// we declare it locally so we don't need to include d3d12.h on Linux syntax
// checks). The real <d3d12.h> pulled in by UwpSdkIncludes.h on MSVC already
// declares it — guard with a macro to avoid a conflicting redeclaration on
// real Windows builds.
#ifndef XWR_D3D12_CREATE_DEVICE_DECLARED
#define XWR_D3D12_CREATE_DEVICE_DECLARED
#ifdef XWR_SYNTAX_CHECK
extern "C" HRESULT WINAPI D3D12CreateDevice(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**);
#endif
#endif

namespace xwr {

extern "C" HRESULT __stdcall Shim_D3D12CreateDevice(IUnknown* pAdapter,
                                                      D3D_FEATURE_LEVEL MinimumFeatureLevel,
                                                      const IID& riid,
                                                      void** ppDevice) {
    // Clamp the requested minimum feature level to 11_0.
    static const int XWR_D3D_FL_11_0 = 0xB000;
    int requested = (int)MinimumFeatureLevel;
    D3D_FEATURE_LEVEL clamped = (requested > XWR_D3D_FL_11_0)
                                  ? (D3D_FEATURE_LEVEL)XWR_D3D_FL_11_0
                                  : MinimumFeatureLevel;
    return ::D3D12CreateDevice(pAdapter, clamped, riid, ppDevice);
}

extern "C" HRESULT __stdcall Shim_D3D12GetDebugInterface(const IID& riid, void** ppvDebug) {
    if (ppvDebug) *ppvDebug = nullptr;
    return E_NOTIMPL;
}

extern "C" HRESULT __stdcall Shim_D3D12SerializeRootSignature(const void* pRootSignature,
                                                                UINT Version, void** ppBlob,
                                                                void** ppErrorBlob) {
    (void)pRootSignature; (void)Version;
    if (ppBlob) *ppBlob = nullptr;
    if (ppErrorBlob) *ppErrorBlob = nullptr;
    return E_NOTIMPL;
}

extern "C" HRESULT __stdcall Shim_D3D12CreateVersionedRootSignatureDeserializer(const void* pBlob,
                                                                                  SIZE_T NumBytes,
                                                                                  const IID& riid,
                                                                                  void** ppDeserializer) {
    (void)pBlob; (void)NumBytes; (void)riid;
    if (ppDeserializer) *ppDeserializer = nullptr;
    return E_NOTIMPL;
}

}  // namespace xwr

// ===========================================================================
// Registration — under both "d3d12" and "d3d12.dll".
// ===========================================================================
REGISTER_SHIM("d3d12", "D3D12CreateDevice", (FARPROC)&xwr::Shim_D3D12CreateDevice);
REGISTER_SHIM("d3d12", "D3D12GetDebugInterface", (FARPROC)&xwr::Shim_D3D12GetDebugInterface);
REGISTER_SHIM("d3d12", "D3D12SerializeRootSignature", (FARPROC)&xwr::Shim_D3D12SerializeRootSignature);
REGISTER_SHIM("d3d12", "D3D12CreateVersionedRootSignatureDeserializer", (FARPROC)&xwr::Shim_D3D12CreateVersionedRootSignatureDeserializer);
REGISTER_SHIM("d3d12.dll", "D3D12CreateDevice", (FARPROC)&xwr::Shim_D3D12CreateDevice);
