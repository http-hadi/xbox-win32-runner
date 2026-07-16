// pe-loader/PeLoader.cpp
// Implementation of the PE loader. See PeLoader.h for the high-level pipeline.
//
// Notes on what the loader intentionally does NOT do (out of scope for UWP):
//   * SxS / activation contexts (.manifest-based assembly binding).
//   * Authenticode signature verification.
//   * COM descriptor / CLR hosting (IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR).
//   * Bound imports (we resolve them as regular imports).
//   * Resource section walking (games usually don't load PE resources).
//   * Code integrity checks (the AppContainer already restricts us).
//
// All memory allocations use VirtualAllocFromApp / VirtualProtectFromApp so
// the loader works inside an AppContainer with the codeGeneration capability.

#include "UwpSdkIncludes.h"

#include "PeLoader.h"

#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cwctype>
#include <algorithm>

// Shims registry interface (declared in shims/ShimRegistry.h).
// We forward-declare the symbol we need rather than include the header, to
// keep the pe-loader independent of the shim layer.
namespace xwr {
class ShimRegistry;
}

// We include the registry header below to call its ResolveExport().
#include "../shims/ShimRegistry.h"
// API-set / VC-runtime forwarder entry points (ApiSetForwarder.cpp,
// VcRuntimePassthrough.cpp). Used by the PeLoader constructor to ensure the
// forwarders run after every REGISTER_SHIM static initializer, and by
// ResolveImport as a LoadLibraryW+GetProcAddress fallback for VC runtime /
// Universal CRT passthrough DLLs.
#include "../shims/ApiSetForwarder.h"

namespace xwr {

// ---------------------------------------------------------------------------
// ReadFileCallback override hook (for tests)
// ---------------------------------------------------------------------------
static ReadFileCallback g_readFileCb = nullptr;
void SetReadFileCallback(ReadFileCallback cb) { g_readFileCb = cb; }

// ---------------------------------------------------------------------------
// Default file reader - uses CreateFileW / ReadFile on UWP.
// Path translation: if the path begins with a drive letter or backslash, it
// is treated as a virtualized game path and redirected to the UWP local folder.
// (The actual path translation table lives in shims/kernel32; the loader only
// needs it for game-shipped DLLs.)
// ---------------------------------------------------------------------------
static bool ReadFileFromDisk(const std::wstring& path, std::vector<uint8_t>& out) {
    if (g_readFileCb) return g_readFileCb(path, out);
#ifdef _MSC_VER
    // UWP: use CreateFile2 (CreateFileW is not available in AppContainer)
    CREATEFILE2_EXTENDED_PARAMETERS params{};
    params.dwSize = sizeof(params);
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    params.dwFileFlags = 0;
    params.dwSecurityQosFlags = 0;
    params.lpSecurityAttributes = nullptr;
    params.hTemplateFile = nullptr;
    HANDLE h = ::CreateFile2(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              OPEN_EXISTING, &params);
#else
    HANDLE h = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz{};
    if (!::GetFileSizeEx(h, &sz)) { ::CloseHandle(h); return false; }
    out.resize(static_cast<size_t>(sz.QuadPart));
    DWORD got = 0;
    BOOL ok = ::ReadFile(h, out.data(), static_cast<DWORD>(out.size()), &got, nullptr);
    ::CloseHandle(h);
    return ok && got == out.size();
}

// ---------------------------------------------------------------------------
// PeLoader ctor / dtor
// ---------------------------------------------------------------------------
PeLoader::PeLoader() {
    // Ensure the API-set forwarder and VC runtime passthrough have been
    // applied. These are idempotent (guarded by std::once_flag), so calling
    // them from every PeLoader constructor is safe. By the time we reach
    // here, every REGISTER_SHIM static initializer across every shim TU has
    // already run (the UWP shell constructs the PeLoader well after process
    // startup), so the forwarders see the full registry.
    ApplyApiSetForwarders();
    ApplyVcRuntimePassthrough();
}
PeLoader::~PeLoader() {
    for (auto* m : m_modules) {
        if (m->baseAddress) {
            ::VirtualFree(reinterpret_cast<LPVOID>(m->baseAddress), 0, MEM_RELEASE);
        }
        if (m->exceptionTable) {
#ifdef _MSC_VER
            ::RtlDeleteFunctionTable(reinterpret_cast<PRUNTIME_FUNCTION>(m->exceptionTable));
#else
            ::RtlDeleteFunctionTable(m->exceptionTable);
#endif
        }
        delete m;
    }
}

void PeLoader::AddDllSearchDir(const std::wstring& dir) {
    m_searchDirs.push_back(dir);
}

// ---------------------------------------------------------------------------
// FindModule
// ---------------------------------------------------------------------------
LoadedModule* PeLoader::FindModule(const std::wstring& name) {
    std::wstring n = name;
    std::transform(n.begin(), n.end(), n.begin(), ::towlower);
    auto it = m_byName.find(n);
    return it == m_byName.end() ? nullptr : it->second;
}

// ---------------------------------------------------------------------------
// LoadModule(name)  — search dirs
// ---------------------------------------------------------------------------
LoadedModule* PeLoader::LoadModule(const std::wstring& name) {
    std::wstring lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    if (auto* cached = FindModule(lower)) return cached;
    if (m_loading.count(lower)) {
        m_lastError = L"Circular dependency detected loading " + name;
        return nullptr;
    }

    // Try each search dir
    for (const auto& dir : m_searchDirs) {
        std::wstring path = dir;
        if (!path.empty() && path.back() != L'\\' && path.back() != L'/') path.push_back(L'\\');
        path += name;
        if (auto* m = LoadModuleFromPath(path)) return m;
    }
    m_lastError = L"Module not found: " + name;
    return nullptr;
}

// ---------------------------------------------------------------------------
// LoadModuleFromPath
// ---------------------------------------------------------------------------
LoadedModule* PeLoader::LoadModuleFromPath(const std::wstring& path) {
    std::wstring lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    // Extract basename for cache lookup
    size_t slash = lower.find_last_of(L"\\/");
    std::wstring base = (slash == std::wstring::npos) ? lower : lower.substr(slash + 1);
    if (auto* cached = m_byName[base]) return cached;
    if (m_loading.count(base)) {
        m_lastError = L"Circular dependency detected loading " + path;
        return nullptr;
    }

    std::vector<uint8_t> bytes;
    if (!ReadFileFromDisk(path, bytes)) {
        m_lastError = L"Failed to read file: " + path;
        return nullptr;
    }
    return LoadFromBuffer(path, bytes);
}

// ---------------------------------------------------------------------------
// LoadFromBuffer — the core pipeline.
// ---------------------------------------------------------------------------
LoadedModule* PeLoader::LoadFromBuffer(const std::wstring& path,
                                        const std::vector<uint8_t>& fileBytes) {
    // Basename
    std::wstring base = path;
    size_t slash = base.find_last_of(L"\\/");
    if (slash != std::wstring::npos) base = base.substr(slash + 1);
    std::wstring lower = base;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    if (auto* cached = m_byName[lower]) return cached;
    if (m_loading.count(lower)) {
        m_lastError = L"Circular dependency loading " + path;
        return nullptr;
    }
    m_loading.insert(lower);

    uint64_t preferredBase = 0;
    uint32_t sizeOfImage = 0, sizeOfHeaders = 0;
    uint16_t numSections = 0, machine = 0, optMagic = 0;
    bool isDll = false;
    uint32_t entryRva = 0;
    const uint8_t* secHeaders = nullptr;

    if (!ParseHeaders(fileBytes.data(), fileBytes.size(),
                       preferredBase, sizeOfImage, sizeOfHeaders,
                       numSections, machine, optMagic, isDll, entryRva, secHeaders)) {
        m_loading.erase(lower);
        return nullptr;
    }

    if (machine != IMAGE_FILE_MACHINE_AMD64) {
        m_lastError = L"Only AMD64 (x64) PE files are supported. " + base;
        m_loading.erase(lower);
        return nullptr;
    }

    uint64_t actualBase = 0;
    if (!AllocateImage(sizeOfImage, actualBase)) {
        m_lastError = L"VirtualAllocFromApp failed for " + base;
        m_loading.erase(lower);
        return nullptr;
    }

    auto* imageBase = reinterpret_cast<uint8_t*>(actualBase);
    if (!CopyImage(imageBase, sizeOfHeaders,
                    fileBytes.data(), fileBytes.size(),
                    secHeaders, numSections, optMagic)) {
        ::VirtualFree(imageBase, 0, MEM_RELEASE);
        m_loading.erase(lower);
        return nullptr;
    }

    // Locate DataDirectory entries we care about.
    const auto* ntHdr = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        imageBase + reinterpret_cast<const IMAGE_DOS_HEADER*>(imageBase)->e_lfanew);
    const auto& dirs = ntHdr->OptionalHeader.DataDirectory;

    if (dirs[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0 &&
        actualBase != preferredBase) {
        ApplyRelocations(imageBase, preferredBase,
                          dirs[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
                          dirs[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
    }

    // Mark all loaded modules so we don't recurse on this one when resolving imports.
    auto* mod = new LoadedModule();
    mod->name = lower;
    mod->path = path;
    mod->baseAddress = actualBase;
    mod->sizeOfImage = sizeOfImage;
    mod->entryPoint = entryRva ? (actualBase + entryRva) : 0;
    mod->isDll = isDll;
    m_modules.push_back(mod);
    m_byName[lower] = mod;

    if (dirs[IMAGE_DIRECTORY_ENTRY_IMPORT].Size > 0) {
        ProcessImports(imageBase,
                        dirs[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                        dirs[IMAGE_DIRECTORY_ENTRY_IMPORT].Size,
                        optMagic);
    }
    if (dirs[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size > 0) {
        ProcessDelayImports(imageBase,
                             dirs[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress,
                             dirs[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size,
                             optMagic);
    }
    if (dirs[IMAGE_DIRECTORY_ENTRY_TLS].Size > 0) {
        ProcessTls(imageBase,
                    dirs[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress,
                    dirs[IMAGE_DIRECTORY_ENTRY_TLS].Size,
                    optMagic);
        mod->tlsProcessed = true;
    }
    if (dirs[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size > 0) {
        RegisterExceptionTables(imageBase,
                                 dirs[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress,
                                 dirs[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size);
    }

    if (isDll) {
        CallDllMain(mod, DLL_PROCESS_ATTACH);
    }

    mod->isLoaded = true;
    m_loading.erase(lower);
    return mod;
}

// ---------------------------------------------------------------------------
// ParseHeaders
// ---------------------------------------------------------------------------
bool PeLoader::ParseHeaders(const uint8_t* base, size_t size,
                              uint64_t& outImageBase, uint32_t& outSizeOfImage,
                              uint32_t& outSizeOfHeaders,
                              uint16_t& outNumSections, uint16_t& outMachine,
                              uint16_t& outOptMagic, bool& outIsDll,
                              uint32_t& outEntryRva,
                              const uint8_t*& outSectionHeaders) {
    if (size < sizeof(IMAGE_DOS_HEADER)) return false;
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    if (dos->e_lfanew <= 0 || static_cast<size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > size)
        return false;

    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

    outMachine      = nt->FileHeader.Machine;
    outNumSections  = nt->FileHeader.NumberOfSections;
    outSizeOfImage  = nt->OptionalHeader.SizeOfImage;
    outSizeOfHeaders = nt->OptionalHeader.SizeOfHeaders;
    outIsDll        = (nt->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
    outEntryRva     = nt->OptionalHeader.AddressOfEntryPoint;
    outOptMagic     = nt->OptionalHeader.Magic;
    outImageBase    = nt->OptionalHeader.ImageBase;

    outSectionHeaders = base + dos->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)
                       + nt->FileHeader.SizeOfOptionalHeader;
    return true;
}

// ---------------------------------------------------------------------------
// AllocateImage
// ---------------------------------------------------------------------------
bool PeLoader::AllocateImage(uint32_t sizeOfImage, uint64_t& outBase) {
    LPVOID p = ::VirtualAllocFromApp(nullptr, sizeOfImage,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!p) return false;
    outBase = reinterpret_cast<uint64_t>(p);
    return true;
}

// ---------------------------------------------------------------------------
// CopyImage (headers + sections)
// ---------------------------------------------------------------------------
bool PeLoader::CopyImage(uint8_t* dest, uint32_t sizeOfHeaders,
                          const uint8_t* src, size_t srcSize,
                          const void* sectionHeadersVoid, uint16_t numSections,
                          uint16_t optMagic) {
    if (sizeOfHeaders > srcSize) return false;
    std::memcpy(dest, src, sizeOfHeaders);

    auto* sec = reinterpret_cast<const IMAGE_SECTION_HEADER*>(sectionHeadersVoid);
    for (uint16_t i = 0; i < numSections; ++i, ++sec) {
        if (sec->SizeOfRawData == 0) continue;
        uint32_t rva = sec->VirtualAddress;
        uint32_t rawSize = sec->SizeOfRawData;
        uint32_t rawPtr  = sec->PointerToRawData;
        if (rawPtr + rawSize > srcSize) {
            // truncated section
            rawSize = static_cast<uint32_t>(srcSize - rawPtr);
        }
        std::memcpy(dest + rva, src + rawPtr, rawSize);
    }
    return true;
}

// ---------------------------------------------------------------------------
// ApplyRelocations
// ---------------------------------------------------------------------------
bool PeLoader::ApplyRelocations(uint8_t* imageBase, uint64_t preferredBase,
                                  uint32_t relocDirRva, uint32_t relocDirSize) {
    auto* p = reinterpret_cast<uint8_t*>(imageBase + relocDirRva);
    auto* end = p + relocDirSize;
    int64_t delta = static_cast<int64_t>(reinterpret_cast<uint64_t>(imageBase) - preferredBase);
    if (delta == 0) return true;

    while (p + sizeof(IMAGE_BASE_RELOCATION) <= end) {
        auto* blk = reinterpret_cast<IMAGE_BASE_RELOCATION*>(p);
        if (blk->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION)) break;
        uint32_t count = (blk->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        WORD* entries = reinterpret_cast<WORD*>(blk + 1);
        for (uint32_t i = 0; i < count; ++i) {
            int type = (entries[i] >> 12) & 0xF;
            int offset = entries[i] & 0xFFF;
            uint8_t* target = imageBase + blk->VirtualAddress + offset;
            switch (type) {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;  // padding
                case IMAGE_REL_BASED_DIR64: {
                    uint64_t* pVal = reinterpret_cast<uint64_t*>(target);
                    *pVal += delta;
                    break;
                }
                case IMAGE_REL_BASED_HIGHLOW: {
                    uint32_t* pVal = reinterpret_cast<uint32_t*>(target);
                    *pVal += static_cast<uint32_t>(delta);
                    break;
                }
                case IMAGE_REL_BASED_HIGH: {
                    uint16_t* pVal = reinterpret_cast<uint16_t*>(target);
                    *pVal = static_cast<uint16_t>((*pVal + (delta >> 16)) & 0xFFFF);
                    break;
                }
                case IMAGE_REL_BASED_LOW: {
                    uint16_t* pVal = reinterpret_cast<uint16_t*>(target);
                    *pVal = static_cast<uint16_t>((*pVal + delta) & 0xFFFF);
                    break;
                }
                default:
                    // Unsupported relocation type — skip but log.
                    break;
            }
        }
        p += blk->SizeOfBlock;
    }
    return true;
}

// ---------------------------------------------------------------------------
// ProcessImports (standard)
// ---------------------------------------------------------------------------
bool PeLoader::ProcessImports(uint8_t* imageBase,
                                uint32_t importDirRva, uint32_t importDirSize,
                                uint16_t optMagic) {
    (void)importDirSize;  // we walk until we hit a zero descriptor
    auto* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(imageBase + importDirRva);
    while (desc->Name != 0) {
        const char* dllName = reinterpret_cast<const char*>(imageBase + desc->Name);
        std::wstring wdll;
        for (const char* p = dllName; *p; ++p) wdll.push_back(static_cast<wchar_t>(*p));
        // Lowercase the DLL name for matching.
        std::transform(wdll.begin(), wdll.end(), wdll.begin(), ::towlower);

        // IMAGE_IMPORT_DESCRIPTOR has a union for OriginalFirstThunk/Characteristics.
        // On MSVC, DUMMYUNIONNAME expands to a named member (u), so we need
        // desc->DUMMYUNIONNAME.OriginalFirstThunk. On our Linux stub, the
        // union is named DUMMYUNIONNAME literally. Both work with the macro.
        // However, some MSVC configs have issues with the macro expansion.
        // The safest approach: cast to a local struct with the same layout
        // but anonymous union, then access OriginalFirstThunk directly.
        struct XWR_IMPORT_DESC {
            union { DWORD Characteristics; DWORD OriginalFirstThunk; };
            DWORD TimeDateStamp;
            DWORD ForwarderChain;
            DWORD Name;
            DWORD FirstThunk;
        };
        XWR_IMPORT_DESC* xdesc = reinterpret_cast<XWR_IMPORT_DESC*>(desc);
        uint32_t oftRva = xdesc->OriginalFirstThunk ? xdesc->OriginalFirstThunk : xdesc->FirstThunk;
        uint32_t ftRva  = xdesc->FirstThunk;
        if (oftRva == 0 || ftRva == 0) { ++desc; continue; }

        // We support 64-bit only (machine check already done).
        auto* oft = reinterpret_cast<uint64_t*>(imageBase + oftRva);
        auto* ft  = reinterpret_cast<uint64_t*>(imageBase + ftRva);
        for (; *oft; ++oft, ++ft) {
            uint64_t funcAddr = 0;
            if (IMAGE_SNAP_BY_ORDINAL64(*oft)) {
                uint16_t ord = static_cast<uint16_t>(IMAGE_ORDINAL64(*oft));
                funcAddr = ResolveImportByOrdinal(wdll, ord);
            } else {
                uint32_t rva = static_cast<uint32_t>(*oft);
                auto* ibn = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(imageBase + rva);
                funcAddr = ResolveImport(wdll, reinterpret_cast<const char*>(ibn->Name));
            }
            *ft = funcAddr;
        }
        ++desc;
    }
    return true;
}

// ---------------------------------------------------------------------------
// ProcessDelayImports
// ---------------------------------------------------------------------------
bool PeLoader::ProcessDelayImports(uint8_t* imageBase,
                                     uint32_t delayDirRva, uint32_t delayDirSize,
                                     uint16_t optMagic) {
    (void)optMagic;
    auto* p = reinterpret_cast<uint8_t*>(imageBase + delayDirRva);
    auto* end = p + delayDirSize;
    auto* desc = reinterpret_cast<IMAGE_DELAYLOAD_DESCRIPTOR*>(p);
    // On MSVC, Attributes is a union; on Linux stub, it's a plain DWORD.
#ifdef _MSC_VER
    DWORD descAttrs = desc->Attributes.AllAttributes;
#else
    DWORD descAttrs = desc->Attributes;
#endif
    while (p + sizeof(*desc) <= end &&
           (descAttrs != 0 || desc->DllNameRVA != 0)) {
        if (desc->DllNameRVA != 0) {
            const char* dllName = reinterpret_cast<const char*>(imageBase + desc->DllNameRVA);
            std::wstring wdll;
            for (const char* q = dllName; *q; ++q) wdll.push_back(static_cast<wchar_t>(*q));
            std::transform(wdll.begin(), wdll.end(), wdll.begin(), ::towlower);

            uint32_t iatRva = desc->ImportAddressTableRVA;
            uint32_t intRva = desc->ImportNameTableRVA;
            if (iatRva && intRva) {
                auto* iat = reinterpret_cast<uint64_t*>(imageBase + iatRva);
                auto* intT = reinterpret_cast<uint64_t*>(imageBase + intRva);
                for (; *intT; ++iat, ++intT) {
                    uint64_t funcAddr = 0;
                    if (IMAGE_SNAP_BY_ORDINAL64(*intT)) {
                        uint16_t ord = static_cast<uint16_t>(IMAGE_ORDINAL64(*intT));
                        funcAddr = ResolveImportByOrdinal(wdll, ord);
                    } else {
                        uint32_t rva = static_cast<uint32_t>(*intT);
                        auto* ibn = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(imageBase + rva);
                        funcAddr = ResolveImport(wdll, reinterpret_cast<const char*>(ibn->Name));
                    }
                    *iat = funcAddr;
                }
            }
        }
        ++desc;
        p = reinterpret_cast<uint8_t*>(desc);
    }
    return true;
}

// ---------------------------------------------------------------------------
// ProcessTls
// ---------------------------------------------------------------------------
bool PeLoader::ProcessTls(uint8_t* imageBase, uint32_t tlsDirRva,
                           uint32_t tlsDirSize, uint16_t optMagic) {
    (void)optMagic; (void)tlsDirSize;
    auto* tls = reinterpret_cast<IMAGE_TLS_DIRECTORY64*>(imageBase + tlsDirRva);
    if (tls->AddressOfCallBacks == 0) return true;

    // Assign a per-module TLS index.
    uint32_t index = m_tlsIndexCounter++;
    if (tls->AddressOfIndex) {
        *reinterpret_cast<uint32_t*>(tls->AddressOfIndex) = index;
    }

    uint64_t* callbacks = reinterpret_cast<uint64_t*>(tls->AddressOfCallBacks);
    uint64_t imageBaseVal = reinterpret_cast<uint64_t>(imageBase);
    for (; *callbacks; ++callbacks) {
        auto* cb = reinterpret_cast<void(*)(void*, uint32_t, void*)>(*callbacks);
        cb(imageBase, DLL_PROCESS_ATTACH, nullptr);
    }
    return true;
}

// ---------------------------------------------------------------------------
// RegisterExceptionTables
// ---------------------------------------------------------------------------
bool PeLoader::RegisterExceptionTables(uint8_t* imageBase,
                                         uint32_t excDirRva, uint32_t excDirSize) {
    if (excDirSize == 0) return true;
    // IMAGE_RUNTIME_FUNCTION_ENTRY array. Each entry is 12 bytes on x64.
    // On MSVC, use the SDK's RUNTIME_FUNCTION type (defined in winnt.h).
    // On Linux stub, define our own.
#ifdef _MSC_VER
    using XWR_RUNTIME_FUNCTION = RUNTIME_FUNCTION;
#else
    struct XWR_RUNTIME_FUNCTION {
        uint32_t BeginAddress;
        uint32_t EndAddress;
        uint32_t UnwindData;
    };
#endif
    auto* tbl = reinterpret_cast<XWR_RUNTIME_FUNCTION*>(imageBase + excDirRva);
    uint32_t count = excDirSize / sizeof(XWR_RUNTIME_FUNCTION);
    if (count == 0) return true;
    // RtlAddFunctionTable wants the entries sorted by BeginAddress.
    std::vector<XWR_RUNTIME_FUNCTION> sorted(tbl, tbl + count);
    std::sort(sorted.begin(), sorted.end(),
              [](const XWR_RUNTIME_FUNCTION& a, const XWR_RUNTIME_FUNCTION& b) {
                  return a.BeginAddress < b.BeginAddress;
              });
    // We need a stable copy in process memory (the image is already there but
    // RtlAddFunctionTable may outlive the image — keep our own copy).
    auto* myCopy = new XWR_RUNTIME_FUNCTION[sorted.size()];
    std::copy(sorted.begin(), sorted.end(), myCopy);
#ifdef _MSC_VER
    BOOL ok = ::RtlAddFunctionTable(reinterpret_cast<PRUNTIME_FUNCTION>(myCopy),
                                     static_cast<DWORD>(sorted.size()),
                                     reinterpret_cast<DWORD64>(imageBase));
#else
    BOOL ok = ::RtlAddFunctionTable(myCopy,
                                     static_cast<DWORD>(sorted.size()),
                                     reinterpret_cast<DWORD64>(imageBase));
#endif
    if (ok) {
        // Stash for cleanup. Find which module this belongs to by imageBase.
        for (auto* m : m_modules) {
            if (m->baseAddress == reinterpret_cast<uint64_t>(imageBase)) {
                m->exceptionTable = myCopy;
                m->exceptionTableCount = static_cast<uint32_t>(sorted.size());
                break;
            }
        }
    } else {
        delete[] myCopy;
    }
    return ok != FALSE;
}

// ---------------------------------------------------------------------------
// CallDllMain
// ---------------------------------------------------------------------------
bool PeLoader::CallDllMain(LoadedModule* mod, uint32_t reason) {
    if (!mod || !mod->entryPoint) return true;
    using DllMainProc = bool(*)(void*, uint32_t, void*);
    auto* fn = reinterpret_cast<DllMainProc>(mod->entryPoint);
    fn(reinterpret_cast<void*>(mod->baseAddress), reason, nullptr);
    return true;
}

// ---------------------------------------------------------------------------
// RunExe
// ---------------------------------------------------------------------------
int PeLoader::RunExe(LoadedModule* exe) {
    if (!exe || !exe->entryPoint) return -1;
    using ExeEntry = void(*)();
    auto* fn = reinterpret_cast<ExeEntry>(exe->entryPoint);
    fn();
    return 0;
}

// ---------------------------------------------------------------------------
// GetExport / GetExportByOrdinal
// ---------------------------------------------------------------------------
uint64_t PeLoader::GetExport(LoadedModule* mod, const char* funcName) {
    if (!mod) return 0;
    auto* base = reinterpret_cast<uint8_t*>(mod->baseAddress);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    uint32_t edRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!edRva) return 0;
    auto* ed = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + edRva);
    auto* names = reinterpret_cast<uint32_t*>(base + ed->AddressOfNames);
    auto* ords  = reinterpret_cast<uint16_t*>(base + ed->AddressOfNameOrdinals);
    auto* funcs = reinterpret_cast<uint32_t*>(base + ed->AddressOfFunctions);
    for (DWORD i = 0; i < ed->NumberOfNames; ++i) {
        const char* n = reinterpret_cast<const char*>(base + names[i]);
        if (std::strcmp(n, funcName) == 0) {
            uint32_t fnRva = funcs[ords[i]];
            // Forwarder? (RVA inside export directory range)
            if (fnRva >= edRva && fnRva < edRva +
                nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size) {
                const char* fwd = reinterpret_cast<const char*>(base + fnRva);
                return ResolveForwarder(fwd);
            }
            return reinterpret_cast<uint64_t>(base + fnRva);
        }
    }
    return 0;
}

uint64_t PeLoader::GetExportByOrdinal(LoadedModule* mod, uint16_t ordinal) {
    if (!mod) return 0;
    auto* base = reinterpret_cast<uint8_t*>(mod->baseAddress);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    uint32_t edRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!edRva) return 0;
    auto* ed = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + edRva);
    uint32_t idx = ordinal - ed->Base;
    if (idx >= ed->NumberOfFunctions) return 0;
    auto* funcs = reinterpret_cast<uint32_t*>(base + ed->AddressOfFunctions);
    uint32_t fnRva = funcs[idx];
    if (fnRva >= edRva && fnRva < edRva +
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size) {
        const char* fwd = reinterpret_cast<const char*>(base + fnRva);
        return ResolveForwarder(fwd);
    }
    return reinterpret_cast<uint64_t>(base + fnRva);
}

// ---------------------------------------------------------------------------
// ResolveForwarder
// ---------------------------------------------------------------------------
uint64_t PeLoader::ResolveForwarder(const char* forwarder) {
    // Format: "MODULE.Function" (or "MODULE.#ordinal")
    char buf[256];
    std::strncpy(buf, forwarder, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    char* dot = std::strchr(buf, '.');
    if (!dot) return 0;
    *dot = 0;
    char* fn = dot + 1;
    std::wstring wmod;
    for (const char* p = buf; *p; ++p) wmod.push_back(static_cast<wchar_t>(*p));
    std::transform(wmod.begin(), wmod.end(), wmod.begin(), ::towlower);
    if (fn[0] == '#') {
        int ord = std::atoi(fn + 1);
        return ResolveImportByOrdinal(wmod, static_cast<uint16_t>(ord));
    }
    return ResolveImport(wmod, fn);
}

// ---------------------------------------------------------------------------
// ResolveImport
// ---------------------------------------------------------------------------
uint64_t PeLoader::ResolveImport(const std::wstring& dllName, const char* funcName) {
    // 1) Try ShimRegistry (covers kernel32, user32, gdi32, etc., AND every
    //    api-ms-win-* DLL after ApplyApiSetForwarders has run, AND every
    //    VC runtime / CRT DLL after ApplyVcRuntimePassthrough has run).
    if (m_shims) {
        if (auto p = m_shims->ResolveExport(dllName, funcName)) return p;
    }
    // 2) Try loaded module exports (game-shipped DLLs)
    LoadedModule* mod = FindModule(dllName);
    if (!mod) mod = EnsureDependencyLoaded(dllName);
    if (mod) {
        if (auto p = GetExport(mod, funcName)) return p;
    }
    // 3) Passthrough fallback: for VC runtime / Universal CRT DLLs
    //    (MSVCP140, VCRUNTIME140, ucrtbase, api-ms-win-crt-*, ...) that UWP
    //    supports natively, LoadLibraryW the real system DLL and
    //    GetProcAddress the function. This handles the case where
    //    ApplyVcRuntimePassthrough hasn't run yet (e.g. the function wasn't
    //    in /tmp/gaps so wasn't pre-registered) — without this, the IAT slot
    //    would stay null and the game would crash on first call.
    if (m_shims) {
        std::wstring dllLower = dllName;
        std::transform(dllLower.begin(), dllLower.end(), dllLower.begin(),
                        [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
        // Strip trailing ".dll" / ".exe" if present (the registry stores
        // basenames without extension, and the passthrough helpers expect
        // the same form).
        if (dllLower.size() > 4) {
            const wchar_t* ext = dllLower.c_str() + dllLower.size() - 4;
            if (ext[0] == L'.' &&
                (ext[1] == L'd' || ext[1] == L'D') &&
                (ext[2] == L'l' || ext[2] == L'L') &&
                (ext[3] == L'l' || ext[3] == L'L')) {
                dllLower = dllLower.substr(0, dllLower.size() - 4);
            } else if (ext[0] == L'.' &&
                       (ext[1] == L'e' || ext[1] == L'E') &&
                       (ext[2] == L'x' || ext[2] == L'X') &&
                       (ext[3] == L'e' || ext[3] == L'E')) {
                dllLower = dllLower.substr(0, dllLower.size() - 4);
            }
        }
        if (IsVcRuntimePassthroughDll(dllLower)) {
            std::wstring realDll = GetRealDllForPassthrough(dllLower);
            if (!realDll.empty()) {
                HMODULE h = ::LoadLibraryW(realDll.c_str());
                if (h) {
                    // Note: GetProcAddress takes LPCSTR. funcName is already
                    // LPCSTR (a char*). We do NOT lowercase it — the real
                    // system DLL exports are case-sensitive.
                    FARPROC p = ::GetProcAddress(h, funcName);
                    if (p) {
                        // Cache the resolution so we don't LoadLibraryW
                        // again next time the same (dll, func) is imported.
                        m_shims->Register(dllLower, funcName, p);
                        return reinterpret_cast<uint64_t>(p);
                    }
                }
            }
        }
    }
    // 4) Final fallback: zero (the IAT slot stays null; game will crash if it calls it)
    return 0;
}

uint64_t PeLoader::ResolveImportByOrdinal(const std::wstring& dllName, uint16_t ord) {
    LoadedModule* mod = FindModule(dllName);
    if (!mod) mod = EnsureDependencyLoaded(dllName);
    if (mod) return GetExportByOrdinal(mod, ord);
    return 0;
}

// ---------------------------------------------------------------------------
// EnsureDependencyLoaded — load a game-shipped DLL if not already present.
// ---------------------------------------------------------------------------
LoadedModule* PeLoader::EnsureDependencyLoaded(const std::wstring& dllName) {
    if (auto* m = FindModule(dllName)) return m;
    // Don't try to load Windows system DLLs that should be shimmed.
    if (m_shims && m_shims->HasModule(dllName)) return nullptr;
    return LoadModule(dllName);
}

// ---------------------------------------------------------------------------
// FindSection / RvaToFileOffset
// ---------------------------------------------------------------------------
const void* PeLoader::FindSection(const void* sectionHeaders, uint16_t numSections,
                                    const char* name) {
    auto* sec = reinterpret_cast<const IMAGE_SECTION_HEADER*>(sectionHeaders);
    for (uint16_t i = 0; i < numSections; ++i, ++sec) {
        if (std::strncmp(reinterpret_cast<const char*>(sec->Name), name, 8) == 0)
            return sec;
    }
    return nullptr;
}

uint32_t PeLoader::RvaToFileOffset(const void* sectionHeaders, uint16_t numSections,
                                     uint32_t rva) {
    auto* sec = reinterpret_cast<const IMAGE_SECTION_HEADER*>(sectionHeaders);
    for (uint16_t i = 0; i < numSections; ++i, ++sec) {
        if (rva >= sec->VirtualAddress &&
            rva < sec->VirtualAddress + sec->Misc.VirtualSize) {
            return sec->PointerToRawData + (rva - sec->VirtualAddress);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// NotifyThreadAttach / NotifyThreadDetach
// ---------------------------------------------------------------------------
void PeLoader::NotifyThreadAttach() {
    for (auto* m : m_modules) {
        if (m->isDll && m->tlsProcessed) {
            // Fire DLL_THREAD_ATTACH; games rarely care.
            CallDllMain(m, DLL_THREAD_ATTACH);
        }
    }
}
void PeLoader::NotifyThreadDetach() {
    for (auto* m : m_modules) {
        if (m->isDll && m->tlsProcessed) {
            CallDllMain(m, DLL_THREAD_DETACH);
        }
    }
}

}  // namespace xwr
