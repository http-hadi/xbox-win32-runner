// shims/ole32/Ole32Shim.cpp
// Win32 ole32 / oleaut32 shim layer. Provides COM init, BSTR / VARIANT
// support, and CoCreateInstance that falls back to E_NOINTERFACE.

#include "UwpSdkIncludes.h"


#include <Windows.h>
#include <string>
#include <cstring>
#include <cstdlib>

#include "../ShimRegistry.h"

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#ifndef COINIT_APARTMENTTHREADED
#define COINIT_APARTMENTTHREADED 0x2
#endif
#ifndef COINIT_MULTITHREADED
#define COINIT_MULTITHREADED 0x0
#endif
#ifndef COINIT_DISABLE_OLE1DDE
#define COINIT_DISABLE_OLE1DDE 0x4
#endif
#ifndef COINIT_SPEED_OVER_MEMORY
#define COINIT_SPEED_OVER_MEMORY 0x8
#endif
#ifndef CLSCTX_INPROC_SERVER
#define CLSCTX_INPROC_SERVER 0x1
#endif
#ifndef CLSCTX_INPROC_HANDLER
#define CLSCTX_INPROC_HANDLER 0x2
#endif
#ifndef CLSCTX_LOCAL_SERVER
#define CLSCTX_LOCAL_SERVER 0x4
#endif
#ifndef CLSCTX_INPROC_SERVER16
#define CLSCTX_INPROC_SERVER16 0x8
#endif
#ifndef CLSCTX_ALL
#define CLSCTX_ALL (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER)
#endif
#ifndef VT_EMPTY
#define VT_EMPTY 0
#endif
#ifndef VT_NULL
#define VT_NULL 1
#endif
#ifndef STGM_READ
#define STGM_READ 0x00000000
#endif
#ifndef STGM_WRITE
#define STGM_WRITE 0x00000001
#endif
#ifndef STGM_READWRITE
#define STGM_READWRITE 0x00000002
#endif
#ifndef STGM_SHARE_DENY_NONE
#define STGM_SHARE_DENY_NONE 0x00000040
#endif
#ifndef STGM_SHARE_EXCLUSIVE
#define STGM_SHARE_EXCLUSIVE 0x00000010
#endif
#ifndef STGM_CREATE
#define STGM_CREATE 0x00001000
#endif
#ifndef STGM_FAILIFTHERE
#define STGM_FAILIFTHERE 0x00000000
#endif
#ifndef STGM_DIRECT
#define STGM_DIRECT 0x00000000
#endif

namespace xwr {

extern "C" HRESULT __stdcall Shim_CoInitialize(LPVOID) { return S_OK; }
extern "C" HRESULT __stdcall Shim_CoInitializeEx(LPVOID, DWORD) { return S_OK; }
extern "C" void __stdcall Shim_CoUninitialize() { }

extern "C" HRESULT __stdcall Shim_CoCreateInstance(const CLSID& rclsid, IUnknown* pUnkOuter,
                                                    DWORD dwClsContext, const IID& riid, void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    // Try the real CoCreateInstance. UWP supports a subset of CLSIDs (e.g. those
    // registered in the package manifest via <com:Class>). If it fails, return
    // E_NOINTERFACE so the caller can degrade gracefully.
    HRESULT hr = ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    if (SUCCEEDED(hr)) return hr;
    return E_NOINTERFACE;
}
extern "C" HRESULT __stdcall Shim_CoGetClassObject(const CLSID&, DWORD, void*, const IID&, void** ppv) {
    if (ppv) *ppv = nullptr;
    return E_NOINTERFACE;
}
extern "C" HRESULT __stdcall Shim_CoRegisterClassObject(DWORD, IUnknown*, DWORD, DWORD, LPDWORD pdwRegister) {
    if (pdwRegister) *pdwRegister = 1;
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_CoRevokeClassObject(DWORD) { return S_OK; }
extern "C" HRESULT __stdcall Shim_CoCreateGuid(GUID* pguid) {
    if (!pguid) return E_POINTER;
    return ::CoCreateGuid(pguid);
}

// ---------------------------------------------------------------------------
// Task memory allocator
// ---------------------------------------------------------------------------
extern "C" LPVOID __stdcall Shim_CoTaskMemAlloc(SIZE_T cb) {
    return std::malloc(cb);
}
extern "C" LPVOID __stdcall Shim_CoTaskMemRealloc(LPVOID pv, SIZE_T cb) {
    return std::realloc(pv, cb);
}
extern "C" void __stdcall Shim_CoTaskMemFree(LPVOID pv) {
    std::free(pv);
}

// ---------------------------------------------------------------------------
// BSTR support
// ---------------------------------------------------------------------------
extern "C" BSTR __stdcall Shim_SysAllocString(const OLECHAR* psz) {
    if (!psz) return nullptr;
    size_t len = std::wcslen(psz);
    size_t bytes = (len + 1) * sizeof(OLECHAR);
    // BSTR layout: 4-byte length prefix + WCHAR[] (no embedded null) + null terminator.
    void* block = std::malloc(sizeof(uint32_t) + bytes);
    if (!block) return nullptr;
    *static_cast<uint32_t*>(block) = (uint32_t)(len * sizeof(OLECHAR));
    void* str = static_cast<char*>(block) + sizeof(uint32_t);
    std::memcpy(str, psz, bytes);
    return (BSTR)str;
}
extern "C" BSTR __stdcall Shim_SysAllocStringLen(const OLECHAR* psz, UINT len) {
    size_t bytes = (size_t)(len + 1) * sizeof(OLECHAR);
    void* block = std::malloc(sizeof(uint32_t) + bytes);
    if (!block) return nullptr;
    *static_cast<uint32_t*>(block) = (uint32_t)(len * sizeof(OLECHAR));
    void* str = static_cast<char*>(block) + sizeof(uint32_t);
    if (psz) std::memcpy(str, psz, len * sizeof(OLECHAR));
    ((OLECHAR*)str)[len] = L'\0';
    return (BSTR)str;
}
extern "C" BSTR __stdcall Shim_SysAllocStringByteLen(LPCSTR psz, UINT len) {
    void* block = std::malloc(sizeof(uint32_t) + (size_t)len + 1);
    if (!block) return nullptr;
    *static_cast<uint32_t*>(block) = len;
    char* str = static_cast<char*>(block) + sizeof(uint32_t);
    if (psz) std::memcpy(str, psz, len);
    str[len] = '\0';
    return (BSTR)str;
}
extern "C" void __stdcall Shim_SysFreeString(BSTR bstr) {
    if (!bstr) return;
    void* block = reinterpret_cast<char*>(bstr) - sizeof(uint32_t);
    std::free(block);
}
extern "C" UINT __stdcall Shim_SysStringLen(BSTR bstr) {
    if (!bstr) return 0;
    uint32_t* pLen = (uint32_t*)(reinterpret_cast<char*>(bstr) - sizeof(uint32_t));
    return *pLen / sizeof(OLECHAR);
}
extern "C" UINT __stdcall Shim_SysStringByteLen(BSTR bstr) {
    if (!bstr) return 0;
    uint32_t* pLen = (uint32_t*)(reinterpret_cast<char*>(bstr) - sizeof(uint32_t));
    return *pLen;
}

// ---------------------------------------------------------------------------
// VARIANT helpers
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_VariantInit(VARIANTARG* pvarg) {
    if (!pvarg) return E_POINTER;
    std::memset(pvarg, 0, sizeof(VARIANTARG));
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_VariantClear(VARIANTARG* pvarg) {
    if (!pvarg) return E_POINTER;
    if (pvarg->vt == VT_BSTR && pvarg->bstrVal) {
        Shim_SysFreeString(pvarg->bstrVal);
    }
    std::memset(pvarg, 0, sizeof(VARIANTARG));
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_VariantCopy(VARIANTARG* pvargDest, const VARIANTARG* pvargSrc) {
    if (!pvargDest || !pvargSrc) return E_POINTER;
    std::memcpy(pvargDest, pvargSrc, sizeof(VARIANTARG));
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_VariantChangeType(VARIANTARG* pvargDest, const VARIANTARG* pvargSrc,
                                                     USHORT, VARTYPE vtNew) {
    if (!pvargDest) return E_POINTER;
    if (pvargSrc) std::memcpy(pvargDest, pvargSrc, sizeof(VARIANTARG));
    pvargDest->vt = vtNew;
    return S_OK;
}

// ---------------------------------------------------------------------------
// Stream / moniker
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_CreateStreamOnHGlobal(HGLOBAL, BOOL, IStream** ppstm) {
    if (ppstm) *ppstm = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_GetHGlobalFromStream(IStream*, HGLOBAL*) { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_CreateBindCtx(DWORD, IBindCtx** ppbc) {
    if (ppbc) *ppbc = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MkParseDisplayName(IBindCtx*, LPCWSTR, ULONG*, IMoniker** ppmk) {
    if (ppmk) *ppmk = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_CreateFileMoniker(LPCWSTR, IMoniker** ppmk) {
    if (ppmk) *ppmk = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_BindMoniker(IMoniker*, DWORD, const IID&, void**) { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_CreateGenericComposite(IMoniker*, IMoniker*, IMoniker** ppmk) {
    if (ppmk) *ppmk = nullptr;
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// OleInitialize / OleUninitialize
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_OleInitialize(LPVOID) { return S_OK; }
extern "C" void __stdcall Shim_OleUninitialize() { }
extern "C" HRESULT __stdcall Shim_RegisterDragDrop(HWND, IUnknown*) { return S_OK; }
extern "C" HRESULT __stdcall Shim_RevokeDragDrop(HWND) { return S_OK; }
extern "C" HRESULT __stdcall Shim_DoDragDrop(IDataObject*, IDropSource*, DWORD, LPDWORD pdwEffect) {
    if (pdwEffect) *pdwEffect = 0;
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// Structured storage
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_StgCreateDocfile(LPCWSTR, DWORD, DWORD, IStorage** ppstgOpen) {
    if (ppstgOpen) *ppstgOpen = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_StgOpenStorage(LPCWSTR, IStorage*, DWORD, SNB, DWORD, IStorage** ppstgOpen) {
    if (ppstgOpen) *ppstgOpen = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_StgIsStorageFile(LPCWSTR) { return S_FALSE; }

}  // namespace xwr

// ===========================================================================
// Registration — under both ole32 and oleaut32.
// ===========================================================================
REGISTER_SHIM("ole32", "CoInitialize", (FARPROC)&xwr::Shim_CoInitialize);
REGISTER_SHIM("ole32", "CoInitializeEx", (FARPROC)&xwr::Shim_CoInitializeEx);
REGISTER_SHIM("ole32", "CoUninitialize", (FARPROC)&xwr::Shim_CoUninitialize);
REGISTER_SHIM("ole32", "CoCreateInstance", (FARPROC)&xwr::Shim_CoCreateInstance);
REGISTER_SHIM("ole32", "CoGetClassObject", (FARPROC)&xwr::Shim_CoGetClassObject);
REGISTER_SHIM("ole32", "CoRegisterClassObject", (FARPROC)&xwr::Shim_CoRegisterClassObject);
REGISTER_SHIM("ole32", "CoRevokeClassObject", (FARPROC)&xwr::Shim_CoRevokeClassObject);
REGISTER_SHIM("ole32", "CoCreateGuid", (FARPROC)&xwr::Shim_CoCreateGuid);
REGISTER_SHIM("ole32", "CoTaskMemAlloc", (FARPROC)&xwr::Shim_CoTaskMemAlloc);
REGISTER_SHIM("ole32", "CoTaskMemRealloc", (FARPROC)&xwr::Shim_CoTaskMemRealloc);
REGISTER_SHIM("ole32", "CoTaskMemFree", (FARPROC)&xwr::Shim_CoTaskMemFree);
REGISTER_SHIM("ole32", "CreateStreamOnHGlobal", (FARPROC)&xwr::Shim_CreateStreamOnHGlobal);
REGISTER_SHIM("ole32", "GetHGlobalFromStream", (FARPROC)&xwr::Shim_GetHGlobalFromStream);
REGISTER_SHIM("ole32", "CreateBindCtx", (FARPROC)&xwr::Shim_CreateBindCtx);
REGISTER_SHIM("ole32", "MkParseDisplayName", (FARPROC)&xwr::Shim_MkParseDisplayName);
REGISTER_SHIM("ole32", "CreateFileMoniker", (FARPROC)&xwr::Shim_CreateFileMoniker);
REGISTER_SHIM("ole32", "BindMoniker", (FARPROC)&xwr::Shim_BindMoniker);
REGISTER_SHIM("ole32", "CreateGenericComposite", (FARPROC)&xwr::Shim_CreateGenericComposite);
REGISTER_SHIM("ole32", "OleInitialize", (FARPROC)&xwr::Shim_OleInitialize);
REGISTER_SHIM("ole32", "OleUninitialize", (FARPROC)&xwr::Shim_OleUninitialize);
REGISTER_SHIM("ole32", "RegisterDragDrop", (FARPROC)&xwr::Shim_RegisterDragDrop);
REGISTER_SHIM("ole32", "RevokeDragDrop", (FARPROC)&xwr::Shim_RevokeDragDrop);
REGISTER_SHIM("ole32", "DoDragDrop", (FARPROC)&xwr::Shim_DoDragDrop);
REGISTER_SHIM("ole32", "StgCreateDocfile", (FARPROC)&xwr::Shim_StgCreateDocfile);
REGISTER_SHIM("ole32", "StgOpenStorage", (FARPROC)&xwr::Shim_StgOpenStorage);
REGISTER_SHIM("ole32", "StgIsStorageFile", (FARPROC)&xwr::Shim_StgIsStorageFile);

REGISTER_SHIM("oleaut32", "SysAllocString", (FARPROC)&xwr::Shim_SysAllocString);
REGISTER_SHIM("oleaut32", "SysAllocStringLen", (FARPROC)&xwr::Shim_SysAllocStringLen);
REGISTER_SHIM("oleaut32", "SysAllocStringByteLen", (FARPROC)&xwr::Shim_SysAllocStringByteLen);
REGISTER_SHIM("oleaut32", "SysFreeString", (FARPROC)&xwr::Shim_SysFreeString);
REGISTER_SHIM("oleaut32", "SysStringLen", (FARPROC)&xwr::Shim_SysStringLen);
REGISTER_SHIM("oleaut32", "SysStringByteLen", (FARPROC)&xwr::Shim_SysStringByteLen);
REGISTER_SHIM("oleaut32", "VariantInit", (FARPROC)&xwr::Shim_VariantInit);
REGISTER_SHIM("oleaut32", "VariantClear", (FARPROC)&xwr::Shim_VariantClear);
REGISTER_SHIM("oleaut32", "VariantCopy", (FARPROC)&xwr::Shim_VariantCopy);
REGISTER_SHIM("oleaut32", "VariantChangeType", (FARPROC)&xwr::Shim_VariantChangeType);
