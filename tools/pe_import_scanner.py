#!/usr/bin/env python3
# =============================================================================
# pe_import_scanner.py
# =============================================================================
# A pure-Python (stdlib-only) PE import scanner for the Xbox Win32 Runner
# project.
#
# Walks a game install directory, parses every .exe / .dll PE file's import
# table (including delay-load imports) from scratch — no `pefile` dependency —
# and cross-references the discovered imports against the REGISTER_SHIM entries
# in the project's shims/*.cpp files to produce a coverage report.
#
# Usage:
#   python3 pe_import_scanner.py --input <game_dir>
#       [--shims-dir <path>]
#       [--output <report.md>]
#       [--format markdown|json]
#
# Default --shims-dir is ./shims relative to this script's parent directory
# (i.e. the project root).
# =============================================================================

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys
from dataclasses import dataclass, field, asdict
from typing import Dict, List, Optional, Set, Tuple

# ---------------------------------------------------------------------------
# PE constants (subset we actually use)
# ---------------------------------------------------------------------------
IMAGE_DOS_SIGNATURE   = 0x5A4D      # "MZ"
IMAGE_NT_SIGNATURE    = 0x00004550  # "PE\0\0"
IMAGE_FILE_MACHINE_I386 = 0x014C
IMAGE_FILE_MACHINE_AMD64 = 0x8664

IMAGE_OPTIONAL_HDR32_MAGIC = 0x10b
IMAGE_OPTIONAL_HDR64_MAGIC = 0x20b

IMAGE_DIRECTORY_ENTRY_IMPORT       = 1   # DataDirectory[1]
IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT = 13  # DataDirectory[13]

IMAGE_ORDINAL_FLAG32 = 0x80000000
IMAGE_ORDINAL_FLAG64 = 0x8000000000000000

# IMAGE_FILE_HEADER is 20 bytes; IMAGE_DATA_DIRECTORY entry is 8 bytes.
SIZEOF_FILE_HEADER       = 20
SIZEOF_SECTION_HEADER    = 40
SIZEOF_IMPORT_DESCRIPTOR = 20
SIZEOF_DELAY_DESCRIPTOR  = 32

# Offset of NumberOfRvaAndSizes / DataDirectory inside OptionalHeader differs
# between 32- and 64-bit flavours.
OPT32_DATA_DIRECTORY_OFFSET = 96   # 92 (offsetof NumberOfRvaAndSizes) + 4
OPT64_DATA_DIRECTORY_OFFSET = 112  # 108 (offsetof NumberOfRvaAndSizes) + 4


# ---------------------------------------------------------------------------
# Data classes for the parsed representation
# ---------------------------------------------------------------------------
@dataclass
class ImportedFunction:
    """A single imported function (by name or by ordinal)."""
    name: str            # "CreateFileW" or "ordinal:42"
    by_ordinal: bool
    ordinal: Optional[int] = None
    hint: Optional[int] = None  # hint value when imported by name


@dataclass
class ImportDescriptor:
    """All functions imported from a single DLL by a single PE file."""
    dll_name: str
    functions: List[ImportedFunction] = field(default_factory=list)
    is_delay_load: bool = False


@dataclass
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    pointer_to_raw_data: int
    size_of_raw_data: int


@dataclass
class PeFile:
    """A parsed PE file (32- or 64-bit) plus its import table."""
    path: str
    # Populated by parse():
    is_valid: bool = False
    parse_error: Optional[str] = None
    machine: int = 0
    is_64bit: bool = False
    sections: List[Section] = field(default_factory=list)
    imports: List[ImportDescriptor] = field(default_factory=list)
    has_forwarder_exports: bool = False  # noted, never recursed

    # ------------------------------------------------------------------
    # PE parsing
    # ------------------------------------------------------------------
    def parse(self) -> bool:
        """Parse the PE file at `self.path`. Returns True on success."""
        try:
            with open(self.path, "rb") as fh:
                data = fh.read()
        except OSError as exc:
            self.parse_error = f"could not open: {exc}"
            return False
        return self._parse_bytes(data)

    def _parse_bytes(self, data: bytes) -> bool:
        if len(data) < 64:
            self.parse_error = "file shorter than DOS header"
            return False

        # --- DOS header ---
        e_magic = struct.unpack_from("<H", data, 0)[0]
        if e_magic != IMAGE_DOS_SIGNATURE:
            self.parse_error = f"bad DOS magic 0x{e_magic:04X} (not MZ)"
            return False
        e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
        if e_lfanew == 0 or e_lfanew + 264 > len(data):
            self.parse_error = "e_lfanew out of range"
            return False

        # --- NT signature ---
        nt_sig = struct.unpack_from("<I", data, e_lfanew)[0]
        if nt_sig != IMAGE_NT_SIGNATURE:
            self.parse_error = f"bad NT signature 0x{nt_sig:08X}"
            return False

        # --- IMAGE_FILE_HEADER (20 bytes immediately after PE\0\0) ---
        fh_off = e_lfanew + 4
        machine, num_sections, _, _, _, size_of_opt_hdr, _ = \
            struct.unpack_from("<HHIIIHH", data, fh_off)
        if machine not in (IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_AMD64):
            self.parse_error = f"unsupported machine 0x{machine:04X}"
            return False
        self.machine = machine
        self.is_64bit = (machine == IMAGE_FILE_MACHINE_AMD64)

        # --- IMAGE_OPTIONAL_HEADER ---
        opt_off = fh_off + SIZEOF_FILE_HEADER
        if opt_off + 2 > len(data):
            self.parse_error = "OptionalHeader truncated"
            return False
        opt_magic = struct.unpack_from("<H", data, opt_off)[0]
        if opt_magic == IMAGE_OPTIONAL_HDR64_MAGIC:
            dd_offset = opt_off + OPT64_DATA_DIRECTORY_OFFSET
        elif opt_magic == IMAGE_OPTIONAL_HDR32_MAGIC:
            dd_offset = opt_off + OPT32_DATA_DIRECTORY_OFFSET
        else:
            self.parse_error = f"bad OptionalHeader magic 0x{opt_magic:04X}"
            return False

        # NumberOfRvaAndSizes is the DWORD immediately before DataDirectory.
        nrva_off = dd_offset - 4
        if nrva_off + 4 > len(data):
            self.parse_error = "NumberOfRvaAndSizes truncated"
            return False
        num_rva = struct.unpack_from("<I", data, nrva_off)[0]
        if num_rva < 16:
            # Some old PEs may have fewer than 16 entries; we only need 1 & 13.
            num_rva = min(num_rva, 16)

        # Read DataDirectory[0..num_rva)
        data_dirs: List[Tuple[int, int]] = []
        for i in range(num_rva):
            entry_off = dd_offset + i * 8
            if entry_off + 8 > len(data):
                break
            va, sz = struct.unpack_from("<II", data, entry_off)
            data_dirs.append((va, sz))

        # --- Section headers (right after OptionalHeader) ---
        sec_off = opt_off + size_of_opt_hdr
        self.sections = []
        for i in range(num_sections):
            so = sec_off + i * SIZEOF_SECTION_HEADER
            if so + SIZEOF_SECTION_HEADER > len(data):
                break
            name_raw = data[so:so + 8]
            name = name_raw.rstrip(b"\x00").decode("latin-1", errors="replace")
            vsize, vaddr, raw_size, raw_ptr = struct.unpack_from("<IIII", data, so + 8)
            self.sections.append(Section(
                name=name,
                virtual_address=vaddr,
                virtual_size=vsize,
                pointer_to_raw_data=raw_ptr,
                size_of_raw_data=raw_size,
            ))

        # --- Import table ---
        if len(data_dirs) > IMAGE_DIRECTORY_ENTRY_IMPORT:
            rva, size = data_dirs[IMAGE_DIRECTORY_ENTRY_IMPORT]
            if rva and size:
                self._parse_import_descriptors(data, rva, is_delay=False)

        # --- Delay-load import table ---
        if len(data_dirs) > IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT:
            rva, size = data_dirs[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]
            if rva and size:
                self._parse_delay_import_descriptors(data, rva)

        self.is_valid = True
        return True

    # ------------------------------------------------------------------
    # RVA → file offset
    # ------------------------------------------------------------------
    def _rva_to_offset(self, rva: int) -> Optional[int]:
        """Translate an RVA to a file offset using the section table."""
        for sec in self.sections:
            vsize = sec.virtual_size or sec.size_of_raw_data
            if sec.virtual_address <= rva < sec.virtual_address + vsize:
                return sec.pointer_to_raw_data + (rva - sec.virtual_address)
        # If not in any section, it may be inside the header itself — try a
        # direct mapping (RVA == file offset for the first section).
        if rva < 0x1000:
            return rva
        return None

    # ------------------------------------------------------------------
    # Standard imports
    # ------------------------------------------------------------------
    def _parse_import_descriptors(self, data: bytes, rva: int, is_delay: bool) -> None:
        """Walk the IMAGE_IMPORT_DESCRIPTOR array (20 bytes each, zero-terminated)."""
        off = self._rva_to_offset(rva)
        if off is None:
            return
        while off + SIZEOF_IMPORT_DESCRIPTOR <= len(data):
            oft, _ts, _fwd, name_rva, ft = struct.unpack_from("<IIIII", data, off)
            if oft == 0 and name_rva == 0 and ft == 0:
                break  # null terminator
            off += SIZEOF_IMPORT_DESCRIPTOR

            dll_name = self._read_cstr_at_rva(data, name_rva) or "<unknown>"
            functions = self._read_thunk_array(data, oft or ft)
            self.imports.append(ImportDescriptor(
                dll_name=dll_name,
                functions=functions,
                is_delay_load=is_delay,
            ))

    # ------------------------------------------------------------------
    # Delay-load imports (IMAGE_DELAYLOAD_DESCRIPTOR, 32 bytes)
    # ------------------------------------------------------------------
    def _parse_delay_import_descriptors(self, data: bytes, rva: int) -> None:
        off = self._rva_to_offset(rva)
        if off is None:
            return
        while off + SIZEOF_DELAY_DESCRIPTOR <= len(data):
            (attrs, dll_rva, _mod_handle_rva,
             iat_rva, int_rva, _biat_rva,
             _uit_rva, _ts) = struct.unpack_from("<IIIIIIII", data, off)
            if attrs == 0 and dll_rva == 0 and int_rva == 0:
                break  # null terminator
            off += SIZEOF_DELAY_DESCRIPTOR

            dll_name = self._read_cstr_at_rva(data, dll_rva) or "<unknown>"
            # Prefer INT; fall back to IAT for bound delay imports.
            thunk_rva = int_rva or iat_rva
            if thunk_rva:
                functions = self._read_thunk_array(data, thunk_rva)
            else:
                functions = []
            self.imports.append(ImportDescriptor(
                dll_name=dll_name,
                functions=functions,
                is_delay_load=True,
            ))

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------
    def _read_cstr_at_rva(self, data: bytes, rva: int) -> Optional[str]:
        off = self._rva_to_offset(rva)
        if off is None:
            return None
        end = data.find(b"\x00", off)
        if end < 0:
            end = len(data)
        try:
            return data[off:end].decode("ascii", errors="replace")
        except Exception:
            return None

    def _read_thunk_array(self, data: bytes, thunk_rva: int) -> List[ImportedFunction]:
        """Walk INT or IAT (array of IMAGE_THUNK_DATA)."""
        funcs: List[ImportedFunction] = []
        off = self._rva_to_offset(thunk_rva)
        if off is None:
            return funcs
        ptr_size = 8 if self.is_64bit else 4
        ordinal_flag = IMAGE_ORDINAL_FLAG64 if self.is_64bit else IMAGE_ORDINAL_FLAG32
        ordinal_mask = 0xFFFF
        idx = 0
        while True:
            pos = off + idx * ptr_size
            if pos + ptr_size > len(data):
                break
            if self.is_64bit:
                (val,) = struct.unpack_from("<Q", data, pos)
            else:
                (val,) = struct.unpack_from("<I", data, pos)
            if val == 0:
                break
            if val & ordinal_flag:
                funcs.append(ImportedFunction(
                    name=f"ordinal:{val & ordinal_mask}",
                    by_ordinal=True,
                    ordinal=val & ordinal_mask,
                ))
            else:
                # Low 31 bits = RVA of IMAGE_IMPORT_BY_NAME
                ibn_rva = val & 0x7FFFFFFF
                name = self._read_import_by_name(data, ibn_rva)
                funcs.append(name)
            idx += 1
        return funcs

    def _read_import_by_name(self, data: bytes, rva: int) -> ImportedFunction:
        """Read 2-byte hint + null-terminated name."""
        off = self._rva_to_offset(rva)
        if off is None or off + 2 > len(data):
            return ImportedFunction(name="<bad-ibn-rva>", by_ordinal=False)
        hint = struct.unpack_from("<H", data, off)[0]
        end = data.find(b"\x00", off + 2)
        if end < 0:
            end = len(data)
        name = data[off + 2:end].decode("ascii", errors="replace")
        return ImportedFunction(name=name, by_ordinal=False, hint=hint)


# ---------------------------------------------------------------------------
# Shim registry extraction
# ---------------------------------------------------------------------------
# Regex for literal REGISTER_SHIM("dll", "func", ...) — the args may be on
# multiple physical lines because the macro body is line-continued with `\`.
REG_SHIM_LITERAL = re.compile(
    r'REGISTER_SHIM\s*\(\s*"([^"]+)"\s*,\s*"([^"]+)"'
)
# #define MACRO(param) ... REGISTER_SHIM(param, "func", ...) ...
# Pattern A: parameter is the DLL name (e.g. XWR_REG_XINPUT(dll) → REGISTER_SHIM(dll, "func", ...))
REG_MACRO_DEF = re.compile(
    r'#define\s+(\w+)\s*\(\s*(\w+)\s*\)(.*?)(?=\n[^\s\\]|\n\s*#|\Z)',
    re.DOTALL,
)
# Pattern B: parameter is the function name, DLL is hardcoded (e.g. XWR_REG_K32(name) → REGISTER_SHIM("kernel32", #name, ...))
# We detect this by looking for REGISTER_SHIM("hardcoded_dll", #param, ...) in the macro body.
REG_MACRO_DEF_FUNCNAME = re.compile(
    r'#define\s+(\w+)\s*\(\s*(\w+)\s*\)(.*?)(?=\n[^\s\\]|\n\s*#|\Z)',
    re.DOTALL,
)
# Macro invocation: MACRO_NAME("dll")  — for Pattern A
REG_MACRO_CALL = re.compile(r'(\w+)\s*\(\s*"([^"]+)"\s*\)')
# Macro invocation: MACRO_NAME(FuncName)  — for Pattern B (unquoted identifier)
REG_MACRO_CALL_FUNCNAME = re.compile(r'(\w+)\s*\(\s*([A-Za-z_]\w*)\s*\)')


def _join_line_continuations(text: str) -> str:
    """Collapse `\\\n` into a space so multi-line macros become one logical line."""
    return re.sub(r'\\\s*\n', ' ', text)


def normalize_dll_key(name: str) -> str:
    """
    Normalise a DLL name for cross-referencing.

    The shim registry stores "kernel32" (no extension) because that's what
    REGISTER_SHIM macros pass, but PE imports use "kernel32.dll". Strip the
    trailing ``.dll`` / ``.exe`` / ``.drv`` / ``.ocx`` / ``.cpl`` extension
    and lowercase so both sides line up. Also accept the bare "kernel32" form
    unchanged.
    """
    n = name.strip().lower()
    for ext in (".dll", ".exe", ".drv", ".ocx", ".cpl", ".sys"):
        if n.endswith(ext):
            n = n[: -len(ext)]
            break
    return n


def build_shim_registry(shims_dir: str) -> Set[Tuple[str, str]]:
    """
    Walk every .cpp file under `shims_dir` and extract all (dll, funcname)
    pairs registered via REGISTER_SHIM.

    Handles both literal REGISTER_SHIM("dll", "func", ...) calls and the
    XWR_REG_XXX(dll) macro pattern used by XInput / Steam shims (where a
    #define XWR_REG_XINPUT(dll) block lists REGISTER_SHIM(dll, "func", ...)
    lines, then is invoked once per DLL name).

    Returns a set of (normalized_dll_key, funcname_lowercased) tuples. The
    DLL key has its ``.dll`` / ``.exe`` suffix stripped and is lowercased so
    that "kernel32" and "kernel32.dll" both resolve to the same entry.
    """
    shims: Set[Tuple[str, str]] = set()
    if not os.path.isdir(shims_dir):
        return shims

    # Collect every .cpp file under shims_dir.
    cpp_files: List[str] = []
    for root, _dirs, files in os.walk(shims_dir):
        for f in files:
            if f.endswith(".cpp"):
                cpp_files.append(os.path.join(root, f))

    # Macro registry for Pattern A (param=DLL name): name -> list of funcnames
    macro_defs: Dict[str, List[str]] = {}
    # Macro registry for Pattern B (param=func name): name -> list of (dll_name, )
    macro_funcname_defs: Dict[str, List[str]] = {}  # name -> list of hardcoded DLL names

    # Pass 1: extract macro definitions and direct literal calls.
    for cpp_path in cpp_files:
        try:
            with open(cpp_path, "r", encoding="utf-8", errors="replace") as fh:
                raw = fh.read()
        except OSError:
            continue
        text = _join_line_continuations(raw)

        # Direct literal REGISTER_SHIM("dll", "func", ...)
        for m in REG_SHIM_LITERAL.finditer(text):
            dll = normalize_dll_key(m.group(1))
            fn = m.group(2).strip().lower()
            if dll and fn:
                shims.add((dll, fn))

        # Pattern A: #define NAME(dll_param) ... REGISTER_SHIM(dll_param, "func", ...) ...
        for m in REG_MACRO_DEF.finditer(text):
            name = m.group(1)
            param = m.group(2)
            body = m.group(3)
            pattern = re.compile(
                r'REGISTER_SHIM\s*\(\s*' + re.escape(param) + r'\s*,\s*"([^"]+)"'
            )
            funcs = [g.strip().lower() for g in pattern.findall(body)]
            if funcs:
                macro_defs[name] = funcs

        # Pattern B: #define NAME(func_param) ... REGISTER_SHIM("hardcoded_dll", #func_param, ...) ...
        # Extract the hardcoded DLL name(s) from the REGISTER_SHIM calls in the body.
        for m in REG_MACRO_DEF_FUNCNAME.finditer(text):
            name = m.group(1)
            param = m.group(2)
            body = m.group(3)
            # Look for REGISTER_SHIM("dll", #param, ...) in the body
            pattern = re.compile(
                r'REGISTER_SHIM\s*\(\s*"([^"]+)"\s*,\s*#\s*' + re.escape(param)
            )
            dlls = [g.strip() for g in pattern.findall(body)]
            if dlls:
                macro_funcname_defs[name] = dlls

    # Pass 2: expand macro invocations.
    for cpp_path in cpp_files:
        try:
            with open(cpp_path, "r", encoding="utf-8", errors="replace") as fh:
                raw = fh.read()
        except OSError:
            continue
        text = _join_line_continuations(raw)

        # Pattern A: NAME("dll") → register (dll, func) for each func in macro_defs[NAME]
        for m in REG_MACRO_CALL.finditer(text):
            name = m.group(1)
            dll = normalize_dll_key(m.group(2))
            if name in macro_defs and dll:
                for fn in macro_defs[name]:
                    shims.add((dll, fn))

        # Pattern B: NAME(FuncName) → register (dll, "funcname") for each dll in macro_funcname_defs[NAME]
        for m in REG_MACRO_CALL_FUNCNAME.finditer(text):
            name = m.group(1)
            func = m.group(2)
            if name in macro_funcname_defs:
                fn_lower = func.lower()
                for dll in macro_funcname_defs[name]:
                    dll_norm = normalize_dll_key(dll)
                    if dll_norm and fn_lower:
                        shims.add((dll_norm, fn_lower))

    return shims


# ---------------------------------------------------------------------------
# Directory walking + per-file scanning
# ---------------------------------------------------------------------------
PE_EXTENSIONS = (".exe", ".dll")


def find_pe_files(root_dir: str) -> List[str]:
    """Return a sorted list of every .exe / .dll under `root_dir`."""
    out: List[str] = []
    if not os.path.isdir(root_dir):
        return out
    for dirpath, _dirs, files in os.walk(root_dir):
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext in PE_EXTENSIONS:
                out.append(os.path.join(dirpath, f))
    out.sort()
    return out


@dataclass
class FileSummary:
    path: str
    valid: bool
    error: Optional[str]
    machine: int
    machine_name: str  # "x64" / "x86" / "unknown"
    total_imports: int
    distinct_dlls: int
    dll_names: List[str]
    is_delay_load_present: bool


def summarize_pe(pe: PeFile) -> FileSummary:
    machine_name = {
        IMAGE_FILE_MACHINE_AMD64: "x64",
        IMAGE_FILE_MACHINE_I386:  "x86",
    }.get(pe.machine, "unknown")
    dll_names = sorted({d.dll_name for d in pe.imports})
    return FileSummary(
        path=pe.path,
        valid=pe.is_valid,
        error=pe.parse_error,
        machine=pe.machine,
        machine_name=machine_name,
        total_imports=sum(len(d.functions) for d in pe.imports),
        distinct_dlls=len(dll_names),
        dll_names=dll_names,
        is_delay_load_present=any(d.is_delay_load for d in pe.imports),
    )


@dataclass
class DllCoverage:
    dll_name: str
    total_calls: int = 0               # total imports across all files
    distinct_functions: Set[str] = field(default_factory=set)
    shimmed_functions: Set[str] = field(default_factory=set)
    missing_functions: Set[str] = field(default_factory=set)
    has_shim_module: bool = False      # True if registry has ≥1 fn for this dll


def scan_directory(input_dir: str, shims_dir: Optional[str] = None
                   ) -> Tuple[List[PeFile], Set[Tuple[str, str]], List[FileSummary]]:
    """
    Walk `input_dir`, parse every PE, build per-file + per-DLL summaries.

    Returns:
      pe_files           – list of PeFile objects (even invalid ones, so the
                            report can mention them)
      shim_registry       – set of (dll_lower, func_lower)
      file_summaries      – one FileSummary per parsed PE
    """
    if shims_dir is None:
        # Default: ./shims relative to the project root (parent of tools/).
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(script_dir)
        shims_dir = os.path.join(project_root, "shims")

    shim_registry = build_shim_registry(shims_dir)
    pe_files: List[PeFile] = []
    summaries: List[FileSummary] = []

    for path in find_pe_files(input_dir):
        pe = PeFile(path=path)
        pe.parse()
        pe_files.append(pe)
        summaries.append(summarize_pe(pe))

    return pe_files, shim_registry, summaries


# ---------------------------------------------------------------------------
# Coverage report
# ---------------------------------------------------------------------------
@dataclass
class CoverageReport:
    """Aggregated coverage data ready to be rendered as markdown / JSON."""
    input_dir: str
    shims_dir: str
    total_files: int
    valid_files: int
    invalid_files: int
    file_summaries: List[FileSummary]
    dll_coverage: List[DllCoverage]
    uncovered_dlls: List[str]
    total_imports: int
    total_shimmed: int
    total_missing: int
    coverage_pct: float
    shim_registry_size: int


def build_coverage_report(input_dir: str, shims_dir: Optional[str] = None
                          ) -> CoverageReport:
    pe_files, shim_registry, summaries = scan_directory(input_dir, shims_dir)

    # Build per-DLL coverage.
    dll_map: Dict[str, DllCoverage] = {}
    for pe in pe_files:
        if not pe.is_valid:
            continue
        for desc in pe.imports:
            dll_key = normalize_dll_key(desc.dll_name)
            if dll_key not in dll_map:
                dll_map[dll_key] = DllCoverage(dll_name=desc.dll_name)
            cov = dll_map[dll_key]
            for fn in desc.functions:
                fnl = fn.name.lower()
                cov.total_calls += 1
                cov.distinct_functions.add(fnl)
                if (dll_key, fnl) in shim_registry:
                    cov.shimmed_functions.add(fnl)
                else:
                    cov.missing_functions.add(fnl)

    # Mark has_shim_module: registry contains ≥1 (dll, *) for this dll_key.
    shim_dlls = {d for (d, _f) in shim_registry}
    for cov in dll_map.values():
        # Re-normalize in case dll_name was modified (it shouldn't be, but be safe).
        key = normalize_dll_key(cov.dll_name)
        cov.has_shim_module = key in shim_dlls

    # Sort: missing-most-first, then alphabetical.
    dll_list = sorted(
        dll_map.values(),
        key=lambda c: (-len(c.missing_functions), c.dll_name.lower()),
    )
    uncovered_dlls = sorted(
        c.dll_name for c in dll_list if not c.has_shim_module
    )

    total_imports  = sum(c.total_calls for c in dll_list)
    total_shimmed  = sum(
        sum(1 for f in c.distinct_functions if f in c.shimmed_functions)
        for c in dll_list
    )
    total_missing  = sum(
        sum(1 for f in c.distinct_functions if f in c.missing_functions)
        for c in dll_list
    )
    distinct_funcs = total_shimmed + total_missing
    coverage_pct = (100.0 * total_shimmed / distinct_funcs) if distinct_funcs else 0.0

    return CoverageReport(
        input_dir=input_dir,
        shims_dir=shims_dir or "(default ./shims)",
        total_files=len(summaries),
        valid_files=sum(1 for s in summaries if s.valid),
        invalid_files=sum(1 for s in summaries if not s.valid),
        file_summaries=summaries,
        dll_coverage=dll_list,
        uncovered_dlls=uncovered_dlls,
        total_imports=total_imports,
        total_shimmed=total_shimmed,
        total_missing=total_missing,
        coverage_pct=coverage_pct,
        shim_registry_size=len(shim_registry),
    )


# ---------------------------------------------------------------------------
# Renderers
# ---------------------------------------------------------------------------
def render_markdown(report: CoverageReport) -> str:
    lines: List[str] = []
    L = lines.append

    L("# PE Import Coverage Report")
    L("")
    L(f"- **Input directory:** `{report.input_dir}`")
    L(f"- **Shims directory:** `{report.shims_dir}`")
    L(f"- **Files scanned:** {report.total_files} "
      f"({report.valid_files} valid, {report.invalid_files} skipped)")
    L(f"- **Shim registry size:** {report.shim_registry_size} (dll, func) pairs")
    L("")

    L("## Overall Coverage")
    L("")
    L("| Metric | Value |")
    L("|---|---|")
    L(f"| Total import calls | {report.total_imports} |")
    L(f"| Distinct functions shimmed | {report.total_shimmed} |")
    L(f"| Distinct functions missing | {report.total_missing} |")
    L(f"| Coverage % | **{report.coverage_pct:.2f}%** |")
    L("")

    if report.uncovered_dlls:
        L("## ⚠ Uncovered DLLs (no shim module)")
        L("")
        L("These DLLs are imported by at least one scanned file but have **no** "
          "REGISTER_SHIM entry at all:")
        L("")
        for dll in report.uncovered_dlls:
            L(f"- `{dll}`")
        L("")

    L("## Per-File Summary")
    L("")
    L("| File | Machine | Imports | Distinct DLLs | Delay-load? | Notes |")
    L("|---|---|---|---|---|---|")
    for s in report.file_summaries:
        rel = os.path.relpath(s.path, report.input_dir) if os.path.isdir(report.input_dir) else s.path
        if not s.valid:
            L(f"| `{rel}` | — | — | — | — | **invalid PE**: {s.error or 'unknown'} |")
            continue
        delay = "yes" if s.is_delay_load_present else "no"
        notes = ""
        if s.total_imports == 0:
            notes = "no imports"
        L(f"| `{rel}` | {s.machine_name} | {s.total_imports} | {s.distinct_dlls} | {delay} | {notes} |")
    L("")

    L("## Per-DLL Coverage")
    L("")
    L("| DLL | Total calls | Distinct funcs | Shimmed | Missing | Coverage % |")
    L("|---|---|---|---|---|---|")
    for c in report.dll_coverage:
        distinct = len(c.distinct_functions)
        pct = (100.0 * len(c.shimmed_functions) / distinct) if distinct else 0.0
        flag = " ⚠" if not c.has_shim_module else ""
        L(f"| `{c.dll_name}`{flag} | {c.total_calls} | {distinct} | "
          f"{len(c.shimmed_functions)} | {len(c.missing_functions)} | "
          f"{pct:.2f}% |")
    L("")

    L("## Missing Functions (grouped by DLL)")
    L("")
    any_missing = False
    for c in report.dll_coverage:
        if not c.missing_functions:
            continue
        any_missing = True
        L(f"### `{c.dll_name}` ({len(c.missing_functions)} missing)")
        L("")
        for fn in sorted(c.missing_functions):
            L(f"- `{fn}`")
        L("")
    if not any_missing:
        L("_(none — every imported function has a matching shim)_")
        L("")

    return "\n".join(lines) + "\n"


def _coverage_to_dict(c: DllCoverage) -> dict:
    return {
        "dll_name": c.dll_name,
        "total_calls": c.total_calls,
        "distinct_functions": sorted(c.distinct_functions),
        "shimmed_functions": sorted(c.shimmed_functions),
        "missing_functions": sorted(c.missing_functions),
        "has_shim_module": c.has_shim_module,
    }


def _summary_to_dict(s: FileSummary) -> dict:
    return {
        "path": s.path,
        "valid": s.valid,
        "error": s.error,
        "machine": s.machine,
        "machine_name": s.machine_name,
        "total_imports": s.total_imports,
        "distinct_dlls": s.distinct_dlls,
        "dll_names": s.dll_names,
        "is_delay_load_present": s.is_delay_load_present,
    }


def render_json(report: CoverageReport) -> str:
    payload = {
        "input_dir": report.input_dir,
        "shims_dir": report.shims_dir,
        "total_files": report.total_files,
        "valid_files": report.valid_files,
        "invalid_files": report.invalid_files,
        "shim_registry_size": report.shim_registry_size,
        "overall": {
            "total_imports": report.total_imports,
            "total_shimmed": report.total_shimmed,
            "total_missing": report.total_missing,
            "coverage_pct": round(report.coverage_pct, 4),
        },
        "uncovered_dlls": report.uncovered_dlls,
        "files": [_summary_to_dict(s) for s in report.file_summaries],
        "dlls": [_coverage_to_dict(c) for c in report.dll_coverage],
    }
    return json.dumps(payload, indent=2, sort_keys=False)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        prog="pe_import_scanner",
        description=(
            "Walk a game install directory, parse every .exe/.dll import "
            "table, and report shim coverage against the project's "
            "REGISTER_SHIM registry."
        ),
    )
    parser.add_argument("--input", "-i", required=True,
                        help="Game install directory to scan.")
    parser.add_argument("--shims-dir", default=None,
                        help="Path to the shims/ directory (default: "
                             "../shims relative to this script).")
    parser.add_argument("--output", "-o", default=None,
                        help="Write report to this file (default: stdout).")
    parser.add_argument("--format", choices=("markdown", "json"),
                        default="markdown",
                        help="Report format (default: markdown).")
    args = parser.parse_args(argv)

    if not os.path.isdir(args.input):
        print(f"error: input directory does not exist: {args.input}",
              file=sys.stderr)
        return 2

    report = build_coverage_report(args.input, args.shims_dir)
    if args.format == "json":
        out_text = render_json(report)
    else:
        out_text = render_markdown(report)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as fh:
            fh.write(out_text)
        print(f"wrote {args.output} "
              f"({report.total_files} files, "
              f"{report.coverage_pct:.2f}% coverage)",
              file=sys.stderr)
    else:
        sys.stdout.write(out_text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
