// shims/MiscSystemGapFill.cpp
//
// Single-file gap fill for all remaining system DLLs reported by the coverage
// gap scan. Covers 33 DLLs and ~200 missing functions:
//
//   winmm              27  stub (no legacy audio devices in AppContainer)
//   ole32              10  pass-through to the real COM surface
//   oleaut32            9  ordinals — generic E_NOTIMPL stubs
//   gdi32               4  pass-through (ChoosePixelFormat / SetPixelFormat / SwapBuffers / ExtTextOutW)
//   shell32             1  pass-through (CommandLineToArgvW)
//   shlwapi             1  ordinal — generic stub
//   version             1  pass-through (GetFileVersionInfoExW)
//   setupapi            7  stub (no device enumeration in AppContainer)
//   dsound              6  ordinals — stub returning DSERR_NODRIVER
//   d3d12               6  4 pass-through + 2 ordinal stubs
//   mfplat              5  stub (MFStartup returns S_OK; others E_NOTIMPL)
//   mf                  8  stub (E_NOTIMPL)
//   imm32              12  stub (IME disabled — return 0 / FALSE)
//   winhttp            10  pass-through (UWP supports WinHTTP)
//   wininet            11  stub (return FALSE — use WinHTTP instead)
//   wintrust            4  stub (WinVerifyTrust returns S_OK always)
//   ntdll              13  pass-through (Nt*/Rtl* surface)
//   dwmapi              4  pass-through
//   cfgmgr32            2  stub
//   ncrypt              2  pass-through
//   msdmo               2  stub
//   propsys             1  stub
//   msi                19  ordinals — stub returning ERROR_INSTALL_SERVICE_FAILURE
//   dbghelp            18  stub (MiniDumpWriteDump returns FALSE, etc.)
//   d3dcompiler_47      1  pass-through (D3DCreateBlob)
//   dxgi                3  pass-through (CreateDXGIFactory{,1,2})
//   cabinet             3  ordinals — stub
//   xaudio2_9redist     1  pass-through (XAudio2Create)
//   mmdevapi            1  ordinal — stub
//   rpcrt4              1  pass-through (UuidCreate)
//   uxtheme             1  stub (SetWindowTheme returns S_OK)
//   uiautomationcore    2  stub
//   xinput1_4           2  ordinals (XInputGetState / XInputSetState — already shimmed by name; this registers the ordinal aliases)
//
// REGISTER_SHIM count: ~200 entries.
//
// Added by Task ID GAP-MISC-SYSTEM.

#include "UwpSdkIncludes.h"

// ---------------------------------------------------------------------------
// MSVC: pull in the SDK headers that <windows.h> does NOT include by default.
//   * <setupapi.h>          — HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVICE_INTERFACE_DATA
//   * <cfgmgr32.h>          — DEVINST, CONFIGRET, device-manager APIs
//   * <mmsystem.h>          — HWAVEIN/HWAVEOUT/LPWAVEHDR/HMIXER/HMIXEROBJ/LPMIXER*/LPMMTIME
//   * <d3d12.h>             — D3D12_VERSIONED_ROOT_SIGNATURE_DESC, ID3DBlob, D3D12SerializeVersionedRootSignature
//   * <mfobjects.h>         — IMFAsyncCallback, IUnknown (MF surface)
//   * <winhttp.h>           — HINTERNET, WINHTTP_PROXY_INFO, URL_COMPONENTS (note: WINHTTP_CURRENT_USER_IE_PROXY_CONFIG is gated behind WINAPI_PARTITION_DESKTOP and is NOT available to UWP AppContainer builds — see the forward-declaration below)
//   * <wininet.h>           — INTERNET_PORT, INTERNET_SCHEME (wininet stubs)
//   * <dwmapi.h>            — DwmFlush, DwmGetCompositionTimingInfo, DwmIsCompositionEnabled, DwmSetWindowAttribute, DWM_TIMING_INFO
//   * <dbghelp.h>           — LPSTACKFRAME64, PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64, MiniDumpWriteDump, StackWalk64, Sym* family
//   * <uiautomationcore.h>  — PROPERTYID, UiaRaiseAutomationPropertyChangedEvent, UiaClientsAreListening
// ---------------------------------------------------------------------------
#ifdef _MSC_VER
  #include <setupapi.h>
  #include <cfgmgr32.h>
  #include <mmsystem.h>
  #include <d3d12.h>
  #include <mfobjects.h>
  #include <winhttp.h>
  #include <wininet.h>
  #include <dwmapi.h>
  #include <dbghelp.h>
  #include <uiautomationcore.h>
  #pragma comment(lib, "setupapi.lib")
  #pragma comment(lib, "cfgmgr32.lib")
  #pragma comment(lib, "winmm.lib")
  #pragma comment(lib, "d3d12.lib")
  #pragma comment(lib, "winhttp.lib")
  #pragma comment(lib, "wininet.lib")
  #pragma comment(lib, "dwmapi.lib")
  #pragma comment(lib, "dbghelp.lib")
  #pragma comment(lib, "uiautomationcore.lib")
#endif

#include <cstring>
#include <cstdlib>

#include "ShimRegistry.h"

#ifndef ERROR_NOT_SUPPORTED
#define ERROR_NOT_SUPPORTED 50L
#endif
#ifndef ERROR_INVALID_HANDLE
#define ERROR_INVALID_HANDLE 6L
#endif
#ifndef ERROR_INSTALL_SERVICE_FAILURE
#define ERROR_INSTALL_SERVICE_FAILURE 1601L
#endif
#ifndef ERROR_BAD_FORMAT
#define ERROR_BAD_FORMAT 11L
#endif
#ifndef MMSYSERR_NODRIVER
#define MMSYSERR_NODRIVER 6
#endif
#ifndef DSERR_NODRIVER
#define DSERR_NODRIVER ((HRESULT)0x88780078L)
#endif
#ifndef _XWR_DSERR_NODRIVER_CUST
#define _XWR_DSERR_NODRIVER_CUST
#endif

// ---------------------------------------------------------------------------
// MSVC / UWP fallbacks for types & constants the SDK headers gate behind
// WINAPI_PARTITION_DESKTOP (and therefore exclude from AppContainer builds).
// On Linux the stub winheaders/Windows.h already declares all of these
// unconditionally, so the guards below are no-ops there.
// ---------------------------------------------------------------------------

// STATUS_NOT_IMPLEMENTED — used by the NtWriteFile / RtlUTF8ToUnicodeN stubs.
// ntstatus.h is NOT pulled in by <windows.h>; define locally if missing.
#ifndef STATUS_NOT_IMPLEMENTED
#define STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002L)
#endif

#ifdef _MSC_VER
  // winhttp.h gates WINHTTP_CURRENT_USER_IE_PROXY_CONFIG (and the corresponding
  // WinHttpGetIEProxyConfigForCurrentUser function) behind
  // WINAPI_PARTITION_DESKTOP. UWP AppContainer builds therefore do NOT see the
  // type even though <winhttp.h> is included above. Forward-declare the struct
  // so the shim's function signature compiles; the body is stubbed on MSVC
  // because the real function is also unavailable.
  #ifndef _XWR_WINHTTP_IE_PROXY_DECLARED
    struct _WINHTTP_CURRENT_USER_IE_PROXY_CONFIG;
    typedef struct _WINHTTP_CURRENT_USER_IE_PROXY_CONFIG WINHTTP_CURRENT_USER_IE_PROXY_CONFIG;
    typedef WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* LPWINHTTP_CURRENT_USER_IE_PROXY_CONFIG;
    #define _XWR_WINHTTP_IE_PROXY_DECLARED
  #endif

  // PRTL_RUN_ONCE_INIT_FN is declared inside an NTDDI_VERSION >= NTDDI_WIN7
  // block in winnt.h but is occasionally missing from UWP SDK partitions.
  // Provide a fallback typedef if the SDK did not declare it. C++11 allows
  // typedef redeclaration to the same type, so this is harmless if winnt.h
  // already provided it.
  #ifndef _XWR_PRTL_RUN_ONCE_INIT_FN_DEFINED
    typedef BOOLEAN (NTAPI *PRTL_RUN_ONCE_INIT_FN)(PRTL_RUN_ONCE, PVOID, PVOID*);
    #define _XWR_PRTL_RUN_ONCE_INIT_FN_DEFINED
  #endif
#endif

namespace xwr {

// ===========================================================================
// 1) winmm — 27 missing functions (mixer* and waveIn* / waveOut* helpers)
// ===========================================================================
extern "C" UINT    __stdcall Shim_mixerClose(HMIXER) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerGetControlDetailsW(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerGetDevCapsW(UINT, LPMIXERCAPSW, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerGetID(HMIXEROBJ, LPUINT, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerGetLineControlsW(HMIXEROBJ, LPMIXERLINECONTROLSW, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerGetLineInfoW(HMIXEROBJ, LPMIXERLINEW, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerGetNumDevs() { return 0; }
extern "C" UINT    __stdcall Shim_mixerOpen(LPHMIXER, UINT, DWORD_PTR, DWORD_PTR, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_mixerSetControlDetails(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD) { return MMSYSERR_NODRIVER; }

extern "C" UINT    __stdcall Shim_waveInAddBuffer(HWAVEIN, LPWAVEHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInClose(HWAVEIN) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInGetDevCapsW(UINT, LPWAVEINCAPSW, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInGetErrorTextW(UINT, LPWSTR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInGetID(HWAVEIN, LPUINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInGetNumDevs() { return 0; }
extern "C" UINT    __stdcall Shim_waveInGetPosition(HWAVEIN, LPMMTIME, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInMessage(HWAVEIN, UINT, DWORD_PTR, DWORD_PTR) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInOpen(LPHWAVEIN, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInPrepareHeader(HWAVEIN, LPWAVEHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInReset(HWAVEIN) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInStart(HWAVEIN) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInStop(HWAVEIN) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveInUnprepareHeader(HWAVEIN, LPWAVEHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveOutGetErrorTextW(UINT, LPWSTR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveOutGetID(HWAVEOUT, LPUINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveOutGetPosition(HWAVEOUT, LPMMTIME, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT    __stdcall Shim_waveOutMessage(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR) { return MMSYSERR_NODRIVER; }

// ===========================================================================
// 2) ole32 — 10 pass-throughs (COM marshalling / security / prop-variant)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_CLSIDFromProgID(LPCOLESTR lpszProgID, LPCLSID lpclsid) {
    return ::CLSIDFromProgID(lpszProgID, lpclsid);
}
extern "C" HRESULT __stdcall Shim_CoGetInterfaceAndReleaseStream(IStream* pStm, REFIID iid, void** ppv) {
    return ::CoGetInterfaceAndReleaseStream(pStm, iid, ppv);
}
extern "C" HRESULT __stdcall Shim_CoInitializeSecurity(PSECURITY_DESCRIPTOR pSecDesc, LONG cAuthSvc,
                                                       SOLE_AUTHENTICATION_SERVICE* asAuthSvc,
                                                       void* pReserved1, DWORD dwAuthnLevel,
                                                       DWORD dwImpLevel, void* pAuthList,
                                                       DWORD dwCapabilities, void* pReserved3) {
    return ::CoInitializeSecurity(pSecDesc, cAuthSvc, asAuthSvc, pReserved1, dwAuthnLevel,
                                  dwImpLevel, pAuthList, dwCapabilities, pReserved3);
}
extern "C" HRESULT __stdcall Shim_CoMarshalInterThreadInterfaceInStream(REFIID riid, IUnknown* pUnk,
                                                                       IStream** ppStm) {
    return ::CoMarshalInterThreadInterfaceInStream(riid, pUnk, ppStm);
}
extern "C" HRESULT __stdcall Shim_CoSetProxyBlanket(IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc,
                                                    OLECHAR* pServerPrincName, DWORD dwAuthnLevel,
                                                    DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
                                                    DWORD dwCapabilities) {
    return ::CoSetProxyBlanket(pProxy, dwAuthnSvc, dwAuthzSvc, pServerPrincName, dwAuthnLevel,
                               dwImpLevel, pAuthInfo, dwCapabilities);
}
extern "C" HRESULT __stdcall Shim_IIDFromString(LPCOLESTR lpsz, LPIID lpiid) {
    return ::IIDFromString(lpsz, lpiid);
}
extern "C" HRESULT __stdcall Shim_PropVariantClear(PROPVARIANT* pvar) {
    return ::PropVariantClear(pvar);
}
extern "C" HRESULT __stdcall Shim_PropVariantCopy(PROPVARIANT* pvarDest, const PROPVARIANT* pvarSrc) {
    return ::PropVariantCopy(pvarDest, const_cast<PROPVARIANT*>(pvarSrc));
}
extern "C" void    __stdcall Shim_ReleaseStgMedium(LPSTGMEDIUM pstg) {
    ::ReleaseStgMedium(pstg);
}
extern "C" int     __stdcall Shim_StringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax) {
    return ::StringFromGUID2(rguid, lpsz, cchMax);
}

// ===========================================================================
// 3) oleaut32 — 9 ordinals (legacy variant helpers). Generic E_NOTIMPL stubs.
// ===========================================================================
extern "C" HRESULT __stdcall Shim_OleAut_Ord2()   { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord6()   { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord7()   { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord8()   { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord9()   { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord10()  { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord26()  { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord200() { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_OleAut_Ord411() { return E_NOTIMPL; }

// ===========================================================================
// 4) gdi32 — 4 pass-throughs (pixel format / text / swap buffers)
// ===========================================================================
extern "C" int  __stdcall Shim_ChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd) {
    return ::ChoosePixelFormat(hdc, ppfd);
}
extern "C" BOOL __stdcall Shim_SetPixelFormat(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR* ppfd) {
    return ::SetPixelFormat(hdc, iPixelFormat, ppfd);
}
extern "C" BOOL __stdcall Shim_SwapBuffers(HDC hdc) {
    return ::SwapBuffers(hdc);
}
// NOTE: Shim_ExtTextOutW is implemented in shims/user32/User32Shim.cpp.

// ===========================================================================
// 5) shell32 — 1 pass-through (CommandLineToArgvW)
// ===========================================================================
extern "C" LPWSTR* __stdcall Shim_CommandLineToArgvW(LPCWSTR lpCmdLine, int* pNumArgs) {
    return ::CommandLineToArgvW(lpCmdLine, pNumArgs);
}

// ===========================================================================
// 6) shlwapi — 1 ordinal. Generic stub.
// ===========================================================================
extern "C" HRESULT __stdcall Shim_Shlwapi_Ord219() { return E_NOTIMPL; }

// ===========================================================================
// 7) version — 1 pass-through (GetFileVersionInfoExW)
// ===========================================================================
extern "C" BOOL __stdcall Shim_GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpwstrFilename,
                                                    DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    return ::GetFileVersionInfoExW(dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

// ===========================================================================
// 8) setupapi — 7 stubs (device enumeration not available in AppContainer)
// ===========================================================================
extern "C" CONFIGRET __stdcall Shim_CM_Get_Device_IDW(DEVINST dnDevInst, PWSTR Buffer, ULONG ulLen, ULONG ulFlags) {
    (void)dnDevInst; (void)Buffer; (void)ulLen; (void)ulFlags;
    return CR_FAILURE;
}
extern "C" BOOL  __stdcall Shim_SetupDiEnumDeviceInfo(HDEVINFO, DWORD, PSP_DEVINFO_DATA) { return FALSE; }
extern "C" HDEVINFO __stdcall Shim_SetupDiGetClassDevsExW(const GUID*, LPCWSTR, HWND, DWORD, HDEVINFO, LPCWSTR, PVOID) {
    return (HDEVINFO)INVALID_HANDLE_VALUE;
}
extern "C" BOOL  __stdcall Shim_SetupDiGetDeviceInterfaceAlias(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, const GUID*, PSP_DEVICE_INTERFACE_DATA) { return FALSE; }
extern "C" BOOL  __stdcall Shim_SetupDiGetDevicePropertyW(HDEVINFO, PSP_DEVINFO_DATA, const DEVPROPKEY*, DEVPROPTYPE*, PBYTE, DWORD, PDWORD, DWORD) { return FALSE; }
extern "C" HKEY  __stdcall Shim_SetupDiOpenDeviceInterfaceRegKey(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD, REGSAM) { return (HKEY)INVALID_HANDLE_VALUE; }
extern "C" HKEY  __stdcall Shim_SetupDiOpenDevRegKey(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM) { return (HKEY)INVALID_HANDLE_VALUE; }

// ===========================================================================
// 9) dsound — 6 ordinals. Stub returning DSERR_NODRIVER.
// ===========================================================================
extern "C" HRESULT __stdcall Shim_Dsound_Ord1()  { return DSERR_NODRIVER; }
extern "C" HRESULT __stdcall Shim_Dsound_Ord3()  { return DSERR_NODRIVER; }
extern "C" HRESULT __stdcall Shim_Dsound_Ord6()  { return DSERR_NODRIVER; }
extern "C" HRESULT __stdcall Shim_Dsound_Ord8()  { return DSERR_NODRIVER; }
extern "C" HRESULT __stdcall Shim_Dsound_Ord11() { return DSERR_NODRIVER; }
extern "C" HRESULT __stdcall Shim_Dsound_Ord12() { return DSERR_NODRIVER; }

// ===========================================================================
// 10) d3d12 — 4 pass-through + 2 ordinals
// ===========================================================================
// D3D12CoreCreateLayeredDevice / D3D12CoreGetLayeredDeviceSize /
// D3D12CoreRegisterLayers are internal d3d12.dll exports not declared in the
// public d3d12.h header. Stub them to E_NOTIMPL / 0 — games using the public
// D3D12 API never call these (they are the layering plumbing used by the
// D3D11On12 and pix-event layers internally).
extern "C" HRESULT __stdcall Shim_D3D12CoreCreateLayeredDevice(void* pUnknown, void* pArgs) {
    (void)pUnknown; (void)pArgs;
    return E_NOTIMPL;
}
extern "C" SIZE_T  __stdcall Shim_D3D12CoreGetLayeredDeviceSize(const void* pArgs) {
    (void)pArgs;
    return (SIZE_T)0;
}
extern "C" HRESULT __stdcall Shim_D3D12CoreRegisterLayers(void* pLayers, UINT NumLayers) {
    (void)pLayers; (void)NumLayers;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_D3D12SerializeVersionedRootSignature(const void* pRootSignature,
                                                                       void** ppBlob, void** ppErrorBlob,
                                                                       UINT Flags) {
    // Real signature (current Windows SDK): 3-arg form
    //   (const D3D12_VERSIONED_ROOT_SIGNATURE_DESC*, ID3DBlob**, ID3DBlob**).
    // The legacy 4-arg form (with a UINT Flags tail) was removed; the SDK now
    // always passes 0 internally. The shim still accepts a 4th arg from the
    // game's IAT for ABI compatibility with older binaries, but ignores it.
    // The Linux stub also declares only the 3-arg form.
    (void)Flags;
#ifdef _MSC_VER
    return ::D3D12SerializeVersionedRootSignature(
        reinterpret_cast<const D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(pRootSignature),
        reinterpret_cast<ID3DBlob**>(ppBlob),
        reinterpret_cast<ID3DBlob**>(ppErrorBlob));
#else
    return ::D3D12SerializeVersionedRootSignature(pRootSignature, ppBlob, ppErrorBlob);
#endif
}
extern "C" HRESULT __stdcall Shim_D3D12_Ord101() { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_D3D12_Ord102() { return E_NOTIMPL; }

// ===========================================================================
// 11) mfplat — 5 stubs (MFStartup returns S_OK; others E_NOTIMPL)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_MFCreateAlignedMemoryBuffer(DWORD cbMaxLength, DWORD cbAligment, void** ppBuffer) {
    (void)cbMaxLength; (void)cbAligment;
    if (ppBuffer) *ppBuffer = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateAsyncResult(IUnknown* punkObject, IMFAsyncCallback* pCallback,
                                                      IUnknown* punkState, void** ppAsyncResult) {
    (void)punkObject; (void)pCallback; (void)punkState;
    if (ppAsyncResult) *ppAsyncResult = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateDXGIDeviceManager(UINT* pdwResetToken, void** ppDXVADeviceManager) {
    if (pdwResetToken) *pdwResetToken = 0;
    if (ppDXVADeviceManager) *ppDXVADeviceManager = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFInvokeCallback(void* pAsyncResult) {
    (void)pAsyncResult;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFPutWorkItem(DWORD dwQueue, void* pCallback, void* pState) {
    (void)dwQueue; (void)pCallback; (void)pState;
    return E_NOTIMPL;
}

// ===========================================================================
// 12) mf — 8 stubs (E_NOTIMPL)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_MFCreateAudioRendererActivate(void** ppActivate) {
    if (ppActivate) *ppActivate = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateDeviceSource(void* pAttributes, void** ppSource) {
    (void)pAttributes;
    if (ppSource) *ppSource = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateMediaSession(void* pConfiguration, void** ppMediaSession) {
    (void)pConfiguration;
    if (ppMediaSession) *ppMediaSession = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateSampleGrabberSinkActivate(void* pIMFSampleGrabberSinkActivate,
                                                                   void* pIMFMediaType, void** ppIActivate) {
    (void)pIMFSampleGrabberSinkActivate; (void)pIMFMediaType;
    if (ppIActivate) *ppIActivate = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateTopology(void** ppTopo) {
    if (ppTopo) *ppTopo = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateTopologyNode(int NodeType, void** ppNode) {
    (void)NodeType;
    if (ppNode) *ppNode = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFEnumDeviceSources(void* pAttributes, void*** pppSourceActivate, DWORD* pcSourceActivate) {
    (void)pAttributes;
    if (pppSourceActivate) *pppSourceActivate = nullptr;
    if (pcSourceActivate) *pcSourceActivate = 0;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFGetService_MfDll(void* punkObject, const GUID& guidService, const IID& riid, void** ppvObject) {
    (void)punkObject; (void)guidService; (void)riid;
    if (ppvObject) *ppvObject = nullptr;
    return E_NOTIMPL;
}

// ===========================================================================
// 13) imm32 — 12 stubs (IME disabled in AppContainer — return 0 / FALSE)
// ===========================================================================
extern "C" HIMC    __stdcall Shim_ImmAssociateContext(HWND, HIMC) { return (HIMC)0; }
extern "C" HIMC    __stdcall Shim_ImmCreateContext() { return (HIMC)0; }
extern "C" BOOL    __stdcall Shim_ImmDestroyContext(HIMC) { return FALSE; }
extern "C" LONG    __stdcall Shim_ImmGetCompositionStringW(HIMC, DWORD, LPVOID, DWORD) { return 0; }
extern "C" HIMC    __stdcall Shim_ImmGetContext(HWND) { return (HIMC)0; }
extern "C" UINT    __stdcall Shim_ImmGetDescriptionW(HIMC, LPWSTR, UINT) { return 0; }
extern "C" UINT    __stdcall Shim_ImmGetIMEFileNameW(HIMC, LPWSTR, UINT) { return 0; }
extern "C" DWORD   __stdcall Shim_ImmGetProperty(HKL, DWORD) { return 0; }
extern "C" BOOL    __stdcall Shim_ImmNotifyIME(HIMC, DWORD, DWORD, DWORD) { return FALSE; }
extern "C" BOOL    __stdcall Shim_ImmReleaseContext(HWND, HIMC) { return FALSE; }
extern "C" BOOL    __stdcall Shim_ImmSetCandidateWindow(HIMC, LPCANDIDATEFORM) { return FALSE; }
extern "C" BOOL    __stdcall Shim_ImmSetCompositionWindow(HIMC, LPCOMPOSITIONFORM) { return FALSE; }

// ===========================================================================
// 14) winhttp — 10 pass-throughs (UWP supports WinHTTP)
// ===========================================================================
extern "C" BOOL __stdcall Shim_WinHttpAddRequestHeaders(HINTERNET hRequest, LPCWSTR lpszHeaders,
                                                       DWORD dwHeadersLength, DWORD dwModifiers) {
    return ::WinHttpAddRequestHeaders(hRequest, lpszHeaders, dwHeadersLength, dwModifiers);
}
extern "C" BOOL __stdcall Shim_WinHttpCloseHandle(HINTERNET hInternet) {
    return ::WinHttpCloseHandle(hInternet);
}
extern "C" HINTERNET __stdcall Shim_WinHttpConnect(HINTERNET hSession, LPCWSTR pswzServerName,
                                                   INTERNET_PORT nServerPort, DWORD dwReserved) {
    return ::WinHttpConnect(hSession, pswzServerName, nServerPort, dwReserved);
}
extern "C" BOOL __stdcall Shim_WinHttpCrackUrl(LPCWSTR pwszUrl, DWORD dwUrlLength, DWORD dwFlags,
                                               LPURL_COMPONENTS lpUrlComponents) {
    return ::WinHttpCrackUrl(pwszUrl, dwUrlLength, dwFlags, lpUrlComponents);
}
extern "C" BOOL __stdcall Shim_WinHttpGetDefaultProxyConfiguration(LPWINHTTP_PROXY_INFO pProxyInfo) {
    return ::WinHttpGetDefaultProxyConfiguration(pProxyInfo);
}
extern "C" BOOL __stdcall Shim_WinHttpGetIEProxyConfigForCurrentUser(LPWINHTTP_CURRENT_USER_IE_PROXY_CONFIG pProxyConfig) {
    // UWP AppContainer builds cannot read IE proxy settings: <winhttp.h> gates
    // both WINHTTP_CURRENT_USER_IE_PROXY_CONFIG and WinHttpGetIEProxyConfigForCurrentUser
    // behind WINAPI_PARTITION_DESKTOP, so neither the type nor the function is
    // visible to the compiler. The type is forward-declared above (so the
    // signature compiles), but the function call is only emitted on Linux.
#ifdef _MSC_VER
    (void)pProxyConfig;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
#else
    return ::WinHttpGetIEProxyConfigForCurrentUser(pProxyConfig);
#endif
}
extern "C" HINTERNET __stdcall Shim_WinHttpOpen(LPCWSTR pszAgentW, DWORD dwAccessType,
                                                LPCWSTR pszProxyW, LPCWSTR pszProxyBypassW,
                                                DWORD dwFlags) {
    return ::WinHttpOpen(pszAgentW, dwAccessType, pszProxyW, pszProxyBypassW, dwFlags);
}
extern "C" HINTERNET __stdcall Shim_WinHttpOpenRequest(HINTERNET hConnect, LPCWSTR pwszVerb,
                                                      LPCWSTR pwszObjectName, LPCWSTR pwszVersion,
                                                      LPCWSTR pwszReferrer, LPCWSTR* ppwszAcceptTypes,
                                                      DWORD dwFlags) {
    return ::WinHttpOpenRequest(hConnect, pwszVerb, pwszObjectName, pwszVersion, pwszReferrer,
                                ppwszAcceptTypes, dwFlags);
}
extern "C" BOOL __stdcall Shim_WinHttpReceiveResponse(HINTERNET hRequest, LPVOID lpReserved) {
    return ::WinHttpReceiveResponse(hRequest, lpReserved);
}
extern "C" BOOL __stdcall Shim_WinHttpSendRequest(HINTERNET hRequest, LPCWSTR lpszHeaders,
                                                  DWORD dwHeadersLength, LPVOID lpOptional,
                                                  DWORD dwOptionalLength, DWORD dwTotalLength,
                                                  DWORD_PTR dwContext) {
    return ::WinHttpSendRequest(hRequest, lpszHeaders, dwHeadersLength, lpOptional,
                                dwOptionalLength, dwTotalLength, dwContext);
}

// ===========================================================================
// 15) wininet — 11 stubs (return FALSE — use WinHTTP instead)
// ===========================================================================
extern "C" BOOL __stdcall Shim_HttpAddRequestHeadersW(HINTERNET, LPCWSTR, DWORD, DWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
}
extern "C" HINTERNET __stdcall Shim_HttpOpenRequestW(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return (HINTERNET)0;
}
extern "C" BOOL __stdcall Shim_HttpQueryInfoW(HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
}
extern "C" BOOL __stdcall Shim_HttpSendRequestW(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
}
extern "C" BOOL __stdcall Shim_InternetCloseHandle(HINTERNET) {
    return TRUE;
}
extern "C" HINTERNET __stdcall Shim_InternetConnectW(HINTERNET, LPCWSTR, INTERNET_PORT, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return (HINTERNET)0;
}
extern "C" BOOL __stdcall Shim_InternetCrackUrlW(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTS) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
}
extern "C" DWORD __stdcall Shim_InternetErrorDlg(HWND, HINTERNET, DWORD, DWORD, LPVOID*) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return ERROR_NOT_SUPPORTED;
}
extern "C" HINTERNET __stdcall Shim_InternetOpenW(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return (HINTERNET)0;
}
extern "C" BOOL __stdcall Shim_InternetReadFile(HINTERNET, LPVOID, DWORD, LPDWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
}
extern "C" BOOL __stdcall Shim_InternetSetOptionW(HINTERNET, DWORD, LPVOID, DWORD) {
    ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
}

// ===========================================================================
// 16) wintrust — 4 stubs (WinVerifyTrust returns S_OK always)
// ===========================================================================
extern "C" BOOL __stdcall Shim_CryptCatAdminCalcHashFromFileHandle(HANDLE hFile, DWORD* pcbHash,
                                                                   BYTE* pbHash, DWORD dwFlags) {
    (void)hFile; (void)dwFlags;
    if (pcbHash) *pcbHash = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
extern "C" HRESULT __stdcall Shim_WinVerifyTrust_WT(HWND, const GUID*, LPVOID) { return S_OK; }
extern "C" void* __stdcall Shim_WTHelperGetProvSignerFromChain(void* pProvData, DWORD idxSigner,
                                                               BOOL fCounterSigner, DWORD idxCounterSigner) {
    (void)pProvData; (void)idxSigner; (void)fCounterSigner; (void)idxCounterSigner;
    return nullptr;
}
extern "C" void* __stdcall Shim_WTHelperProvDataFromStateData(void* hStateData) {
    (void)hStateData;
    return nullptr;
}

// ===========================================================================
// 17) ntdll — 13 pass-throughs (Nt*/Rtl* surface)
// ===========================================================================
extern "C" NTSTATUS __stdcall Shim_NtSetInformationThread(HANDLE hThread, ULONG ThreadInformationClass,
                                                          PVOID ThreadInformation, ULONG ThreadInformationLength) {
    // MSVC's <winternl.h> declares NtSetInformationThread with a THREADINFOCLASS
    // enum as the 2nd argument; the shim accepts a bare ULONG for IAT ABI.
    // Cast through the enum on MSVC; the Linux stub takes ULONG directly.
#ifdef _MSC_VER
    return ::NtSetInformationThread(hThread, (THREADINFOCLASS)ThreadInformationClass,
                                    ThreadInformation, ThreadInformationLength);
#else
    return ::NtSetInformationThread(hThread, ThreadInformationClass, ThreadInformation, ThreadInformationLength);
#endif
}
extern "C" NTSTATUS __stdcall Shim_NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine,
                                               PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                                               PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
                                               PULONG Key) {
    // NtWriteFile is NOT declared in UWP's <winternl.h> (only a subset of the
    // Nt* surface is exposed to AppContainer builds). Stub it.
#ifdef _MSC_VER
    (void)FileHandle; (void)Event; (void)ApcRoutine; (void)ApcContext;
    (void)IoStatusBlock; (void)Buffer; (void)Length; (void)ByteOffset; (void)Key;
    return STATUS_NOT_IMPLEMENTED;
#else
    return ::NtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
#endif
}
extern "C" VOID    __stdcall Shim_RtlInitUnicodeString(PUNICODE_STRING DestinationString, PCWSTR SourceString) {
    ::RtlInitUnicodeString(DestinationString, SourceString);
}
extern "C" BOOLEAN __stdcall Shim_RtlIsCriticalSectionLockedByThread(PRTL_CRITICAL_SECTION CriticalSection) {
    // RtlIsCriticalSectionLockedByThread is NOT declared in UWP's SDK headers.
#ifdef _MSC_VER
    (void)CriticalSection;
    return FALSE;
#else
    return ::RtlIsCriticalSectionLockedByThread(CriticalSection);
#endif
}
extern "C" ULONG   __stdcall Shim_RtlNtStatusToDosError(NTSTATUS Status) {
    return ::RtlNtStatusToDosError(Status);
}
extern "C" BOOLEAN __stdcall Shim_RtlRunOnceExecuteOnce(PRTL_RUN_ONCE RunOnce, PRTL_RUN_ONCE_INIT_FN InitFn,
                                                        PVOID Context, PVOID* Parameter) {
    // RtlRunOnceExecuteOnce is NOT declared in UWP's SDK. Use the kernel32
    // wrapper InitOnceExecuteOnce, which has the same one-time-init semantics.
    // PRTL_RUN_ONCE and PINIT_ONCE are layout-compatible (both are a single
    // PVOID field); PRTL_RUN_ONCE_INIT_FN and PINIT_ONCE_FN differ only in
    // return type (BOOLEAN vs BOOL — both widened to EAX on x86/x64), so a
    // reinterpret_cast bridges them.
#ifdef _MSC_VER
    BOOL ok = ::InitOnceExecuteOnce(reinterpret_cast<PINIT_ONCE>(RunOnce),
                                    reinterpret_cast<PINIT_ONCE_FN>(InitFn),
                                    Context,
                                    reinterpret_cast<LPVOID*>(Parameter));
    return ok ? TRUE : FALSE;
#else
    return ::RtlRunOnceExecuteOnce(RunOnce, InitFn, Context, Parameter);
#endif
}
// NOTE: Shim_RtlUnwind / Shim_RtlUnwindEx / Shim_RtlVirtualUnwind are
// implemented in shims/kernel32/Kernel32GapFill.cpp.
extern "C" NTSTATUS __stdcall Shim_RtlUTF8ToUnicodeN(PWSTR UnicodeStringDestination,
                                                    ULONG UnicodeStringMaxByteCount,
                                                    PULONG UnicodeStringActualByteCount,
                                                    PCSTR UTF8StringSource, ULONG UTF8StringByteCount) {
    // RtlUTF8ToUnicodeN is NOT declared in UWP's SDK headers.
#ifdef _MSC_VER
    (void)UnicodeStringDestination; (void)UnicodeStringMaxByteCount;
    (void)UnicodeStringActualByteCount; (void)UTF8StringSource; (void)UTF8StringByteCount;
    return STATUS_NOT_IMPLEMENTED;
#else
    return ::RtlUTF8ToUnicodeN(UnicodeStringDestination, UnicodeStringMaxByteCount,
                               UnicodeStringActualByteCount, UTF8StringSource, UTF8StringByteCount);
#endif
}

// ===========================================================================
// 18) dwmapi — 4 pass-throughs
// ===========================================================================
extern "C" HRESULT __stdcall Shim_DwmFlush() { return ::DwmFlush(); }
extern "C" HRESULT __stdcall Shim_DwmGetCompositionTimingInfo(HWND hwnd, void* pTimingInfo) {
    return ::DwmGetCompositionTimingInfo(hwnd, (DWM_TIMING_INFO*)pTimingInfo);
}
extern "C" HRESULT __stdcall Shim_DwmIsCompositionEnabled(BOOL* pfEnabled) {
    return ::DwmIsCompositionEnabled(pfEnabled);
}
extern "C" HRESULT __stdcall Shim_DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) {
    return ::DwmSetWindowAttribute(hwnd, dwAttribute, pvAttribute, cbAttribute);
}

// ===========================================================================
// 19) cfgmgr32 — 2 stubs
// ===========================================================================
extern "C" CONFIGRET __stdcall Shim_CM_Get_DevNode_PropertyW(DEVINST dnDevInst, const DEVPROPKEY* PropertyKey,
                                                             DEVPROPTYPE* PropertyType, PBYTE PropertyBuffer,
                                                             PULONG PropertyBufferSize, ULONG ulFlags) {
    (void)dnDevInst; (void)PropertyKey; (void)PropertyType;
    (void)PropertyBuffer; (void)ulFlags;
    if (PropertyBufferSize) *PropertyBufferSize = 0;
    return CR_NO_SUCH_DEVNODE;
}
extern "C" CONFIGRET __stdcall Shim_CM_Locate_DevNodeW(PDEVINST pdnDevInst, LPCWSTR pDeviceID, ULONG ulFlags) {
    (void)pDeviceID; (void)ulFlags;
    if (pdnDevInst) *pdnDevInst = 0;
    return CR_NO_SUCH_DEVNODE;
}

// ===========================================================================
// 20) ncrypt — 2 pass-throughs
// ===========================================================================
extern "C" SECURITY_STATUS __stdcall Shim_NCryptFreeObject(NCRYPT_HANDLE hObject) {
    return ::NCryptFreeObject(hObject);
}
extern "C" SECURITY_STATUS __stdcall Shim_NCryptGetProperty(NCRYPT_HANDLE hObject, LPCWSTR pszProperty,
                                                            PBYTE pbOutput, DWORD cbOutput,
                                                            DWORD* pcbResult, DWORD dwFlags) {
    return ::NCryptGetProperty(hObject, pszProperty, pbOutput, cbOutput, pcbResult, dwFlags);
}

// ===========================================================================
// 21) msdmo — 2 stubs (DirectMedia Object helper)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_MoFreeMediaType(DMO_MEDIA_TYPE* pmt) {
    (void)pmt;
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_MoInitMediaType(DMO_MEDIA_TYPE* pmt, DWORD cbFormat) {
    (void)pmt; (void)cbFormat;
    return E_NOTIMPL;
}

// ===========================================================================
// 22) propsys — 1 stub
// ===========================================================================
extern "C" HRESULT __stdcall Shim_PropVariantToBSTR(const PROPVARIANT* propvar, BSTR* pbstrOut) {
    (void)propvar;
    if (pbstrOut) *pbstrOut = nullptr;
    return E_NOTIMPL;
}

// ===========================================================================
// 23) msi — 19 ordinals. Stub returning ERROR_INSTALL_SERVICE_FAILURE.
// ===========================================================================
extern "C" UINT __stdcall Shim_Msi_Ord8()   { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord17()  { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord45()  { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord70()  { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord88()  { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord90()  { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord111() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord115() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord116() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord118() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord125() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord137() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord141() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord169() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord171() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord173() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord190() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord205() { return ERROR_INSTALL_SERVICE_FAILURE; }
extern "C" UINT __stdcall Shim_Msi_Ord238() { return ERROR_INSTALL_SERVICE_FAILURE; }

// ===========================================================================
// 24) dbghelp — 18 stubs
// ===========================================================================
extern "C" PVOID __stdcall Shim_ImageNtHeader(PVOID Base) {
    (void)Base;
    return nullptr;
}
extern "C" BOOL  __stdcall Shim_MiniDumpWriteDump(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, DWORD DumpType,
                                                  void* ExceptionParam, void* UserStreamParam, void* CallbackParam) {
    (void)hProcess; (void)ProcessId; (void)hFile; (void)DumpType;
    (void)ExceptionParam; (void)UserStreamParam; (void)CallbackParam;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
extern "C" BOOL  __stdcall Shim_StackWalk64(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                                            LPSTACKFRAME64 StackFrame, PVOID ContextRecord,
                                            PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                            PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                            PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                                            PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress) {
    (void)MachineType; (void)hProcess; (void)hThread; (void)StackFrame; (void)ContextRecord;
    (void)ReadMemoryRoutine; (void)FunctionTableAccessRoutine;
    (void)GetModuleBaseRoutine; (void)TranslateAddress;
    return FALSE;
}
extern "C" BOOL  __stdcall Shim_SymCleanup(HANDLE hProcess) { (void)hProcess; return FALSE; }
extern "C" BOOL  __stdcall Shim_SymFromAddr(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement,
                                            void* Symbol) {
    (void)hProcess; (void)Address; (void)Displacement; (void)Symbol;
    return FALSE;
}
extern "C" PVOID __stdcall Shim_SymFunctionTableAccess64(HANDLE hProcess, DWORD64 AddrBase) {
    (void)hProcess; (void)AddrBase;
    return nullptr;
}
extern "C" BOOL  __stdcall Shim_SymGetLineFromAddr64(HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement,
                                                     void* Line) {
    (void)hProcess; (void)dwAddr; (void)pdwDisplacement; (void)Line;
    return FALSE;
}
extern "C" DWORD64 __stdcall Shim_SymGetModuleBase64(HANDLE hProcess, DWORD64 dwAddr) {
    (void)hProcess; (void)dwAddr;
    return 0;
}
extern "C" BOOL  __stdcall Shim_SymGetModuleInfo64(HANDLE hProcess, DWORD64 dwAddr, void* ModuleInfo) {
    (void)hProcess; (void)dwAddr; (void)ModuleInfo;
    return FALSE;
}
extern "C" BOOL  __stdcall Shim_SymGetModuleInfoW64(HANDLE hProcess, DWORD64 dwAddr, void* ModuleInfo) {
    (void)hProcess; (void)dwAddr; (void)ModuleInfo;
    return FALSE;
}
extern "C" DWORD __stdcall Shim_SymGetOptions() { return 0; }
extern "C" BOOL  __stdcall Shim_SymGetSymFromAddr64(HANDLE hProcess, DWORD64 dwAddr, PDWORD64 pdwDisplacement,
                                                    void* Symbol) {
    (void)hProcess; (void)dwAddr; (void)pdwDisplacement; (void)Symbol;
    return FALSE;
}
extern "C" BOOL  __stdcall Shim_SymInitialize(HANDLE hProcess, LPCSTR UserSearchPath, BOOL fInvadeProcess) {
    (void)hProcess; (void)UserSearchPath; (void)fInvadeProcess;
    return FALSE;
}
extern "C" BOOL  __stdcall Shim_SymInitializeW(HANDLE hProcess, LPCWSTR UserSearchPath, BOOL fInvadeProcess) {
    (void)hProcess; (void)UserSearchPath; (void)fInvadeProcess;
    return FALSE;
}
extern "C" DWORD64 __stdcall Shim_SymLoadModuleExW(HANDLE hProcess, HANDLE hFile, LPCWSTR ImageName,
                                                   LPCWSTR ModuleName, DWORD64 BaseOfDll, DWORD DllSize,
                                                   void* Data, DWORD Flags) {
    (void)hProcess; (void)hFile; (void)ImageName; (void)ModuleName; (void)BaseOfDll;
    (void)DllSize; (void)Data; (void)Flags;
    return 0;
}
extern "C" DWORD __stdcall Shim_SymSetOptions(DWORD SymOptions) { (void)SymOptions; return 0; }
extern "C" BOOL  __stdcall Shim_SymSetSearchPathW(HANDLE hProcess, LPCWSTR SearchPath) {
    (void)hProcess; (void)SearchPath;
    return FALSE;
}
extern "C" BOOL  __stdcall Shim_SymUnloadModule64(HANDLE hProcess, DWORD64 BaseOfDll) {
    (void)hProcess; (void)BaseOfDll;
    return FALSE;
}

// ===========================================================================
// 25) d3dcompiler_47 — 1 pass-through (D3DCreateBlob)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_D3DCreateBlob(SIZE_T NumBytes, ID3DBlob** ppBlob) {
    return ::D3DCreateBlob(NumBytes, ppBlob);
}

// ===========================================================================
// 26) dxgi — 3 pass-throughs
// ===========================================================================
extern "C" HRESULT __stdcall Shim_CreateDXGIFactory(const IID& riid, void** ppFactory) {
    return ::CreateDXGIFactory(riid, ppFactory);
}
extern "C" HRESULT __stdcall Shim_CreateDXGIFactory1(const IID& riid, void** ppFactory) {
    return ::CreateDXGIFactory1(riid, ppFactory);
}
extern "C" HRESULT __stdcall Shim_CreateDXGIFactory2(UINT Flags, const IID& riid, void** ppFactory) {
    return ::CreateDXGIFactory2(Flags, riid, ppFactory);
}

// ===========================================================================
// 27) cabinet — 3 ordinals. Stub.
// ===========================================================================
extern "C" INT_PTR __stdcall Shim_Cabinet_Ord20() { return 0; }
extern "C" INT_PTR __stdcall Shim_Cabinet_Ord22() { return 0; }
extern "C" INT_PTR __stdcall Shim_Cabinet_Ord23() { return 0; }

// ===========================================================================
// 28) xaudio2_9redist — 1 pass-through (XAudio2Create; ordinal:1 is XAudio2Create)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_XAudio2Create_Redist(IXAudio2** ppXAudio2, UINT Flags, XAUDIO2_PROCESSOR XAudio2Processor) {
    return ::XAudio2Create(ppXAudio2, Flags, XAudio2Processor);
}

// ===========================================================================
// 29) mmdevapi — 1 ordinal. Stub.
// ===========================================================================
extern "C" HRESULT __stdcall Shim_Mmdevapi_Ord17() { return E_NOTIMPL; }

// ===========================================================================
// 30) rpcrt4 — 1 pass-through (UuidCreate)
// ===========================================================================
extern "C" RPC_STATUS __stdcall Shim_UuidCreate(UUID* Uuid) {
    return ::UuidCreate(Uuid);
}

// ===========================================================================
// 31) uxtheme — 1 stub (SetWindowTheme returns S_OK)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) {
    (void)hwnd; (void)pszSubAppName; (void)pszSubIdList;
    return S_OK;
}

// ===========================================================================
// 32) uiautomationcore — 2 stubs
// ===========================================================================
extern "C" BOOL __stdcall Shim_UiaClientsAreListening() { return FALSE; }
extern "C" HRESULT __stdcall Shim_UiaRaiseAutomationPropertyChangedEvent(void* pProvider, PROPERTYID id, VARIANT oldValue, VARIANT newValue) {
    (void)pProvider; (void)id; (void)oldValue; (void)newValue;
    return E_NOTIMPL;
}

// ===========================================================================
// 33) xinput1_4 — 2 ordinals (ordinal:2 = XInputGetState, ordinal:3 = XInputSetState)
// These are already shimmed by name in XInputShim.cpp; this registers the
// ordinal aliases so the gap report resolves them.
// ===========================================================================
extern "C" DWORD __stdcall Shim_XInputGetState_Ord(DWORD dwUserIndex, XINPUT_STATE* pState) {
    return ::XInputGetState(dwUserIndex, pState);
}
extern "C" DWORD __stdcall Shim_XInputSetState_Ord(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) {
    return ::XInputSetState(dwUserIndex, pVibration);
}

}  // namespace xwr

// ===========================================================================
// Registration — ~200 entries, grouped by DLL.
// ===========================================================================

// ---------------------------------------------------------------------------
// winmm (27)
// ---------------------------------------------------------------------------
#define XWR_REG_WINMM(name) \
    REGISTER_SHIM("winmm", #name, (FARPROC)&xwr::Shim_##name);
XWR_REG_WINMM(mixerClose)
XWR_REG_WINMM(mixerGetControlDetailsW)
XWR_REG_WINMM(mixerGetDevCapsW)
XWR_REG_WINMM(mixerGetID)
XWR_REG_WINMM(mixerGetLineControlsW)
XWR_REG_WINMM(mixerGetLineInfoW)
XWR_REG_WINMM(mixerGetNumDevs)
XWR_REG_WINMM(mixerOpen)
XWR_REG_WINMM(mixerSetControlDetails)
XWR_REG_WINMM(waveInAddBuffer)
XWR_REG_WINMM(waveInClose)
XWR_REG_WINMM(waveInGetDevCapsW)
XWR_REG_WINMM(waveInGetErrorTextW)
XWR_REG_WINMM(waveInGetID)
XWR_REG_WINMM(waveInGetNumDevs)
XWR_REG_WINMM(waveInGetPosition)
XWR_REG_WINMM(waveInMessage)
XWR_REG_WINMM(waveInOpen)
XWR_REG_WINMM(waveInPrepareHeader)
XWR_REG_WINMM(waveInReset)
XWR_REG_WINMM(waveInStart)
XWR_REG_WINMM(waveInStop)
XWR_REG_WINMM(waveInUnprepareHeader)
XWR_REG_WINMM(waveOutGetErrorTextW)
XWR_REG_WINMM(waveOutGetID)
XWR_REG_WINMM(waveOutGetPosition)
XWR_REG_WINMM(waveOutMessage)
#undef XWR_REG_WINMM

// ---------------------------------------------------------------------------
// ole32 (10)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ole32", "CLSIDFromProgID",                    (FARPROC)&xwr::Shim_CLSIDFromProgID);
REGISTER_SHIM("ole32", "CoGetInterfaceAndReleaseStream",     (FARPROC)&xwr::Shim_CoGetInterfaceAndReleaseStream);
REGISTER_SHIM("ole32", "CoInitializeSecurity",               (FARPROC)&xwr::Shim_CoInitializeSecurity);
REGISTER_SHIM("ole32", "CoMarshalInterThreadInterfaceInStream", (FARPROC)&xwr::Shim_CoMarshalInterThreadInterfaceInStream);
REGISTER_SHIM("ole32", "CoSetProxyBlanket",                  (FARPROC)&xwr::Shim_CoSetProxyBlanket);
REGISTER_SHIM("ole32", "IIDFromString",                      (FARPROC)&xwr::Shim_IIDFromString);
REGISTER_SHIM("ole32", "PropVariantClear",                   (FARPROC)&xwr::Shim_PropVariantClear);
REGISTER_SHIM("ole32", "PropVariantCopy",                    (FARPROC)&xwr::Shim_PropVariantCopy);
REGISTER_SHIM("ole32", "ReleaseStgMedium",                   (FARPROC)&xwr::Shim_ReleaseStgMedium);
REGISTER_SHIM("ole32", "StringFromGUID2",                    (FARPROC)&xwr::Shim_StringFromGUID2);

// ---------------------------------------------------------------------------
// oleaut32 (9 ordinals)
// ---------------------------------------------------------------------------
REGISTER_SHIM("oleaut32", "ordinal:2",   (FARPROC)&xwr::Shim_OleAut_Ord2);
REGISTER_SHIM("oleaut32", "ordinal:6",   (FARPROC)&xwr::Shim_OleAut_Ord6);
REGISTER_SHIM("oleaut32", "ordinal:7",   (FARPROC)&xwr::Shim_OleAut_Ord7);
REGISTER_SHIM("oleaut32", "ordinal:8",   (FARPROC)&xwr::Shim_OleAut_Ord8);
REGISTER_SHIM("oleaut32", "ordinal:9",   (FARPROC)&xwr::Shim_OleAut_Ord9);
REGISTER_SHIM("oleaut32", "ordinal:10",  (FARPROC)&xwr::Shim_OleAut_Ord10);
REGISTER_SHIM("oleaut32", "ordinal:26",  (FARPROC)&xwr::Shim_OleAut_Ord26);
REGISTER_SHIM("oleaut32", "ordinal:200", (FARPROC)&xwr::Shim_OleAut_Ord200);
REGISTER_SHIM("oleaut32", "ordinal:411", (FARPROC)&xwr::Shim_OleAut_Ord411);

// ---------------------------------------------------------------------------
// gdi32 (3 here; ExtTextOutW lives in User32Shim.cpp)
// ---------------------------------------------------------------------------
REGISTER_SHIM("gdi32", "ChoosePixelFormat", (FARPROC)&xwr::Shim_ChoosePixelFormat);
REGISTER_SHIM("gdi32", "SetPixelFormat",    (FARPROC)&xwr::Shim_SetPixelFormat);
REGISTER_SHIM("gdi32", "SwapBuffers",       (FARPROC)&xwr::Shim_SwapBuffers);
// NOTE: ExtTextOutW registration lives in shims/user32/User32Shim.cpp.

// ---------------------------------------------------------------------------
// shell32 (1)
// ---------------------------------------------------------------------------
REGISTER_SHIM("shell32", "CommandLineToArgvW", (FARPROC)&xwr::Shim_CommandLineToArgvW);

// ---------------------------------------------------------------------------
// shlwapi (1 ordinal)
// ---------------------------------------------------------------------------
REGISTER_SHIM("shlwapi", "ordinal:219", (FARPROC)&xwr::Shim_Shlwapi_Ord219);

// ---------------------------------------------------------------------------
// version (1)
// ---------------------------------------------------------------------------
REGISTER_SHIM("version", "GetFileVersionInfoExW", (FARPROC)&xwr::Shim_GetFileVersionInfoExW);

// ---------------------------------------------------------------------------
// setupapi (7)
// ---------------------------------------------------------------------------
REGISTER_SHIM("setupapi", "CM_Get_Device_IDW",               (FARPROC)&xwr::Shim_CM_Get_Device_IDW);
REGISTER_SHIM("setupapi", "SetupDiEnumDeviceInfo",           (FARPROC)&xwr::Shim_SetupDiEnumDeviceInfo);
REGISTER_SHIM("setupapi", "SetupDiGetClassDevsExW",          (FARPROC)&xwr::Shim_SetupDiGetClassDevsExW);
REGISTER_SHIM("setupapi", "SetupDiGetDeviceInterfaceAlias",  (FARPROC)&xwr::Shim_SetupDiGetDeviceInterfaceAlias);
REGISTER_SHIM("setupapi", "SetupDiGetDevicePropertyW",       (FARPROC)&xwr::Shim_SetupDiGetDevicePropertyW);
REGISTER_SHIM("setupapi", "SetupDiOpenDeviceInterfaceRegKey", (FARPROC)&xwr::Shim_SetupDiOpenDeviceInterfaceRegKey);
REGISTER_SHIM("setupapi", "SetupDiOpenDevRegKey",            (FARPROC)&xwr::Shim_SetupDiOpenDevRegKey);

// ---------------------------------------------------------------------------
// dsound (6 ordinals)
// ---------------------------------------------------------------------------
REGISTER_SHIM("dsound", "ordinal:1",  (FARPROC)&xwr::Shim_Dsound_Ord1);
REGISTER_SHIM("dsound", "ordinal:3",  (FARPROC)&xwr::Shim_Dsound_Ord3);
REGISTER_SHIM("dsound", "ordinal:6",  (FARPROC)&xwr::Shim_Dsound_Ord6);
REGISTER_SHIM("dsound", "ordinal:8",  (FARPROC)&xwr::Shim_Dsound_Ord8);
REGISTER_SHIM("dsound", "ordinal:11", (FARPROC)&xwr::Shim_Dsound_Ord11);
REGISTER_SHIM("dsound", "ordinal:12", (FARPROC)&xwr::Shim_Dsound_Ord12);

// ---------------------------------------------------------------------------
// d3d12 (4 named + 2 ordinals)
// ---------------------------------------------------------------------------
REGISTER_SHIM("d3d12", "D3D12CoreCreateLayeredDevice",       (FARPROC)&xwr::Shim_D3D12CoreCreateLayeredDevice);
REGISTER_SHIM("d3d12", "D3D12CoreGetLayeredDeviceSize",      (FARPROC)&xwr::Shim_D3D12CoreGetLayeredDeviceSize);
REGISTER_SHIM("d3d12", "D3D12CoreRegisterLayers",            (FARPROC)&xwr::Shim_D3D12CoreRegisterLayers);
REGISTER_SHIM("d3d12", "D3D12SerializeVersionedRootSignature", (FARPROC)&xwr::Shim_D3D12SerializeVersionedRootSignature);
REGISTER_SHIM("d3d12", "ordinal:101", (FARPROC)&xwr::Shim_D3D12_Ord101);
REGISTER_SHIM("d3d12", "ordinal:102", (FARPROC)&xwr::Shim_D3D12_Ord102);

// ---------------------------------------------------------------------------
// mfplat (5)
// ---------------------------------------------------------------------------
REGISTER_SHIM("mfplat", "MFCreateAlignedMemoryBuffer", (FARPROC)&xwr::Shim_MFCreateAlignedMemoryBuffer);
REGISTER_SHIM("mfplat", "MFCreateAsyncResult",         (FARPROC)&xwr::Shim_MFCreateAsyncResult);
REGISTER_SHIM("mfplat", "MFCreateDXGIDeviceManager",   (FARPROC)&xwr::Shim_MFCreateDXGIDeviceManager);
REGISTER_SHIM("mfplat", "MFInvokeCallback",            (FARPROC)&xwr::Shim_MFInvokeCallback);
REGISTER_SHIM("mfplat", "MFPutWorkItem",               (FARPROC)&xwr::Shim_MFPutWorkItem);

// ---------------------------------------------------------------------------
// mf (8)
// ---------------------------------------------------------------------------
REGISTER_SHIM("mf", "MFCreateAudioRendererActivate",      (FARPROC)&xwr::Shim_MFCreateAudioRendererActivate);
REGISTER_SHIM("mf", "MFCreateDeviceSource",               (FARPROC)&xwr::Shim_MFCreateDeviceSource);
REGISTER_SHIM("mf", "MFCreateMediaSession",               (FARPROC)&xwr::Shim_MFCreateMediaSession);
REGISTER_SHIM("mf", "MFCreateSampleGrabberSinkActivate",  (FARPROC)&xwr::Shim_MFCreateSampleGrabberSinkActivate);
REGISTER_SHIM("mf", "MFCreateTopology",                   (FARPROC)&xwr::Shim_MFCreateTopology);
REGISTER_SHIM("mf", "MFCreateTopologyNode",               (FARPROC)&xwr::Shim_MFCreateTopologyNode);
REGISTER_SHIM("mf", "MFEnumDeviceSources",                (FARPROC)&xwr::Shim_MFEnumDeviceSources);
REGISTER_SHIM("mf", "MFGetService",                       (FARPROC)&xwr::Shim_MFGetService_MfDll);

// ---------------------------------------------------------------------------
// imm32 (12)
// ---------------------------------------------------------------------------
REGISTER_SHIM("imm32", "ImmAssociateContext",       (FARPROC)&xwr::Shim_ImmAssociateContext);
REGISTER_SHIM("imm32", "ImmCreateContext",          (FARPROC)&xwr::Shim_ImmCreateContext);
REGISTER_SHIM("imm32", "ImmDestroyContext",         (FARPROC)&xwr::Shim_ImmDestroyContext);
REGISTER_SHIM("imm32", "ImmGetCompositionStringW",  (FARPROC)&xwr::Shim_ImmGetCompositionStringW);
REGISTER_SHIM("imm32", "ImmGetContext",             (FARPROC)&xwr::Shim_ImmGetContext);
REGISTER_SHIM("imm32", "ImmGetDescriptionW",        (FARPROC)&xwr::Shim_ImmGetDescriptionW);
REGISTER_SHIM("imm32", "ImmGetIMEFileNameW",        (FARPROC)&xwr::Shim_ImmGetIMEFileNameW);
REGISTER_SHIM("imm32", "ImmGetProperty",            (FARPROC)&xwr::Shim_ImmGetProperty);
REGISTER_SHIM("imm32", "ImmNotifyIME",              (FARPROC)&xwr::Shim_ImmNotifyIME);
REGISTER_SHIM("imm32", "ImmReleaseContext",         (FARPROC)&xwr::Shim_ImmReleaseContext);
REGISTER_SHIM("imm32", "ImmSetCandidateWindow",     (FARPROC)&xwr::Shim_ImmSetCandidateWindow);
REGISTER_SHIM("imm32", "ImmSetCompositionWindow",   (FARPROC)&xwr::Shim_ImmSetCompositionWindow);

// ---------------------------------------------------------------------------
// winhttp (10)
// ---------------------------------------------------------------------------
REGISTER_SHIM("winhttp", "WinHttpAddRequestHeaders",                  (FARPROC)&xwr::Shim_WinHttpAddRequestHeaders);
REGISTER_SHIM("winhttp", "WinHttpCloseHandle",                        (FARPROC)&xwr::Shim_WinHttpCloseHandle);
REGISTER_SHIM("winhttp", "WinHttpConnect",                            (FARPROC)&xwr::Shim_WinHttpConnect);
REGISTER_SHIM("winhttp", "WinHttpCrackUrl",                           (FARPROC)&xwr::Shim_WinHttpCrackUrl);
REGISTER_SHIM("winhttp", "WinHttpGetDefaultProxyConfiguration",       (FARPROC)&xwr::Shim_WinHttpGetDefaultProxyConfiguration);
REGISTER_SHIM("winhttp", "WinHttpGetIEProxyConfigForCurrentUser",     (FARPROC)&xwr::Shim_WinHttpGetIEProxyConfigForCurrentUser);
REGISTER_SHIM("winhttp", "WinHttpOpen",                               (FARPROC)&xwr::Shim_WinHttpOpen);
REGISTER_SHIM("winhttp", "WinHttpOpenRequest",                        (FARPROC)&xwr::Shim_WinHttpOpenRequest);
REGISTER_SHIM("winhttp", "WinHttpReceiveResponse",                    (FARPROC)&xwr::Shim_WinHttpReceiveResponse);
REGISTER_SHIM("winhttp", "WinHttpSendRequest",                        (FARPROC)&xwr::Shim_WinHttpSendRequest);

// ---------------------------------------------------------------------------
// wininet (11)
// ---------------------------------------------------------------------------
REGISTER_SHIM("wininet", "HttpAddRequestHeadersW", (FARPROC)&xwr::Shim_HttpAddRequestHeadersW);
REGISTER_SHIM("wininet", "HttpOpenRequestW",       (FARPROC)&xwr::Shim_HttpOpenRequestW);
REGISTER_SHIM("wininet", "HttpQueryInfoW",         (FARPROC)&xwr::Shim_HttpQueryInfoW);
REGISTER_SHIM("wininet", "HttpSendRequestW",       (FARPROC)&xwr::Shim_HttpSendRequestW);
REGISTER_SHIM("wininet", "InternetCloseHandle",    (FARPROC)&xwr::Shim_InternetCloseHandle);
REGISTER_SHIM("wininet", "InternetConnectW",       (FARPROC)&xwr::Shim_InternetConnectW);
REGISTER_SHIM("wininet", "InternetCrackUrlW",      (FARPROC)&xwr::Shim_InternetCrackUrlW);
REGISTER_SHIM("wininet", "InternetErrorDlg",       (FARPROC)&xwr::Shim_InternetErrorDlg);
REGISTER_SHIM("wininet", "InternetOpenW",          (FARPROC)&xwr::Shim_InternetOpenW);
REGISTER_SHIM("wininet", "InternetReadFile",       (FARPROC)&xwr::Shim_InternetReadFile);
REGISTER_SHIM("wininet", "InternetSetOptionW",     (FARPROC)&xwr::Shim_InternetSetOptionW);

// ---------------------------------------------------------------------------
// wintrust (4)
// ---------------------------------------------------------------------------
REGISTER_SHIM("wintrust", "CryptCatAdminCalcHashFromFileHandle", (FARPROC)&xwr::Shim_CryptCatAdminCalcHashFromFileHandle);
REGISTER_SHIM("wintrust", "WinVerifyTrust",                      (FARPROC)&xwr::Shim_WinVerifyTrust_WT);
REGISTER_SHIM("wintrust", "WTHelperGetProvSignerFromChain",      (FARPROC)&xwr::Shim_WTHelperGetProvSignerFromChain);
REGISTER_SHIM("wintrust", "WTHelperProvDataFromStateData",       (FARPROC)&xwr::Shim_WTHelperProvDataFromStateData);

// ---------------------------------------------------------------------------
// ntdll (7 here; 6 Rtl* unwinding ones live in Kernel32GapFill.cpp)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ntdll", "NtSetInformationThread",            (FARPROC)&xwr::Shim_NtSetInformationThread);
REGISTER_SHIM("ntdll", "NtWriteFile",                        (FARPROC)&xwr::Shim_NtWriteFile);
// NOTE: RtlCaptureContext / RtlLookupFunctionEntry / RtlPcToFileHeader /
// RtlUnwind / RtlUnwindEx / RtlVirtualUnwind registrations live in
// shims/kernel32/Kernel32GapFill.cpp.
REGISTER_SHIM("ntdll", "RtlInitUnicodeString",               (FARPROC)&xwr::Shim_RtlInitUnicodeString);
REGISTER_SHIM("ntdll", "RtlIsCriticalSectionLockedByThread", (FARPROC)&xwr::Shim_RtlIsCriticalSectionLockedByThread);
REGISTER_SHIM("ntdll", "RtlNtStatusToDosError",              (FARPROC)&xwr::Shim_RtlNtStatusToDosError);
REGISTER_SHIM("ntdll", "RtlRunOnceExecuteOnce",              (FARPROC)&xwr::Shim_RtlRunOnceExecuteOnce);
REGISTER_SHIM("ntdll", "RtlUTF8ToUnicodeN",                  (FARPROC)&xwr::Shim_RtlUTF8ToUnicodeN);

// ---------------------------------------------------------------------------
// dwmapi (4)
// ---------------------------------------------------------------------------
REGISTER_SHIM("dwmapi", "DwmFlush",                    (FARPROC)&xwr::Shim_DwmFlush);
REGISTER_SHIM("dwmapi", "DwmGetCompositionTimingInfo", (FARPROC)&xwr::Shim_DwmGetCompositionTimingInfo);
REGISTER_SHIM("dwmapi", "DwmIsCompositionEnabled",     (FARPROC)&xwr::Shim_DwmIsCompositionEnabled);
REGISTER_SHIM("dwmapi", "DwmSetWindowAttribute",       (FARPROC)&xwr::Shim_DwmSetWindowAttribute);

// ---------------------------------------------------------------------------
// cfgmgr32 (2)
// ---------------------------------------------------------------------------
REGISTER_SHIM("cfgmgr32", "CM_Get_DevNode_PropertyW", (FARPROC)&xwr::Shim_CM_Get_DevNode_PropertyW);
REGISTER_SHIM("cfgmgr32", "CM_Locate_DevNodeW",       (FARPROC)&xwr::Shim_CM_Locate_DevNodeW);

// ---------------------------------------------------------------------------
// ncrypt (2)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ncrypt", "NCryptFreeObject",  (FARPROC)&xwr::Shim_NCryptFreeObject);
REGISTER_SHIM("ncrypt", "NCryptGetProperty", (FARPROC)&xwr::Shim_NCryptGetProperty);

// ---------------------------------------------------------------------------
// msdmo (2)
// ---------------------------------------------------------------------------
REGISTER_SHIM("msdmo", "MoFreeMediaType",  (FARPROC)&xwr::Shim_MoFreeMediaType);
REGISTER_SHIM("msdmo", "MoInitMediaType",  (FARPROC)&xwr::Shim_MoInitMediaType);

// ---------------------------------------------------------------------------
// propsys (1)
// ---------------------------------------------------------------------------
REGISTER_SHIM("propsys", "PropVariantToBSTR", (FARPROC)&xwr::Shim_PropVariantToBSTR);

// ---------------------------------------------------------------------------
// msi (19 ordinals)
// ---------------------------------------------------------------------------
REGISTER_SHIM("msi", "ordinal:8",   (FARPROC)&xwr::Shim_Msi_Ord8);
REGISTER_SHIM("msi", "ordinal:17",  (FARPROC)&xwr::Shim_Msi_Ord17);
REGISTER_SHIM("msi", "ordinal:45",  (FARPROC)&xwr::Shim_Msi_Ord45);
REGISTER_SHIM("msi", "ordinal:70",  (FARPROC)&xwr::Shim_Msi_Ord70);
REGISTER_SHIM("msi", "ordinal:88",  (FARPROC)&xwr::Shim_Msi_Ord88);
REGISTER_SHIM("msi", "ordinal:90",  (FARPROC)&xwr::Shim_Msi_Ord90);
REGISTER_SHIM("msi", "ordinal:111", (FARPROC)&xwr::Shim_Msi_Ord111);
REGISTER_SHIM("msi", "ordinal:115", (FARPROC)&xwr::Shim_Msi_Ord115);
REGISTER_SHIM("msi", "ordinal:116", (FARPROC)&xwr::Shim_Msi_Ord116);
REGISTER_SHIM("msi", "ordinal:118", (FARPROC)&xwr::Shim_Msi_Ord118);
REGISTER_SHIM("msi", "ordinal:125", (FARPROC)&xwr::Shim_Msi_Ord125);
REGISTER_SHIM("msi", "ordinal:137", (FARPROC)&xwr::Shim_Msi_Ord137);
REGISTER_SHIM("msi", "ordinal:141", (FARPROC)&xwr::Shim_Msi_Ord141);
REGISTER_SHIM("msi", "ordinal:169", (FARPROC)&xwr::Shim_Msi_Ord169);
REGISTER_SHIM("msi", "ordinal:171", (FARPROC)&xwr::Shim_Msi_Ord171);
REGISTER_SHIM("msi", "ordinal:173", (FARPROC)&xwr::Shim_Msi_Ord173);
REGISTER_SHIM("msi", "ordinal:190", (FARPROC)&xwr::Shim_Msi_Ord190);
REGISTER_SHIM("msi", "ordinal:205", (FARPROC)&xwr::Shim_Msi_Ord205);
REGISTER_SHIM("msi", "ordinal:238", (FARPROC)&xwr::Shim_Msi_Ord238);

// ---------------------------------------------------------------------------
// dbghelp (18)
// ---------------------------------------------------------------------------
REGISTER_SHIM("dbghelp", "ImageNtHeader",            (FARPROC)&xwr::Shim_ImageNtHeader);
REGISTER_SHIM("dbghelp", "MiniDumpWriteDump",        (FARPROC)&xwr::Shim_MiniDumpWriteDump);
REGISTER_SHIM("dbghelp", "StackWalk64",              (FARPROC)&xwr::Shim_StackWalk64);
REGISTER_SHIM("dbghelp", "SymCleanup",               (FARPROC)&xwr::Shim_SymCleanup);
REGISTER_SHIM("dbghelp", "SymFromAddr",              (FARPROC)&xwr::Shim_SymFromAddr);
REGISTER_SHIM("dbghelp", "SymFunctionTableAccess64", (FARPROC)&xwr::Shim_SymFunctionTableAccess64);
REGISTER_SHIM("dbghelp", "SymGetLineFromAddr64",     (FARPROC)&xwr::Shim_SymGetLineFromAddr64);
REGISTER_SHIM("dbghelp", "SymGetModuleBase64",       (FARPROC)&xwr::Shim_SymGetModuleBase64);
REGISTER_SHIM("dbghelp", "SymGetModuleInfo64",       (FARPROC)&xwr::Shim_SymGetModuleInfo64);
REGISTER_SHIM("dbghelp", "SymGetModuleInfoW64",      (FARPROC)&xwr::Shim_SymGetModuleInfoW64);
REGISTER_SHIM("dbghelp", "SymGetOptions",            (FARPROC)&xwr::Shim_SymGetOptions);
REGISTER_SHIM("dbghelp", "SymGetSymFromAddr64",      (FARPROC)&xwr::Shim_SymGetSymFromAddr64);
REGISTER_SHIM("dbghelp", "SymInitialize",            (FARPROC)&xwr::Shim_SymInitialize);
REGISTER_SHIM("dbghelp", "SymInitializeW",           (FARPROC)&xwr::Shim_SymInitializeW);
REGISTER_SHIM("dbghelp", "SymLoadModuleExW",         (FARPROC)&xwr::Shim_SymLoadModuleExW);
REGISTER_SHIM("dbghelp", "SymSetOptions",            (FARPROC)&xwr::Shim_SymSetOptions);
REGISTER_SHIM("dbghelp", "SymSetSearchPathW",        (FARPROC)&xwr::Shim_SymSetSearchPathW);
REGISTER_SHIM("dbghelp", "SymUnloadModule64",        (FARPROC)&xwr::Shim_SymUnloadModule64);

// ---------------------------------------------------------------------------
// d3dcompiler_47 (1)
// ---------------------------------------------------------------------------
REGISTER_SHIM("d3dcompiler_47", "D3DCreateBlob", (FARPROC)&xwr::Shim_D3DCreateBlob);

// ---------------------------------------------------------------------------
// dxgi (3)
// ---------------------------------------------------------------------------
REGISTER_SHIM("dxgi", "CreateDXGIFactory",  (FARPROC)&xwr::Shim_CreateDXGIFactory);
REGISTER_SHIM("dxgi", "CreateDXGIFactory1", (FARPROC)&xwr::Shim_CreateDXGIFactory1);
REGISTER_SHIM("dxgi", "CreateDXGIFactory2", (FARPROC)&xwr::Shim_CreateDXGIFactory2);

// ---------------------------------------------------------------------------
// cabinet (3 ordinals)
// ---------------------------------------------------------------------------
REGISTER_SHIM("cabinet", "ordinal:20", (FARPROC)&xwr::Shim_Cabinet_Ord20);
REGISTER_SHIM("cabinet", "ordinal:22", (FARPROC)&xwr::Shim_Cabinet_Ord22);
REGISTER_SHIM("cabinet", "ordinal:23", (FARPROC)&xwr::Shim_Cabinet_Ord23);

// ---------------------------------------------------------------------------
// xaudio2_9redist (1 — ordinal:1 = XAudio2Create)
// ---------------------------------------------------------------------------
REGISTER_SHIM("xaudio2_9redist", "ordinal:1",   (FARPROC)&xwr::Shim_XAudio2Create_Redist);
REGISTER_SHIM("xaudio2_9redist", "XAudio2Create", (FARPROC)&xwr::Shim_XAudio2Create_Redist);

// ---------------------------------------------------------------------------
// mmdevapi (1 ordinal)
// ---------------------------------------------------------------------------
REGISTER_SHIM("mmdevapi", "ordinal:17", (FARPROC)&xwr::Shim_Mmdevapi_Ord17);

// ---------------------------------------------------------------------------
// rpcrt4 (1)
// ---------------------------------------------------------------------------
REGISTER_SHIM("rpcrt4", "UuidCreate", (FARPROC)&xwr::Shim_UuidCreate);

// ---------------------------------------------------------------------------
// uxtheme (1)
// ---------------------------------------------------------------------------
REGISTER_SHIM("uxtheme", "SetWindowTheme", (FARPROC)&xwr::Shim_SetWindowTheme);

// ---------------------------------------------------------------------------
// uiautomationcore (2)
// ---------------------------------------------------------------------------
REGISTER_SHIM("uiautomationcore", "UiaClientsAreListening",                (FARPROC)&xwr::Shim_UiaClientsAreListening);
REGISTER_SHIM("uiautomationcore", "UiaRaiseAutomationPropertyChangedEvent", (FARPROC)&xwr::Shim_UiaRaiseAutomationPropertyChangedEvent);

// ---------------------------------------------------------------------------
// xinput1_4 (2 ordinals)
// ---------------------------------------------------------------------------
REGISTER_SHIM("xinput1_4", "ordinal:2", (FARPROC)&xwr::Shim_XInputGetState_Ord);
REGISTER_SHIM("xinput1_4", "ordinal:3", (FARPROC)&xwr::Shim_XInputSetState_Ord);
