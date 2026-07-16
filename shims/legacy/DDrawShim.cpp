// shims/legacy/DDrawShim.cpp
// Stub for ddraw. DirectDrawCreate / DirectDrawCreateEx return fake
// IDirectDraw / IDirectDraw7 pointers so legacy games that probe for
// ddraw during init don't crash. No rendering is performed.

#include "UwpSdkIncludes.h"
#include <Windows.h>
#include <atomic>

#include "../ShimRegistry.h"

#ifndef DD_OK
#define DD_OK ((HRESULT)0L)
#endif
#ifndef DDERR_INVALIDDIRECTDRAWGUID
#define DDERR_INVALIDDIRECTDRAWGUID ((HRESULT)MAKE_DDRAW_HRESULT(0x878))
#endif
#ifndef MAKE_DDRAW_HRESULT
#define MAKE_DDRAW_HRESULT(code) ((HRESULT)((unsigned long)(0x878) << 16 | (unsigned long)(code)))
#endif

namespace xwr {

static std::atomic<uint32_t> g_ddrawFake{0x6000};

extern "C" HRESULT __stdcall Shim_DirectDrawCreate(const GUID* lpGUID, void** lplpDD, IUnknown* pUnkOuter) {
    (void)lpGUID; (void)pUnkOuter;
    if (!lplpDD) return DDERR_INVALIDDIRECTDRAWGUID;
    *lplpDD = (void*)(uintptr_t)g_ddrawFake.fetch_add(1);
    return DD_OK;
}
extern "C" HRESULT __stdcall Shim_DirectDrawCreateEx(const GUID* lpGUID, void** lplpDD, const IID& iid, IUnknown* pUnkOuter) {
    (void)lpGUID; (void)iid; (void)pUnkOuter;
    if (!lplpDD) return DDERR_INVALIDDIRECTDRAWGUID;
    *lplpDD = (void*)(uintptr_t)g_ddrawFake.fetch_add(1);
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_DirectDrawEnumerateExW(void* lpCallback, void* lpContext, DWORD dwFlags) {
    (void)lpCallback; (void)lpContext; (void)dwFlags;
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_DirectDrawEnumerateExA(void* lpCallback, void* lpContext, DWORD dwFlags) {
    (void)lpCallback; (void)lpContext; (void)dwFlags;
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_DirectDrawEnumerateW(void* lpCallback, void* lpContext) {
    (void)lpCallback; (void)lpContext;
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_DirectDrawEnumerateA(void* lpCallback, void* lpContext) {
    (void)lpCallback; (void)lpContext;
    return S_OK;
}

}  // namespace xwr

REGISTER_SHIM("ddraw", "DirectDrawCreate", (FARPROC)&xwr::Shim_DirectDrawCreate);
REGISTER_SHIM("ddraw", "DirectDrawCreateEx", (FARPROC)&xwr::Shim_DirectDrawCreateEx);
REGISTER_SHIM("ddraw", "DirectDrawEnumerateExW", (FARPROC)&xwr::Shim_DirectDrawEnumerateExW);
REGISTER_SHIM("ddraw", "DirectDrawEnumerateExA", (FARPROC)&xwr::Shim_DirectDrawEnumerateExA);
REGISTER_SHIM("ddraw", "DirectDrawEnumerateW", (FARPROC)&xwr::Shim_DirectDrawEnumerateW);
REGISTER_SHIM("ddraw", "DirectDrawEnumerateA", (FARPROC)&xwr::Shim_DirectDrawEnumerateA);
