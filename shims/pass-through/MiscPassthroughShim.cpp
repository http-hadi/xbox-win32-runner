// shims/pass-through/MiscPassthroughShim.cpp
// Pass-through shims for D2D1 / DWrite / WIC / Crypt32 / BCrypt. These UWP
// APIs are all AppContainer-safe so we just forward to the real functions.

#include "UwpSdkIncludes.h"


#include "../ShimRegistry.h"

namespace xwr {

// ---------------------------------------------------------------------------
// d2d1
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_D2D1CreateFactory(int factoryType, const IID& riid,
                                                     const void* pFactoryOptions, void** ppIFactory) {
    return ::D2D1CreateFactory(factoryType, riid, pFactoryOptions, ppIFactory);
}

// ---------------------------------------------------------------------------
// dwrite
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_DWriteCreateFactory(int factoryType, const IID& iid,
                                                       IUnknown** factory) {
    return ::DWriteCreateFactory(factoryType, iid, factory);
}

// ---------------------------------------------------------------------------
// windowscodecs (WIC)
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_WICCreateImagingFactory(UINT SDKVersion, IWICImagingFactory** ppIImagingFactory) {
    return ::WICCreateImagingFactory(SDKVersion, ppIImagingFactory);
}
extern "C" HRESULT __stdcall Shim_WICConvertBitmapSource(const GUID& dstFormat,
                                                          IWICBitmapSource* pISrc,
                                                          IWICBitmapSource** ppIDst) {
    return ::WICConvertBitmapSource(dstFormat, pISrc, ppIDst);
}

// ---------------------------------------------------------------------------
// crypt32
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_CryptAcquireContextW(HCRYPTPROV* phProv, LPCWSTR szContainer,
                                                     LPCWSTR szProvider, DWORD dwProvType, DWORD dwFlags) {
    return ::CryptAcquireContextW(phProv, szContainer, szProvider, dwProvType, dwFlags);
}
extern "C" BOOL __stdcall Shim_CryptGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE* pbBuffer) {
    return ::CryptGenRandom(hProv, dwLen, pbBuffer);
}

// ---------------------------------------------------------------------------
// bcrypt — pass through (forward declarations live in Windows.h)
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_BCryptOpenAlgorithmProvider(void** phAlgorithm, LPCWSTR pszAlgId,
                                                            LPCWSTR pszImplementation, ULONG dwFlags) {
    return ::BCryptOpenAlgorithmProvider(phAlgorithm, pszAlgId, pszImplementation, dwFlags);
}
extern "C" BOOL __stdcall Shim_BCryptGenRandom(void* hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer,
                                                ULONG dwFlags) {
    return ::BCryptGenRandom(hAlgorithm, pbBuffer, cbBuffer, dwFlags);
}
extern "C" BOOL __stdcall Shim_BCryptCloseAlgorithmProvider(void* hAlgorithm, ULONG dwFlags) {
    return ::BCryptCloseAlgorithmProvider(hAlgorithm, dwFlags);
}

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

REGISTER_SHIM("crypt32", "CryptAcquireContextW", (FARPROC)&xwr::Shim_CryptAcquireContextW);
REGISTER_SHIM("crypt32", "CryptGenRandom", (FARPROC)&xwr::Shim_CryptGenRandom);

REGISTER_SHIM("bcrypt", "BCryptOpenAlgorithmProvider", (FARPROC)&xwr::Shim_BCryptOpenAlgorithmProvider);
REGISTER_SHIM("bcrypt", "BCryptGenRandom", (FARPROC)&xwr::Shim_BCryptGenRandom);
REGISTER_SHIM("bcrypt", "BCryptCloseAlgorithmProvider", (FARPROC)&xwr::Shim_BCryptCloseAlgorithmProvider);
