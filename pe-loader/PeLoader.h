// pe-loader/PeLoader.h
// PE (Portable Executable) loader for the Xbox Win32 Runner.
//
// Loads native x86-64 Windows .exe/.dll files into the UWP process's address
// space using VirtualAllocFromApp + VirtualProtectFromApp (the only memory
// primitives UWP AppContainer allows for code generation).
//
// Pipeline:
//   1) MapFile           - read the PE file into memory
//   2) ParseHeaders      - DOS / NT / Section headers
//   3) AllocateImage     - VirtualAllocFromApp with SizeOfImage
//   4) CopyHeaders       - DOS + NT + Section headers
//   5) CopySections      - copy raw section bytes
//   6) ProcessRelocs     - IMAGE_REL_BASED_DIR64
//   7) ResolveImports    - standard + delay-load
//   8) ProcessTls        - run TLS callbacks
//   9) RegisterEhTables  - RtlAddFunctionTable
//  10) CallEntry         - DllMain(DLL_PROCESS_ATTACH) or exe entrypoint

#pragma once
#include <Windows.h>

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace xwr {

// Forward-declared elsewhere
class ShimRegistry;

// Information about a loaded module (exe or dll).
struct LoadedModule {
    std::wstring    name;          // lowercased basename (e.g. L"tbb.dll")
    std::wstring    path;          // absolute path the module was loaded from
    uint64_t        baseAddress = 0; // image base in process memory
    uint64_t        sizeOfImage = 0; // SizeOfImage in bytes
    uint64_t        entryPoint  = 0; // AddressOfEntryPoint (RVA → VA)
    bool            isDll       = false;
    bool            isLoaded    = false;
    bool            tlsProcessed = false;
    void*           exceptionTable = nullptr;
    uint32_t        exceptionTableCount = 0;
};

// Main loader class.
class PeLoader {
public:
    PeLoader();
    ~PeLoader();

    // Set the shim registry used to resolve imports.
    void SetShimRegistry(ShimRegistry* shims) { m_shims = shims; }

    // Add a directory to the DLL search path (e.g. game's "Engine/Binaries/Win64").
    void AddDllSearchDir(const std::wstring& dir);

    // Load and (if DLL) run DllMain. Returns the module info, or nullptr on failure.
    // `name` is the requested DLL name (basename only); resolved via search dirs.
    LoadedModule* LoadModule(const std::wstring& name);

    // Load a module from an absolute path. Bypasses search dirs.
    LoadedModule* LoadModuleFromPath(const std::wstring& path);

    // Run a previously-loaded EXE module. Calls the entrypoint with the
    // CRT-style (lpThreadParameter = NULL) signature and returns its exit code.
    int RunExe(LoadedModule* exe);

    // Resolve an exported function from a loaded module.
    // Returns 0 if not found.
    uint64_t GetExport(LoadedModule* mod, const char* funcName);
    uint64_t GetExportByOrdinal(LoadedModule* mod, uint16_t ordinal);

    // Find a loaded module by lowercased basename. Returns nullptr if absent.
    LoadedModule* FindModule(const std::wstring& name);

    // Hook: notify all loaded DLLs that a thread was created/destroyed.
    void NotifyThreadAttach();
    void NotifyThreadDetach();

    // Last error message (human-readable).
    const std::wstring& LastError() const { return m_lastError; }

private:
    // Internal helper - actually loads from an open PE buffer.
    LoadedModule* LoadFromBuffer(const std::wstring& path,
                                  const std::vector<uint8_t>& fileBytes);

    // Step 1: parse DOS / NT headers; verify magic, machine, sections.
    bool ParseHeaders(const uint8_t* base, size_t size,
                       uint64_t& outImageBase, uint32_t& outSizeOfImage,
                       uint32_t& outSizeOfHeaders,
                       uint16_t& outNumSections,
                       uint16_t& outMachine,
                       uint16_t& outOptMagic,
                       bool& outIsDll,
                       uint32_t& outEntryRva,
                       const uint8_t*& outSectionHeaders);

    // Step 2: allocate the image in process memory.
    bool AllocateImage(uint32_t sizeOfImage, uint64_t& outBase);

    // Step 3: copy headers and section bytes.
    bool CopyImage(uint8_t* dest, uint32_t sizeOfHeaders,
                    const uint8_t* src, size_t srcSize,
                    const void* sectionHeaders, uint16_t numSections,
                    uint16_t optMagic);

    // Step 4: apply base relocations (IMAGE_REL_BASED_DIR64 and friends).
    bool ApplyRelocations(uint8_t* imageBase, uint64_t preferredBase,
                           uint32_t relocDirRva, uint32_t relocDirSize);

    // Step 5: process the standard import table.
    bool ProcessImports(uint8_t* imageBase,
                         uint32_t importDirRva, uint32_t importDirSize,
                         uint16_t optMagic);

    // Step 6: process delay-load imports.
    bool ProcessDelayImports(uint8_t* imageBase,
                              uint32_t delayDirRva, uint32_t delayDirSize,
                              uint16_t optMagic);

    // Step 7: invoke TLS callbacks.
    bool ProcessTls(uint8_t* imageBase, uint32_t tlsDirRva, uint32_t tlsDirSize,
                     uint16_t optMagic);

    // Step 8: register exception tables via RtlAddFunctionTable.
    bool RegisterExceptionTables(uint8_t* imageBase,
                                  uint32_t excDirRva, uint32_t excDirSize);

    // Step 9: call DllMain(DLL_PROCESS_ATTACH) for a loaded DLL.
    bool CallDllMain(LoadedModule* mod, uint32_t reason);

    // Look up a section by name. Returns nullptr if not found.
    const void* FindSection(const void* sectionHeaders, uint16_t numSections,
                             const char* name);

    // Convert RVA → file offset by walking section headers.
    uint32_t RvaToFileOffset(const void* sectionHeaders, uint16_t numSections,
                              uint32_t rva);

    // Resolve a DLL import by name, recursing into ShimRegistry + loaded modules.
    uint64_t ResolveImport(const std::wstring& dllName, const char* funcName);
    uint64_t ResolveImportByOrdinal(const std::wstring& dllName, uint16_t ordinal);

    // Resolve a forwarder export like "NTDLL.RtlAllocateHeap".
    uint64_t ResolveForwarder(const char* forwarder);

    // Recursively load a game-shipped DLL (search dirs + system shim).
    LoadedModule* EnsureDependencyLoaded(const std::wstring& dllName);

    ShimRegistry*                       m_shims = nullptr;
    std::vector<std::wstring>           m_searchDirs;
    std::vector<LoadedModule*>          m_modules;      // ownership
    std::unordered_map<std::wstring, LoadedModule*> m_byName;  // lowercased basename → module

    // Stack of modules currently being loaded, to detect circular dependencies.
    std::unordered_set<std::wstring>   m_loading;

    std::wstring                        m_lastError;

    // Scratch
    uint32_t                            m_tlsIndexCounter = 0;
};

// Callback used by PeLoader to read a file's bytes from disk.
// (We expose this so tests / shims can intercept file IO.)
using ReadFileCallback = bool(*)(const std::wstring& path,
                                  std::vector<uint8_t>& outBytes);
void SetReadFileCallback(ReadFileCallback cb);

}  // namespace xwr
