// shims/pass-through/MiscPassthroughShim.cpp
// Pass-through shims for D2D1 / DWrite / WIC. These UWP APIs are all
// AppContainer-safe so we just forward to the real functions.
// (Crypt32 / BCrypt shims used to live here too, but are now implemented
// in shims/advapi32/Advapi32Shim.cpp — see note near end of file.)

#include "UwpSdkIncludes.h"

// ---------------------------------------------------------------------------
// MSVC: pull in the headers for the four factory families we forward to.
// <windows.h> does NOT include d2d1.h / dwrite.h / wincodec.h by default;
// wincrypt.h IS pulled in via the NOCRYPT-undef path, but re-including is
// a no-op (idempotent) and makes the dependency explicit.
// ---------------------------------------------------------------------------
#ifdef _MSC_VER
  #include <d2d1.h>
  #include <dwrite.h>
  #include <wincodec.h>
  #include <wincrypt.h>
  #pragma comment(lib, "d2d1.lib")
  #pragma comment(lib, "dwrite.lib")
  #pragma comment(lib, "windowscodecs.lib")
  #pragma comment(lib, "advapi32.lib")
#endif

#include "../ShimRegistry.h"

namespace xwr {

// ---------------------------------------------------------------------------
// d2d1
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_D2D1CreateFactory(int factoryType, const IID& riid,
                                                     const void* pFactoryOptions, void** ppIFactory) {
#ifdef _MSC_VER
    return ::D2D1CreateFactory((D2D1_FACTORY_TYPE)factoryType, riid,
                               (const D2D1_FACTORY_OPTIONS*)pFactoryOptions, ppIFactory);
#else
    return ::D2D1CreateFactory(factoryType, riid, pFactoryOptions, ppIFactory);
#endif
}

// ---------------------------------------------------------------------------
// dwrite
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_DWriteCreateFactory(int factoryType, const IID& iid,
                                                       IUnknown** factory) {
#ifdef _MSC_VER
    return ::DWriteCreateFactory((DWRITE_FACTORY_TYPE)factoryType, iid, factory);
#else
    return ::DWriteCreateFactory(factoryType, iid, factory);
#endif
}

// ---------------------------------------------------------------------------
// windowscodecs (WIC)
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_WICCreateImagingFactory(UINT SDKVersion, IWICImagingFactory** ppIImagingFactory) {
    // WICCreateImagingFactory is not exported by name in the modern Windows SDK;
    // create the factory via CoCreateInstance with CLSID_WICImagingFactory instead.
    (void)SDKVersion;
    if (!ppIImagingFactory) return E_POINTER;
    *ppIImagingFactory = nullptr;
#ifdef _MSC_VER
    return CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                            IID_PPV_ARGS(ppIImagingFactory));
#else
    // Linux stub: declare the GUIDs locally — they're not in the stub header.
    static const GUID _xwr_CLSID_WICImagingFactory = {
        0xcaf5f5fa, 0x7a11, 0x4b9c, {0x8a, 0x05, 0x66, 0x7d, 0xfe, 0xb3, 0xa9, 0x7c}};
    static const GUID _xwr_IID_IWICImagingFactory = {
        0xec5ec8a9, 0xc395, 0x4314, {0x9c, 0x77, 0x54, 0xd7, 0xa9, 0x35, 0xa7, 0xad}};
    return CoCreateInstance(_xwr_CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                            _xwr_IID_IWICImagingFactory, (void**)ppIImagingFactory);
#endif
}
extern "C" HRESULT __stdcall Shim_WICConvertBitmapSource(const GUID& dstFormat,
                                                          IWICBitmapSource* pISrc,
                                                          IWICBitmapSource** ppIDst) {
    return ::WICConvertBitmapSource(dstFormat, pISrc, ppIDst);
}

// NOTE: crypt32 / bcrypt shims (CryptAcquireContextW, CryptGenRandom,
// BCryptOpenAlgorithmProvider, BCryptGenRandom, BCryptCloseAlgorithmProvider)
// are implemented in shims/advapi32/Advapi32Shim.cpp. They were previously
// also defined here, causing LNK2005 duplicate-symbol errors at link time.

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("d2d1", "D2D1CreateFactory", (FARPROC)&xwr::Shim_D2D1CreateFactory);
REGISTER_SHIM("d2d1.dll", "D2D1CreateFactory", (FARPROC)&xwr::Shim_D2D1CreateFactory);

REGISTER_SHIM("dwrite", "DWriteCreateFactory", (FARPROC)&xwr::Shim_DWriteCreateFactory);
REGISTER_SHIM("dwrite.dll", "DWriteCreateFactory", (FARPROC)&xwr::Shim_DWriteCreateFactory);

REGISTER_SHIM("windowscodecs", "WICCreateImagingFactory", (FARPROC)&xwr::Shim_WICCreateImagingFactory);
REGISTER_SHIM("windowscodecs", "WICConvertBitmapSource", (FARPROC)&xwr::Shim_WICConvertBitmapSource);

// NOTE: crypt32 / bcrypt registrations intentionally omitted here — see note
// above. Those symbols are registered from shims/advapi32/Advapi32Shim.cpp.
