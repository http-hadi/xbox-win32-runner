// shims/advapi32/Advapi32Shim.cpp
// Win32 advapi32 shim layer. Provides an in-memory registry simulation
// plus stubs for token / crypto APIs that UWP doesn't fully expose.

#include "UwpSdkIncludes.h"


#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <cwctype>
#include <cstring>

#include "../ShimRegistry.h"

#ifndef ERROR_MORE_DATA
#define ERROR_MORE_DATA 234L
#endif
#ifndef ERROR_NO_MORE_ITEMS
#define ERROR_NO_MORE_ITEMS 259L
#endif

// Token / crypto constants that the UWP SDK subset omits.
#ifndef TOKEN_QUERY
#define TOKEN_QUERY 0x0008
#endif
#ifndef TOKEN_DUPLICATE
#define TOKEN_DUPLICATE 0x0002
#endif
#ifndef TOKEN_IMPERSONATE
#define TOKEN_IMPERSONATE 0x0004
#endif
#ifndef TokenUser
#define TokenUser 1
#endif
#ifndef TokenGroups
#define TokenGroups 2
#endif
#ifndef TokenPrivileges
#define TokenPrivileges 3
#endif
#ifndef TokenOwner
#define TokenOwner 4
#endif
#ifndef TokenPrimaryGroup
#define TokenPrimaryGroup 5
#endif
#ifndef TokenDefaultDacl
#define TokenDefaultDacl 6
#endif
#ifndef TokenSource
#define TokenSource 7
#endif
#ifndef TokenType
#define TokenType 8
#endif
#ifndef TokenImpersonationLevel
#define TokenImpersonationLevel 9
#endif
#ifndef TokenStatistics
#define TokenStatistics 10
#endif
#ifndef PROV_RSA_FULL
#define PROV_RSA_FULL 1
#endif
#ifndef PROV_RSA_SIG
#define PROV_RSA_SIG 2
#endif
#ifndef PROV_DSS
#define PROV_DSS 3
#endif
#ifndef PROV_RSA_AES
#define PROV_RSA_AES 24
#endif
#ifndef CRYPT_VERIFYCONTEXT
#define CRYPT_VERIFYCONTEXT 0xF0000000
#endif
#ifndef CRYPT_NEWKEYSET
#define CRYPT_NEWKEYSET 0x00000008
#endif
#ifndef CRYPT_DELETEKEYSET
#define CRYPT_DELETEKEYSET 0x00000010
#endif
#ifndef CRYPT_MACHINE_KEYSET
#define CRYPT_MACHINE_KEYSET 0x00000020
#endif
#ifndef NERR_BufTooSmall
#define NERR_BufTooSmall 2123
#endif
#ifndef NERR_Success
#define NERR_Success 0
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// Registry store: parent HKEY + subkey path → map of value name → bytes
// ---------------------------------------------------------------------------
struct RegKey {
    HKEY         parent = 0;
    std::wstring subkey;
};

static std::atomic<uint32_t> g_nextHkey{0x100000};
static std::atomic<uint32_t> g_nextToken{0x200000};
static std::atomic<uint32_t> g_nextCrypt{0x300000};

static std::mutex& RegMutex() {
    static std::mutex m;
    return m;
}
static std::map<std::pair<HANDLE, std::wstring>, RegKey>& RegKeyTable() {
    static std::map<std::pair<HANDLE, std::wstring>, RegKey> t;
    return t;
}
// Mapping: fake HKEY → (parent + subkey)
static std::map<HKEY, RegKey>& RegHkeyTable() {
    static std::map<HKEY, RegKey> t;
    return t;
}
// Mapping: (fake HKEY, value name) → (type, bytes)
static std::map<std::pair<HKEY, std::wstring>, std::pair<DWORD, std::vector<BYTE>>>& RegValueTable() {
    static std::map<std::pair<HKEY, std::wstring>, std::pair<DWORD, std::vector<BYTE>>> t;
    return t;
}

static HKEY AllocRegKey(HKEY parent, const std::wstring& subkey) {
    HKEY h = (HKEY)g_nextHkey.fetch_add(1);
    RegKey rk{parent, subkey};
    RegKeyTable()[std::make_pair((HANDLE)parent, subkey)] = rk;
    RegHkeyTable()[h] = rk;
    return h;
}

static std::wstring Lower(const std::wstring& s) {
    std::wstring r = s;
    for (auto& c : r) c = static_cast<wchar_t>(towlower(c));
    return r;
}

// ---------------------------------------------------------------------------
// RegCreateKey / RegOpenKey
// ---------------------------------------------------------------------------
extern "C" LONG __stdcall Shim_RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions,
                                              REGSAM samDesired, PHKEY phkResult) {
    (void)ulOptions; (void)samDesired;
    if (!phkResult) return ERROR_INVALID_PARAMETER;
    std::wstring sk = lpSubKey ? lpSubKey : L"";
    std::lock_guard<std::mutex> lk(RegMutex());
    *phkResult = AllocRegKey(hKey, sk);
    return ERROR_SUCCESS;
}
extern "C" LONG __stdcall Shim_RegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) {
    return Shim_RegOpenKeyExW(hKey, lpSubKey, 0, 0, phkResult);
}
extern "C" LONG __stdcall Shim_RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved,
                                                LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired,
                                                LPSECURITY_ATTRIBUTES lpSecAttr, PHKEY phkResult,
                                                LPDWORD lpdwDisposition) {
    (void)Reserved; (void)lpClass; (void)dwOptions; (void)samDesired; (void)lpSecAttr;
    if (lpdwDisposition) *lpdwDisposition = 2;  // REG_OPENED_EXISTING_KEY
    return Shim_RegOpenKeyExW(hKey, lpSubKey, 0, 0, phkResult);
}
extern "C" LONG __stdcall Shim_RegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) {
    return Shim_RegOpenKeyExW(hKey, lpSubKey, 0, 0, phkResult);
}
extern "C" LONG __stdcall Shim_RegCloseKey(HKEY hKey) {
    std::lock_guard<std::mutex> lk(RegMutex());
    RegHkeyTable().erase(hKey);
    return ERROR_SUCCESS;
}
extern "C" LONG __stdcall Shim_RegFlushKey(HKEY) { return ERROR_SUCCESS; }

// ---------------------------------------------------------------------------
// Value queries
// ---------------------------------------------------------------------------
extern "C" LONG __stdcall Shim_RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
                                                 LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    (void)lpReserved;
    std::wstring name = lpValueName ? lpValueName : L"";
    std::lock_guard<std::mutex> lk(RegMutex());
    auto it = RegValueTable().find(std::make_pair(hKey, Lower(name)));
    if (it == RegValueTable().end()) {
        if (lpcbData) *lpcbData = 0;
        return ERROR_FILE_NOT_FOUND;
    }
    if (lpType) *lpType = it->second.first;
    if (lpData && lpcbData) {
        if (*lpcbData < it->second.second.size()) {
            *lpcbData = (DWORD)it->second.second.size();
            return ERROR_MORE_DATA;
        }
        std::memcpy(lpData, it->second.second.data(), it->second.second.size());
    }
    if (lpcbData) *lpcbData = (DWORD)it->second.second.size();
    return ERROR_SUCCESS;
}
extern "C" LONG __stdcall Shim_RegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved,
                                                DWORD dwType, const BYTE* lpData, DWORD cbData) {
    (void)Reserved;
    std::wstring name = lpValueName ? lpValueName : L"";
    std::vector<BYTE> bytes;
    if (lpData && cbData) bytes.assign(lpData, lpData + cbData);
    std::lock_guard<std::mutex> lk(RegMutex());
    RegValueTable()[std::make_pair(hKey, Lower(name))] = std::make_pair(dwType, std::move(bytes));
    return ERROR_SUCCESS;
}
extern "C" LONG __stdcall Shim_RegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey) {
    (void)hKey; (void)lpSubKey;
    return ERROR_SUCCESS;
}
extern "C" LONG __stdcall Shim_RegDeleteValueW(HKEY hKey, LPCWSTR lpValueName) {
    std::wstring name = lpValueName ? lpValueName : L"";
    std::lock_guard<std::mutex> lk(RegMutex());
    RegValueTable().erase(std::make_pair(hKey, Lower(name)));
    return ERROR_SUCCESS;
}
extern "C" LONG __stdcall Shim_RegEnumKeyExW(HKEY, DWORD, LPWSTR lpName, LPDWORD lpcchName,
                                               LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcchClass,
                                               PFILETIME lpftLastWriteTime) {
    if (lpName && lpcchName) *lpName = L'\0';
    if (lpcchName) *lpcchName = 0;
    if (lpClass) *lpClass = L'\0';
    if (lpcchClass) *lpcchClass = 0;
    if (lpftLastWriteTime) std::memset(lpftLastWriteTime, 0, sizeof(FILETIME));
    return ERROR_NO_MORE_ITEMS;
}
extern "C" LONG __stdcall Shim_RegEnumValueW(HKEY, DWORD, LPWSTR lpValueName, LPDWORD lpcchValueName,
                                               LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                                               LPDWORD lpcbData) {
    (void)lpReserved; (void)lpType; (void)lpData; (void)lpcbData;
    if (lpValueName && lpcchValueName) *lpValueName = L'\0';
    if (lpcchValueName) *lpcchValueName = 0;
    return ERROR_NO_MORE_ITEMS;
}

// ---------------------------------------------------------------------------
// Services — return failure gracefully
// ---------------------------------------------------------------------------
extern "C" SC_HANDLE __stdcall Shim_OpenSCManagerW(LPCWSTR lpMachineName, LPCWSTR lpDatabaseName, DWORD dwDesiredAccess) {
    (void)lpMachineName; (void)lpDatabaseName; (void)dwDesiredAccess;
    return (SC_HANDLE)0;
}
extern "C" BOOL __stdcall Shim_CloseServiceHandle(SC_HANDLE hSCObject) { (void)hSCObject; return TRUE; }
extern "C" SC_HANDLE __stdcall Shim_CreateServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, LPCWSTR lpDisplayName,
                                                    DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType,
                                                    DWORD dwErrorControl, LPCWSTR lpBinaryPathName, LPWSTR lpLoadOrderGroup,
                                                    LPDWORD lpdwTagId, LPCWSTR lpDependencies, LPCWSTR lpServiceStartName,
                                                    LPCWSTR lpPassword) {
    (void)hSCManager; (void)lpServiceName; (void)lpDisplayName; (void)dwDesiredAccess;
    (void)dwServiceType; (void)dwStartType; (void)dwErrorControl; (void)lpBinaryPathName;
    (void)lpLoadOrderGroup; (void)lpdwTagId; (void)lpDependencies; (void)lpServiceStartName; (void)lpPassword;
    return (SC_HANDLE)0;
}
extern "C" SC_HANDLE __stdcall Shim_OpenServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, DWORD dwDesiredAccess) {
    (void)hSCManager; (void)lpServiceName; (void)dwDesiredAccess;
    return (SC_HANDLE)0;
}
extern "C" BOOL __stdcall Shim_DeleteService(SC_HANDLE hService) { (void)hService; return FALSE; }
extern "C" BOOL __stdcall Shim_StartServiceW(SC_HANDLE hService, DWORD dwNumServiceArgs, LPCWSTR* lpServiceArgVectors) {
    (void)hService; (void)dwNumServiceArgs; (void)lpServiceArgVectors;
    return FALSE;
}
extern "C" BOOL __stdcall Shim_ControlService(SC_HANDLE hService, DWORD dwControl, LPSERVICE_STATUS lpServiceStatus) {
    (void)hService; (void)dwControl; (void)lpServiceStatus;
    return FALSE;
}
extern "C" BOOL __stdcall Shim_QueryServiceStatus(SC_HANDLE hService, LPSERVICE_STATUS lpServiceStatus) {
    (void)hService; (void)lpServiceStatus;
    return FALSE;
}

// ---------------------------------------------------------------------------
// SID / Security
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_AllocateAndInitializeSid(const void*, BYTE, DWORD, DWORD, DWORD,
                                                        DWORD, DWORD, DWORD, DWORD, DWORD, void**) {
    return FALSE;
}
extern "C" void* __stdcall Shim_InitializeSecurityDescriptor(void* p, DWORD) {
    return p;
}
extern "C" BOOL __stdcall Shim_SetSecurityDescriptorDacl(void*, BOOL, void*, BOOL) { return TRUE; }
extern "C" BOOL __stdcall Shim_InitializeAcl(void*, DWORD, DWORD) { return TRUE; }
extern "C" BOOL __stdcall Shim_AddAccessAllowedAce(void*, DWORD, DWORD, void*) { return TRUE; }

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_OpenProcessToken(HANDLE, DWORD, PHKEY phToken) {
    if (!phToken) return FALSE;
    *phToken = (HKEY)g_nextToken.fetch_add(1);
    return TRUE;
}
extern "C" BOOL __stdcall Shim_OpenThreadToken(HANDLE, DWORD, BOOL, PHKEY phToken) {
    if (!phToken) return FALSE;
    *phToken = (HKEY)g_nextToken.fetch_add(1);
    return TRUE;
}
extern "C" BOOL __stdcall Shim_GetTokenInformation(HANDLE, int, LPVOID, DWORD, LPDWORD) {
    return FALSE;
}
extern "C" BOOL __stdcall Shim_AdjustTokenPrivileges(HANDLE, BOOL, void*, DWORD, void*, PDWORD) {
    return TRUE;
}
extern "C" BOOL __stdcall Shim_GetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer) {
    if (!pcbBuffer) return FALSE;
    const wchar_t name[] = L"XboxUser";
    size_t need = (sizeof(name) / sizeof(wchar_t));  // includes terminator
    if (!lpBuffer || *pcbBuffer < (DWORD)need) {
        *pcbBuffer = (DWORD)need;
        return FALSE;
    }
    std::memcpy(lpBuffer, name, need * sizeof(wchar_t));
    *pcbBuffer = (DWORD)need - 1;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_LookupAccountNameW(LPCWSTR, LPCWSTR, void*, LPDWORD, LPWSTR, LPDWORD, void*) {
    return FALSE;
}

// ---------------------------------------------------------------------------
// Crypto
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_CryptAcquireContextW(HCRYPTPROV* phProv, LPCWSTR, LPCWSTR,
                                                     DWORD dwProvType, DWORD dwFlags) {
    (void)dwProvType; (void)dwFlags;
    if (!phProv) return FALSE;
    *phProv = (HCRYPTPROV)g_nextCrypt.fetch_add(1);
    return TRUE;
}
extern "C" BOOL __stdcall Shim_CryptReleaseContext(HCRYPTPROV, DWORD) { return TRUE; }
extern "C" BOOL __stdcall Shim_CryptGenRandom(HCRYPTPROV, DWORD dwLen, BYTE* pbBuffer) {
    if (!pbBuffer || !dwLen) return FALSE;
    for (DWORD i = 0; i < dwLen; ++i) pbBuffer[i] = (BYTE)(rand() & 0xFF);
    return TRUE;
}

// ---------------------------------------------------------------------------
// BCrypt — pass through (UWP supports BCrypt directly)
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
REGISTER_SHIM("advapi32", "RegOpenKeyExW", (FARPROC)&xwr::Shim_RegOpenKeyExW);
REGISTER_SHIM("advapi32", "RegOpenKeyExA", (FARPROC)&xwr::Shim_RegOpenKeyExW);
REGISTER_SHIM("advapi32", "RegOpenKeyW", (FARPROC)&xwr::Shim_RegOpenKeyW);
REGISTER_SHIM("advapi32", "RegCreateKeyExW", (FARPROC)&xwr::Shim_RegCreateKeyExW);
REGISTER_SHIM("advapi32", "RegCreateKeyExA", (FARPROC)&xwr::Shim_RegCreateKeyExW);
REGISTER_SHIM("advapi32", "RegCreateKeyW", (FARPROC)&xwr::Shim_RegCreateKeyW);
REGISTER_SHIM("advapi32", "RegCloseKey", (FARPROC)&xwr::Shim_RegCloseKey);
REGISTER_SHIM("advapi32", "RegFlushKey", (FARPROC)&xwr::Shim_RegFlushKey);
REGISTER_SHIM("advapi32", "RegQueryValueExW", (FARPROC)&xwr::Shim_RegQueryValueExW);
REGISTER_SHIM("advapi32", "RegQueryValueExA", (FARPROC)&xwr::Shim_RegQueryValueExW);
REGISTER_SHIM("advapi32", "RegSetValueExW", (FARPROC)&xwr::Shim_RegSetValueExW);
REGISTER_SHIM("advapi32", "RegSetValueExA", (FARPROC)&xwr::Shim_RegSetValueExW);
REGISTER_SHIM("advapi32", "RegDeleteKeyW", (FARPROC)&xwr::Shim_RegDeleteKeyW);
REGISTER_SHIM("advapi32", "RegDeleteValueW", (FARPROC)&xwr::Shim_RegDeleteValueW);
REGISTER_SHIM("advapi32", "RegEnumKeyExW", (FARPROC)&xwr::Shim_RegEnumKeyExW);
REGISTER_SHIM("advapi32", "RegEnumValueW", (FARPROC)&xwr::Shim_RegEnumValueW);
REGISTER_SHIM("advapi32", "OpenSCManagerW", (FARPROC)&xwr::Shim_OpenSCManagerW);
REGISTER_SHIM("advapi32", "CloseServiceHandle", (FARPROC)&xwr::Shim_CloseServiceHandle);
REGISTER_SHIM("advapi32", "CreateServiceW", (FARPROC)&xwr::Shim_CreateServiceW);
REGISTER_SHIM("advapi32", "OpenServiceW", (FARPROC)&xwr::Shim_OpenServiceW);
REGISTER_SHIM("advapi32", "DeleteService", (FARPROC)&xwr::Shim_DeleteService);
REGISTER_SHIM("advapi32", "StartServiceW", (FARPROC)&xwr::Shim_StartServiceW);
REGISTER_SHIM("advapi32", "ControlService", (FARPROC)&xwr::Shim_ControlService);
REGISTER_SHIM("advapi32", "QueryServiceStatus", (FARPROC)&xwr::Shim_QueryServiceStatus);
REGISTER_SHIM("advapi32", "AllocateAndInitializeSid", (FARPROC)&xwr::Shim_AllocateAndInitializeSid);
REGISTER_SHIM("advapi32", "InitializeSecurityDescriptor", (FARPROC)&xwr::Shim_InitializeSecurityDescriptor);
REGISTER_SHIM("advapi32", "SetSecurityDescriptorDacl", (FARPROC)&xwr::Shim_SetSecurityDescriptorDacl);
REGISTER_SHIM("advapi32", "InitializeAcl", (FARPROC)&xwr::Shim_InitializeAcl);
REGISTER_SHIM("advapi32", "AddAccessAllowedAce", (FARPROC)&xwr::Shim_AddAccessAllowedAce);
REGISTER_SHIM("advapi32", "OpenProcessToken", (FARPROC)&xwr::Shim_OpenProcessToken);
REGISTER_SHIM("advapi32", "OpenThreadToken", (FARPROC)&xwr::Shim_OpenThreadToken);
REGISTER_SHIM("advapi32", "GetTokenInformation", (FARPROC)&xwr::Shim_GetTokenInformation);
REGISTER_SHIM("advapi32", "AdjustTokenPrivileges", (FARPROC)&xwr::Shim_AdjustTokenPrivileges);
REGISTER_SHIM("advapi32", "GetUserNameW", (FARPROC)&xwr::Shim_GetUserNameW);
REGISTER_SHIM("advapi32", "LookupAccountNameW", (FARPROC)&xwr::Shim_LookupAccountNameW);
REGISTER_SHIM("advapi32", "CryptAcquireContextW", (FARPROC)&xwr::Shim_CryptAcquireContextW);
REGISTER_SHIM("advapi32", "CryptReleaseContext", (FARPROC)&xwr::Shim_CryptReleaseContext);
REGISTER_SHIM("advapi32", "CryptGenRandom", (FARPROC)&xwr::Shim_CryptGenRandom);
REGISTER_SHIM("advapi32", "BCryptOpenAlgorithmProvider", (FARPROC)&xwr::Shim_BCryptOpenAlgorithmProvider);
REGISTER_SHIM("advapi32", "BCryptGenRandom", (FARPROC)&xwr::Shim_BCryptGenRandom);
REGISTER_SHIM("advapi32", "BCryptCloseAlgorithmProvider", (FARPROC)&xwr::Shim_BCryptCloseAlgorithmProvider);
