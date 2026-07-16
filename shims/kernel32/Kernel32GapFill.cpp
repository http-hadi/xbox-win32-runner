// shims/kernel32/Kernel32GapFill.cpp
// Implements the remaining 313 KERNEL32.dll functions that Half Sword imports
// but were missing from Kernel32Shim.cpp.
//
// Most are direct pass-throughs to the real Win32 API (UWP supports them).
// A few are stubs returning reasonable defaults for APIs that UWP blocks:
//   - Job objects (QueryInformationJobObject)
//   - Toolhelp snapshot (CreateToolhelp32Snapshot, Process32*, Thread32*)
//   - PSAPI (K32*)
//   - Fibers (CreateFiber, etc.) — UE5 doesn't use fibers in practice
//
// All entries are registered for both "kernel32" and "kernelbase" because
// kernel32 forwards many of these to kernelbase, and Half Sword imports some
// of them under each name.
//
// Each function is `extern "C" __stdcall Shim_<OriginalName>` so the PE loader
// can drop its address directly into a game's IAT.

#include "UwpSdkIncludes.h"

// ---------------------------------------------------------------------------
// MSVC: <windows.h> already brings in most of the kernel32 surface, but a
// handful of file/IO declarations live in <fileapi.h> and the broader
// <winbase.h>. Re-including them is idempotent (guarded) and makes the
// dependency explicit for GetFileSize / MoveFileW / etc. used below.
// ---------------------------------------------------------------------------
#ifdef _MSC_VER
  #include <fileapi.h>
  #include <winbase.h>
  #pragma comment(lib, "kernel32.lib")
#endif

#include <Windows.h>
#include <atomic>
#include <mutex>
#include <cstring>

#include "../ShimRegistry.h"

namespace xwr {

// ---------------------------------------------------------------------------
// Static state for FLS (Fiber Local Storage) — UWP doesn't expose Fls*
// directly, so we keep a tiny in-process table. The slots hold a value +
// an optional destructor callback that fires on thread/fiber exit (we skip
// the destructor since UE5 rarely relies on it).
// ---------------------------------------------------------------------------
static std::mutex g_flsMutex;
static PFLS_CALLBACK_FUNCTION g_flsCallbacks[128] = {};
static DWORD g_flsNextSlot = 0;

// Per-thread FLS values: keyed by thread id → slot → value. UE5 typically
// only calls Fls* from one or two threads, so a small table is fine.
struct FlsValue { DWORD tid = 0; PVOID values[128] = {}; };
static constexpr size_t kFlsThreadTableCap = 64;
static FlsValue g_flsThreads[kFlsThreadTableCap];
static std::mutex g_flsThreadMutex;

static PVOID FlsGetThreadValue(DWORD slot) {
    DWORD tid = ::GetCurrentThreadId();
    std::lock_guard<std::mutex> lk(g_flsThreadMutex);
    for (auto& tv : g_flsThreads) {
        if (tv.tid == tid) return tv.values[slot];
    }
    return nullptr;
}

static void FlsSetThreadValue(DWORD slot, PVOID v) {
    DWORD tid = ::GetCurrentThreadId();
    std::lock_guard<std::mutex> lk(g_flsThreadMutex);
    for (auto& tv : g_flsThreads) {
        if (tv.tid == tid) { tv.values[slot] = v; return; }
    }
    for (auto& tv : g_flsThreads) {
        if (tv.tid == 0) { tv.tid = tid; tv.values[slot] = v; return; }
    }
}

// ===========================================================================
// SRW Locks (7)
// ===========================================================================
extern "C" VOID __stdcall Shim_InitializeSRWLock(PSRWLOCK s)            { ::InitializeSRWLock(s); }
extern "C" VOID __stdcall Shim_AcquireSRWLockExclusive(PSRWLOCK s)      { ::AcquireSRWLockExclusive(s); }
extern "C" VOID __stdcall Shim_AcquireSRWLockShared(PSRWLOCK s)         { ::AcquireSRWLockShared(s); }
extern "C" VOID __stdcall Shim_ReleaseSRWLockExclusive(PSRWLOCK s)      { ::ReleaseSRWLockExclusive(s); }
extern "C" VOID __stdcall Shim_ReleaseSRWLockShared(PSRWLOCK s)         { ::ReleaseSRWLockShared(s); }
extern "C" BOOLEAN __stdcall Shim_TryAcquireSRWLockExclusive(PSRWLOCK s){ return ::TryAcquireSRWLockExclusive(s); }

// ===========================================================================
// Condition Variables (5)
// ===========================================================================
extern "C" VOID __stdcall Shim_InitializeConditionVariable(PCONDITION_VARIABLE cv) { ::InitializeConditionVariable(cv); }
extern "C" VOID __stdcall Shim_WakeConditionVariable(PCONDITION_VARIABLE cv)       { ::WakeConditionVariable(cv); }
extern "C" VOID __stdcall Shim_WakeAllConditionVariable(PCONDITION_VARIABLE cv)    { ::WakeAllConditionVariable(cv); }
extern "C" BOOL  __stdcall Shim_SleepConditionVariableCS(PCONDITION_VARIABLE cv, PCRITICAL_SECTION cs, DWORD ms)  { return ::SleepConditionVariableCS(cv, cs, ms); }
extern "C" BOOL  __stdcall Shim_SleepConditionVariableSRW(PCONDITION_VARIABLE cv, PSRWLOCK s, DWORD ms, ULONG f)   { return ::SleepConditionVariableSRW(cv, s, ms, f); }

// ===========================================================================
// Critical-section extensions (3)
// ===========================================================================
extern "C" BOOL  __stdcall Shim_InitializeCriticalSectionEx(LPCRITICAL_SECTION cs, DWORD spin, DWORD flags) { return ::InitializeCriticalSectionEx(cs, spin, flags); }
extern "C" DWORD __stdcall Shim_SetCriticalSectionSpinCount(LPCRITICAL_SECTION cs, DWORD spin)              { return ::SetCriticalSectionSpinCount(cs, spin); }
extern "C" BOOL  __stdcall Shim_TryEnterCriticalSection(LPCRITICAL_SECTION cs)                              { return ::TryEnterCriticalSection(cs); }

// ===========================================================================
// SList (5)
// ===========================================================================
extern "C" VOID         __stdcall Shim_InitializeSListHead(PSLIST_HEADER h)                         { ::InitializeSListHead(h); }
extern "C" PSLIST_ENTRY __stdcall Shim_InterlockedFlushSList(PSLIST_HEADER h)                       { return ::InterlockedFlushSList(h); }
extern "C" PSLIST_ENTRY __stdcall Shim_InterlockedPopEntrySList(PSLIST_HEADER h)                              { return ::InterlockedPopEntrySList(h); }
extern "C" PSLIST_ENTRY __stdcall Shim_InterlockedPushEntrySList(PSLIST_HEADER h, PSLIST_ENTRY e)   { return ::InterlockedPushEntrySList(h, e); }
extern "C" USHORT       __stdcall Shim_QueryDepthSList(PSLIST_HEADER h)                             { return ::QueryDepthSList(h); }

// ===========================================================================
// InitOnce (3)
// ===========================================================================
extern "C" BOOL __stdcall Shim_InitOnceBeginInitialize(PINIT_ONCE o, DWORD f, PBOOL s, LPVOID* ctx)    { return ::InitOnceBeginInitialize(o, f, s, ctx); }
extern "C" BOOL __stdcall Shim_InitOnceComplete(PINIT_ONCE o, DWORD f, LPVOID ctx)                     { return ::InitOnceComplete(o, f, ctx); }
extern "C" BOOL __stdcall Shim_InitOnceExecuteOnce(PINIT_ONCE o, PINIT_ONCE_FN fn, PVOID p, LPVOID* c) { return ::InitOnceExecuteOnce(o, fn, p, c); }

// ===========================================================================
// Interlocked (4) — std::atomic under the hood; signatures match Win32.
// ===========================================================================
extern "C" LONG __stdcall Shim_InterlockedCompareExchange(LONG volatile* d, LONG e, LONG c) {
    std::atomic<LONG>* a = reinterpret_cast<std::atomic<LONG>*>(const_cast<LONG*>(d));
    return a->compare_exchange_strong(*(reinterpret_cast<LONG*>(&c)), e) ? *a : *a;  // dummy use to avoid unused warnings
}
extern "C" LONG __stdcall Shim_InterlockedDecrement(LONG volatile* d) {
    std::atomic<LONG>* a = reinterpret_cast<std::atomic<LONG>*>(const_cast<LONG*>(d));
    return a->fetch_sub(1, std::memory_order_acq_rel) - 1;
}
extern "C" LONG __stdcall Shim_InterlockedExchange(LONG volatile* d, LONG v) {
    std::atomic<LONG>* a = reinterpret_cast<std::atomic<LONG>*>(const_cast<LONG*>(d));
    return a->exchange(v);
}
extern "C" LONG __stdcall Shim_InterlockedIncrement(LONG volatile* d) {
    std::atomic<LONG>* a = reinterpret_cast<std::atomic<LONG>*>(const_cast<LONG*>(d));
    return a->fetch_add(1, std::memory_order_acq_rel) + 1;
}

// ===========================================================================
// TLS (4) — pass through.
// ===========================================================================
extern "C" DWORD  __stdcall Shim_TlsAlloc()                     { return ::TlsAlloc(); }
extern "C" BOOL   __stdcall Shim_TlsFree(DWORD slot)            { return ::TlsFree(slot); }
extern "C" LPVOID __stdcall Shim_TlsGetValue(DWORD slot)        { return ::TlsGetValue(slot); }
extern "C" BOOL   __stdcall Shim_TlsSetValue(DWORD slot, LPVOID v) { return ::TlsSetValue(slot, v); }

// ===========================================================================
// FLS (4) — in-process table (see top of file).
// ===========================================================================
extern "C" DWORD __stdcall Shim_FlsAlloc(PFLS_CALLBACK_FUNCTION cb) {
    std::lock_guard<std::mutex> lk(g_flsMutex);
    if (g_flsNextSlot >= 128) { ::SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FLS_OUT_OF_INDEXES; }
    DWORD slot = g_flsNextSlot++;
    g_flsCallbacks[slot] = cb;
    return slot;
}
extern "C" BOOL __stdcall Shim_FlsFree(DWORD slot) {
    std::lock_guard<std::mutex> lk(g_flsMutex);
    if (slot >= 128) { ::SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    g_flsCallbacks[slot] = nullptr;
    return TRUE;
}
extern "C" PVOID __stdcall Shim_FlsGetValue(DWORD slot) {
    if (slot >= 128) { ::SetLastError(ERROR_INVALID_PARAMETER); return nullptr; }
    return FlsGetThreadValue(slot);
}
extern "C" BOOL __stdcall Shim_FlsSetValue(DWORD slot, PVOID v) {
    if (slot >= 128) { ::SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    FlsSetThreadValue(slot, v);
    return TRUE;
}

// ===========================================================================
// Fibers (7) — stub (UE5 doesn't use fibers; UWP doesn't expose them).
// ===========================================================================
extern "C" LPVOID __stdcall Shim_CreateFiber(SIZE_T, LPFIBER_START_ROUTINE, LPVOID)             { ::SetLastError(ERROR_NOT_SUPPORTED); return nullptr; }
extern "C" BOOL   __stdcall Shim_ConvertFiberToThread()                                          { return TRUE; }
extern "C" LPVOID __stdcall Shim_ConvertThreadToFiber(LPVOID)                                    { return reinterpret_cast<LPVOID>(1); }
extern "C" LPVOID __stdcall Shim_ConvertThreadToFiberEx(LPVOID, DWORD)                           { return reinterpret_cast<LPVOID>(1); }
extern "C" VOID   __stdcall Shim_DeleteFiber(LPVOID)                                             { /* no-op */ }
extern "C" VOID   __stdcall Shim_SwitchToFiber(LPVOID)                                           { /* no-op */ }
extern "C" BOOL   __stdcall Shim_IsThreadAFiber()                                                { return FALSE; }

// ===========================================================================
// Vectored exception / Unhandled exception (5)
// ===========================================================================
extern "C" PVOID __stdcall Shim_AddVectoredExceptionHandler(ULONG first, PVECTORED_EXCEPTION_HANDLER h) { return ::AddVectoredExceptionHandler(first, h); }
extern "C" ULONG __stdcall Shim_RemoveVectoredExceptionHandler(PVOID h)                                  { return ::RemoveVectoredExceptionHandler(h); }
extern "C" VOID  __stdcall Shim_RaiseException(DWORD code, DWORD flags, DWORD n, const ULONG_PTR* args) { ::RaiseException(code, flags, n, args); }
extern "C" LONG  __stdcall Shim_UnhandledExceptionFilter(LPEXCEPTION_POINTERS ep)                        { return ::UnhandledExceptionFilter(ep); }
extern "C" BOOL  __stdcall Shim_SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER f)              { return ::SetUnhandledExceptionFilter(f) ? TRUE : FALSE; }

// ===========================================================================
// RTL unwind (7)
// ===========================================================================
extern "C" VOID                          __stdcall Shim_RtlCaptureContext(PCONTEXT c)                          { ::RtlCaptureContext(c); }
extern "C" void                          __stdcall Shim_RtlCaptureStackBackTrace(ULONG skip, ULONG n, PVOID* b, PULONG h) { ::RtlCaptureStackBackTrace(skip, n, b, h); }
extern "C" PIMAGE_RUNTIME_FUNCTION_ENTRY __stdcall Shim_RtlLookupFunctionEntry(DWORD64 pc, PDWORD64 base, PUNWIND_HISTORY_TABLE entry) { return ::RtlLookupFunctionEntry(pc, base, entry); }
extern "C" PVOID                         __stdcall Shim_RtlPcToFileHeader(PVOID pc, PVOID* base)              { return ::RtlPcToFileHeader(pc, base); }
extern "C" VOID                          __stdcall Shim_RtlUnwind(PVOID a, PVOID b, PEXCEPTION_RECORD r, PVOID c)        { ::RtlUnwind(a, b, r, c); }
extern "C" VOID                          __stdcall Shim_RtlUnwindEx(PVOID a, PVOID b, PEXCEPTION_RECORD r, PVOID c, PCONTEXT ctx, PUNWIND_HISTORY_TABLE t) { ::RtlUnwindEx(a, b, r, c, ctx, t); }
extern "C" PEXCEPTION_ROUTINE            __stdcall Shim_RtlVirtualUnwind(DWORD v, DWORD64 a, DWORD64 b, PIMAGE_RUNTIME_FUNCTION_ENTRY e, PCONTEXT c, PVOID* p, PULONG_PTR s, PKNONVOLATILE_CONTEXT_POINTERS r) { return ::RtlVirtualUnwind(v, a, b, e, c, p, s, r); }

// ===========================================================================
// Process / thread info (many)
// ===========================================================================
extern "C" VOID  __stdcall Shim_ExitProcess(UINT code)                { ::TerminateProcess(::GetCurrentProcess(), code); }
extern "C" VOID  __stdcall Shim_ExitThread(DWORD code)                { ::ExitThread(code); }
extern "C" BOOL  __stdcall Shim_CreateProcessA(LPCSTR a, LPSTR b, LPSECURITY_ATTRIBUTES c, LPSECURITY_ATTRIBUTES d, BOOL e, DWORD f, LPVOID g, LPCSTR h, LPSTARTUPINFOA i, LPPROCESS_INFORMATION j) { return ::CreateProcessA(a, b, c, d, e, f, g, h, i, j); }
extern "C" BOOL  __stdcall Shim_CreateProcessW(LPCWSTR a, LPWSTR b, LPSECURITY_ATTRIBUTES c, LPSECURITY_ATTRIBUTES d, BOOL e, DWORD f, LPVOID g, LPCWSTR h, LPSTARTUPINFOW i, LPPROCESS_INFORMATION j) { return ::CreateProcessW(a, b, c, d, e, f, g, h, i, j); }
extern "C" BOOL  __stdcall Shim_GetExitCodeProcess(HANDLE h, LPDWORD c)        { return ::GetExitCodeProcess(h, c); }
extern "C" BOOL  __stdcall Shim_GetExitCodeThread(HANDLE h, LPDWORD c)         { return ::GetExitCodeThread(h, c); }
extern "C" BOOL  __stdcall Shim_GetProcessTimes(HANDLE h, LPFILETIME a, LPFILETIME b, LPFILETIME c, LPFILETIME d) { return ::GetProcessTimes(h, a, b, c, d); }
extern "C" BOOL  __stdcall Shim_GetThreadTimes(HANDLE h, LPFILETIME a, LPFILETIME b, LPFILETIME c, LPFILETIME d)  { return ::GetThreadTimes(h, a, b, c, d); }
extern "C" DWORD __stdcall Shim_GetProcessId(HANDLE h)                        { return ::GetProcessId(h); }
extern "C" DWORD __stdcall Shim_GetThreadId(HANDLE h)                         { return ::GetThreadId(h); }
extern "C" BOOL  __stdcall Shim_GetProcessHandleCount(HANDLE h, PDWORD c)     { return ::GetProcessHandleCount(h, c); }
extern "C" DWORD __stdcall Shim_GetPriorityClass(HANDLE h)                    { return ::GetPriorityClass(h); }
extern "C" BOOL  __stdcall Shim_QueryFullProcessImageNameW(HANDLE h, DWORD f, LPWSTR buf, PDWORD sz) { return ::QueryFullProcessImageNameW(h, f, buf, sz); }
extern "C" BOOL  __stdcall Shim_IsProcessInJob(HANDLE p, HANDLE j, PBOOL r)   { *r = FALSE; return TRUE; }
extern "C" BOOL  __stdcall Shim_ProcessIdToSessionId(DWORD pid, PDWORD sid)   { return ::ProcessIdToSessionId(pid, sid); }
extern "C" BOOL  __stdcall Shim_TerminateProcess(HANDLE h, UINT c)            { return ::TerminateProcess(h, c); }
extern "C" BOOL  __stdcall Shim_TerminateThread(HANDLE h, DWORD c)            { return ::TerminateThread(h, c); }
extern "C" DWORD __stdcall Shim_ResumeThread(HANDLE h)                        { return ::ResumeThread(h); }
extern "C" DWORD __stdcall Shim_SuspendThread(HANDLE h)                       { return ::SuspendThread(h); }
extern "C" HANDLE __stdcall Shim_OpenProcess(DWORD a, BOOL b, DWORD c)        { return ::OpenProcess(a, b, c); }
extern "C" HANDLE __stdcall Shim_OpenThread(DWORD a, BOOL b, DWORD c)         { return ::OpenThread(a, b, c); }
extern "C" BOOL  __stdcall Shim_DuplicateHandle(HANDLE a, HANDLE b, HANDLE c, LPHANDLE d, DWORD e, BOOL f, DWORD g) { return ::DuplicateHandle(a, b, c, d, e, f, g); }
extern "C" BOOL  __stdcall Shim_GetProcessAffinityMask(HANDLE h, PDWORD_PTR m, PDWORD_PTR s) { return ::GetProcessAffinityMask(h, m, s); }

// ===========================================================================
// Thread control (16)
// ===========================================================================
extern "C" BOOL   __stdcall Shim_SetThreadPriority(HANDLE h, int p)             { return ::SetThreadPriority(h, p); }
extern "C" int    __stdcall Shim_GetThreadPriority(HANDLE h)                    { return ::GetThreadPriority(h); }
extern "C" DWORD_PTR __stdcall Shim_SetThreadAffinityMask(HANDLE h, DWORD_PTR m){ return ::SetThreadAffinityMask(h, m); }
extern "C" DWORD_PTR __stdcall Shim_SetThreadIdealProcessor(HANDLE h, DWORD p)  { return ::SetThreadIdealProcessor(h, p); }
extern "C" BOOL   __stdcall Shim_GetThreadIdealProcessorEx(HANDLE h, PPROCESSOR_NUMBER out) { return ::GetThreadIdealProcessorEx(h, out); }
extern "C" BOOL   __stdcall Shim_SetThreadGroupAffinity(HANDLE h, const GROUP_AFFINITY* in, PGROUP_AFFINITY out) { return ::SetThreadGroupAffinity(h, in, out); }
extern "C" BOOL   __stdcall Shim_SetThreadStackGuarantee(PULONG sz)             { return ::SetThreadStackGuarantee(sz); }
extern "C" HRESULT __stdcall Shim_SetThreadDescription(HANDLE h, PCWSTR d)      { return ::SetThreadDescription(h, d); }
extern "C" DWORD  __stdcall Shim_GetCurrentProcessorNumber()                    { return ::GetCurrentProcessorNumber(); }
extern "C" VOID   __stdcall Shim_GetCurrentProcessorNumberEx(PPROCESSOR_NUMBER p) { ::GetCurrentProcessorNumberEx(p); }
extern "C" VOID   __stdcall Shim_GetCurrentThreadStackLimits(PULONG_PTR low, PULONG_PTR high) { ::GetCurrentThreadStackLimits(low, high); }
extern "C" DWORD_PTR __stdcall Shim_GetActiveProcessorCount(WORD g)             { return ::GetActiveProcessorCount(g); }
extern "C" DWORD  __stdcall Shim_GetMaximumProcessorCount(WORD g)               { return ::GetMaximumProcessorCount(g); }
extern "C" EXECUTION_STATE __stdcall Shim_SetThreadExecutionState(EXECUTION_STATE s) { return ::SetThreadExecutionState(s); }
extern "C" BOOL   __stdcall Shim_GetThreadContext(HANDLE h, PCONTEXT c)         { return ::GetThreadContext(h, c); }
extern "C" LCID   __stdcall Shim_GetThreadLocale()                              { return ::GetThreadLocale(); }

// ===========================================================================
// NUMA (5)
// ===========================================================================
extern "C" BOOL   __stdcall Shim_GetNumaHighestNodeNumber(PULONG n)              { return ::GetNumaHighestNodeNumber(n); }
extern "C" BOOL   __stdcall Shim_GetNumaNodeProcessorMask(UCHAR n, PULONGLONG m) { return ::GetNumaNodeProcessorMask(n, m); }
extern "C" BOOL   __stdcall Shim_GetNumaNodeProcessorMaskEx(USHORT n, PGROUP_AFFINITY m) { return ::GetNumaNodeProcessorMaskEx(n, m); }
extern "C" BOOL   __stdcall Shim_GetNumaProcessorNode(UCHAR p, PUCHAR n)             { return ::GetNumaProcessorNode(p, n); }
extern "C" BOOL   __stdcall Shim_GetNumaProcessorNodeEx(PPROCESSOR_NUMBER p, PUSHORT n) { return ::GetNumaProcessorNodeEx(p, n); }

// ===========================================================================
// Logical processor info (2)
// ===========================================================================
extern "C" BOOL __stdcall Shim_GetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buf, PDWORD len) { return ::GetLogicalProcessorInformation(buf, len); }
extern "C" BOOL __stdcall Shim_GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP r, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buf, PDWORD len) { return ::GetLogicalProcessorInformationEx(r, buf, len); }

// ===========================================================================
// File ops / mapping (many)
// ===========================================================================
extern "C" HRESULT __stdcall Shim_CopyFile2(LPCWSTR a, LPCWSTR b, COPYFILE2_EXTENDED_PARAMETERS* p)   { return ::CopyFile2(a, b, p); }
extern "C" HANDLE  __stdcall Shim_CreateFile2(LPCWSTR a, DWORD b, DWORD c, DWORD d, const CREATEFILE2_EXTENDED_PARAMETERS* e) { return ::CreateFile2(a, b, c, d, const_cast<LPCREATEFILE2_EXTENDED_PARAMETERS>(e)); }
extern "C" HANDLE  __stdcall Shim_CreateFileMappingA(HANDLE a, LPSECURITY_ATTRIBUTES b, DWORD c, DWORD d, DWORD e, LPCSTR f)   { return ::CreateFileMappingA(a, b, c, d, e, f); }
extern "C" HANDLE  __stdcall Shim_CreateFileMappingW(HANDLE a, LPSECURITY_ATTRIBUTES b, DWORD c, DWORD d, DWORD e, LPCWSTR f)  { return ::CreateFileMappingW(a, b, c, d, e, f); }
extern "C" LPVOID  __stdcall Shim_MapViewOfFile(HANDLE a, DWORD b, DWORD c, DWORD d, SIZE_T e)        { return ::MapViewOfFile(a, b, c, d, e); }
extern "C" LPVOID  __stdcall Shim_MapViewOfFileEx(HANDLE a, DWORD b, DWORD c, DWORD d, SIZE_T e, LPVOID f) { return ::MapViewOfFileEx(a, b, c, d, e, f); }
extern "C" BOOL    __stdcall Shim_UnmapViewOfFile(LPCVOID p)                                          { return ::UnmapViewOfFile(p); }
extern "C" BOOL    __stdcall Shim_FlushViewOfFile(LPCVOID p, SIZE_T n)                                { return ::FlushViewOfFile(p, n); }
extern "C" BOOL    __stdcall Shim_FlushInstructionCache(HANDLE h, LPCVOID p, SIZE_T n)                { return ::FlushInstructionCache(h, p, n); }
extern "C" BOOL    __stdcall Shim_VirtualUnlock(LPVOID p, SIZE_T n)                                   { return ::VirtualUnlock(p, n); }
extern "C" BOOL    __stdcall Shim_GetFileInformationByHandle(HANDLE h, LPBY_HANDLE_FILE_INFORMATION i) { return ::GetFileInformationByHandle(h, i); }
extern "C" BOOL    __stdcall Shim_GetFileInformationByHandleEx(HANDLE h, FILE_INFO_BY_HANDLE_CLASS c, LPVOID b, DWORD n) { return ::GetFileInformationByHandleEx(h, c, b, n); }
extern "C" BOOL    __stdcall Shim_SetFileInformationByHandle(HANDLE h, FILE_INFO_BY_HANDLE_CLASS c, LPVOID b, DWORD n)   { return ::SetFileInformationByHandle(h, c, b, n); }
extern "C" DWORD   __stdcall Shim_GetFileAttributesA(LPCSTR p)                                        { return ::GetFileAttributesA(p); }
extern "C" BOOL    __stdcall Shim_GetFileTime(HANDLE h, LPFILETIME a, LPFILETIME b, LPFILETIME c)     { return ::GetFileTime(h, a, b, c); }
extern "C" BOOL    __stdcall Shim_SetFileTime(HANDLE h, const FILETIME* a, const FILETIME* b, const FILETIME* c) { return ::SetFileTime(h, a, b, c); }
extern "C" DWORD   __stdcall Shim_GetFileType(HANDLE h)                                               { return ::GetFileType(h); }
extern "C" DWORD   __stdcall Shim_GetFinalPathNameByHandleW(HANDLE h, LPWSTR b, DWORD n, DWORD f)     { return ::GetFinalPathNameByHandleW(h, b, n, f); }
extern "C" BOOL    __stdcall Shim_SetFileCompletionNotificationModes(HANDLE h, UCHAR f)               { return ::SetFileCompletionNotificationModes(h, f); }
extern "C" BOOL    __stdcall Shim_SetEndOfFile(HANDLE h)                                              { return ::SetEndOfFile(h); }
extern "C" HANDLE  __stdcall Shim_FindFirstFileExA(LPCSTR a, FINDEX_INFO_LEVELS b, LPVOID c, FINDEX_SEARCH_OPS d, LPVOID e, DWORD f) { return ::FindFirstFileExA(a, b, c, d, e, f); }
extern "C" BOOL    __stdcall Shim_FindNextFileA(HANDLE h, LPWIN32_FIND_DATAA d)                       { return ::FindNextFileA(h, d); }
extern "C" HRSRC   __stdcall Shim_FindResourceW(HMODULE m, LPCWSTR a, LPCWSTR b)                     { return ::FindResourceW(m, a, b); }
extern "C" HGLOBAL __stdcall Shim_LoadResource(HMODULE m, HRSRC r)                                    { return ::LoadResource(m, r); }
extern "C" LPVOID  __stdcall Shim_LockResource(HGLOBAL r)                                             { return ::LockResource(r); }
extern "C" DWORD   __stdcall Shim_SizeofResource(HMODULE m, HRSRC r)                                  { return ::SizeofResource(m, r); }
extern "C" HANDLE  __stdcall Shim_ReOpenFile(HANDLE a, DWORD b, DWORD c, DWORD d)                     { return ::ReOpenFile(a, b, c, d); }
extern "C" BOOL    __stdcall Shim_ReplaceFileW(LPCWSTR a, LPCWSTR b, LPCWSTR c, DWORD d, LPVOID e, LPVOID f) { return ::ReplaceFileW(a, b, c, d, e, f); }
extern "C" BOOL    __stdcall Shim_MoveFileExA(LPCSTR a, LPCSTR b, DWORD f)                            { return ::MoveFileExA(a, b, f); }
extern "C" BOOL    __stdcall Shim_AreFileApisANSI()                                                   { return ::AreFileApisANSI(); }
extern "C" HANDLE  __stdcall Shim_CreateNamedPipeW(LPCWSTR a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, LPSECURITY_ATTRIBUTES h) { return ::CreateNamedPipeW(a, b, c, d, e, f, g, h); }
extern "C" BOOL    __stdcall Shim_ConnectNamedPipe(HANDLE h, LPOVERLAPPED o)                          { return ::ConnectNamedPipe(h, o); }
extern "C" BOOL    __stdcall Shim_PeekNamedPipe(HANDLE h, LPVOID b, DWORD c, LPDWORD d, LPDWORD e, LPDWORD f) { return ::PeekNamedPipe(h, b, c, d, e, f); }
extern "C" BOOL    __stdcall Shim_SetNamedPipeHandleState(HANDLE h, LPDWORD a, LPDWORD b, LPDWORD c)  { return ::SetNamedPipeHandleState(h, a, b, c); }
extern "C" BOOL    __stdcall Shim_GetNamedPipeClientProcessId(HANDLE h, PULONG p)                     { return ::GetNamedPipeClientProcessId(h, p); }
extern "C" BOOL    __stdcall Shim_GetNamedPipeServerProcessId(HANDLE h, PULONG p)                     { return ::GetNamedPipeServerProcessId(h, p); }
// Arg 5 (lpCollectDataTimeout) is LPDWORD, not LPWSTR — matches the real
// GetNamedPipeHandleStateW signature in <winbase.h>.
extern "C" BOOL    __stdcall Shim_GetNamedPipeHandleStateW(HANDLE h, LPDWORD a, LPDWORD b, LPDWORD c, LPDWORD d, LPWSTR e, DWORD f) { return ::GetNamedPipeHandleStateW(h, a, b, c, d, e, f); }
extern "C" BOOL    __stdcall Shim_WaitNamedPipeW(LPCWSTR a, DWORD b)                                  { return ::WaitNamedPipeW(a, b); }
extern "C" BOOL    __stdcall Shim_CreatePipe(PHANDLE a, PHANDLE b, LPSECURITY_ATTRIBUTES c, DWORD d)  { return ::CreatePipe(a, b, c, d); }
extern "C" BOOL    __stdcall Shim_DeviceIoControl(HANDLE h, DWORD c, LPVOID a, DWORD b, LPVOID o, DWORD e, LPDWORD f, LPOVERLAPPED g) { return ::DeviceIoControl(h, c, a, b, o, e, f, g); }
extern "C" BOOL    __stdcall Shim_ReadDirectoryChangesW(HANDLE h, LPVOID b, DWORD c, BOOL d, DWORD e, LPDWORD f, LPOVERLAPPED g, LPOVERLAPPED_COMPLETION_ROUTINE r) { return ::ReadDirectoryChangesW(h, b, c, d, e, f, g, r); }

// ===========================================================================
// IO completion (8)
// ===========================================================================
extern "C" HANDLE __stdcall Shim_CreateIoCompletionPort(HANDLE a, HANDLE b, ULONG_PTR c, DWORD d)    { return ::CreateIoCompletionPort(a, b, c, d); }
extern "C" BOOL   __stdcall Shim_GetQueuedCompletionStatus(HANDLE h, LPDWORD n, PULONG_PTR k, LPOVERLAPPED* o, DWORD ms) { return ::GetQueuedCompletionStatus(h, n, k, o, ms); }
// GetQueuedCompletionStatusEx arg 6 is BOOL (not PBOOL) — dereference r into
// a temporary so we pass a plain BOOL to the real API.
extern "C" BOOL   __stdcall Shim_GetQueuedCompletionStatusEx(HANDLE h, LPOVERLAPPED_ENTRY e, ULONG c, PULONG n, DWORD ms, PBOOL r) {
    BOOL alertable = r ? *r : FALSE;
    BOOL ok = ::GetQueuedCompletionStatusEx(h, e, c, n, ms, alertable);
    return ok;
}
extern "C" BOOL   __stdcall Shim_PostQueuedCompletionStatus(HANDLE h, DWORD n, ULONG_PTR k, LPOVERLAPPED o) { return ::PostQueuedCompletionStatus(h, n, k, o); }
extern "C" BOOL   __stdcall Shim_CancelIo(HANDLE h)                                                   { return ::CancelIo(h); }
extern "C" BOOL   __stdcall Shim_CancelIoEx(HANDLE h, LPOVERLAPPED o)                                 { return ::CancelIoEx(h, o); }
extern "C" BOOL   __stdcall Shim_CancelSynchronousIo(HANDLE h)                                        { return ::CancelSynchronousIo(h); }
extern "C" BOOL   __stdcall Shim_GetOverlappedResult(HANDLE h, LPOVERLAPPED o, LPDWORD n, BOOL w)     { return ::GetOverlappedResult(h, o, n, w); }

// ===========================================================================
// Waitable timers (5)
// ===========================================================================
extern "C" HANDLE __stdcall Shim_CreateWaitableTimerA(LPSECURITY_ATTRIBUTES a, BOOL b, LPCSTR c)      { return ::CreateWaitableTimerA(a, b, c); }
extern "C" HANDLE __stdcall Shim_CreateWaitableTimerW(LPSECURITY_ATTRIBUTES a, BOOL b, LPCWSTR c)     { return ::CreateWaitableTimerW(a, b, c); }
extern "C" HANDLE __stdcall Shim_CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES a, LPCWSTR b, DWORD c, DWORD d) { return ::CreateWaitableTimerExW(a, b, c, d); }
extern "C" BOOL   __stdcall Shim_CancelWaitableTimer(HANDLE h)                                        { return ::CancelWaitableTimer(h); }
// SetWaitableTimer arg 4 is PTIMERAPCROUTINE on MSVC — cast cb to that exact
// type so MSVC accepts the call (void* → PTIMERAPCROUTINE isn't an implicit
// conversion). On the Linux stub, the signature already takes void*.
extern "C" BOOL   __stdcall Shim_SetWaitableTimer(HANDLE h, const LARGE_INTEGER* t, LONG p, void* cb, LPVOID a, BOOL r) {
#ifdef _MSC_VER
    return ::SetWaitableTimer(h, t, p, reinterpret_cast<PTIMERAPCROUTINE>(cb), a, r);
#else
    return ::SetWaitableTimer(h, t, p, cb, a, r);
#endif
}

// ===========================================================================
// Thread pool / timer queue / wait registration (12)
// ===========================================================================
extern "C" PTP_WAIT __stdcall Shim_CreateThreadpoolWait(PTP_WAIT_CALLBACK a, PVOID b, PVOID c)        { return ::CreateThreadpoolWait(a, b, reinterpret_cast<PTP_CALLBACK_ENVIRON>(c)); }
extern "C" VOID     __stdcall Shim_SetThreadpoolWait(PTP_WAIT a, HANDLE b, PVOID c)                   { ::SetThreadpoolWait(a, b, reinterpret_cast<PFILETIME>(c)); }
extern "C" VOID     __stdcall Shim_WaitForThreadpoolWaitCallbacks(PTP_WAIT a, BOOL b)                 { ::WaitForThreadpoolWaitCallbacks(a, b); }
extern "C" VOID     __stdcall Shim_CloseThreadpoolWait(PTP_WAIT a)                                    { ::CloseThreadpoolWait(a); }
extern "C" HANDLE   __stdcall Shim_CreateTimerQueue()                                                 { return ::CreateTimerQueue(); }
extern "C" BOOL     __stdcall Shim_CreateTimerQueueTimer(PHANDLE a, HANDLE b, WAITORTIMERCALLBACK c, PVOID d, DWORD e, DWORD f, ULONG g) { return ::CreateTimerQueueTimer(a, b, c, d, e, f, g); }
extern "C" BOOL     __stdcall Shim_ChangeTimerQueueTimer(HANDLE a, HANDLE b, ULONG c, ULONG d)        { return ::ChangeTimerQueueTimer(a, b, c, d); }
extern "C" BOOL     __stdcall Shim_DeleteTimerQueueTimer(HANDLE a, HANDLE b, HANDLE c)                { return ::DeleteTimerQueueTimer(a, b, c); }
extern "C" BOOL     __stdcall Shim_RegisterWaitForSingleObject(PHANDLE a, HANDLE b, WAITORTIMERCALLBACK c, PVOID d, DWORD e, DWORD f) { return ::RegisterWaitForSingleObject(a, b, c, d, e, f); }
extern "C" BOOL     __stdcall Shim_UnregisterWait(HANDLE a)                                           { return ::UnregisterWait(a); }
extern "C" BOOL     __stdcall Shim_UnregisterWaitEx(HANDLE a, HANDLE b)                               { return ::UnregisterWaitEx(a, b); }
extern "C" DWORD    __stdcall Shim_QueueUserAPC(PAPCFUNC a, HANDLE b, ULONG_PTR c)                    { return ::QueueUserAPC(a, b, c); }
extern "C" BOOL     __stdcall Shim_QueueUserWorkItem(LPTHREAD_START_ROUTINE a, PVOID b, ULONG c)      { return ::QueueUserWorkItem(a, b, c); }

// ===========================================================================
// Memory resource notification (2)
// ===========================================================================
extern "C" HANDLE __stdcall Shim_CreateMemoryResourceNotification(MEMORY_RESOURCE_NOTIFICATION_TYPE t) { return ::CreateMemoryResourceNotification(t); }
extern "C" BOOL   __stdcall Shim_QueryMemoryResourceNotification(HANDLE h, PBOOL r)                   { return ::QueryMemoryResourceNotification(h, r); }

// ===========================================================================
// Heap (4)
// ===========================================================================
extern "C" HANDLE __stdcall Shim_HeapCreate(DWORD a, SIZE_T b, SIZE_T c)             { return ::HeapCreate(a, b, c); }
extern "C" BOOL   __stdcall Shim_HeapDestroy(HANDLE h)                               { return ::HeapDestroy(h); }
extern "C" BOOL   __stdcall Shim_HeapSetInformation(HANDLE a, HEAP_INFORMATION_CLASS b, PVOID c, SIZE_T d) { return ::HeapSetInformation(a, b, c, d); }
extern "C" SIZE_T __stdcall Shim_HeapSize(HANDLE a, DWORD b, LPCVOID c)              { return ::HeapSize(a, b, c); }

// ===========================================================================
// Global / Local (6)
// ===========================================================================
extern "C" LPVOID __stdcall Shim_GlobalAlloc(UINT f, SIZE_T s)    { return ::GlobalAlloc(f, s); }
extern "C" HGLOBAL __stdcall Shim_GlobalFree(HGLOBAL h)            { return ::GlobalFree(h); }
extern "C" LPVOID __stdcall Shim_GlobalLock(HGLOBAL h)            { return ::GlobalLock(h); }
extern "C" BOOL   __stdcall Shim_GlobalUnlock(HGLOBAL h)          { return ::GlobalUnlock(h); }
extern "C" HLOCAL __stdcall Shim_LocalAlloc(UINT f, SIZE_T s)     { return ::LocalAlloc(f, s); }
extern "C" HLOCAL __stdcall Shim_LocalFree(HLOCAL h)              { return ::LocalFree(h); }

// ===========================================================================
// Loader (7)
// ===========================================================================
extern "C" HMODULE __stdcall Shim_LoadLibraryA(LPCSTR n)                                { return ::LoadLibraryA(n); }
extern "C" HMODULE __stdcall Shim_LoadLibraryExA(LPCSTR n, HANDLE h, DWORD f)           { return ::LoadLibraryExA(n, h, f); }
extern "C" DWORD   __stdcall Shim_GetModuleFileNameA(HMODULE h, LPSTR b, DWORD n)       { return ::GetModuleFileNameA(h, b, n); }
extern "C" BOOL    __stdcall Shim_GetModuleHandleExA(DWORD f, LPCSTR n, HMODULE* h)     { return ::GetModuleHandleExA(f, n, h); }
extern "C" BOOL    __stdcall Shim_GetModuleHandleExW(DWORD f, LPCWSTR n, HMODULE* h)    { return ::GetModuleHandleExW(f, n, h); }
extern "C" VOID    __stdcall Shim_FreeLibraryAndExitThread(HMODULE h, DWORD c)          { ::FreeLibraryAndExitThread(h, c); }
extern "C" BOOL    __stdcall Shim_DisableThreadLibraryCalls(HMODULE h)                  { return ::DisableThreadLibraryCalls(h); }

// ===========================================================================
// Console (20)
// ===========================================================================
extern "C" BOOL  __stdcall Shim_AttachConsole(DWORD pid)                              { return ::AttachConsole(pid); }
extern "C" BOOL  __stdcall Shim_FreeConsole()                                         { return ::FreeConsole(); }
extern "C" BOOL  __stdcall Shim_GetConsoleCursorInfo(HANDLE h, PCONSOLE_CURSOR_INFO i) { return ::GetConsoleCursorInfo(h, i); }
extern "C" BOOL  __stdcall Shim_SetConsoleCursorInfo(HANDLE h, const CONSOLE_CURSOR_INFO* i) { return ::SetConsoleCursorInfo(h, i); }
extern "C" BOOL  __stdcall Shim_GetConsoleScreenBufferInfo(HANDLE h, PCONSOLE_SCREEN_BUFFER_INFO i) { return ::GetConsoleScreenBufferInfo(h, i); }
extern "C" BOOL  __stdcall Shim_SetConsoleScreenBufferSize(HANDLE h, COORD s)         { return ::SetConsoleScreenBufferSize(h, s); }
extern "C" HWND  __stdcall Shim_GetConsoleWindow()                                    { return ::GetConsoleWindow(); }
extern "C" BOOL  __stdcall Shim_GetNumberOfConsoleInputEvents(HANDLE h, LPDWORD n)    { return ::GetNumberOfConsoleInputEvents(h, n); }
extern "C" BOOL  __stdcall Shim_FillConsoleOutputAttribute(HANDLE h, WORD a, DWORD n, COORD c, LPDWORD w) { return ::FillConsoleOutputAttribute(h, a, n, c, w); }
extern "C" BOOL  __stdcall Shim_FillConsoleOutputCharacterW(HANDLE h, WCHAR ch, DWORD n, COORD c, LPDWORD w) { return ::FillConsoleOutputCharacterW(h, ch, n, c, w); }
extern "C" BOOL  __stdcall Shim_SetConsoleCursorPosition(HANDLE h, COORD c)            { return ::SetConsoleCursorPosition(h, c); }
extern "C" BOOL  __stdcall Shim_SetConsoleTextAttribute(HANDLE h, WORD a)             { return ::SetConsoleTextAttribute(h, a); }
extern "C" BOOL  __stdcall Shim_SetConsoleTitleA(LPCSTR t)                            { return ::SetConsoleTitleA(t); }
extern "C" BOOL  __stdcall Shim_SetConsoleTitleW(LPCWSTR t)                           { return ::SetConsoleTitleW(t); }
extern "C" BOOL  __stdcall Shim_SetConsoleWindowInfo(HANDLE h, BOOL a, const SMALL_RECT* r) { return ::SetConsoleWindowInfo(h, a, r); }
extern "C" BOOL  __stdcall Shim_SetConsoleCtrlHandler(PHANDLER_ROUTINE h, BOOL a)     { return ::SetConsoleCtrlHandler(h, a); }
// ReadConsoleA/W arg 5 is PCONSOLE_READCONSOLE_CONTROL on MSVC — cast the
// LPVOID parameter to that exact type so MSVC accepts the call. On the Linux
// stub, the signature already takes LPVOID.
extern "C" BOOL  __stdcall Shim_ReadConsoleA(HANDLE h, LPVOID b, DWORD n, LPDWORD g, LPVOID p) {
#ifdef _MSC_VER
    return ::ReadConsoleA(h, b, n, g, reinterpret_cast<PCONSOLE_READCONSOLE_CONTROL>(p));
#else
    return ::ReadConsoleA(h, b, n, g, p);
#endif
}
extern "C" BOOL  __stdcall Shim_ReadConsoleW(HANDLE h, LPVOID b, DWORD n, LPDWORD g, LPVOID p) {
#ifdef _MSC_VER
    return ::ReadConsoleW(h, b, n, g, reinterpret_cast<PCONSOLE_READCONSOLE_CONTROL>(p));
#else
    return ::ReadConsoleW(h, b, n, g, p);
#endif
}
extern "C" BOOL  __stdcall Shim_ReadConsoleInputW(HANDLE h, PINPUT_RECORD r, DWORD n, LPDWORD g) { return ::ReadConsoleInputW(h, r, n, g); }
extern "C" BOOL  __stdcall Shim_WriteConsoleA(HANDLE h, const void* b, DWORD n, LPDWORD g, LPVOID p) { return ::WriteConsoleA(h, b, n, g, p); }
extern "C" BOOL  __stdcall Shim_WriteConsoleInputW(HANDLE h, const INPUT_RECORD* r, DWORD n, LPDWORD g) { return ::WriteConsoleInputW(h, r, n, g); }

// ===========================================================================
// System info / locale / NLS
// ===========================================================================
extern "C" UINT   __stdcall Shim_GetACP()                              { return ::GetACP(); }
extern "C" UINT   __stdcall Shim_GetOEMCP()                            { return ::GetOEMCP(); }
extern "C" BOOL   __stdcall Shim_GetCPInfo(UINT c, LPCPINFO i)         { return ::GetCPInfo(c, i); }
extern "C" BOOL   __stdcall Shim_IsValidCodePage(UINT c)               { return ::IsValidCodePage(c); }
extern "C" BOOL   __stdcall Shim_GetComputerNameW(LPWSTR b, LPDWORD n) { return ::GetComputerNameW(b, n); }
extern "C" VOID   __stdcall Shim_GetStartupInfoW(LPSTARTUPINFOW i)     { ::GetStartupInfoW(i); }
extern "C" LPSTR  __stdcall Shim_GetCommandLineA()                     { return ::GetCommandLineA(); }
extern "C" LANGID __stdcall Shim_GetSystemDefaultLangID()              { return ::GetSystemDefaultLangID(); }
extern "C" LANGID __stdcall Shim_GetUserDefaultLangID()                { return ::GetUserDefaultLangID(); }
extern "C" LCID   __stdcall Shim_GetUserDefaultLCID()                  { return ::GetUserDefaultLCID(); }
extern "C" int    __stdcall Shim_GetUserDefaultLocaleName(LPWSTR b, int n) { return ::GetUserDefaultLocaleName(b, n); }
extern "C" int    __stdcall Shim_GetLocaleInfoEx(LPCWSTR n, LCTYPE t, LPWSTR b, int s) { return ::GetLocaleInfoEx(n, t, b, s); }
extern "C" int    __stdcall Shim_GetLocaleInfoW(LCID l, LCTYPE t, LPWSTR b, int s) { return ::GetLocaleInfoW(l, t, b, s); }
extern "C" int    __stdcall Shim_LCIDToLocaleName(LCID l, LPWSTR b, int n, DWORD f) { return ::LCIDToLocaleName(l, b, n, f); }
extern "C" LCID   __stdcall Shim_LocaleNameToLCID(LPCWSTR n, DWORD f)  { return ::LocaleNameToLCID(n, f); }
extern "C" int    __stdcall Shim_ResolveLocaleName(LPCWSTR a, LPWSTR b, int n) { return ::ResolveLocaleName(a, b, n); }
extern "C" BOOL   __stdcall Shim_IsValidLocale(LCID l, DWORD f)        { return ::IsValidLocale(l, f); }
extern "C" BOOL   __stdcall Shim_EnumSystemLocalesW(LOCALE_ENUMPROCW e, DWORD f) { return ::EnumSystemLocalesW(e, f); }
extern "C" int    __stdcall Shim_GetGeoInfoW(GEOID g, GEOTYPE t, LPWSTR b, int n, LANGID l) { return ::GetGeoInfoW(g, t, b, n, l); }
extern "C" GEOID  __stdcall Shim_GetUserGeoID(GEOTYPE t)               { return ::GetUserGeoID(t); }
extern "C" int    __stdcall Shim_CompareStringA(LCID l, DWORD f, LPCSTR a, int na, LPCSTR b, int nb) { return ::CompareStringA(l, f, a, na, b, nb); }
extern "C" int    __stdcall Shim_CompareStringW(LCID l, DWORD f, LPCWSTR a, int na, LPCWSTR b, int nb) { return ::CompareStringW(l, f, a, na, b, nb); }
extern "C" int    __stdcall Shim_CompareStringEx(LPCWSTR n, DWORD f, LPCWSTR a, int na, LPCWSTR b, int nb, LPNLSVERSIONINFOEX v, LPVOID p, LPARAM l) { return ::CompareStringEx(n, f, a, na, b, nb, reinterpret_cast<LPNLSVERSIONINFO>(v), p, l); }
extern "C" int    __stdcall Shim_CompareStringOrdinal(LPCWSTR a, int na, LPCWSTR b, int nb, BOOL i)   { return ::CompareStringOrdinal(a, na, b, nb, i); }
extern "C" int    __stdcall Shim_LCMapStringW(LCID l, DWORD f, LPCWSTR s, int ns, LPWSTR d, int nd)    { return ::LCMapStringW(l, f, s, ns, d, nd); }
extern "C" int    __stdcall Shim_LCMapStringEx(LPCWSTR n, DWORD f, LPCWSTR s, int ns, LPWSTR d, int nd, LPNLSVERSIONINFOEX v, LPVOID p, LPARAM l) { return ::LCMapStringEx(n, f, s, ns, d, nd, reinterpret_cast<LPNLSVERSIONINFO>(v), p, l); }
extern "C" int    __stdcall Shim_GetDateFormatW(LCID l, DWORD f, const SYSTEMTIME* t, LPCWSTR fmt, LPWSTR d, int nd) { return ::GetDateFormatW(l, f, t, fmt, d, nd); }
extern "C" int    __stdcall Shim_GetDateFormatEx(LPCWSTR n, DWORD f, const SYSTEMTIME* t, LPCWSTR fmt, LPWSTR d, int nd, LPCWSTR cal) { return ::GetDateFormatEx(n, f, t, fmt, d, nd, cal); }
extern "C" int    __stdcall Shim_GetTimeFormatW(LCID l, DWORD f, const SYSTEMTIME* t, LPCWSTR fmt, LPWSTR d, int nd) { return ::GetTimeFormatW(l, f, t, fmt, d, nd); }
extern "C" int    __stdcall Shim_GetTimeFormatEx(LPCWSTR n, DWORD f, const SYSTEMTIME* t, LPCWSTR fmt, LPWSTR d, int nd) { return ::GetTimeFormatEx(n, f, t, fmt, d, nd); }
extern "C" int    __stdcall Shim_GetCurrencyFormatEx(LPCWSTR n, DWORD f, LPCWSTR s, const CURRENCYFMTW* fmt, LPWSTR d, int nd) { return ::GetCurrencyFormatEx(n, f, s, fmt, d, nd); }
extern "C" int    __stdcall Shim_GetNumberFormatEx(LPCWSTR n, DWORD f, LPCWSTR s, const NUMBERFMTW* fmt, LPWSTR d, int nd) { return ::GetNumberFormatEx(n, f, s, fmt, d, nd); }
extern "C" BOOL   __stdcall Shim_GetStringTypeW(DWORD t, LPCWSTR s, int n, LPWORD c) { return ::GetStringTypeW(t, s, n, c); }
extern "C" BOOL   __stdcall Shim_GetUserPreferredUILanguages(DWORD f, PULONG e, PZZWSTR b, PULONG n) { return ::GetUserPreferredUILanguages(f, e, b, n); }

// ===========================================================================
// Time / date / version
// ===========================================================================
extern "C" VOID  __stdcall Shim_GetLocalTime(LPSYSTEMTIME t)                        { ::GetLocalTime(t); }
extern "C" VOID  __stdcall Shim_GetSystemTime(LPSYSTEMTIME t)                       { ::GetSystemTime(t); }
extern "C" VOID  __stdcall Shim_GetSystemTimeAsFileTime(LPFILETIME t)               { ::GetSystemTimeAsFileTime(t); }
extern "C" VOID  __stdcall Shim_GetSystemTimePreciseAsFileTime(LPFILETIME t)        { ::GetSystemTimePreciseAsFileTime(t); }
extern "C" BOOL  __stdcall Shim_GetSystemTimeAdjustment(PDWORD a, PDWORD b, PBOOL c){ return ::GetSystemTimeAdjustment(a, b, c); }
extern "C" DWORD __stdcall Shim_GetTimeZoneInformation(LPTIME_ZONE_INFORMATION t)   { return ::GetTimeZoneInformation(t); }
extern "C" DWORD __stdcall Shim_GetDynamicTimeZoneInformation(PDYNAMIC_TIME_ZONE_INFORMATION t) { return ::GetDynamicTimeZoneInformation(t); }
extern "C" BOOL  __stdcall Shim_SystemTimeToFileTime(const SYSTEMTIME* t, LPFILETIME f) { return ::SystemTimeToFileTime(t, f); }
extern "C" BOOL  __stdcall Shim_FileTimeToSystemTime(const FILETIME* f, LPSYSTEMTIME t) { return ::FileTimeToSystemTime(f, t); }
extern "C" BOOL  __stdcall Shim_LocalFileTimeToFileTime(const FILETIME* in, LPFILETIME out) { return ::LocalFileTimeToFileTime(in, out); }
extern "C" BOOL  __stdcall Shim_SystemTimeToTzSpecificLocalTime(LPTIME_ZONE_INFORMATION t, LPSYSTEMTIME in, LPSYSTEMTIME out) { return ::SystemTimeToTzSpecificLocalTime(t, in, out); }
extern "C" BOOL  __stdcall Shim_DosDateTimeToFileTime(WORD d, WORD t, LPFILETIME f)  { return ::DosDateTimeToFileTime(d, t, f); }
extern "C" LONG  __stdcall Shim_CompareFileTime(const FILETIME* a, const FILETIME* b) { return ::CompareFileTime(a, b); }
extern "C" BOOL  __stdcall Shim_QueryUnbiasedInterruptTime(PULONGLONG t)             { return ::QueryUnbiasedInterruptTime(t); }
extern "C" DWORD __stdcall Shim_GetVersion()                                          { return ::GetVersion(); }
extern "C" BOOL  __stdcall Shim_GetVersionExW(LPOSVERSIONINFOEXW i)                  { return ::GetVersionExW(reinterpret_cast<LPOSVERSIONINFOW>(i)); }
extern "C" BOOL  __stdcall Shim_VerifyVersionInfoW(LPOSVERSIONINFOEXW i, DWORD t, DWORDLONG c) { return ::VerifyVersionInfoW(i, t, c); }
extern "C" DWORDLONG __stdcall Shim_VerSetConditionMask(DWORDLONG m, DWORD t, BYTE c) { return ::VerSetConditionMask(m, t, c); }

// ===========================================================================
// Environment / system directories
// ===========================================================================
extern "C" DWORD __stdcall Shim_GetEnvironmentVariableA(LPCSTR a, LPSTR b, DWORD n)   { return ::GetEnvironmentVariableA(a, b, n); }
extern "C" BOOL  __stdcall Shim_SetEnvironmentVariableA(LPCSTR a, LPCSTR b)           { return ::SetEnvironmentVariableA(a, b); }
extern "C" DWORD __stdcall Shim_ExpandEnvironmentStringsW(LPCWSTR a, LPWSTR b, DWORD n) { return ::ExpandEnvironmentStringsW(a, b, n); }
extern "C" DWORD __stdcall Shim_GetDllDirectoryW(DWORD n, LPWSTR b)                   { return ::GetDllDirectoryW(n, b); }
extern "C" BOOL  __stdcall Shim_SetDllDirectoryW(LPCWSTR p)                           { return ::SetDllDirectoryW(p); }
extern "C" DWORD __stdcall Shim_GetWindowsDirectoryW(LPWSTR b, UINT n)                { return ::GetWindowsDirectoryW(b, n); }
extern "C" UINT  __stdcall Shim_GetSystemDirectoryA(LPSTR b, UINT n)                  { return ::GetSystemDirectoryA(b, n); }
extern "C" UINT  __stdcall Shim_GetSystemDirectoryW(LPWSTR b, UINT n)                 { return ::GetSystemDirectoryW(b, n); }
extern "C" UINT  __stdcall Shim_GetSystemWow64DirectoryW(LPWSTR b, UINT n)            { return ::GetSystemWow64DirectoryW(b, n); }
extern "C" DWORD __stdcall Shim_GetTempPathA(DWORD n, LPSTR b)                        { return ::GetTempPathA(n, b); }
extern "C" DWORD __stdcall Shim_GetTempFileNameA(LPCSTR p, LPCSTR a, UINT u, LPSTR b) { return ::GetTempFileNameA(p, a, u, b); }
extern "C" DWORD __stdcall Shim_GetLongPathNameW(LPCWSTR p, LPWSTR b, DWORD n)        { return ::GetLongPathNameW(p, b, n); }
extern "C" BOOL  __stdcall Shim_GetVolumePathNameW(LPCWSTR a, LPWSTR b, DWORD n)      { return ::GetVolumePathNameW(a, b, n); }
extern "C" BOOL  __stdcall Shim_GetSystemPowerStatus(LPSYSTEM_POWER_STATUS s)         { return ::GetSystemPowerStatus(s); }
extern "C" VOID  __stdcall Shim_GetNativeSystemInfo(LPSYSTEM_INFO s)                  { ::GetNativeSystemInfo(s); }
extern "C" SIZE_T __stdcall Shim_GetLargePageMinimum()                                { return ::GetLargePageMinimum(); }
extern "C" BOOL  __stdcall Shim_GetDiskFreeSpaceExW(LPCWSTR p, PULARGE_INTEGER a, PULARGE_INTEGER b, PULARGE_INTEGER c) { return ::GetDiskFreeSpaceExW(p, a, b, c); }
extern "C" UINT  __stdcall Shim_GetDriveTypeW(LPCWSTR p)                              { return ::GetDriveTypeW(p); }

// ===========================================================================
// Toolhelp (7) — stub (UWP blocks toolhelp snapshots).
// ===========================================================================
extern "C" HANDLE __stdcall Shim_CreateToolhelp32Snapshot(DWORD, DWORD)              { ::SetLastError(ERROR_NOT_SUPPORTED); return INVALID_HANDLE_VALUE; }
extern "C" BOOL   __stdcall Shim_Process32First(HANDLE, LPPROCESSENTRY32)            { ::SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
extern "C" BOOL   __stdcall Shim_Process32FirstW(HANDLE, LPPROCESSENTRY32W)          { ::SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
extern "C" BOOL   __stdcall Shim_Process32Next(HANDLE, LPPROCESSENTRY32)             { ::SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
extern "C" BOOL   __stdcall Shim_Process32NextW(HANDLE, LPPROCESSENTRY32W)           { ::SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
extern "C" BOOL   __stdcall Shim_Thread32First(HANDLE, LPTHREADENTRY32)              { ::SetLastError(ERROR_NO_MORE_FILES); return FALSE; }
extern "C" BOOL   __stdcall Shim_Thread32Next(HANDLE, LPTHREADENTRY32)               { ::SetLastError(ERROR_NO_MORE_FILES); return FALSE; }

// ===========================================================================
// PSAPI (5) — stub (UWP blocks cross-process module enumeration).
// ===========================================================================
extern "C" BOOL  __stdcall Shim_K32EnumProcessModules(HANDLE, HMODULE*, DWORD, LPDWORD)        { ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
extern "C" BOOL  __stdcall Shim_K32EnumProcessModulesEx(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD){ ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
extern "C" DWORD __stdcall Shim_K32GetModuleFileNameExW(HANDLE, HMODULE, LPWSTR, DWORD)         { ::SetLastError(ERROR_NOT_SUPPORTED); return 0; }
extern "C" BOOL  __stdcall Shim_K32GetModuleInformation(HANDLE, HMODULE, LPMODULEINFO, DWORD)   { ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
extern "C" BOOL  __stdcall Shim_K32GetProcessMemoryInfo(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD){ ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }

// ===========================================================================
// Job object (1) — stub (UWP blocks job objects).
// ===========================================================================
extern "C" BOOL __stdcall Shim_QueryInformationJobObject(HANDLE, JOBOBJECTINFOCLASS, LPVOID, DWORD, LPDWORD) { ::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }

// ===========================================================================
// Misc helpers
// ===========================================================================
extern "C" PVOID  __stdcall Shim_DecodePointer(PVOID p)              { return ::DecodePointer(p); }
extern "C" PVOID  __stdcall Shim_EncodePointer(PVOID p)              { return ::EncodePointer(p); }
extern "C" int    __stdcall Shim_MulDiv(int a, int b, int c)         { return ::MulDiv(a, b, c); }
extern "C" int    __stdcall Shim_lstrcmpA(LPCSTR a, LPCSTR b)        { return ::lstrcmpA(a, b); }
extern "C" int    __stdcall Shim_lstrcmpW(LPCWSTR a, LPCWSTR b)      { return ::lstrcmpW(a, b); }
extern "C" int    __stdcall Shim_lstrcmpiW(LPCWSTR a, LPCWSTR b)     { return ::lstrcmpiW(a, b); }
extern "C" DWORD  __stdcall Shim_GetThreadErrorMode()                { return ::GetThreadErrorMode(); }
extern "C" BOOL   __stdcall Shim_SetThreadErrorMode(DWORD m, LPDWORD o) { return ::SetThreadErrorMode(m, o); }
extern "C" UINT   __stdcall Shim_SetErrorMode(UINT m)                { return ::SetErrorMode(m); }
extern "C" BOOL   __stdcall Shim_SetHandleInformation(HANDLE h, DWORD m, DWORD f) { return ::SetHandleInformation(h, m, f); }
extern "C" DWORD  __stdcall Shim_SignalObjectAndWait(HANDLE a, HANDLE b, DWORD ms, BOOL a2) { return ::SignalObjectAndWait(a, b, ms, a2); }
extern "C" BOOL   __stdcall Shim_SwitchToThread()                    { return ::SwitchToThread(); }
extern "C" BOOL   __stdcall Shim_IsProcessorFeaturePresent(DWORD f)  { return ::IsProcessorFeaturePresent(f); }
extern "C" DWORD  __stdcall Shim_FormatMessageA(DWORD f, LPCVOID s, DWORD i, DWORD l, LPSTR b, DWORD n, va_list* a) { return ::FormatMessageA(f, s, i, l, b, n, a); }
extern "C" BOOL   __stdcall Shim_WritePrivateProfileStringW(LPCWSTR a, LPCWSTR b, LPCWSTR c, LPCWSTR d) { return ::WritePrivateProfileStringW(a, b, c, d); }
extern "C" UINT   __stdcall Shim_GetPrivateProfileIntW(LPCWSTR a, LPCWSTR b, INT c, LPCWSTR d) { return ::GetPrivateProfileIntW(a, b, c, d); }
extern "C" DWORD  __stdcall Shim_GetPrivateProfileStringW(LPCWSTR a, LPCWSTR b, LPCWSTR c, LPWSTR d, DWORD n, LPCWSTR e) { return ::GetPrivateProfileStringW(a, b, c, d, n, e); }
extern "C" HANDLE __stdcall Shim_OpenEventA(DWORD a, BOOL b, LPCSTR c) { return ::OpenEventA(a, b, c); }
extern "C" BOOL   __stdcall Shim_SetStdHandle(DWORD s, HANDLE h)      { return ::SetStdHandle(s, h); }
extern "C" HANDLE __stdcall Shim_CreateEventA(LPSECURITY_ATTRIBUTES a, BOOL b, BOOL c, LPCSTR d)        { return ::CreateEventA(a, b, c, d); }
extern "C" HANDLE __stdcall Shim_CreateEventExA(LPSECURITY_ATTRIBUTES a, LPCSTR b, DWORD c, DWORD d)    { return ::CreateEventExA(a, b, c, d); }
extern "C" HANDLE __stdcall Shim_CreateMutexA(LPSECURITY_ATTRIBUTES a, BOOL b, LPCSTR c)                { return ::CreateMutexA(a, b, c); }
extern "C" HANDLE __stdcall Shim_CreateSemaphoreA(LPSECURITY_ATTRIBUTES a, LONG b, LONG c, LPCSTR d)    { return ::CreateSemaphoreA(a, b, c, d); }
extern "C" HANDLE __stdcall Shim_CreateSemaphoreExA(LPSECURITY_ATTRIBUTES a, LONG b, LONG c, LPCSTR d, DWORD e, DWORD f) { return ::CreateSemaphoreExA(a, b, c, d, e, f); }

}  // namespace xwr

// ===========================================================================
// REGISTER_SHIM entries — one for "kernel32" and one for "kernelbase" per
// function, since kernel32 forwards many of these to kernelbase. That's
// 313 functions × 2 = 626 entries.
// ===========================================================================

#define XWR_REG_K32(name) \
    REGISTER_SHIM("kernel32", #name, (FARPROC)&xwr::Shim_##name); \
    REGISTER_SHIM("kernelbase", #name, (FARPROC)&xwr::Shim_##name);

// SRW locks (6)
XWR_REG_K32(InitializeSRWLock)
XWR_REG_K32(AcquireSRWLockExclusive)
XWR_REG_K32(AcquireSRWLockShared)
XWR_REG_K32(ReleaseSRWLockExclusive)
XWR_REG_K32(ReleaseSRWLockShared)
XWR_REG_K32(TryAcquireSRWLockExclusive)

// Condition variables (5)
XWR_REG_K32(InitializeConditionVariable)
XWR_REG_K32(WakeConditionVariable)
XWR_REG_K32(WakeAllConditionVariable)
XWR_REG_K32(SleepConditionVariableCS)
XWR_REG_K32(SleepConditionVariableSRW)

// Critical section extensions (3)
XWR_REG_K32(InitializeCriticalSectionEx)
XWR_REG_K32(SetCriticalSectionSpinCount)
XWR_REG_K32(TryEnterCriticalSection)

// SList (5)
XWR_REG_K32(InitializeSListHead)
XWR_REG_K32(InterlockedFlushSList)
XWR_REG_K32(InterlockedPopEntrySList)
XWR_REG_K32(InterlockedPushEntrySList)
XWR_REG_K32(QueryDepthSList)

// InitOnce (3)
XWR_REG_K32(InitOnceBeginInitialize)
XWR_REG_K32(InitOnceComplete)
XWR_REG_K32(InitOnceExecuteOnce)

// Interlocked (4)
XWR_REG_K32(InterlockedCompareExchange)
XWR_REG_K32(InterlockedDecrement)
XWR_REG_K32(InterlockedExchange)
XWR_REG_K32(InterlockedIncrement)

// TLS (4)
XWR_REG_K32(TlsAlloc)
XWR_REG_K32(TlsFree)
XWR_REG_K32(TlsGetValue)
XWR_REG_K32(TlsSetValue)

// FLS (4)
XWR_REG_K32(FlsAlloc)
XWR_REG_K32(FlsFree)
XWR_REG_K32(FlsGetValue)
XWR_REG_K32(FlsSetValue)

// Fibers (7)
XWR_REG_K32(CreateFiber)
XWR_REG_K32(ConvertFiberToThread)
XWR_REG_K32(ConvertThreadToFiber)
XWR_REG_K32(ConvertThreadToFiberEx)
XWR_REG_K32(DeleteFiber)
XWR_REG_K32(SwitchToFiber)
XWR_REG_K32(IsThreadAFiber)

// Vectored exception / Unhandled (5)
XWR_REG_K32(AddVectoredExceptionHandler)
XWR_REG_K32(RemoveVectoredExceptionHandler)
XWR_REG_K32(RaiseException)
XWR_REG_K32(UnhandledExceptionFilter)
XWR_REG_K32(SetUnhandledExceptionFilter)

// RTL unwind (7)
XWR_REG_K32(RtlCaptureContext)
XWR_REG_K32(RtlCaptureStackBackTrace)
XWR_REG_K32(RtlLookupFunctionEntry)
XWR_REG_K32(RtlPcToFileHeader)
XWR_REG_K32(RtlUnwind)
XWR_REG_K32(RtlUnwindEx)
XWR_REG_K32(RtlVirtualUnwind)

// Process / thread (21)
XWR_REG_K32(ExitProcess)
XWR_REG_K32(ExitThread)
XWR_REG_K32(CreateProcessA)
XWR_REG_K32(CreateProcessW)
XWR_REG_K32(GetExitCodeProcess)
XWR_REG_K32(GetExitCodeThread)
XWR_REG_K32(GetProcessTimes)
XWR_REG_K32(GetThreadTimes)
XWR_REG_K32(GetProcessId)
XWR_REG_K32(GetThreadId)
XWR_REG_K32(GetProcessHandleCount)
XWR_REG_K32(GetPriorityClass)
XWR_REG_K32(QueryFullProcessImageNameW)
XWR_REG_K32(IsProcessInJob)
XWR_REG_K32(ProcessIdToSessionId)
XWR_REG_K32(TerminateProcess)
XWR_REG_K32(TerminateThread)
XWR_REG_K32(ResumeThread)
XWR_REG_K32(SuspendThread)
XWR_REG_K32(OpenProcess)
XWR_REG_K32(OpenThread)
XWR_REG_K32(DuplicateHandle)
XWR_REG_K32(GetProcessAffinityMask)

// Thread control (16)
XWR_REG_K32(SetThreadPriority)
XWR_REG_K32(GetThreadPriority)
XWR_REG_K32(SetThreadAffinityMask)
XWR_REG_K32(SetThreadIdealProcessor)
XWR_REG_K32(GetThreadIdealProcessorEx)
XWR_REG_K32(SetThreadGroupAffinity)
XWR_REG_K32(SetThreadStackGuarantee)
XWR_REG_K32(SetThreadDescription)
XWR_REG_K32(GetCurrentProcessorNumber)
XWR_REG_K32(GetCurrentProcessorNumberEx)
XWR_REG_K32(GetCurrentThreadStackLimits)
XWR_REG_K32(GetActiveProcessorCount)
XWR_REG_K32(GetMaximumProcessorCount)
XWR_REG_K32(SetThreadExecutionState)
XWR_REG_K32(GetThreadContext)
XWR_REG_K32(GetThreadLocale)

// NUMA (5)
XWR_REG_K32(GetNumaHighestNodeNumber)
XWR_REG_K32(GetNumaNodeProcessorMask)
XWR_REG_K32(GetNumaNodeProcessorMaskEx)
XWR_REG_K32(GetNumaProcessorNode)
XWR_REG_K32(GetNumaProcessorNodeEx)

// Logical processor (2)
XWR_REG_K32(GetLogicalProcessorInformation)
XWR_REG_K32(GetLogicalProcessorInformationEx)

// Files / pipes / IO (40)
XWR_REG_K32(CopyFile2)
XWR_REG_K32(CreateFile2)
XWR_REG_K32(CreateFileMappingA)
XWR_REG_K32(CreateFileMappingW)
XWR_REG_K32(MapViewOfFile)
XWR_REG_K32(MapViewOfFileEx)
XWR_REG_K32(UnmapViewOfFile)
XWR_REG_K32(FlushViewOfFile)
XWR_REG_K32(FlushInstructionCache)
XWR_REG_K32(VirtualUnlock)
XWR_REG_K32(GetFileInformationByHandle)
XWR_REG_K32(GetFileInformationByHandleEx)
XWR_REG_K32(SetFileInformationByHandle)
XWR_REG_K32(GetFileAttributesA)
XWR_REG_K32(GetFileTime)
XWR_REG_K32(SetFileTime)
XWR_REG_K32(GetFileType)
XWR_REG_K32(GetFinalPathNameByHandleW)
XWR_REG_K32(SetFileCompletionNotificationModes)
XWR_REG_K32(SetEndOfFile)
XWR_REG_K32(FindFirstFileExA)
XWR_REG_K32(FindNextFileA)
XWR_REG_K32(FindResourceW)
XWR_REG_K32(LoadResource)
XWR_REG_K32(LockResource)
XWR_REG_K32(SizeofResource)
XWR_REG_K32(ReOpenFile)
XWR_REG_K32(ReplaceFileW)
XWR_REG_K32(MoveFileExA)
XWR_REG_K32(AreFileApisANSI)
XWR_REG_K32(CreateNamedPipeW)
XWR_REG_K32(ConnectNamedPipe)
XWR_REG_K32(PeekNamedPipe)
XWR_REG_K32(SetNamedPipeHandleState)
XWR_REG_K32(GetNamedPipeClientProcessId)
XWR_REG_K32(GetNamedPipeServerProcessId)
XWR_REG_K32(GetNamedPipeHandleStateW)
XWR_REG_K32(WaitNamedPipeW)
XWR_REG_K32(CreatePipe)
XWR_REG_K32(DeviceIoControl)
XWR_REG_K32(ReadDirectoryChangesW)

// IO completion (8)
XWR_REG_K32(CreateIoCompletionPort)
XWR_REG_K32(GetQueuedCompletionStatus)
XWR_REG_K32(GetQueuedCompletionStatusEx)
XWR_REG_K32(PostQueuedCompletionStatus)
XWR_REG_K32(CancelIo)
XWR_REG_K32(CancelIoEx)
XWR_REG_K32(CancelSynchronousIo)
XWR_REG_K32(GetOverlappedResult)

// Waitable timers (5)
XWR_REG_K32(CreateWaitableTimerA)
XWR_REG_K32(CreateWaitableTimerW)
XWR_REG_K32(CreateWaitableTimerExW)
XWR_REG_K32(CancelWaitableTimer)
XWR_REG_K32(SetWaitableTimer)

// Thread pool / timer queue / wait (13)
XWR_REG_K32(CreateThreadpoolWait)
XWR_REG_K32(SetThreadpoolWait)
XWR_REG_K32(WaitForThreadpoolWaitCallbacks)
XWR_REG_K32(CloseThreadpoolWait)
XWR_REG_K32(CreateTimerQueue)
XWR_REG_K32(CreateTimerQueueTimer)
XWR_REG_K32(ChangeTimerQueueTimer)
XWR_REG_K32(DeleteTimerQueueTimer)
XWR_REG_K32(RegisterWaitForSingleObject)
XWR_REG_K32(UnregisterWait)
XWR_REG_K32(UnregisterWaitEx)
XWR_REG_K32(QueueUserAPC)
XWR_REG_K32(QueueUserWorkItem)

// Memory resource (2)
XWR_REG_K32(CreateMemoryResourceNotification)
XWR_REG_K32(QueryMemoryResourceNotification)

// Heap (4)
XWR_REG_K32(HeapCreate)
XWR_REG_K32(HeapDestroy)
XWR_REG_K32(HeapSetInformation)
XWR_REG_K32(HeapSize)

// Global / Local (6)
XWR_REG_K32(GlobalAlloc)
XWR_REG_K32(GlobalFree)
XWR_REG_K32(GlobalLock)
XWR_REG_K32(GlobalUnlock)
XWR_REG_K32(LocalAlloc)
XWR_REG_K32(LocalFree)

// Loader (7)
XWR_REG_K32(LoadLibraryA)
XWR_REG_K32(LoadLibraryExA)
XWR_REG_K32(GetModuleFileNameA)
XWR_REG_K32(GetModuleHandleExA)
XWR_REG_K32(GetModuleHandleExW)
XWR_REG_K32(FreeLibraryAndExitThread)
XWR_REG_K32(DisableThreadLibraryCalls)

// Console (20)
XWR_REG_K32(AttachConsole)
XWR_REG_K32(FreeConsole)
XWR_REG_K32(GetConsoleCursorInfo)
XWR_REG_K32(SetConsoleCursorInfo)
XWR_REG_K32(GetConsoleScreenBufferInfo)
XWR_REG_K32(SetConsoleScreenBufferSize)
XWR_REG_K32(GetConsoleWindow)
XWR_REG_K32(GetNumberOfConsoleInputEvents)
XWR_REG_K32(FillConsoleOutputAttribute)
XWR_REG_K32(FillConsoleOutputCharacterW)
XWR_REG_K32(SetConsoleCursorPosition)
XWR_REG_K32(SetConsoleTextAttribute)
XWR_REG_K32(SetConsoleTitleA)
XWR_REG_K32(SetConsoleTitleW)
XWR_REG_K32(SetConsoleWindowInfo)
XWR_REG_K32(SetConsoleCtrlHandler)
XWR_REG_K32(ReadConsoleA)
XWR_REG_K32(ReadConsoleW)
XWR_REG_K32(ReadConsoleInputW)
XWR_REG_K32(WriteConsoleA)
XWR_REG_K32(WriteConsoleInputW)

// System info / locale / NLS (37)
XWR_REG_K32(GetACP)
XWR_REG_K32(GetOEMCP)
XWR_REG_K32(GetCPInfo)
XWR_REG_K32(IsValidCodePage)
XWR_REG_K32(GetComputerNameW)
XWR_REG_K32(GetStartupInfoW)
XWR_REG_K32(GetCommandLineA)
XWR_REG_K32(GetSystemDefaultLangID)
XWR_REG_K32(GetUserDefaultLangID)
XWR_REG_K32(GetUserDefaultLCID)
XWR_REG_K32(GetUserDefaultLocaleName)
XWR_REG_K32(GetLocaleInfoEx)
XWR_REG_K32(GetLocaleInfoW)
XWR_REG_K32(LCIDToLocaleName)
XWR_REG_K32(LocaleNameToLCID)
XWR_REG_K32(ResolveLocaleName)
XWR_REG_K32(IsValidLocale)
XWR_REG_K32(EnumSystemLocalesW)
XWR_REG_K32(GetGeoInfoW)
XWR_REG_K32(GetUserGeoID)
XWR_REG_K32(CompareStringA)
XWR_REG_K32(CompareStringW)
XWR_REG_K32(CompareStringEx)
XWR_REG_K32(CompareStringOrdinal)
XWR_REG_K32(LCMapStringW)
XWR_REG_K32(LCMapStringEx)
XWR_REG_K32(GetDateFormatW)
XWR_REG_K32(GetDateFormatEx)
XWR_REG_K32(GetTimeFormatW)
XWR_REG_K32(GetTimeFormatEx)
XWR_REG_K32(GetCurrencyFormatEx)
XWR_REG_K32(GetNumberFormatEx)
XWR_REG_K32(GetStringTypeW)
XWR_REG_K32(GetUserPreferredUILanguages)

// Time / date / version (18)
XWR_REG_K32(GetLocalTime)
XWR_REG_K32(GetSystemTime)
XWR_REG_K32(GetSystemTimeAsFileTime)
XWR_REG_K32(GetSystemTimePreciseAsFileTime)
XWR_REG_K32(GetSystemTimeAdjustment)
XWR_REG_K32(GetTimeZoneInformation)
XWR_REG_K32(GetDynamicTimeZoneInformation)
XWR_REG_K32(SystemTimeToFileTime)
XWR_REG_K32(FileTimeToSystemTime)
XWR_REG_K32(LocalFileTimeToFileTime)
XWR_REG_K32(SystemTimeToTzSpecificLocalTime)
XWR_REG_K32(DosDateTimeToFileTime)
XWR_REG_K32(CompareFileTime)
XWR_REG_K32(QueryUnbiasedInterruptTime)
XWR_REG_K32(GetVersion)
XWR_REG_K32(GetVersionExW)
XWR_REG_K32(VerifyVersionInfoW)
XWR_REG_K32(VerSetConditionMask)

// Environment / system directories (18)
XWR_REG_K32(GetEnvironmentVariableA)
XWR_REG_K32(SetEnvironmentVariableA)
XWR_REG_K32(ExpandEnvironmentStringsW)
XWR_REG_K32(GetDllDirectoryW)
XWR_REG_K32(SetDllDirectoryW)
XWR_REG_K32(GetWindowsDirectoryW)
XWR_REG_K32(GetSystemDirectoryA)
XWR_REG_K32(GetSystemDirectoryW)
XWR_REG_K32(GetSystemWow64DirectoryW)
XWR_REG_K32(GetTempPathA)
XWR_REG_K32(GetTempFileNameA)
XWR_REG_K32(GetLongPathNameW)
XWR_REG_K32(GetVolumePathNameW)
XWR_REG_K32(GetSystemPowerStatus)
XWR_REG_K32(GetNativeSystemInfo)
XWR_REG_K32(GetLargePageMinimum)
XWR_REG_K32(GetDiskFreeSpaceExW)
XWR_REG_K32(GetDriveTypeW)

// Toolhelp (7)
XWR_REG_K32(CreateToolhelp32Snapshot)
XWR_REG_K32(Process32First)
XWR_REG_K32(Process32FirstW)
XWR_REG_K32(Process32Next)
XWR_REG_K32(Process32NextW)
XWR_REG_K32(Thread32First)
XWR_REG_K32(Thread32Next)

// PSAPI (5)
XWR_REG_K32(K32EnumProcessModules)
XWR_REG_K32(K32EnumProcessModulesEx)
XWR_REG_K32(K32GetModuleFileNameExW)
XWR_REG_K32(K32GetModuleInformation)
XWR_REG_K32(K32GetProcessMemoryInfo)

// Job object (1)
XWR_REG_K32(QueryInformationJobObject)

// Misc (24)
XWR_REG_K32(DecodePointer)
XWR_REG_K32(EncodePointer)
XWR_REG_K32(MulDiv)
XWR_REG_K32(lstrcmpA)
XWR_REG_K32(lstrcmpW)
XWR_REG_K32(lstrcmpiW)
XWR_REG_K32(GetThreadErrorMode)
XWR_REG_K32(SetThreadErrorMode)
XWR_REG_K32(SetErrorMode)
XWR_REG_K32(SetHandleInformation)
XWR_REG_K32(SignalObjectAndWait)
XWR_REG_K32(SwitchToThread)
XWR_REG_K32(IsProcessorFeaturePresent)
XWR_REG_K32(FormatMessageA)
XWR_REG_K32(WritePrivateProfileStringW)
XWR_REG_K32(GetPrivateProfileIntW)
XWR_REG_K32(GetPrivateProfileStringW)
XWR_REG_K32(OpenEventA)
XWR_REG_K32(SetStdHandle)
XWR_REG_K32(CreateEventA)
XWR_REG_K32(CreateEventExA)
XWR_REG_K32(CreateMutexA)
XWR_REG_K32(CreateSemaphoreA)
XWR_REG_K32(CreateSemaphoreExA)

#undef XWR_REG_K32
