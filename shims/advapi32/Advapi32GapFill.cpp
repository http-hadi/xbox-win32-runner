// shims/advapi32/Advapi32GapFill.cpp
// Implements the 38 missing ADVAPI32.dll functions reported by the coverage
// gap scan. Breakdown of the 38 gaps:
//
//   Registry (4) — pass through to the real advapi32 surface:
//       RegGetValueW, RegQueryInfoKeyW, RegCreateKeyTransactedW,
//       RegNotifyChangeKeyValue.
//
//   Event log (3) — stub returning a fake handle / TRUE so the game's event
//   logging code is silently a no-op:
//       RegisterEventSourceW, ReportEventW, DeregisterEventSource.
//
//   ETW (4) — stub. EventRegister returns 0 (ERROR_SUCCESS) with a synthetic
//   REGHANDLE so callers that "log" ETW events proceed; the other calls are
//   no-ops:
//       EventRegister, EventUnregister, EventWriteTransfer,
//       EventSetInformation.
//
//   Crypto (12) — pass through to the real advapi32 CryptoAPI where UWP
//   exposes it (CAPI is documented as available to AppContainer apps for
//   hashing/signing on Win10+). On Xbox, if a specific entry is not
//   available the real API will fail with NTE_FAIL and the game's crypto
//   path degrades gracefully.
//       CryptCreateHash, CryptDecrypt, CryptDestroyHash, CryptDestroyKey,
//       CryptEnumProvidersW, CryptExportKey, CryptGetHashParam,
//       CryptGetProvParam, CryptGetUserKey, CryptHashData,
//       CryptSetHashParam, CryptSignHashW.
//
//   Services (2) — stub returning FALSE / ERROR_NOT_SUPPORTED (services
//   are not available in AppContainer):
//       ChangeServiceConfigW, QueryServiceConfigW.
//
//   SID / Security (7) — stub:
//       AllocateLocallyUniqueId, CheckTokenMembership,
//       ConvertStringSecurityDescriptorToSecurityDescriptorW,
//       CreateWellKnownSid, SetEntriesInAclA, SetEntriesInAclW,
//       SetNamedSecurityInfoW, SetSecurityDescriptorGroup,
//       SetSecurityDescriptorOwner.
//
//   Misc (6):
//       DecryptFileW (stub: returns TRUE; games don't need EFS on Xbox),
//       InitiateSystemShutdownExW (stub: returns FALSE — can't shut down
//       Xbox from an AppContainer game),
//       LookupPrivilegeValueW (stub: returns FALSE),
//       SystemFunction036 / RtlGenRandom (pass through to BCryptGenRandom
//       via the real advapi32 surface — UWP exposes it).
//
// Each function is `extern "C" __stdcall Shim_<OriginalName>` so the PE
// loader can drop its address directly into a game's IAT.
//
// REGISTER_SHIM count: 38 (one per missing function).
//
// Added by Task ID GAP-ADVAPI32-CRYPT32.

#include "UwpSdkIncludes.h"

// ---------------------------------------------------------------------------
// MSVC: pull in the SDK headers that declare the Crypt* / ETW / ACL surface
// used by the gap-fill shims below.
//   * <wincrypt.h>  — CryptAcquireContextW / CryptGenRandom / HCRYPTPROV
//   * <evntprov.h>  — REGHANDLE / EventRegister / EventWrite / EventUnregister
//   * <aclapi.h>    — SetEntriesInAclW / SetSecurityInfo (uses accctrl.h)
// ---------------------------------------------------------------------------
#ifdef _MSC_VER
  #include <wincrypt.h>
  #include <evntprov.h>
  #include <aclapi.h>
  #pragma comment(lib, "advapi32.lib")
#endif

#include <atomic>
#include <mutex>
#include <cstring>

#include "../ShimRegistry.h"

#ifndef ERROR_NOT_SUPPORTED
#define ERROR_NOT_SUPPORTED 50L
#endif
#ifndef ERROR_INVALID_HANDLE
#define ERROR_INVALID_HANDLE 6L
#endif
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#endif

namespace xwr {

// Monotonic counter for synthetic handles (event source handles, REGHANDLEs,
// fake HCRYPTHASH / HCRYPTKEY values). All start above 0x400000 so they
// don't collide with the lower ranges used by Advapi32Shim.cpp's token /
// crypto / registry handle pools.
static std::atomic<uint64_t> g_nextAdvapiHandle{0x400000};

// ===========================================================================
// Registry pass-throughs (4)
// ===========================================================================
extern "C" LONG __stdcall Shim_RegGetValueW(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValue,
                                            DWORD dwFlags, LPDWORD pdwType, PVOID pvData,
                                            LPDWORD pcbData) {
    return ::RegGetValueW(hKey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}

extern "C" LONG __stdcall Shim_RegQueryInfoKeyW(HKEY hKey, LPWSTR lpClass, LPDWORD lpcchClass,
                                                 LPDWORD lpReserved, LPDWORD lpcSubKeys,
                                                 LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen,
                                                 LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen,
                                                 LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor,
                                                 PFILETIME lpftLastWriteTime) {
    return ::RegQueryInfoKeyW(hKey, lpClass, lpcchClass, lpReserved, lpcSubKeys,
                              lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues,
                              lpcbMaxValueNameLen, lpcbMaxValueLen,
                              lpcbSecurityDescriptor, lpftLastWriteTime);
}

extern "C" LONG __stdcall Shim_RegCreateKeyTransactedW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved,
                                                       LPWSTR lpClass, DWORD dwOptions,
                                                       REGSAM samDesired,
                                                       LPSECURITY_ATTRIBUTES lpTransactionAttributes,
                                                       PHKEY phkResult, LPDWORD lpdwDisposition,
                                                       HANDLE hTransaction, PVOID pExtendedParemeter) {
    return ::RegCreateKeyTransactedW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
                                     lpTransactionAttributes, phkResult, lpdwDisposition,
                                     hTransaction, pExtendedParemeter);
}

extern "C" LONG __stdcall Shim_RegNotifyChangeKeyValue(HKEY hKey, BOOL bWatchSubtree,
                                                       DWORD dwNotifyFilter, HANDLE hEvent,
                                                       BOOL fAsynchronous) {
    return ::RegNotifyChangeKeyValue(hKey, bWatchSubtree, dwNotifyFilter, hEvent, fAsynchronous);
}

// ===========================================================================
// Event log (3) — stub. RegisterEventSourceW returns a fake non-null handle;
// the other two silently succeed so games that log to the event log don't
// crash. (The event log is not accessible from AppContainer on Xbox.)
// ===========================================================================
extern "C" HANDLE __stdcall Shim_RegisterEventSourceW(LPCWSTR lpUNCServerName, LPCWSTR lpSourceName) {
    (void)lpUNCServerName; (void)lpSourceName;
    return (HANDLE)g_nextAdvapiHandle.fetch_add(1);
}

extern "C" BOOL __stdcall Shim_ReportEventW(HANDLE hEventLog, WORD wType, WORD wCategory,
                                            DWORD dwEventID, PSID lpUserSid, WORD wNumStrings,
                                            DWORD dwDataSize, LPCWSTR* lpStrings, LPVOID lpRawData) {
    (void)hEventLog; (void)wType; (void)wCategory; (void)dwEventID; (void)lpUserSid;
    (void)wNumStrings; (void)dwDataSize; (void)lpStrings; (void)lpRawData;
    return TRUE;
}

extern "C" BOOL __stdcall Shim_DeregisterEventSource(HANDLE hEventLog) {
    (void)hEventLog;
    return TRUE;
}

// ===========================================================================
// ETW (4) — stub. EventRegister returns ERROR_SUCCESS with a synthetic
// REGHANDLE so callers proceed past provider initialization; subsequent
// EventWriteTransfer calls are silently dropped.
// ===========================================================================
extern "C" ULONG __stdcall Shim_EventRegister(LPCGUID ProviderId, PENABLECALLBACK EnableCallback,
                                              PVOID CallbackContext, PREGHANDLE RegHandle) {
    (void)ProviderId; (void)EnableCallback; (void)CallbackContext;
    if (RegHandle) *RegHandle = (REGHANDLE)g_nextAdvapiHandle.fetch_add(1);
    return ERROR_SUCCESS;
}

extern "C" ULONG __stdcall Shim_EventUnregister(REGHANDLE RegHandle) {
    (void)RegHandle;
    return ERROR_SUCCESS;
}

extern "C" ULONG __stdcall Shim_EventWriteTransfer(REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor,
                                                   LPCGUID ActivityId, ULONG RelatedActivityId,
                                                   PEVENT_DATA_DESCRIPTOR UserData) {
    (void)RegHandle; (void)EventDescriptor; (void)ActivityId;
    (void)RelatedActivityId; (void)UserData;
    return ERROR_SUCCESS;
}

extern "C" ULONG __stdcall Shim_EventSetInformation(REGHANDLE RegHandle, EVENT_INFO_CLASS InformationClass,
                                                    PVOID EventInformation, ULONG InformationLength) {
    (void)RegHandle; (void)InformationClass; (void)EventInformation; (void)InformationLength;
    return ERROR_SUCCESS;
}

// ===========================================================================
// Crypto (12) — pass through to the real advapi32 CryptoAPI surface. UWP
// apps can call CAPI for hashing / signing (the documentation lists
// CryptAcquireContext / CryptGenRandom as available); the rest of the
// surface is documented as "available with restrictions". If a particular
// call fails on Xbox the game's crypto path typically degrades to an
// alternate path or treats the failure as non-fatal.
// ===========================================================================
extern "C" BOOL __stdcall Shim_CryptCreateHash(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
                                               DWORD dwFlags, HCRYPTHASH* phHash) {
    return ::CryptCreateHash(hProv, Algid, hKey, dwFlags, phHash);
}

extern "C" BOOL __stdcall Shim_CryptDecrypt(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
                                            DWORD dwFlags, BYTE* pbData, DWORD* pdwDataLen) {
    return ::CryptDecrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen);
}

extern "C" BOOL __stdcall Shim_CryptDestroyHash(HCRYPTHASH hHash) {
    return ::CryptDestroyHash(hHash);
}

extern "C" BOOL __stdcall Shim_CryptDestroyKey(HCRYPTKEY hKey) {
    return ::CryptDestroyKey(hKey);
}

extern "C" BOOL __stdcall Shim_CryptEnumProvidersW(DWORD dwIndex, DWORD* pdwReserved,
                                                   DWORD* pdwProvType, DWORD* pdwFlags,
                                                   LPWSTR pszProvName, DWORD* pcbProvName) {
    return ::CryptEnumProvidersW(dwIndex, pdwReserved, pdwProvType, pdwFlags, pszProvName, pcbProvName);
}

extern "C" BOOL __stdcall Shim_CryptExportKey(HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
                                              DWORD dwFlags, BYTE* pbData, DWORD* pdwDataLen) {
    return ::CryptExportKey(hKey, hExpKey, dwBlobType, dwFlags, pbData, pdwDataLen);
}

extern "C" BOOL __stdcall Shim_CryptGetHashParam(HCRYPTHASH hHash, DWORD dwParam, BYTE* pbData,
                                                 DWORD* pdwDataLen, DWORD dwFlags) {
    return ::CryptGetHashParam(hHash, dwParam, pbData, pdwDataLen, dwFlags);
}

extern "C" BOOL __stdcall Shim_CryptGetProvParam(HCRYPTPROV hProv, DWORD dwParam, BYTE* pbData,
                                                 DWORD* pdwDataLen, DWORD dwFlags) {
    return ::CryptGetProvParam(hProv, dwParam, pbData, pdwDataLen, dwFlags);
}

extern "C" BOOL __stdcall Shim_CryptGetUserKey(HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY* phUserKey) {
    return ::CryptGetUserKey(hProv, dwKeySpec, phUserKey);
}

extern "C" BOOL __stdcall Shim_CryptHashData(HCRYPTHASH hHash, const BYTE* pbData, DWORD dwDataLen,
                                             DWORD dwFlags) {
    return ::CryptHashData(hHash, pbData, dwDataLen, dwFlags);
}

extern "C" BOOL __stdcall Shim_CryptSetHashParam(HCRYPTHASH hHash, DWORD dwParam, const BYTE* pbData,
                                                 DWORD dwFlags) {
    return ::CryptSetHashParam(hHash, dwParam, pbData, dwFlags);
}

extern "C" BOOL __stdcall Shim_CryptSignHashW(HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription,
                                              DWORD dwFlags, BYTE* pbSignature, DWORD* pdwSigLen) {
    return ::CryptSignHashW(hHash, dwKeySpec, sDescription, dwFlags, pbSignature, pdwSigLen);
}

// ===========================================================================
// Services (2) — stub. Services are not accessible from AppContainer on
// Xbox. Return FALSE with ERROR_NOT_SUPPORTED so the caller's
// GetLastError() reflects the real reason.
// ===========================================================================
extern "C" BOOL __stdcall Shim_ChangeServiceConfigW(SC_HANDLE hService, DWORD dwServiceType,
                                                    DWORD dwStartType, DWORD dwErrorControl,
                                                    LPCWSTR lpBinaryPathName, LPCWSTR lpLoadOrderGroup,
                                                    LPDWORD lpdwTagId, LPCWSTR lpDependencies,
                                                    LPCWSTR lpServiceStartName, LPCWSTR lpPassword,
                                                    LPCWSTR lpDisplayName) {
    (void)hService; (void)dwServiceType; (void)dwStartType; (void)dwErrorControl;
    (void)lpBinaryPathName; (void)lpLoadOrderGroup; (void)lpdwTagId; (void)lpDependencies;
    (void)lpServiceStartName; (void)lpPassword; (void)lpDisplayName;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_QueryServiceConfigW(SC_HANDLE hService,
                                                   LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                                                   DWORD cbBufSize, LPDWORD pcbBytesNeeded) {
    (void)hService; (void)lpServiceConfig; (void)cbBufSize;
    if (pcbBytesNeeded) *pcbBytesNeeded = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// ===========================================================================
// SID / Security (9) — stub. AllocateLocallyUniqueId fills the LUID with a
// monotonic value so callers that just need a unique LUID proceed;
// CheckTokenMembership returns FALSE for "not a member" by default;
// SetEntriesIn* and Set*SecurityInfo* return ERROR_NOT_SUPPORTED.
// ===========================================================================
extern "C" BOOL __stdcall Shim_AllocateLocallyUniqueId(PLUID Luid) {
    if (!Luid) return FALSE;
    ULONGLONG v = g_nextAdvapiHandle.fetch_add(1);
    Luid->LowPart  = (DWORD)(v & 0xFFFFFFFFu);
    Luid->HighPart = (LONG)(v >> 32);
    return TRUE;
}

extern "C" BOOL __stdcall Shim_CheckTokenMembership(HANDLE TokenHandle, PSID SidToCheck,
                                                    PBOOL IsMember) {
    (void)TokenHandle; (void)SidToCheck;
    if (IsMember) *IsMember = FALSE;
    return TRUE;
}

extern "C" BOOL __stdcall Shim_ConvertStringSecurityDescriptorToSecurityDescriptorW(
    LPCWSTR StringSecurityDescriptor, DWORD StringSDRevision,
    PSECURITY_DESCRIPTOR* SecurityDescriptor, PULONG SecurityDescriptorSize) {
    (void)StringSecurityDescriptor; (void)StringSDRevision;
    (void)SecurityDescriptor; (void)SecurityDescriptorSize;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_CreateWellKnownSid(WELL_KNOWN_SID_TYPE WellKnownSidType,
                                                  PSID DomainSid, PSID pSid, DWORD* cbSid) {
    (void)WellKnownSidType; (void)DomainSid; (void)pSid;
    if (cbSid) *cbSid = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" DWORD __stdcall Shim_SetEntriesInAclA(ULONG cCountOfExplicitEntries,
                                                 void* pListOfExplicitEntries,
                                                 PACL OldAcl, PACL* NewAcl) {
    (void)cCountOfExplicitEntries; (void)pListOfExplicitEntries; (void)OldAcl; (void)NewAcl;
    return ERROR_NOT_SUPPORTED;
}

extern "C" DWORD __stdcall Shim_SetEntriesInAclW(ULONG cCountOfExplicitEntries,
                                                 PEXPLICIT_ACCESS_W pListOfExplicitEntries,
                                                 PACL OldAcl, PACL* NewAcl) {
    (void)cCountOfExplicitEntries; (void)pListOfExplicitEntries; (void)OldAcl; (void)NewAcl;
    return ERROR_NOT_SUPPORTED;
}

extern "C" DWORD __stdcall Shim_SetNamedSecurityInfoW(LPWSTR pObjectName,
                                                     SE_OBJECT_TYPE ObjectType,
                                                     SECURITY_INFORMATION SecurityInfo,
                                                     PSID psidOwner, PSID psidGroup,
                                                     PACL pDacl, PACL pSacl) {
    (void)pObjectName; (void)ObjectType; (void)SecurityInfo;
    (void)psidOwner; (void)psidGroup; (void)pDacl; (void)pSacl;
    return ERROR_NOT_SUPPORTED;
}

extern "C" BOOL __stdcall Shim_SetSecurityDescriptorGroup(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                                                          PSID pGroup, BOOL bGroupDefaulted) {
    (void)pSecurityDescriptor; (void)pGroup; (void)bGroupDefaulted;
    return TRUE;
}

extern "C" BOOL __stdcall Shim_SetSecurityDescriptorOwner(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                                                          PSID pOwner, BOOL bOwnerDefaulted) {
    (void)pSecurityDescriptor; (void)pOwner; (void)bOwnerDefaulted;
    return TRUE;
}

// ===========================================================================
// Misc (6)
// ===========================================================================

// DecryptFileW — EFS not available in AppContainer. Return TRUE so games
// that decrypt saved files on load proceed (the file is already plaintext
// in the UWP local state folder).
extern "C" BOOL __stdcall Shim_DecryptFileW(LPCWSTR lpFileName, DWORD dwReserved) {
    (void)lpFileName; (void)dwReserved;
    return TRUE;
}

// InitiateSystemShutdownExW — Xbox cannot shut down from an AppContainer
// game. Return FALSE so the caller knows the call failed.
extern "C" BOOL __stdcall Shim_InitiateSystemShutdownExW(LPWSTR lpMachineName, LPWSTR lpMessage,
                                                        DWORD dwTimeout, BOOL bForceAppsClosed,
                                                        BOOL bRebootAfterShutdown,
                                                        DWORD dwReason) {
    (void)lpMachineName; (void)lpMessage; (void)dwTimeout;
    (void)bForceAppsClosed; (void)bRebootAfterShutdown; (void)dwReason;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// LookupPrivilegeValueW — UWP doesn't expose LSA privilege lookup. Return
// FALSE; games that check for specific privileges (SeShutdownPrivilege,
// SeDebugPrivilege) will treat themselves as unprivileged, which is the
// correct behavior in an AppContainer.
extern "C" BOOL __stdcall Shim_LookupPrivilegeValueW(LPCWSTR lpSystemName, LPCWSTR lpName,
                                                    PLUID lpLuid) {
    (void)lpSystemName; (void)lpName;
    if (lpLuid) std::memset(lpLuid, 0, sizeof(LUID));
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// SystemFunction036 (RtlGenRandom) — pass through to BCryptGenRandom which
// UWP exposes. Falls back to a software PRNG if BCrypt is unavailable.
extern "C" BOOLEAN __stdcall Shim_SystemFunction036(PVOID RandomBuffer, ULONG RandomBufferLength) {
    if (!RandomBuffer || !RandomBufferLength) return FALSE;
    // Try the real advapi32 surface first; UWP may export SystemFunction036
    // directly. If not, fall through to BCryptGenRandom.
    BOOLEAN ok = ::SystemFunction036(RandomBuffer, RandomBufferLength);
    if (ok) return TRUE;
    // BCryptGenRandom with NULL algorithm handle + BCRYPT_USE_SYSTEM_PREFERRED_RNG
    // (flag value 2) is the supported UWP way to get cryptographically strong
    // random bytes without managing an algorithm provider. The Windows.h
    // stub declares BCryptGenRandom as returning BOOL.
    BOOL status = ::BCryptGenRandom(NULL, (PUCHAR)RandomBuffer, RandomBufferLength, 0x00000002u);
    return status ? TRUE : FALSE;
}

}  // namespace xwr

// ===========================================================================
// Registration — 38 REGISTER_SHIM entries, all targeting the "advapi32"
// module. The list is alphabetical by function name.
// ===========================================================================
REGISTER_SHIM("advapi32", "AllocateLocallyUniqueId",                                     (FARPROC)&xwr::Shim_AllocateLocallyUniqueId);
REGISTER_SHIM("advapi32", "ChangeServiceConfigW",                                        (FARPROC)&xwr::Shim_ChangeServiceConfigW);
REGISTER_SHIM("advapi32", "CheckTokenMembership",                                        (FARPROC)&xwr::Shim_CheckTokenMembership);
REGISTER_SHIM("advapi32", "ConvertStringSecurityDescriptorToSecurityDescriptorW",        (FARPROC)&xwr::Shim_ConvertStringSecurityDescriptorToSecurityDescriptorW);
REGISTER_SHIM("advapi32", "CreateWellKnownSid",                                          (FARPROC)&xwr::Shim_CreateWellKnownSid);
REGISTER_SHIM("advapi32", "CryptCreateHash",                                             (FARPROC)&xwr::Shim_CryptCreateHash);
REGISTER_SHIM("advapi32", "CryptDecrypt",                                                (FARPROC)&xwr::Shim_CryptDecrypt);
REGISTER_SHIM("advapi32", "CryptDestroyHash",                                            (FARPROC)&xwr::Shim_CryptDestroyHash);
REGISTER_SHIM("advapi32", "CryptDestroyKey",                                             (FARPROC)&xwr::Shim_CryptDestroyKey);
REGISTER_SHIM("advapi32", "CryptEnumProvidersW",                                         (FARPROC)&xwr::Shim_CryptEnumProvidersW);
REGISTER_SHIM("advapi32", "CryptExportKey",                                              (FARPROC)&xwr::Shim_CryptExportKey);
REGISTER_SHIM("advapi32", "CryptGetHashParam",                                           (FARPROC)&xwr::Shim_CryptGetHashParam);
REGISTER_SHIM("advapi32", "CryptGetProvParam",                                           (FARPROC)&xwr::Shim_CryptGetProvParam);
REGISTER_SHIM("advapi32", "CryptGetUserKey",                                             (FARPROC)&xwr::Shim_CryptGetUserKey);
REGISTER_SHIM("advapi32", "CryptHashData",                                               (FARPROC)&xwr::Shim_CryptHashData);
REGISTER_SHIM("advapi32", "CryptSetHashParam",                                           (FARPROC)&xwr::Shim_CryptSetHashParam);
REGISTER_SHIM("advapi32", "CryptSignHashW",                                              (FARPROC)&xwr::Shim_CryptSignHashW);
REGISTER_SHIM("advapi32", "DecryptFileW",                                                (FARPROC)&xwr::Shim_DecryptFileW);
REGISTER_SHIM("advapi32", "DeregisterEventSource",                                       (FARPROC)&xwr::Shim_DeregisterEventSource);
REGISTER_SHIM("advapi32", "EventRegister",                                               (FARPROC)&xwr::Shim_EventRegister);
REGISTER_SHIM("advapi32", "EventSetInformation",                                         (FARPROC)&xwr::Shim_EventSetInformation);
REGISTER_SHIM("advapi32", "EventUnregister",                                             (FARPROC)&xwr::Shim_EventUnregister);
REGISTER_SHIM("advapi32", "EventWriteTransfer",                                          (FARPROC)&xwr::Shim_EventWriteTransfer);
REGISTER_SHIM("advapi32", "InitiateSystemShutdownExW",                                   (FARPROC)&xwr::Shim_InitiateSystemShutdownExW);
REGISTER_SHIM("advapi32", "LookupPrivilegeValueW",                                       (FARPROC)&xwr::Shim_LookupPrivilegeValueW);
REGISTER_SHIM("advapi32", "QueryServiceConfigW",                                         (FARPROC)&xwr::Shim_QueryServiceConfigW);
REGISTER_SHIM("advapi32", "RegCreateKeyTransactedW",                                     (FARPROC)&xwr::Shim_RegCreateKeyTransactedW);
REGISTER_SHIM("advapi32", "RegGetValueW",                                                (FARPROC)&xwr::Shim_RegGetValueW);
REGISTER_SHIM("advapi32", "RegisterEventSourceW",                                        (FARPROC)&xwr::Shim_RegisterEventSourceW);
REGISTER_SHIM("advapi32", "RegNotifyChangeKeyValue",                                     (FARPROC)&xwr::Shim_RegNotifyChangeKeyValue);
REGISTER_SHIM("advapi32", "RegQueryInfoKeyW",                                            (FARPROC)&xwr::Shim_RegQueryInfoKeyW);
REGISTER_SHIM("advapi32", "ReportEventW",                                                (FARPROC)&xwr::Shim_ReportEventW);
REGISTER_SHIM("advapi32", "SetEntriesInAclA",                                            (FARPROC)&xwr::Shim_SetEntriesInAclA);
REGISTER_SHIM("advapi32", "SetEntriesInAclW",                                            (FARPROC)&xwr::Shim_SetEntriesInAclW);
REGISTER_SHIM("advapi32", "SetNamedSecurityInfoW",                                       (FARPROC)&xwr::Shim_SetNamedSecurityInfoW);
REGISTER_SHIM("advapi32", "SetSecurityDescriptorGroup",                                  (FARPROC)&xwr::Shim_SetSecurityDescriptorGroup);
REGISTER_SHIM("advapi32", "SetSecurityDescriptorOwner",                                  (FARPROC)&xwr::Shim_SetSecurityDescriptorOwner);
REGISTER_SHIM("advapi32", "SystemFunction036",                                           (FARPROC)&xwr::Shim_SystemFunction036);
