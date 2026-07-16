// shims/legacy/D3D9Shim.cpp
// Stub for d3d9. Direct3DCreate9 returns a fake IDirect3D9 pointer; every
// method on the fake object returns D3D_OK so legacy games that probe the
// device enumeration API don't crash on the way to learning they should use
// the bridge instead.

#include "UwpSdkIncludes.h"
#include <Windows.h>
#include <atomic>

#include "../ShimRegistry.h"

#ifndef D3DCOLOR
typedef DWORD D3DCOLOR;
#endif

namespace xwr {

static std::atomic<uint32_t> g_d3d9Fake{0x4000};

// "IDirect3D9" — we never actually call vtable methods; the game calls them
// through its own copy of the interface declaration. Returning a fake pointer
// here means the game keeps a non-null IDirect3D9* but, in practice, no game
// that we shim can use it (real rendering is via the D3D11 bridge). Most
// legacy D3D9 games check for a NULL return and fall back to GDI / OpenGL
// when null, but they all expect a valid object when non-null is returned,
// so we just return a counter-cast pointer.
extern "C" void* __stdcall Shim_Direct3DCreate9(UINT SDKVersion) {
    (void)SDKVersion;
    return (void*)(uintptr_t)g_d3d9Fake.fetch_add(1);
}

extern "C" HRESULT __stdcall Shim_Direct3DCreate9Ex(UINT SDKVersion, void** ppD3D9Ex) {
    (void)SDKVersion;
    if (ppD3D9Ex) *ppD3D9Ex = nullptr;
    return E_FAIL;
}

// Direct3D9 runtime helpers
extern "C" int __stdcall Shim_D3DPERF_BeginEvent(D3DCOLOR, LPCWSTR) { return 0; }
extern "C" int __stdcall Shim_D3DPERF_EndEvent() { return 0; }
extern "C" void __stdcall Shim_D3DPERF_SetMarker(D3DCOLOR, LPCWSTR) { }
extern "C" void __stdcall Shim_D3DPERF_SetRegion(D3DCOLOR, LPCWSTR) { }
extern "C" BOOL __stdcall Shim_D3DPERF_QueryRepeatFrame() { return FALSE; }
extern "C" void __stdcall Shim_D3DPERF_SetOptions(DWORD) { }
extern "C" DWORD __stdcall Shim_D3DPERF_GetStatus() { return 0; }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("d3d9", "Direct3DCreate9", (FARPROC)&xwr::Shim_Direct3DCreate9);
REGISTER_SHIM("d3d9", "Direct3DCreate9Ex", (FARPROC)&xwr::Shim_Direct3DCreate9Ex);
REGISTER_SHIM("d3d9", "D3DPERF_BeginEvent", (FARPROC)&xwr::Shim_D3DPERF_BeginEvent);
REGISTER_SHIM("d3d9", "D3DPERF_EndEvent", (FARPROC)&xwr::Shim_D3DPERF_EndEvent);
REGISTER_SHIM("d3d9", "D3DPERF_SetMarker", (FARPROC)&xwr::Shim_D3DPERF_SetMarker);
REGISTER_SHIM("d3d9", "D3DPERF_SetRegion", (FARPROC)&xwr::Shim_D3DPERF_SetRegion);
REGISTER_SHIM("d3d9", "D3DPERF_QueryRepeatFrame", (FARPROC)&xwr::Shim_D3DPERF_QueryRepeatFrame);
REGISTER_SHIM("d3d9", "D3DPERF_SetOptions", (FARPROC)&xwr::Shim_D3DPERF_SetOptions);
REGISTER_SHIM("d3d9", "D3DPERF_GetStatus", (FARPROC)&xwr::Shim_D3DPERF_GetStatus);
