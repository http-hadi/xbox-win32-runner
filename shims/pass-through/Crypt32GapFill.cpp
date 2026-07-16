// shims/pass-through/Crypt32GapFill.cpp
// Implements the 24 missing CRYPT32.dll functions reported by the coverage
// gap scan. Breakdown of the 24 gaps:
//
//   Certificate store (8):
//       CertOpenStore, CertOpenSystemStoreA, CertOpenSystemStoreW,
//       CertCloseStore, CertEnumCertificatesInStore,
//       CertAddCertificateContextToStore, CertCreateContext,
//       CertDuplicateCertificateContext.
//
//   Certificate context lifetime (3):
//       CertFreeCertificateContext, CertFreeCertificateChain,
//       CertFreeCertificateChainEngine.
//
//   Certificate chain (3):
//       CertGetCertificateChain, CertCreateCertificateChainEngine,
//       CertVerifyCertificateChainPolicy.
//
//   Certificate lookup / properties (6):
//       CertFindCertificateInStore, CertGetCertificateContextProperty,
//       CertSetCertificateContextProperty, CertGetEnhancedKeyUsage,
//       CertGetIntendedKeyUsage, CertGetNameStringA, CertGetNameStringW.
//
//   PFX / private key / hash (3):
//       CryptAcquireCertificatePrivateKey, CryptHashPublicKeyInfo,
//       PFXExportCertStoreEx.
//
// Strategy: ALL certificate functions stub (return FALSE or 0). Games rarely
// use certificate APIs, and when they do it's for online services (DRM
// verification, telemetry signing) that won't work on Xbox anyway. Stubs
// let the game's init code fail-fast through its normal "no cert available"
// path instead of crashing on a missing export.
//
//   - CertOpenSystemStoreW / CertOpenSystemStoreA / CertOpenStore return 0
//     (no store).
//   - CertGetCertificateChain returns FALSE (no chain).
//   - CertEnumCertificatesInStore / CertFindCertificateInStore return NULL
//     (no cert).
//   - CertCloseStore / CertFreeCertificateContext / CertFreeCertificateChain
//     / CertFreeCertificateChainEngine return TRUE (or void) so cleanup
//     paths proceed.
//   - CertGetNameStringA/W return 0 (no name string written).
//   - CryptAcquireCertificatePrivateKey / CryptHashPublicKeyInfo /
//     PFXExportCertStoreEx return FALSE.
//
// Each function is `extern "C" __stdcall Shim_<OriginalName>` so the PE
// loader can drop its address directly into a game's IAT.
//
// REGISTER_SHIM count: 24 (one per missing function).
//
// Added by Task ID GAP-ADVAPI32-CRYPT32.

#include "UwpSdkIncludes.h"
#include <Windows.h>

#include "../ShimRegistry.h"

#ifndef ERROR_NOT_SUPPORTED
#define ERROR_NOT_SUPPORTED 50L
#endif

namespace xwr {

// ===========================================================================
// Certificate store (8)
// ===========================================================================
extern "C" BOOL __stdcall Shim_CertAddCertificateContextToStore(HCERTSTORE hCertStore,
                                                               PCCERT_CONTEXT pCertContext,
                                                               DWORD dwAddDisposition,
                                                               PCCERT_CONTEXT* ppStoreContext) {
    (void)hCertStore; (void)pCertContext; (void)dwAddDisposition;
    if (ppStoreContext) *ppStoreContext = NULL;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_CertCloseStore(HCERTSTORE hCertStore, DWORD dwFlags) {
    (void)hCertStore; (void)dwFlags;
    return TRUE;
}

extern "C" PCCERT_CONTEXT __stdcall Shim_CertCreateContext(DWORD dwContextType, DWORD dwEncodingType,
                                                          const BYTE* pbEncoded, DWORD cbEncoded,
                                                          DWORD dwFlags, void* pCreatePara) {
    (void)dwContextType; (void)dwEncodingType; (void)pbEncoded;
    (void)cbEncoded; (void)dwFlags; (void)pCreatePara;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

extern "C" PCCERT_CONTEXT __stdcall Shim_CertDuplicateCertificateContext(PCCERT_CONTEXT pCertContext) {
    // Real CertDuplicateCertificateContext returns the same pointer (it just
    // bumps a refcount). For the stub we return the input so callers that
    // immediately free the duplicate don't crash; the underlying "context"
    // doesn't exist anyway.
    return pCertContext;
}

extern "C" PCCERT_CONTEXT __stdcall Shim_CertEnumCertificatesInStore(HCERTSTORE hCertStore,
                                                                    PCCERT_CONTEXT pPrevCertContext) {
    (void)hCertStore;
    // Real semantics: passing pPrevCertContext=NULL starts enumeration;
    // passing a previous context returns the next. We always return NULL
    // (no certificates in the (nonexistent) store).
    (void)pPrevCertContext;
    return NULL;
}

extern "C" PCCERT_CONTEXT __stdcall Shim_CertFindCertificateInStore(HCERTSTORE hCertStore,
                                                                   DWORD dwCertEncodingType,
                                                                   DWORD dwFindFlags,
                                                                   DWORD dwFindType,
                                                                   const void* pvFindPara,
                                                                   PCCERT_CONTEXT pPrevCertContext) {
    (void)hCertStore; (void)dwCertEncodingType; (void)dwFindFlags;
    (void)dwFindType; (void)pvFindPara; (void)pPrevCertContext;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

extern "C" HCERTSTORE __stdcall Shim_CertOpenStore(LPCSTR lpszStoreProvider, DWORD dwEncodingType,
                                                  HCRYPTPROV hCryptProv, DWORD dwFlags,
                                                  const void* pvPara) {
    (void)lpszStoreProvider; (void)dwEncodingType; (void)hCryptProv;
    (void)dwFlags; (void)pvPara;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (HCERTSTORE)0;
}

extern "C" HCERTSTORE __stdcall Shim_CertOpenSystemStoreA(HCRYPTPROV hCryptProv,
                                                         LPCSTR szSubsystemProtocol) {
    (void)hCryptProv; (void)szSubsystemProtocol;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (HCERTSTORE)0;
}

extern "C" HCERTSTORE __stdcall Shim_CertOpenSystemStoreW(HCRYPTPROV hCryptProv,
                                                         LPCWSTR szSubsystemProtocol) {
    (void)hCryptProv; (void)szSubsystemProtocol;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (HCERTSTORE)0;
}

// ===========================================================================
// Certificate context lifetime (3) — silent success
// ===========================================================================
extern "C" void __stdcall Shim_CertFreeCertificateChain(PCCERT_CHAIN_CONTEXT pChainContext) {
    (void)pChainContext;
}

extern "C" void __stdcall Shim_CertFreeCertificateChainEngine(HCERTCHAINENGINE hChainEngine) {
    (void)hChainEngine;
}

extern "C" BOOL __stdcall Shim_CertFreeCertificateContext(PCCERT_CONTEXT pCertContext) {
    (void)pCertContext;
    return TRUE;
}

// ===========================================================================
// Certificate chain (3)
// ===========================================================================
extern "C" HCERTCHAINENGINE __stdcall Shim_CertCreateCertificateChainEngine(const void* pConfig) {
    (void)pConfig;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (HCERTCHAINENGINE)0;
}

extern "C" BOOL __stdcall Shim_CertGetCertificateChain(HCERTCHAINENGINE hChainEngine,
                                                      PCCERT_CONTEXT pCertContext,
                                                      LPFILETIME pTime,
                                                      HCERTSTORE hAdditionalStore,
                                                      const void* pChainPara,
                                                      DWORD dwFlags,
                                                      void* pvReserved,
                                                      PCCERT_CHAIN_CONTEXT* ppChainContext) {
    (void)hChainEngine; (void)pCertContext; (void)pTime; (void)hAdditionalStore;
    (void)pChainPara; (void)dwFlags; (void)pvReserved;
    if (ppChainContext) *ppChainContext = NULL;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_CertVerifyCertificateChainPolicy(LPCSTR pszPolicyOID,
                                                               PCCERT_CHAIN_CONTEXT pChainContext,
                                                               void* pPolicyPara,
                                                               void* pPolicyStatus) {
    (void)pszPolicyOID; (void)pChainContext; (void)pPolicyPara; (void)pPolicyStatus;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// ===========================================================================
// Certificate lookup / properties (6)
// ===========================================================================
extern "C" BOOL __stdcall Shim_CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
                                                                DWORD dwPropId,
                                                                void* pvData,
                                                                DWORD* pcbData) {
    (void)pCertContext; (void)dwPropId; (void)pvData;
    if (pcbData) *pcbData = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_CertGetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext, DWORD dwFlags,
                                                      void* pUsage, DWORD* pcbUsage) {
    (void)pCertContext; (void)dwFlags; (void)pUsage;
    if (pcbUsage) *pcbUsage = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_CertGetIntendedKeyUsage(PCCERT_CONTEXT pCertContext,
                                                      BYTE* pbKeyUsage, DWORD cbKeyUsage) {
    (void)pCertContext;
    if (pbKeyUsage && cbKeyUsage) std::memset(pbKeyUsage, 0, cbKeyUsage);
    return FALSE;
}

extern "C" DWORD __stdcall Shim_CertGetNameStringA(PCCERT_CONTEXT pCertContext, DWORD dwType,
                                                   DWORD dwFlags, void* pvTypePara,
                                                   LPSTR pszNameString, DWORD cchNameString) {
    (void)pCertContext; (void)dwType; (void)dwFlags; (void)pvTypePara;
    (void)pszNameString; (void)cchNameString;
    return 0;
}

extern "C" DWORD __stdcall Shim_CertGetNameStringW(PCCERT_CONTEXT pCertContext, DWORD dwType,
                                                   DWORD dwFlags, void* pvTypePara,
                                                   LPWSTR pszNameString, DWORD cchNameString) {
    (void)pCertContext; (void)dwType; (void)dwFlags; (void)pvTypePara;
    (void)pszNameString; (void)cchNameString;
    return 0;
}

extern "C" BOOL __stdcall Shim_CertSetCertificateContextProperty(PCCERT_CONTEXT pCertContext,
                                                                DWORD dwPropId, DWORD dwFlags,
                                                                const void* pvData) {
    (void)pCertContext; (void)dwPropId; (void)dwFlags; (void)pvData;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// ===========================================================================
// PFX / private key / hash (3)
// ===========================================================================
extern "C" BOOL __stdcall Shim_CryptAcquireCertificatePrivateKey(PCCERT_CONTEXT pCert,
                                                                DWORD dwFlags,
                                                                void* pvParameters,
                                                                void* phCryptProvOrNCryptKey,
                                                                DWORD* pdwKeySpec,
                                                                BOOL* pfCallerFreeProvOrNCryptKey) {
    (void)pCert; (void)dwFlags; (void)pvParameters;
    if (phCryptProvOrNCryptKey) *(void**)phCryptProvOrNCryptKey = NULL;
    if (pdwKeySpec) *pdwKeySpec = 0;
    if (pfCallerFreeProvOrNCryptKey) *pfCallerFreeProvOrNCryptKey = FALSE;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_CryptHashPublicKeyInfo(HCRYPTPROV hCryptProv, ALG_ID Algid,
                                                     DWORD dwFlags, DWORD dwCertEncodingType,
                                                     const void* pInfo, BYTE* pbHash,
                                                     DWORD* pcbHash) {
    (void)hCryptProv; (void)Algid; (void)dwFlags; (void)dwCertEncodingType;
    (void)pInfo; (void)pbHash;
    if (pcbHash) *pcbHash = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

extern "C" BOOL __stdcall Shim_PFXExportCertStoreEx(HCERTSTORE hStore, PCRYPT_DATA_BLOB pPFX,
                                                   LPCWSTR szPassword, DWORD dwFlags, void* pvReserved) {
    (void)hStore; (void)szPassword; (void)dwFlags; (void)pvReserved;
    if (pPFX) {
        pPFX->cbData = 0;
        pPFX->pbData = NULL;
    }
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

}  // namespace xwr

// ===========================================================================
// Registration — 24 REGISTER_SHIM entries, all targeting the "crypt32"
// module. The list is alphabetical by function name.
// ===========================================================================
REGISTER_SHIM("crypt32", "CertAddCertificateContextToStore", (FARPROC)&xwr::Shim_CertAddCertificateContextToStore);
REGISTER_SHIM("crypt32", "CertCloseStore",                    (FARPROC)&xwr::Shim_CertCloseStore);
REGISTER_SHIM("crypt32", "CertCreateCertificateChainEngine",  (FARPROC)&xwr::Shim_CertCreateCertificateChainEngine);
REGISTER_SHIM("crypt32", "CertCreateContext",                 (FARPROC)&xwr::Shim_CertCreateContext);
REGISTER_SHIM("crypt32", "CertDuplicateCertificateContext",   (FARPROC)&xwr::Shim_CertDuplicateCertificateContext);
REGISTER_SHIM("crypt32", "CertEnumCertificatesInStore",       (FARPROC)&xwr::Shim_CertEnumCertificatesInStore);
REGISTER_SHIM("crypt32", "CertFindCertificateInStore",        (FARPROC)&xwr::Shim_CertFindCertificateInStore);
REGISTER_SHIM("crypt32", "CertFreeCertificateChain",          (FARPROC)&xwr::Shim_CertFreeCertificateChain);
REGISTER_SHIM("crypt32", "CertFreeCertificateChainEngine",    (FARPROC)&xwr::Shim_CertFreeCertificateChainEngine);
REGISTER_SHIM("crypt32", "CertFreeCertificateContext",        (FARPROC)&xwr::Shim_CertFreeCertificateContext);
REGISTER_SHIM("crypt32", "CertGetCertificateChain",           (FARPROC)&xwr::Shim_CertGetCertificateChain);
REGISTER_SHIM("crypt32", "CertGetCertificateContextProperty", (FARPROC)&xwr::Shim_CertGetCertificateContextProperty);
REGISTER_SHIM("crypt32", "CertGetEnhancedKeyUsage",           (FARPROC)&xwr::Shim_CertGetEnhancedKeyUsage);
REGISTER_SHIM("crypt32", "CertGetIntendedKeyUsage",           (FARPROC)&xwr::Shim_CertGetIntendedKeyUsage);
REGISTER_SHIM("crypt32", "CertGetNameStringA",                (FARPROC)&xwr::Shim_CertGetNameStringA);
REGISTER_SHIM("crypt32", "CertGetNameStringW",                (FARPROC)&xwr::Shim_CertGetNameStringW);
REGISTER_SHIM("crypt32", "CertOpenStore",                     (FARPROC)&xwr::Shim_CertOpenStore);
REGISTER_SHIM("crypt32", "CertOpenSystemStoreA",              (FARPROC)&xwr::Shim_CertOpenSystemStoreA);
REGISTER_SHIM("crypt32", "CertOpenSystemStoreW",              (FARPROC)&xwr::Shim_CertOpenSystemStoreW);
REGISTER_SHIM("crypt32", "CertSetCertificateContextProperty", (FARPROC)&xwr::Shim_CertSetCertificateContextProperty);
REGISTER_SHIM("crypt32", "CertVerifyCertificateChainPolicy",  (FARPROC)&xwr::Shim_CertVerifyCertificateChainPolicy);
REGISTER_SHIM("crypt32", "CryptAcquireCertificatePrivateKey", (FARPROC)&xwr::Shim_CryptAcquireCertificatePrivateKey);
REGISTER_SHIM("crypt32", "CryptHashPublicKeyInfo",            (FARPROC)&xwr::Shim_CryptHashPublicKeyInfo);
REGISTER_SHIM("crypt32", "PFXExportCertStoreEx",              (FARPROC)&xwr::Shim_PFXExportCertStoreEx);
