// shims/stubs/DirectSoundShim.cpp
// Stub for dsound. DirectSoundCreate returns DSERR_NODRIVER (no audio
// device available — UWP doesn't expose legacy DirectSound).

#include "UwpSdkIncludes.h"
#include <Windows.h>

#include "../ShimRegistry.h"

namespace xwr {

extern "C" HRESULT __stdcall Shim_DirectSoundCreate(const GUID* pcGuidDevice, void** ppDS, void* pUnkOuter) {
    (void)pcGuidDevice; (void)pUnkOuter;
    if (ppDS) *ppDS = nullptr;
    return DSERR_NODRIVER;
}
extern "C" HRESULT __stdcall Shim_DirectSoundCreate8(const GUID* pcGuidDevice, void** ppDS, void* pUnkOuter) {
    (void)pcGuidDevice; (void)pUnkOuter;
    if (ppDS) *ppDS = nullptr;
    return DSERR_NODRIVER;
}
extern "C" HRESULT __stdcall Shim_DirectSoundEnumerateW(void* pDSEnumCallback, void* pContext) {
    (void)pDSEnumCallback; (void)pContext;
    return DS_OK;
}
extern "C" HRESULT __stdcall Shim_DirectSoundEnumerateA(void* pDSEnumCallback, void* pContext) {
    (void)pDSEnumCallback; (void)pContext;
    return DS_OK;
}
extern "C" HRESULT __stdcall Shim_DirectSoundCaptureCreate(const GUID* pcGuidDevice, void** ppDSC, void* pUnkOuter) {
    (void)pcGuidDevice; (void)pUnkOuter;
    if (ppDSC) *ppDSC = nullptr;
    return DSERR_NODRIVER;
}
extern "C" HRESULT __stdcall Shim_DirectSoundCaptureCreate8(const GUID* pcGuidDevice, void** ppDSC, void* pUnkOuter) {
    (void)pcGuidDevice; (void)pUnkOuter;
    if (ppDSC) *ppDSC = nullptr;
    return DSERR_NODRIVER;
}
extern "C" HRESULT __stdcall Shim_DirectSoundCaptureEnumerateW(void* pDSEnumCallback, void* pContext) {
    (void)pDSEnumCallback; (void)pContext;
    return DS_OK;
}
extern "C" HRESULT __stdcall Shim_DirectSoundFullDuplexCreate(const GUID*, const GUID*, const void*, const void*, HWND, DWORD, void**, void**, void*) {
    return DSERR_NODRIVER;
}
extern "C" HRESULT __stdcall Shim_DirectSoundGetSpeakerConfig(LPDWORD pdwSpeakerConfig) {
    if (pdwSpeakerConfig) *pdwSpeakerConfig = 0x00000001;  // DSSPEAKER_STEREO
    return DS_OK;
}
extern "C" HRESULT __stdcall Shim_DirectSoundSetSpeakerConfig(DWORD) { return DS_OK; }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("dsound", "DirectSoundCreate", (FARPROC)&xwr::Shim_DirectSoundCreate);
REGISTER_SHIM("dsound", "DirectSoundCreate8", (FARPROC)&xwr::Shim_DirectSoundCreate8);
REGISTER_SHIM("dsound", "DirectSoundEnumerateW", (FARPROC)&xwr::Shim_DirectSoundEnumerateW);
REGISTER_SHIM("dsound", "DirectSoundEnumerateA", (FARPROC)&xwr::Shim_DirectSoundEnumerateA);
REGISTER_SHIM("dsound", "DirectSoundCaptureCreate", (FARPROC)&xwr::Shim_DirectSoundCaptureCreate);
REGISTER_SHIM("dsound", "DirectSoundCaptureCreate8", (FARPROC)&xwr::Shim_DirectSoundCaptureCreate8);
REGISTER_SHIM("dsound", "DirectSoundCaptureEnumerateW", (FARPROC)&xwr::Shim_DirectSoundCaptureEnumerateW);
REGISTER_SHIM("dsound", "DirectSoundFullDuplexCreate", (FARPROC)&xwr::Shim_DirectSoundFullDuplexCreate);
REGISTER_SHIM("dsound", "DirectSoundGetSpeakerConfig", (FARPROC)&xwr::Shim_DirectSoundGetSpeakerConfig);
REGISTER_SHIM("dsound", "DirectSoundSetSpeakerConfig", (FARPROC)&xwr::Shim_DirectSoundSetSpeakerConfig);
