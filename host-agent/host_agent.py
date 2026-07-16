#!/usr/bin/env python3
# =============================================================================
# host_agent.py
# =============================================================================
# A pure-stdlib Python HTTP server that brokers between a developer's PC and
# the Xbox Win32 Runner UWP shell. It scans Steam / Epic / GOG / Game Pass
# directories for installed games, exposes a small REST API for browsing them,
# and can push a selected .exe + its DLLs to the Xbox device portal over HTTP.
#
# Pure Python 3 stdlib only — no Flask / FastAPI / requests / pefile.
#
# Usage:
#   python3 host_agent.py --port 8000
#       [--xbox 192.168.1.100:11443]
#       [--scan-dir /extra/path/to/scan]
#       [--shims-dir /path/to/project/shims]
#       [--scanner /path/to/pe_import_scanner.py]
#
# Endpoints:
#   GET  /health                  → {"status":"ok","version":"1.0"}
#   GET  /games                   → list of detected games
#   GET  /games/<id>              → single-game detail + import coverage scan
#   POST /scan                    → re-scan game directories
#   POST /games/<id>/run          → upload the game's .exe to the Xbox
# =============================================================================

from __future__ import annotations

import argparse
import hashlib
import http.server
import json
import os
import re
import socketserver
import subprocess
import sys
import threading
import urllib.parse
import urllib.request
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# Import the PE scanner from ../tools/pe_import_scanner.py
_HERE = os.path.dirname(os.path.abspath(__file__))
_TOOLS_DIR = os.path.join(os.path.dirname(_HERE), "tools")
if _TOOLS_DIR not in sys.path:
    sys.path.insert(0, _TOOLS_DIR)

try:
    from pe_import_scanner import build_coverage_report, render_json  # noqa: E402
except Exception:  # pragma: no cover — scanner import should never break the server
    build_coverage_report = None  # type: ignore
    render_json = None  # type: ignore


# =============================================================================
# Game detection
# =============================================================================
@dataclass
class Game:
    """A detected installed game."""
    id: str               # stable hash of the install dir
    name: str             # human-friendly name (folder name)
    exe_path: str         # path to the main .exe
    install_dir: str      # directory containing the .exe (game root)
    source: str           # "steam" / "epic" / "gog" / "xboxgamepass" / "manual"
    size_bytes: int = 0   # .exe file size (for display / sorting)


def _is_windows() -> bool:
    return os.name == "nt"


def _sha_id(text: str) -> str:
    """Stable 12-char hex ID derived from a string (install dir)."""
    return hashlib.sha1(text.encode("utf-8")).hexdigest()[:12]


# ---------------------------------------------------------------------------
# VDF parser (for Steam libraryfolders.vdf)
# ---------------------------------------------------------------------------
def _parse_vdf(text: str) -> Dict:
    """
    Minimal Valve Data Format parser. Returns a nested dict.

    VDF is a quoted-key / quoted-value language with `{ ... }` blocks. This
    parser is intentionally tiny — it only needs to read `libraryfolders.vdf`,
    where the top-level structure is::

        "libraryfolders"
        {
            "0" { "path" "/home/.../steam" "apps" { "570" "..." ... } }
            "1" { ... }
        }
    """
    # Strip // line comments and /* */ block comments.
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)

    tokens = re.findall(r'"[^"]*"|[\{\}]', text)
    root: Dict = {}
    stack: List[Tuple[Dict, Optional[str]]] = [(root, None)]
    for tok in tokens:
        if tok == "{":
            # Begin a child block under the most-recently-seen key in the
            # current dict.
            current, pending_key = stack[-1]
            if pending_key is None:
                continue
            new_block: Dict = {}
            current[pending_key] = new_block
            stack.append((new_block, None))
        elif tok == "}":
            if len(stack) > 1:
                stack.pop()
        else:
            tok = tok[1:-1]  # strip quotes
            current, pending_key = stack[-1]
            if pending_key is None:
                stack[-1] = (current, tok)
            else:
                # Value for the pending key.
                current[pending_key] = tok
                stack[-1] = (current, None)
    return root


# ---------------------------------------------------------------------------
# Per-source scanners
# ---------------------------------------------------------------------------
def _find_main_exe(install_dir: str, game_name_hint: str) -> Optional[str]:
    """
    Pick the main .exe for a game folder.

    Heuristics (in order):
      1. Match the folder name (e.g. ``Half Sword/HalfSword.exe``).
      2. Exclude uninstallers (filename contains "unins" or "uninstall").
      3. Pick the largest remaining .exe by file size.
    """
    if not os.path.isdir(install_dir):
        return None
    # Hint: folder name with non-alphanumerics stripped, case-insensitive.
    hint = re.sub(r"[^A-Za-z0-9]", "", game_name_hint).lower()

    candidates: List[Tuple[int, str, str]] = []  # (size, basename, path)
    for dirpath, _dirs, files in os.walk(install_dir):
        for f in files:
            if not f.lower().endswith(".exe"):
                continue
            base = f[:-4]
            lower = base.lower()
            if "unins" in lower or "uninstall" in lower:
                continue
            full = os.path.join(dirpath, f)
            try:
                size = os.path.getsize(full)
            except OSError:
                size = 0
            candidates.append((size, base, full))

    if not candidates:
        return None

    # 1. Folder-name match (case-insensitive substring either way).
    if hint:
        for size, base, full in candidates:
            base_norm = re.sub(r"[^A-Za-z0-9]", "", base).lower()
            if hint in base_norm or base_norm in hint:
                return full

    # 2. Largest .exe.
    candidates.sort(key=lambda t: (-t[0], t[1]))
    return candidates[0][2]


def scan_steam() -> List[Game]:
    """
    Scan Steam library folders for installed games.

    On Windows: prefers ``HKEY_CURRENT_USER\\Software\\Valve\\Steam`` ``SteamPath``
    (parsed via ``winreg``). On Linux: prefers ``~/.steam/steam/steamapps/
    libraryfolders.vdf``. Both paths collapse to the same VDF format.
    """
    steam_root = None
    if _is_windows():
        try:
            import winreg  # type: ignore
            with winreg.OpenKey(
                winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam"
            ) as key:
                steam_root, _ = winreg.QueryValueEx(key, "SteamPath")
        except Exception:
            steam_root = None
    else:
        # Linux: ~/.steam/steam is the canonical symlink.
        for candidate in (
            os.path.expanduser("~/.steam/steam"),
            os.path.expanduser("~/.local/share/Steam"),
            os.path.expanduser("~/.steam"),
        ):
            if os.path.isdir(candidate):
                steam_root = candidate
                break

    if not steam_root:
        return []

    vdf_path = os.path.join(steam_root, "steamapps", "libraryfolders.vdf")
    if not os.path.isfile(vdf_path):
        # Fall back to scanning just the default steamapps/common directory.
        return _scan_steam_common(steam_root, [steam_root])

    try:
        with open(vdf_path, "r", encoding="utf-8", errors="replace") as fh:
            vdf_text = fh.read()
    except OSError:
        return []

    parsed = _parse_vdf(vdf_text)
    library = parsed.get("libraryfolders", {})
    library_paths: List[str] = []
    if isinstance(library, dict):
        for entry in library.values():
            if isinstance(entry, dict):
                p = entry.get("path")
                if isinstance(p, str) and os.path.isdir(p):
                    library_paths.append(p)
    if not library_paths:
        library_paths = [steam_root]

    return _scan_steam_common(steam_root, library_paths)


def _scan_steam_common(steam_root: str, library_paths: List[str]) -> List[Game]:
    """Walk ``<library>/steamapps/common/*`` for each Steam library."""
    games: List[Game] = []
    for lib in library_paths:
        common = os.path.join(lib, "steamapps", "common")
        if not os.path.isdir(common):
            continue
        for entry in sorted(os.listdir(common)):
            full = os.path.join(common, entry)
            if not os.path.isdir(full):
                continue
            exe = _find_main_exe(full, entry)
            if not exe:
                continue
            size = 0
            try:
                size = os.path.getsize(exe)
            except OSError:
                pass
            games.append(Game(
                id=_sha_id(full),
                name=entry,
                exe_path=exe,
                install_dir=full,
                source="steam",
                size_bytes=size,
            ))
    return games


def scan_epic() -> List[Game]:
    """
    Scan ``C:\\Program Files\\Epic Games\\*`` for installed games. On non-Windows
    hosts this returns an empty list (Epic doesn't install outside Windows).
    """
    if not _is_windows():
        return []
    return _scan_simple_root(r"C:\Program Files\Epic Games", source="epic")


def scan_gog() -> List[Game]:
    """Scan ``C:\\GOG Games\\*``."""
    if not _is_windows():
        return []
    return _scan_simple_root(r"C:\GOG Games", source="gog")


def scan_xbox_game_pass() -> List[Game]:
    """
    Scan ``C:\\Program Files\\WindowsApps\\*`` for installed games.

    WindowsApps is normally ACL-locked to the local user; even on Windows this
    usually returns an empty list without elevation.
    """
    if not _is_windows():
        return []
    return _scan_simple_root(r"C:\Program Files\WindowsApps", source="xboxgamepass")


def _scan_simple_root(root: str, source: str) -> List[Game]:
    if not os.path.isdir(root):
        return []
    games: List[Game] = []
    for entry in sorted(os.listdir(root)):
        full = os.path.join(root, entry)
        if not os.path.isdir(full):
            continue
        exe = _find_main_exe(full, entry)
        if not exe:
            continue
        size = 0
        try:
            size = os.path.getsize(exe)
        except OSError:
            pass
        games.append(Game(
            id=_sha_id(full),
            name=entry,
            exe_path=exe,
            install_dir=full,
            source=source,
            size_bytes=size,
        ))
    return games


def scan_manual(scan_dirs: List[str]) -> List[Game]:
    """Scan extra directories the user passed on the CLI."""
    games: List[Game] = []
    for d in scan_dirs:
        if not os.path.isdir(d):
            continue
        # Treat each immediate subfolder as a game.
        for entry in sorted(os.listdir(d)):
            full = os.path.join(d, entry)
            if not os.path.isdir(full):
                continue
            exe = _find_main_exe(full, entry)
            if not exe:
                continue
            size = 0
            try:
                size = os.path.getsize(exe)
            except OSError:
                pass
            games.append(Game(
                id=_sha_id(full),
                name=entry,
                exe_path=exe,
                install_dir=full,
                source="manual",
                size_bytes=size,
            ))
    return games


def scan_all(scan_dirs: Optional[List[str]] = None) -> List[Game]:
    """Run every per-source scanner and concatenate the results."""
    games: List[Game] = []
    games.extend(scan_steam())
    games.extend(scan_epic())
    games.extend(scan_gog())
    games.extend(scan_xbox_game_pass())
    if scan_dirs:
        games.extend(scan_manual(scan_dirs))
    # De-duplicate by install_dir (a game may live under more than one scanner).
    seen: set = set()
    unique: List[Game] = []
    for g in games:
        key = os.path.normpath(g.install_dir)
        if key in seen:
            continue
        seen.add(key)
        unique.append(g)
    return unique


# =============================================================================
# Xbox push (urllib multipart upload — stub for the real device portal API)
# =============================================================================
def push_exe_to_xbox(xbox_host: str, exe_path: str) -> Dict:
    """
    Upload `exe_path` to the Xbox device portal at `xbox_host:11443`.

    This is a stub: the real Windows Device Portal uses a different endpoint
    (``/api/app/packagemanager/package``) and a specific naming convention.
    The interface here is sufficient for end-to-end testing of the host agent
    itself; a follow-up agent will wire in the real WDP multipart format.
    """
    if not xbox_host:
        return {"ok": False, "error": "no --xbox configured"}
    url = f"https://{xbox_host}/api/files"
    if not os.path.isfile(exe_path):
        return {"ok": False, "error": f"exe not found: {exe_path}"}

    try:
        with open(exe_path, "rb") as fh:
            exe_bytes = fh.read()
    except OSError as exc:
        return {"ok": False, "error": f"could not read exe: {exc}"}

    boundary = "----xwr-host-agent-" + hashlib.sha1(exe_path.encode()).hexdigest()
    filename = os.path.basename(exe_path)
    body = (
        f"--{boundary}\r\n".encode()
        + b'Content-Disposition: form-data; name="file"; filename="'
        + filename.encode()
        + b'"\r\n'
        + b"Content-Type: application/octet-stream\r\n\r\n"
        + exe_bytes
        + b"\r\n"
        + f"--{boundary}--\r\n".encode()
    )
    headers = {
        "Content-Type": f"multipart/form-data; boundary={boundary}",
        "Content-Length": str(len(body)),
    }
    req = urllib.request.Request(url, data=body, headers=headers, method="POST")
    try:
        # NOTE: the Xbox dev portal uses a self-signed cert. In production
        # this would use ssl.create_default_context with cert verification
        # disabled. Here we keep the stub as-is.
        with urllib.request.urlopen(req, timeout=30) as resp:
            payload = resp.read().decode("utf-8", errors="replace")
            return {
                "ok": True,
                "status": resp.status,
                "url": url,
                "filename": filename,
                "size": len(exe_bytes),
                "response": payload[:512],
            }
    except Exception as exc:
        return {"ok": False, "error": str(exc), "url": url}


# =============================================================================
# HTTP server
# =============================================================================
class GameStore:
    """In-memory cache of detected games. Thread-safe via a Lock."""

    def __init__(self, scan_dirs: Optional[List[str]] = None) -> None:
        self._lock = threading.Lock()
        self._games: Dict[str, Game] = {}
        self._scan_dirs = scan_dirs or []
        self.refresh()

    def refresh(self) -> int:
        with self._lock:
            games = scan_all(self._scan_dirs)
            self._games = {g.id: g for g in games}
            return len(self._games)

    def all(self) -> List[Game]:
        with self._lock:
            return list(self._games.values())

    def get(self, game_id: str) -> Optional[Game]:
        with self._lock:
            return self._games.get(game_id)


# ---------------------------------------------------------------------------
# Multipart encoder (used by /games/<id>/run, since urllib doesn't have one)
# ---------------------------------------------------------------------------
def _multipart(files: List[Tuple[str, str, bytes]], boundary: str) -> bytes:
    """Build a multipart/form-data body from (field_name, filename, bytes)."""
    parts: List[bytes] = []
    for field, filename, data in files:
        parts.append(f"--{boundary}\r\n".encode())
        parts.append(
            f'Content-Disposition: form-data; name="{field}"; '
            f'filename="{filename}"\r\n'
            .encode()
        )
        parts.append(b"Content-Type: application/octet-stream\r\n\r\n")
        parts.append(data)
        parts.append(b"\r\n")
    parts.append(f"--{boundary}--\r\n".encode())
    return b"".join(parts)


# ---------------------------------------------------------------------------
# HTTP request handler
# ---------------------------------------------------------------------------
class HostAgentHandler(http.server.BaseHTTPRequestHandler):
    """REST API request handler. Bound to a GameStore via the server."""

    # ----- overrides --------------------------------------------------
    def log_message(self, fmt: str, *args) -> None:  # type: ignore[override]
        # Quieter logging — single line per request.
        sys.stderr.write(
            "[%s] %s - %s\n" % (self.log_date_time_string(),
                                self.address_string(), fmt % args)
        )

    # ----- routing ----------------------------------------------------
    def do_GET(self) -> None:  # noqa: N802 (BaseHTTPRequestHandler API)
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path.rstrip("/") or "/"
        if path == "/health":
            self._send_json({"status": "ok", "version": "1.0"})
            return
        if path == "/games":
            games = self.server.game_store.all()  # type: ignore[attr-defined]
            self._send_json([self._game_brief(g) for g in games])
            return
        if path.startswith("/games/"):
            game_id = path[len("/games/"):]
            game = self.server.game_store.get(game_id)  # type: ignore[attr-defined]
            if not game:
                self._send_json({"error": "not found", "id": game_id}, 404)
                return
            self._send_json(self._game_detail(game))
            return
        self._send_json({"error": "not found", "path": path}, 404)

    def do_POST(self) -> None:  # noqa: N802 (BaseHTTPRequestHandler API)
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path.rstrip("/") or "/"
        if path == "/scan":
            count = self.server.game_store.refresh()  # type: ignore[attr-defined]
            self._send_json({"ok": True, "games": count})
            return
        if path.endswith("/run") and path.startswith("/games/"):
            game_id = path[len("/games/"):-len("/run")]
            game = self.server.game_store.get(game_id)  # type: ignore[attr-defined]
            if not game:
                self._send_json({"error": "not found", "id": game_id}, 404)
                return
            result = push_exe_to_xbox(self.server.xbox_host, game.exe_path)  # type: ignore[attr-defined]
            status = 200 if result.get("ok") else 502
            self._send_json({"game_id": game_id, "result": result}, status)
            return
        self._send_json({"error": "not found", "path": path}, 404)

    # ----- helpers ----------------------------------------------------
    def _send_json(self, payload, status: int = 200) -> None:
        body = json.dumps(payload, indent=2, sort_keys=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _game_brief(self, game: Game) -> Dict:
        return {
            "id": game.id,
            "name": game.name,
            "exe_path": game.exe_path,
            "install_dir": game.install_dir,
            "source": game.source,
            "size_bytes": game.size_bytes,
        }

    def _game_detail(self, game: Game) -> Dict:
        brief = self._game_brief(game)
        # Run the PE import scanner on the install dir, in-process when the
        # module is importable (preferred), falling back to a subprocess
        # invocation of the scanner script.
        brief["coverage"] = self._scan_coverage(game.install_dir)
        return brief

    def _scan_coverage(self, install_dir: str) -> Dict:
        """Run the PE import scanner against `install_dir` and return its JSON."""
        # Try in-process first (faster, no subprocess overhead).
        if build_coverage_report is not None:
            try:
                report = build_coverage_report(install_dir)
                return {
                    "ok": True,
                    "total_files": report.total_files,
                    "valid_files": report.valid_files,
                    "invalid_files": report.invalid_files,
                    "total_imports": report.total_imports,
                    "total_shimmed": report.total_shimmed,
                    "total_missing": report.total_missing,
                    "coverage_pct": round(report.coverage_pct, 4),
                    "uncovered_dlls": report.uncovered_dlls,
                }
            except Exception as exc:
                return {"ok": False, "error": f"in-process scan failed: {exc}"}

        # Fallback: invoke tools/pe_import_scanner.py as a subprocess.
        scanner = getattr(self.server, "scanner_path", None)  # type: ignore[attr-defined]
        if not scanner or not os.path.isfile(scanner):
            return {"ok": False, "error": "scanner script not available"}
        try:
            proc = subprocess.run(
                [sys.executable, scanner, "--input", install_dir,
                 "--format", "json"],
                capture_output=True, text=True, timeout=60,
            )
            if proc.returncode != 0:
                return {"ok": False, "error": proc.stderr.strip()}
            data = json.loads(proc.stdout)
            overall = data.get("overall", {})
            return {
                "ok": True,
                "total_files": data.get("total_files"),
                "valid_files": data.get("valid_files"),
                "invalid_files": data.get("invalid_files"),
                "total_imports": overall.get("total_imports"),
                "total_shimmed": overall.get("total_shimmed"),
                "total_missing": overall.get("total_missing"),
                "coverage_pct": overall.get("coverage_pct"),
                "uncovered_dlls": data.get("uncovered_dlls", []),
            }
        except subprocess.TimeoutExpired:
            return {"ok": False, "error": "scanner timed out (>60s)"}
        except Exception as exc:
            return {"ok": False, "error": str(exc)}


class ThreadingHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    """Threaded HTTP server (so concurrent /scan + /games doesn't block)."""
    daemon_threads = True
    allow_reuse_address = True


# =============================================================================
# CLI
# =============================================================================
def _default_scanner_path() -> str:
    return os.path.join(_TOOLS_DIR, "pe_import_scanner.py")


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        prog="host_agent",
        description=(
            "Host-side agent for the Xbox Win32 Runner. Scans for installed "
            "games, exposes a REST API, and can push .exe files to the Xbox."
        ),
    )
    parser.add_argument("--port", type=int, default=8000,
                        help="HTTP listen port (default: 8000).")
    parser.add_argument("--host", default="0.0.0.0",
                        help="HTTP listen host (default: 0.0.0.0).")
    parser.add_argument("--xbox", default=None,
                        help="Xbox device portal host:port "
                             "(e.g. 192.168.1.100:11443). "
                             "Required for /games/<id>/run to succeed.")
    parser.add_argument("--scan-dir", action="append", default=[],
                        help="Additional directory to scan for games "
                             "(may be passed multiple times).")
    parser.add_argument("--scanner", default=_default_scanner_path(),
                        help="Path to pe_import_scanner.py (default: "
                             "<project>/tools/pe_import_scanner.py).")
    parser.add_argument("--shims-dir", default=None,
                        help="(Reserved) override shims/ directory.")
    args = parser.parse_args(argv)

    store = GameStore(scan_dirs=args.scan_dir)
    server = ThreadingHTTPServer((args.host, args.port), HostAgentHandler)
    server.game_store = store                  # type: ignore[attr-defined]
    server.xbox_host = args.xbox               # type: ignore[attr-defined]
    server.scanner_path = args.scanner         # type: ignore[attr-defined]

    print(f"host_agent listening on http://{args.host}:{args.port}")
    print(f"  games detected: {len(store.all())}")
    print(f"  xbox host:     {args.xbox or '(not configured)'}")
    print(f"  scanner:       {args.scanner}")
    if args.scan_dir:
        print(f"  extra scans:   {args.scan_dir}")
    print("endpoints:")
    print("  GET  /health")
    print("  GET  /games")
    print("  GET  /games/<id>")
    print("  POST /scan")
    print("  POST /games/<id>/run")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nshutting down …", file=sys.stderr)
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
