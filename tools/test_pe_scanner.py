#!/usr/bin/env python3
# =============================================================================
# test_pe_scanner.py
# =============================================================================
# Unit tests for pe_import_scanner.py. Builds a synthetic 64-bit PE in memory
# with a known import table and asserts the scanner correctly parses it.
#
# Pure stdlib (struct / io / os / sys / tempfile / unittest). Run with:
#   python3 tools/test_pe_scanner.py
# =============================================================================

import io
import os
import struct
import sys
import tempfile
import unittest
import shutil

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe_import_scanner import (
    PeFile,
    ImportDescriptor,
    build_shim_registry,
    scan_directory,
    build_coverage_report,
    render_markdown,
    render_json,
    IMAGE_FILE_MACHINE_AMD64,
)

# ---------------------------------------------------------------------------
# PE constants (mirrors pe_import_scanner.py)
# ---------------------------------------------------------------------------
IMAGE_DOS_SIGNATURE       = 0x5A4D
IMAGE_NT_SIGNATURE        = 0x00004550
IMAGE_FILE_MACHINE_AMD64  = 0x8664
IMAGE_OPTIONAL_HDR64_MAGIC = 0x20B
IMAGE_DIRECTORY_ENTRY_IMPORT       = 1
IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT = 13
SIZEOF_FILE_HEADER       = 20
SIZEOF_SECTION_HEADER    = 40
SIZEOF_IMPORT_DESCRIPTOR = 20
SIZEOF_DELAY_DESCRIPTOR  = 32
SIZEOF_OPT_HDR64         = 240   # standard PE32+ optional header


def align_up(value: int, align: int) -> int:
    """Round `value` up to the next multiple of `align`."""
    return (value + align - 1) & ~(align - 1)


def cstr(text: str) -> bytes:
    """ASCII null-terminated string."""
    return text.encode("ascii") + b"\x00"


def build_minimal_x64_pe(imports=None) -> bytes:
    """
    Construct a minimal but valid 64-bit PE file with the given imports.

    `imports` is a list of (dll_name, [func_name, ...]) tuples. Each function
    is imported by name (IMAGE_IMPORT_BY_NAME). The file layout is:

        +-----------------+
        | DOS header (64) |
        +-----------------+
        | NT sig (4)      |
        +-----------------+
        | FILE_HEADER(20) |
        +-----------------+
        | OPT_HDR64 (240) |
        +-----------------+
        | .idata sec (40) |
        +-----------------+
        | .idata body:    |
        |   - import desc |
        |   - thunk arrays|
        |   - hint/name   |
        |   - dll name str|
        +-----------------+

    All RVAs are computed from a single section starting at RVA 0x1000.
    """
    imports = imports or []

    # --- Layout: figure out all sizes / offsets up front ---
    file_alignment = 0x200
    section_alignment = 0x1000
    section_rva = section_alignment   # .idata lives at RVA 0x1000
    headers_size = align_up(
        64 + 4 + SIZEOF_FILE_HEADER + SIZEOF_OPT_HDR64 + SIZEOF_SECTION_HEADER,
        file_alignment,
    )
    section_raw_off = headers_size
    body = bytearray()

    # We'll lay out the body in this order:
    #   1. Import descriptor array (N+1 entries; +1 = null terminator)
    #   2. Per-DLL INT arrays (each terminated by 0)
    #   3. Per-DLL IAT arrays (each terminated by 0)
    #   4. IMAGE_IMPORT_BY_NAME entries
    #   5. DLL name strings

    # Pre-compute slot sizes.
    ptr_size = 8
    num_descs = len(imports)
    desc_array_size = (num_descs + 1) * SIZEOF_IMPORT_DESCRIPTOR

    int_arrays_size = sum((len(fns) + 1) * ptr_size for _, fns in imports)
    iat_arrays_size = int_arrays_size  # INT == IAT contents for bound by-name imports

    # hint/name entries: 2 bytes hint + len(name) + 1 null + 1 pad to align(2)
    def ibn_size(name: str) -> int:
        return 2 + len(name) + 1 + (1 if (len(name) % 2 == 0) else 0)

    ibn_total_size = sum(ibn_size(fn) for _, fns in imports for fn in fns)
    dll_names_size = sum(len(dn) + 1 for dn, _ in imports)

    # Offsets within the section body.
    desc_off       = 0
    int_arrays_off = desc_off + desc_array_size
    iat_arrays_off = int_arrays_off + int_arrays_size
    ibn_off        = iat_arrays_off + iat_arrays_size
    dll_names_off  = ibn_off + ibn_total_size
    section_body_size = align_up(dll_names_off + dll_names_size, file_alignment)

    # Helper: convert section-body offset to RVA.
    def body_off_to_rva(off: int) -> int:
        return section_rva + off

    # --- Build the body now ---
    # We need to write the descriptors with INT / IAT / Name RVAs that point
    # to the per-DLL slots, and the thunk arrays must contain RVAs to the
    # IMAGE_IMPORT_BY_NAME entries. So we precompute the offsets of every
    # per-DLL slot.

    per_dll_int_off = []
    per_dll_iat_off = []
    cursor = int_arrays_off
    for _, fns in imports:
        per_dll_int_off.append(cursor)
        cursor += (len(fns) + 1) * ptr_size
    cursor = iat_arrays_off
    for _, fns in imports:
        per_dll_iat_off.append(cursor)
        cursor += (len(fns) + 1) * ptr_size

    ibn_cursor = ibn_off
    per_fn_ibn_off = {}  # (dll, fn) -> body offset
    for dll, fns in imports:
        for fn in fns:
            per_fn_ibn_off[(dll, fn)] = ibn_cursor
            ibn_cursor += ibn_size(fn)

    dll_name_cursor = dll_names_off
    per_dll_name_off = []
    for dll, _ in imports:
        per_dll_name_off.append(dll_name_cursor)
        dll_name_cursor += len(dll) + 1

    # 1. Import descriptor array.
    descs = bytearray()
    for i, (dll, fns) in enumerate(imports):
        oft_rva = body_off_to_rva(per_dll_int_off[i])
        name_rva = body_off_to_rva(per_dll_name_off[i])
        ft_rva  = body_off_to_rva(per_dll_iat_off[i])
        descs += struct.pack("<IIIII",
                             oft_rva,    # OriginalFirstThunk
                             0,          # TimeDateStamp
                             0,          # ForwarderChain
                             name_rva,   # Name
                             ft_rva)     # FirstThunk
    descs += b"\x00" * SIZEOF_IMPORT_DESCRIPTOR  # null terminator

    # 2. INT arrays.
    int_arrays = bytearray()
    for i, (dll, fns) in enumerate(imports):
        for fn in fns:
            ibn_rva = body_off_to_rva(per_fn_ibn_off[(dll, fn)])
            int_arrays += struct.pack("<Q", ibn_rva)
        int_arrays += b"\x00" * ptr_size  # terminator

    # 3. IAT arrays (same contents as INT for by-name imports).
    iat_arrays = bytearray()
    for i, (dll, fns) in enumerate(imports):
        for fn in fns:
            ibn_rva = body_off_to_rva(per_fn_ibn_off[(dll, fn)])
            iat_arrays += struct.pack("<Q", ibn_rva)
        iat_arrays += b"\x00" * ptr_size

    # 4. IMAGE_IMPORT_BY_NAME entries.
    ibn_blob = bytearray()
    for dll, fns in imports:
        for fn in fns:
            ibn_blob += struct.pack("<H", 0)   # hint = 0
            ibn_blob += fn.encode("ascii") + b"\x00"
            if (len(fn) % 2) == 0:
                ibn_blob += b"\x00"  # pad to align(2)

    # 5. DLL name strings.
    dll_name_blob = b"".join(cstr(d) for d, _ in imports)

    body = bytearray(section_body_size)
    body[desc_off:desc_off + len(descs)]                   = descs
    body[int_arrays_off:int_arrays_off + len(int_arrays)]   = int_arrays
    body[iat_arrays_off:iat_arrays_off + len(iat_arrays)]   = iat_arrays
    body[ibn_off:ibn_off + len(ibn_blob)]                   = ibn_blob
    body[dll_names_off:dll_names_off + len(dll_name_blob)]  = dll_name_blob

    # --- DOS header (64 bytes) ---
    dos = bytearray(64)
    struct.pack_into("<H", dos, 0, IMAGE_DOS_SIGNATURE)  # e_magic
    struct.pack_into("<I", dos, 0x3C, 64)                # e_lfanew

    # --- NT signature ---
    nt_sig = struct.pack("<I", IMAGE_NT_SIGNATURE)

    # --- IMAGE_FILE_HEADER (20 bytes) ---
    file_hdr = struct.pack(
        "<HHIIIHH",
        IMAGE_FILE_MACHINE_AMD64,    # Machine
        1,                            # NumberOfSections
        0,                            # TimeDateStamp
        0,                            # PointerToSymbolTable
        0,                            # NumberOfSymbols
        SIZEOF_OPT_HDR64,             # SizeOfOptionalHeader
        0x0022,                       # Characteristics (EXECUTABLE_IMAGE | LARGE_ADDRESS_AWARE)
    )

    # --- IMAGE_OPTIONAL_HEADER64 (240 bytes) ---
    # We construct it as a zero-filled 240-byte buffer and patch the fields
    # the parser actually reads.
    opt = bytearray(SIZEOF_OPT_HDR64)
    struct.pack_into("<H", opt, 0,  IMAGE_OPTIONAL_HDR64_MAGIC)  # Magic
    struct.pack_into("<B", opt, 2,  14)   # MajorLinkerVersion
    struct.pack_into("<B", opt, 3,  0)    # MinorLinkerVersion
    struct.pack_into("<I", opt, 4,  0x1000)  # SizeOfCode
    struct.pack_into("<I", opt, 8,  section_body_size)  # SizeOfInitializedData
    struct.pack_into("<I", opt, 16, 0x100000)  # AddressOfEntryPoint
    struct.pack_into("<I", opt, 20, 0x1000)   # BaseOfCode
    struct.pack_into("<Q", opt, 24, 0x0000000140000000)  # ImageBase
    struct.pack_into("<I", opt, 32, section_alignment)  # SectionAlignment
    struct.pack_into("<I", opt, 36, file_alignment)     # FileAlignment
    struct.pack_into("<H", opt, 40, 6)    # MajorOperatingSystemVersion
    struct.pack_into("<H", opt, 42, 0)    # MinorOperatingSystemVersion
    struct.pack_into("<H", opt, 44, 0)    # MajorImageVersion
    struct.pack_into("<H", opt, 46, 0)    # MinorImageVersion
    struct.pack_into("<H", opt, 48, 6)    # MajorSubsystemVersion
    struct.pack_into("<H", opt, 50, 0)    # MinorSubsystemVersion
    struct.pack_into("<I", opt, 52, 0)    # Win32VersionValue
    struct.pack_into("<I", opt, 56,
                     align_up(headers_size + section_body_size, section_alignment))  # SizeOfImage
    struct.pack_into("<I", opt, 60, headers_size)  # SizeOfHeaders
    struct.pack_into("<I", opt, 64, 0)    # CheckSum
    struct.pack_into("<H", opt, 68, 3)    # Subsystem (WINDOWS_CUI)
    struct.pack_into("<H", opt, 70, 0x8160)  # DllCharacteristics
    struct.pack_into("<Q", opt, 72, 0x100000)  # SizeOfStackReserve
    struct.pack_into("<Q", opt, 80, 0x1000)    # SizeOfStackCommit
    struct.pack_into("<Q", opt, 88, 0x100000)  # SizeOfHeapReserve
    struct.pack_into("<Q", opt, 96, 0x1000)    # SizeOfHeapCommit
    struct.pack_into("<I", opt, 104, 0)        # LoaderFlags
    struct.pack_into("<I", opt, 108, 16)       # NumberOfRvaAndSizes

    # DataDirectory[16] starts at offset 112.
    # Index 1 = Import table.
    import_dir_rva = body_off_to_rva(desc_off)
    import_dir_size = desc_array_size
    struct.pack_into("<II", opt, 112 + 1 * 8, import_dir_rva, import_dir_size)
    # Delay import dir is left at zero (no delay imports in this minimal PE).

    # --- Section header (.idata) ---
    sec_hdr = bytearray(SIZEOF_SECTION_HEADER)
    sec_hdr[0:8] = b".idata\x00\x00"
    struct.pack_into("<I", sec_hdr, 8,  section_body_size)   # VirtualSize
    struct.pack_into("<I", sec_hdr, 12, section_rva)         # VirtualAddress
    struct.pack_into("<I", sec_hdr, 16, section_body_size)   # SizeOfRawData
    struct.pack_into("<I", sec_hdr, 20, section_raw_off)     # PointerToRawData
    struct.pack_into("<I", sec_hdr, 24, 0)                   # PointerToRelocations
    struct.pack_into("<I", sec_hdr, 28, 0)                   # PointerToLinenumbers
    struct.pack_into("<H", sec_hdr, 32, 0)                   # NumberOfRelocations
    struct.pack_into("<H", sec_hdr, 34, 0)                   # NumberOfLinenumbers
    struct.pack_into("<I", sec_hdr, 36, 0x40000040)          # Characteristics (INITIALIZED_DATA | READ)

    # --- Assemble ---
    out = bytearray()
    out += dos
    out += nt_sig
    out += file_hdr
    out += opt
    out += sec_hdr
    # Pad headers to file alignment.
    pad = headers_size - len(out)
    if pad > 0:
        out += b"\x00" * pad
    out += body

    return bytes(out)


def build_minimal_x64_pe_with_delay_import(dll_name: str, fn_names) -> bytes:
    """
    Build a minimal x64 PE with a delay-load import directory only (no standard
    imports). Used to test delay-import parsing.

    The body layout for the delay-import variant is:

        +------------------------------+
        | .idata body:                 |
        |   1. IMAGE_DELAYLOAD_DESC[]  |  32 bytes each, null-terminated
        |   2. INT (thunk array)       |  N entries + 0
        |   3. IAT (thunk array)       |  N entries + 0
        |   4. IMAGE_IMPORT_BY_NAME[]  |  hint(2) + name + pad
        |   5. DLL name string         |
        +------------------------------+
    """
    imports = [(dll_name, list(fn_names))]
    file_alignment = 0x200
    section_alignment = 0x1000
    section_rva = section_alignment
    headers_size = align_up(
        64 + 4 + SIZEOF_FILE_HEADER + SIZEOF_OPT_HDR64 + SIZEOF_SECTION_HEADER,
        file_alignment,
    )
    section_raw_off = headers_size

    ptr_size = 8
    num_descs = len(imports)
    desc_array_size = (num_descs + 1) * SIZEOF_DELAY_DESCRIPTOR  # 32-byte entries

    int_arrays_size = sum((len(fns) + 1) * ptr_size for _, fns in imports)
    iat_arrays_size = int_arrays_size

    def ibn_size(name: str) -> int:
        return 2 + len(name) + 1 + (1 if (len(name) % 2 == 0) else 0)

    ibn_total_size = sum(ibn_size(fn) for _, fns in imports for fn in fns)
    dll_names_size = sum(len(dn) + 1 for dn, _ in imports)

    desc_off       = 0
    int_arrays_off = desc_off + desc_array_size
    iat_arrays_off = int_arrays_off + int_arrays_size
    ibn_off        = iat_arrays_off + iat_arrays_size
    dll_names_off  = ibn_off + ibn_total_size
    section_body_size = align_up(dll_names_off + dll_names_size, file_alignment)

    def body_off_to_rva(off: int) -> int:
        return section_rva + off

    # Pre-compute per-DLL INT / IAT / DLL-name offsets.
    per_dll_int_off = []
    cursor = int_arrays_off
    for _, fns in imports:
        per_dll_int_off.append(cursor)
        cursor += (len(fns) + 1) * ptr_size
    per_dll_iat_off = []
    cursor = iat_arrays_off
    for _, fns in imports:
        per_dll_iat_off.append(cursor)
        cursor += (len(fns) + 1) * ptr_size
    ibn_cursor = ibn_off
    per_fn_ibn_off = {}
    for dll, fns in imports:
        for fn in fns:
            per_fn_ibn_off[(dll, fn)] = ibn_cursor
            ibn_cursor += ibn_size(fn)
    dll_name_cursor = dll_names_off
    per_dll_name_off = []
    for dll, _ in imports:
        per_dll_name_off.append(dll_name_cursor)
        dll_name_cursor += len(dll) + 1

    # 1. IMAGE_DELAYLOAD_DESCRIPTOR array (32 bytes each, null-terminated).
    descs = bytearray()
    for i, (dll, fns) in enumerate(imports):
        name_rva = body_off_to_rva(per_dll_name_off[i])
        oft_rva  = body_off_to_rva(per_dll_int_off[i])
        ft_rva   = body_off_to_rva(per_dll_iat_off[i])
        descs += struct.pack(
            "<IIIIIIII",
            0x00000001,   # Attributes (RVA-based)
            name_rva,     # DllNameRVA
            0,            # ModuleHandleRVA
            ft_rva,       # ImportAddressTableRVA
            oft_rva,      # ImportNameTableRVA
            0,            # BoundImportAddressTableRVA
            0,            # UnloadInformationTableRVA
            0,            # TimeDateStamp
        )
    descs += b"\x00" * SIZEOF_DELAY_DESCRIPTOR  # null terminator

    # 2. INT arrays.
    int_arrays = bytearray()
    for i, (dll, fns) in enumerate(imports):
        for fn in fns:
            ibn_rva = body_off_to_rva(per_fn_ibn_off[(dll, fn)])
            int_arrays += struct.pack("<Q", ibn_rva)
        int_arrays += b"\x00" * ptr_size

    # 3. IAT arrays (same contents as INT for by-name imports).
    iat_arrays = bytearray()
    for i, (dll, fns) in enumerate(imports):
        for fn in fns:
            ibn_rva = body_off_to_rva(per_fn_ibn_off[(dll, fn)])
            iat_arrays += struct.pack("<Q", ibn_rva)
        iat_arrays += b"\x00" * ptr_size

    # 4. IMAGE_IMPORT_BY_NAME entries.
    ibn_blob = bytearray()
    for dll, fns in imports:
        for fn in fns:
            ibn_blob += struct.pack("<H", 0)   # hint = 0
            ibn_blob += fn.encode("ascii") + b"\x00"
            if (len(fn) % 2) == 0:
                ibn_blob += b"\x00"

    # 5. DLL name strings.
    dll_name_blob = b"".join(cstr(d) for d, _ in imports)

    body = bytearray(section_body_size)
    body[desc_off:desc_off + len(descs)]                   = descs
    body[int_arrays_off:int_arrays_off + len(int_arrays)]  = int_arrays
    body[iat_arrays_off:iat_arrays_off + len(iat_arrays)]  = iat_arrays
    body[ibn_off:ibn_off + len(ibn_blob)]                   = ibn_blob
    body[dll_names_off:dll_names_off + len(dll_name_blob)] = dll_name_blob

    # --- DOS / NT / File / Optional headers (same as standard PE) ---
    dos = bytearray(64)
    struct.pack_into("<H", dos, 0, IMAGE_DOS_SIGNATURE)
    struct.pack_into("<I", dos, 0x3C, 64)

    nt_sig = struct.pack("<I", IMAGE_NT_SIGNATURE)

    file_hdr = struct.pack(
        "<HHIIIHH",
        IMAGE_FILE_MACHINE_AMD64, 1, 0, 0, 0,
        SIZEOF_OPT_HDR64, 0x0022,
    )

    opt = bytearray(SIZEOF_OPT_HDR64)
    struct.pack_into("<H", opt, 0,  IMAGE_OPTIONAL_HDR64_MAGIC)
    struct.pack_into("<B", opt, 2,  14)
    struct.pack_into("<I", opt, 4,  0x1000)
    struct.pack_into("<I", opt, 8,  section_body_size)
    struct.pack_into("<I", opt, 16, 0x100000)
    struct.pack_into("<I", opt, 20, 0x1000)
    struct.pack_into("<Q", opt, 24, 0x0000000140000000)
    struct.pack_into("<I", opt, 32, section_alignment)
    struct.pack_into("<I", opt, 36, file_alignment)
    struct.pack_into("<H", opt, 40, 6)
    struct.pack_into("<H", opt, 48, 6)
    struct.pack_into("<I", opt, 56,
                     align_up(headers_size + section_body_size, section_alignment))
    struct.pack_into("<I", opt, 60, headers_size)
    struct.pack_into("<H", opt, 68, 3)
    struct.pack_into("<H", opt, 70, 0x8160)
    struct.pack_into("<Q", opt, 72, 0x100000)
    struct.pack_into("<Q", opt, 80, 0x1000)
    struct.pack_into("<Q", opt, 88, 0x100000)
    struct.pack_into("<Q", opt, 96, 0x1000)
    struct.pack_into("<I", opt, 104, 0)
    struct.pack_into("<I", opt, 108, 16)

    # DataDirectory[13] (DELAY_IMPORT) points to the descriptor array.
    delay_dir_rva = body_off_to_rva(desc_off)
    delay_dir_size = desc_array_size
    struct.pack_into("<II", opt, 112 + 13 * 8, delay_dir_rva, delay_dir_size)

    # Section header for .idata.
    sec_hdr = bytearray(SIZEOF_SECTION_HEADER)
    sec_hdr[0:8] = b".idata\x00\x00"
    struct.pack_into("<I", sec_hdr, 8,  section_body_size)
    struct.pack_into("<I", sec_hdr, 12, section_rva)
    struct.pack_into("<I", sec_hdr, 16, section_body_size)
    struct.pack_into("<I", sec_hdr, 20, section_raw_off)
    struct.pack_into("<I", sec_hdr, 36, 0x40000040)

    out = bytearray()
    out += dos + nt_sig + file_hdr + opt + sec_hdr
    pad = headers_size - len(out)
    if pad > 0:
        out += b"\x00" * pad
    out += body
    return bytes(out)


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
class TestPeScanner(unittest.TestCase):

    # ----- PE parsing ------------------------------------------------------
    def test_parse_minimal_x64_pe(self):
        pe_bytes = build_minimal_x64_pe(
            imports=[("kernel32.dll", ["CreateFileW", "CloseHandle"])]
        )
        with tempfile.NamedTemporaryFile(suffix=".exe", delete=False) as f:
            f.write(pe_bytes)
            path = f.name
        try:
            pe = PeFile(path)
            self.assertTrue(pe.parse(), msg=f"parse failed: {pe.parse_error}")
            self.assertTrue(pe.is_valid)
            self.assertEqual(pe.machine, IMAGE_FILE_MACHINE_AMD64)
            self.assertTrue(pe.is_64bit)
            self.assertEqual(len(pe.imports), 1)
            desc = pe.imports[0]
            self.assertIsInstance(desc, ImportDescriptor)
            self.assertEqual(desc.dll_name.lower(), "kernel32.dll")
            self.assertEqual(len(desc.functions), 2)
            names = {fn.name for fn in desc.functions}
            self.assertEqual(names, {"CreateFileW", "CloseHandle"})
            for fn in desc.functions:
                self.assertFalse(fn.by_ordinal)
        finally:
            os.unlink(path)

    def test_parse_multiple_dlls(self):
        pe_bytes = build_minimal_x64_pe(imports=[
            ("kernel32.dll", ["CreateFileW", "CloseHandle", "ReadFile"]),
            ("user32.dll",   ["MessageBoxW", "GetForegroundWindow"]),
            ("ws2_32.dll",   ["WSAStartup"]),
        ])
        with tempfile.NamedTemporaryFile(suffix=".exe", delete=False) as f:
            f.write(pe_bytes)
            path = f.name
        try:
            pe = PeFile(path)
            self.assertTrue(pe.parse())
            self.assertEqual(len(pe.imports), 3)
            dll_names = {d.dll_name.lower() for d in pe.imports}
            self.assertEqual(dll_names,
                             {"kernel32.dll", "user32.dll", "ws2_32.dll"})
            # Each descriptor has the right number of funcs.
            by_dll = {d.dll_name.lower(): d for d in pe.imports}
            self.assertEqual(len(by_dll["kernel32.dll"].functions), 3)
            self.assertEqual(len(by_dll["user32.dll"].functions), 2)
            self.assertEqual(len(by_dll["ws2_32.dll"].functions), 1)
        finally:
            os.unlink(path)

    def test_parse_no_imports(self):
        # An empty imports list should still produce a valid (if useless) PE.
        pe_bytes = build_minimal_x64_pe(imports=[])
        with tempfile.NamedTemporaryFile(suffix=".exe", delete=False) as f:
            f.write(pe_bytes)
            path = f.name
        try:
            pe = PeFile(path)
            self.assertTrue(pe.parse())
            self.assertEqual(pe.imports, [])
        finally:
            os.unlink(path)

    def test_parse_invalid_pe(self):
        with tempfile.NamedTemporaryFile(suffix=".exe", delete=False) as f:
            f.write(b"not a PE file, just text")
            path = f.name
        try:
            pe = PeFile(path)
            self.assertFalse(pe.parse())
            self.assertFalse(pe.is_valid)
            self.assertIsNotNone(pe.parse_error)
            self.assertIn("DOS", pe.parse_error)
        finally:
            os.unlink(path)

    def test_parse_delay_import(self):
        pe_bytes = build_minimal_x64_pe_with_delay_import(
            "version.dll", ["GetFileVersionInfoW", "VerQueryValueW"]
        )
        with tempfile.NamedTemporaryFile(suffix=".exe", delete=False) as f:
            f.write(pe_bytes)
            path = f.name
        try:
            pe = PeFile(path)
            self.assertTrue(pe.parse(), msg=f"parse failed: {pe.parse_error}")
            # Should have at least one delay-load descriptor.
            self.assertTrue(any(d.is_delay_load for d in pe.imports),
                            msg=f"no delay-load imports found: {pe.imports}")
            delay = [d for d in pe.imports if d.is_delay_load]
            self.assertEqual(len(delay), 1)
            self.assertEqual(delay[0].dll_name.lower(), "version.dll")
            self.assertEqual(len(delay[0].functions), 2)
        finally:
            os.unlink(path)

    # ----- Shim registry extraction ---------------------------------------
    def test_shim_registry_extraction(self):
        # Build a fake shims/ dir with one .cpp that has both literal and
        # macro-expansion REGISTER_SHIM calls.
        with tempfile.TemporaryDirectory() as tmp:
            shims_dir = os.path.join(tmp, "shims")
            os.makedirs(shims_dir)
            fake_cpp = os.path.join(shims_dir, "FakeShim.cpp")
            with open(fake_cpp, "w") as f:
                f.write(
                    '#include <Windows.h>\n'
                    'extern "C" __declspec(dllexport) void Shim_Foo() {}\n'
                    'extern "C" __declspec(dllexport) void Shim_Bar() {}\n'
                    'extern "C" __declspec(dllexport) void Shim_Baz() {}\n'
                    '\n'
                    '// Literal registrations:\n'
                    'REGISTER_SHIM("kernel32", "CreateFileW", (FARPROC)&Shim_Foo);\n'
                    'REGISTER_SHIM("kernel32", "CloseHandle", (FARPROC)&Shim_Bar);\n'
                    '\n'
                    '// Macro pattern (like XInput / Steam):\n'
                    '#define XWR_REG_FAKE(dll)                                    \\\n'
                    '    REGISTER_SHIM(dll, "FakeInit", (FARPROC)&Shim_Baz);       \\\n'
                    '    REGISTER_SHIM(dll, "FakeShutdown", (FARPROC)&Shim_Baz);\n'
                    '\n'
                    'XWR_REG_FAKE("fake1.dll")\n'
                    'XWR_REG_FAKE("fake2.dll")\n'
                )
            shims = build_shim_registry(shims_dir)
            # 2 literal + 2 macros × 2 DLLs = 6 entries.
            # After normalize_dll_key(), the .dll suffix is stripped so
            # "fake1.dll" → "fake1" and "fake2.dll" → "fake2".
            self.assertEqual(len(shims), 6)
            self.assertIn(("kernel32", "createfilew"), shims)
            self.assertIn(("kernel32", "closehandle"), shims)
            self.assertIn(("fake1", "fakeinit"), shims)
            self.assertIn(("fake1", "fakeshutdown"), shims)
            self.assertIn(("fake2", "fakeinit"), shims)
            self.assertIn(("fake2", "fakeshutdown"), shims)

    def test_shim_registry_against_project_shims(self):
        # Sanity check: the real shims/ dir should yield hundreds of entries.
        here = os.path.dirname(os.path.abspath(__file__))
        proj_root = os.path.dirname(here)
        shims_dir = os.path.join(proj_root, "shims")
        if not os.path.isdir(shims_dir):
            self.skipTest("project shims dir not found")
        shims = build_shim_registry(shims_dir)
        # We know from the worklog that there are ~843 REGISTER_SHIM entries
        # (plus macro expansions for xinput / steam). So 500+ is a safe lower
        # bound.
        self.assertGreater(len(shims), 500,
                           msg=f"only {len(shims)} shims extracted — registry bug?")
        # Spot-check a few well-known shims.
        self.assertIn(("kernel32", "createfilew"), shims)
        self.assertIn(("user32", "messagew"), shims) if False else None  # placeholder
        self.assertIn(("user32", "getforegroundwindow"), shims)
        # xinput macro expansion: 4 DLLs × 6 funcs.
        for dll in ("xinput1_4", "xinput1_3", "xinput1_2", "xinput9_1_0"):
            self.assertIn((dll, "xinputgetstate"), shims)

    # ----- Coverage report -------------------------------------------------
    def test_coverage_report(self):
        # Build a synthetic PE that imports 1 shimmed + 1 unshimmed function
        # from kernel32.dll, and one function from a totally uncovered DLL.
        pe_bytes = build_minimal_x64_pe(imports=[
            ("kernel32.dll", ["CreateFileW", "SomeMissingKernelFunc"]),
            ("totally_unknown.dll", ["MysteryFunc"]),
        ])
        with tempfile.TemporaryDirectory() as tmp:
            game_dir = os.path.join(tmp, "game")
            os.makedirs(game_dir)
            with open(os.path.join(game_dir, "Game.exe"), "wb") as f:
                f.write(pe_bytes)

            # Use the project's real shims dir so CreateFileW resolves.
            here = os.path.dirname(os.path.abspath(__file__))
            proj_root = os.path.dirname(here)
            shims_dir = os.path.join(proj_root, "shims")

            report = build_coverage_report(game_dir, shims_dir)

            # File-level.
            self.assertEqual(report.total_files, 1)
            self.assertEqual(report.valid_files, 1)
            self.assertEqual(report.invalid_files, 0)

            # Per-DLL coverage.
            dll_map = {c.dll_name.lower(): c for c in report.dll_coverage}
            self.assertIn("kernel32.dll", dll_map)
            self.assertIn("totally_unknown.dll", dll_map)

            kc = dll_map["kernel32.dll"]
            self.assertIn("createfilew", kc.shimmed_functions)
            self.assertIn("somemissingkernelfunc", kc.missing_functions)
            self.assertTrue(kc.has_shim_module)

            uc = dll_map["totally_unknown.dll"]
            self.assertFalse(uc.has_shim_module)
            self.assertIn("mysteryfunc", uc.missing_functions)
            self.assertIn("totally_unknown.dll", report.uncovered_dlls)

            # Overall: 1 shimmed, 2 missing → coverage 1/3.
            self.assertEqual(report.total_shimmed, 1)
            self.assertEqual(report.total_missing, 2)
            self.assertAlmostEqual(report.coverage_pct, 100.0 * 1 / 3, places=2)

            # Renderers should not throw.
            md = render_markdown(report)
            self.assertIn("PE Import Coverage Report", md)
            self.assertIn("kernel32.dll", md)
            self.assertIn("totally_unknown.dll", md)
            js = render_json(report)
            self.assertIn('"dlls"', js)
            self.assertIn('"uncovered_dlls"', js)

    def test_scan_directory_returns_all_files(self):
        # Create a fake game dir with 2 synthetic PEs and 1 non-PE file.
        pe1 = build_minimal_x64_pe(imports=[("kernel32.dll", ["CreateFileW"])])
        pe2 = build_minimal_x64_pe(imports=[("user32.dll", ["MessageBoxW"])])
        with tempfile.TemporaryDirectory() as tmp:
            game_dir = os.path.join(tmp, "game")
            os.makedirs(game_dir)
            with open(os.path.join(game_dir, "A.exe"), "wb") as f:
                f.write(pe1)
            with open(os.path.join(game_dir, "B.dll"), "wb") as f:
                f.write(pe2)
            with open(os.path.join(game_dir, "readme.txt"), "w") as f:
                f.write("not a PE")

            pe_files, _shims, summaries = scan_directory(game_dir)
            self.assertEqual(len(pe_files), 2)
            self.assertEqual(len(summaries), 2)
            paths = sorted(os.path.basename(p.path) for p in pe_files)
            self.assertEqual(paths, ["A.exe", "B.dll"])
            for s in summaries:
                self.assertTrue(s.valid)
                self.assertEqual(s.machine_name, "x64")


if __name__ == "__main__":
    unittest.main(verbosity=2)
