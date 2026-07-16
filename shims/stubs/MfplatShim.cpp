// shims/stubs/MfplatShim.cpp
// Stub for mfplat / mfreadwrite. MFStartup succeeds; everything else returns
// E_NOTIMPL so games that try to use Media Foundation can detect the absence
// and fall back to a different code path.

#include "UwpSdkIncludes.h"
#include <Windows.h>

#include "../ShimRegistry.h"

#ifndef MFSTARTUP_LITE
#define MFSTARTUP_LITE 0x1
#endif
#ifndef MF_API_VERSION
#define MF_API_VERSION 0x70
#endif

namespace xwr {

extern "C" HRESULT __stdcall Shim_MFStartup(ULONG Version, DWORD dwFlags) {
    (void)Version; (void)dwFlags;
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_MFShutdown() { return S_OK; }
extern "C" HRESULT __stdcall Shim_MFCreateAttributes(void** ppMFAttributes, UINT32 cInitialSize) {
    (void)cInitialSize;
    if (ppMFAttributes) *ppMFAttributes = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateMediaType(void** ppMFType) {
    if (ppMFType) *ppMFType = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateSample(void** ppMFSample) {
    if (ppMFSample) *ppMFSample = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateMediaBuffer(void** ppMFMediaBuffer) {
    if (ppMFMediaBuffer) *ppMFMediaBuffer = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateMemoryBuffer(DWORD cbMaxLength, void** ppBuffer) {
    (void)cbMaxLength;
    if (ppBuffer) *ppBuffer = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateEventQueue(void** ppMFEventQueue) {
    if (ppMFEventQueue) *ppMFEventQueue = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateSourceResolver(void** ppISourceResolver) {
    if (ppISourceResolver) *ppISourceResolver = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFGetService(void* punkObject, const GUID& guidService, const IID& riid, void** ppvObject) {
    (void)punkObject; (void)guidService; (void)riid;
    if (ppvObject) *ppvObject = nullptr;
    return E_NOTIMPL;
}

// mfreadwrite
extern "C" HRESULT __stdcall Shim_MFCreateSinkWriterFromURL(LPCWSTR, void*, void*, void** ppSinkWriter) {
    if (ppSinkWriter) *ppSinkWriter = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateSourceReaderFromURL(LPCWSTR, void*, void** ppSourceReader) {
    if (ppSourceReader) *ppSourceReader = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateSinkWriterFromMediaSink(void*, void*, void** ppSinkWriter) {
    if (ppSinkWriter) *ppSinkWriter = nullptr;
    return E_NOTIMPL;
}
extern "C" HRESULT __stdcall Shim_MFCreateSourceReaderFromByteStream(void*, void*, void** ppSourceReader) {
    if (ppSourceReader) *ppSourceReader = nullptr;
    return E_NOTIMPL;
}

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("mfplat", "MFStartup", (FARPROC)&xwr::Shim_MFStartup);
REGISTER_SHIM("mfplat", "MFShutdown", (FARPROC)&xwr::Shim_MFShutdown);
REGISTER_SHIM("mfplat", "MFCreateAttributes", (FARPROC)&xwr::Shim_MFCreateAttributes);
REGISTER_SHIM("mfplat", "MFCreateMediaType", (FARPROC)&xwr::Shim_MFCreateMediaType);
REGISTER_SHIM("mfplat", "MFCreateSample", (FARPROC)&xwr::Shim_MFCreateSample);
REGISTER_SHIM("mfplat", "MFCreateMediaBuffer", (FARPROC)&xwr::Shim_MFCreateMediaBuffer);
REGISTER_SHIM("mfplat", "MFCreateMemoryBuffer", (FARPROC)&xwr::Shim_MFCreateMemoryBuffer);
REGISTER_SHIM("mfplat", "MFCreateEventQueue", (FARPROC)&xwr::Shim_MFCreateEventQueue);
REGISTER_SHIM("mfplat", "MFCreateSourceResolver", (FARPROC)&xwr::Shim_MFCreateSourceResolver);
REGISTER_SHIM("mfplat", "MFGetService", (FARPROC)&xwr::Shim_MFGetService);

REGISTER_SHIM("mfreadwrite", "MFCreateSinkWriterFromURL", (FARPROC)&xwr::Shim_MFCreateSinkWriterFromURL);
REGISTER_SHIM("mfreadwrite", "MFCreateSourceReaderFromURL", (FARPROC)&xwr::Shim_MFCreateSourceReaderFromURL);
REGISTER_SHIM("mfreadwrite", "MFCreateSinkWriterFromMediaSink", (FARPROC)&xwr::Shim_MFCreateSinkWriterFromMediaSink);
REGISTER_SHIM("mfreadwrite", "MFCreateSourceReaderFromByteStream", (FARPROC)&xwr::Shim_MFCreateSourceReaderFromByteStream);
