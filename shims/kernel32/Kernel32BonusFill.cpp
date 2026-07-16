// shims/kernel32/Kernel32BonusFill.cpp
// Additional KERNEL32 functions found missing on real Windows binaries
// (Python launcher w64.exe) that weren't in Half Sword's gap report but
// are common CRT-init functions.

#include "UwpSdkIncludes.h"

#include <Windows.h>
#include "../ShimRegistry.h"

namespace xwr {

// SearchPathW — UWP supports this for searching system directories.
extern "C" DWORD __stdcall Shim_SearchPathW(LPCWSTR path, LPCWSTR fileName, LPCWSTR ext,
                                              DWORD bufLen, LPWSTR buffer, LPWSTR* filePart) {
    return ::SearchPathW(path, fileName, ext, bufLen, buffer, filePart);
}

// Job objects — UWP blocks these. Stub to failure.
extern "C" HANDLE __stdcall Shim_CreateJobObjectA(LPSECURITY_ATTRIBUTES sa, LPCSTR name) {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return nullptr;
}
extern "C" HANDLE __stdcall Shim_CreateJobObjectW(LPSECURITY_ATTRIBUTES sa, LPCWSTR name) {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return nullptr;
}
extern "C" BOOL __stdcall Shim_AssignProcessToJobObject(HANDLE job, HANDLE proc) {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}
extern "C" BOOL __stdcall Shim_SetInformationJobObject(HANDLE job, int infoClass,
                                                         LPVOID info, DWORD infoLen) {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}
extern "C" BOOL __stdcall Shim_QueryInformationJobObject(HANDLE job, int infoClass,
                                                           LPVOID info, DWORD infoLen, LPDWORD retLen) {
    ::SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

// SetHandleCount — legacy Win16 API. Modern Windows ignores it and returns the
// value passed in. CRT init calls it with 512.
extern "C" UINT __stdcall Shim_SetHandleCount(UINT cHandles) {
    return cHandles;
}

}  // namespace xwr

REGISTER_SHIM("kernel32", "SearchPathW", (FARPROC)&xwr::Shim_SearchPathW);
REGISTER_SHIM("kernel32", "CreateJobObjectA", (FARPROC)&xwr::Shim_CreateJobObjectA);
REGISTER_SHIM("kernel32", "CreateJobObjectW", (FARPROC)&xwr::Shim_CreateJobObjectW);
REGISTER_SHIM("kernel32", "AssignProcessToJobObject", (FARPROC)&xwr::Shim_AssignProcessToJobObject);
REGISTER_SHIM("kernel32", "SetInformationJobObject", (FARPROC)&xwr::Shim_SetInformationJobObject);
REGISTER_SHIM("kernel32", "QueryInformationJobObject", (FARPROC)&xwr::Shim_QueryInformationJobObject);
REGISTER_SHIM("kernel32", "SetHandleCount", (FARPROC)&xwr::Shim_SetHandleCount);
REGISTER_SHIM("kernelbase", "SearchPathW", (FARPROC)&xwr::Shim_SearchPathW);
REGISTER_SHIM("kernelbase", "CreateJobObjectA", (FARPROC)&xwr::Shim_CreateJobObjectA);
REGISTER_SHIM("kernelbase", "CreateJobObjectW", (FARPROC)&xwr::Shim_CreateJobObjectW);
REGISTER_SHIM("kernelbase", "AssignProcessToJobObject", (FARPROC)&xwr::Shim_AssignProcessToJobObject);
REGISTER_SHIM("kernelbase", "SetInformationJobObject", (FARPROC)&xwr::Shim_SetInformationJobObject);
REGISTER_SHIM("kernelbase", "QueryInformationJobObject", (FARPROC)&xwr::Shim_QueryInformationJobObject);
REGISTER_SHIM("kernelbase", "SetHandleCount", (FARPROC)&xwr::Shim_SetHandleCount);
