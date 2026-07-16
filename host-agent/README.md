# Host Agent (Xbox Win32 Runner)

A pure-Python-stdlib HTTP server that brokers between a developer's PC and
the Xbox Win32 Runner UWP shell on an Xbox Series X/S in Developer Mode.

- **Discovers** installed games on Steam, Epic, GOG and Xbox Game Pass.
- **Exposes** a small REST API for browsing and re-scanning.
- **Pushes** a selected game's `.exe` to the Xbox device portal.

No external dependencies — only the Python 3.9+ standard library.

## Requirements

- Python 3.9 or newer (uses `dataclasses`, type hints, `http.server`).
- The companion `tools/pe_import_scanner.py` (imported in-process for
  the `/games/<id>` coverage scan; falls back to a subprocess if the
  module isn't importable).

## Running

```bash
cd host-agent
python3 host_agent.py --port 8000
```

Optional arguments:

| Flag            | Default                                         | Notes                                               |
|-----------------|-------------------------------------------------|-----------------------------------------------------|
| `--port`        | `8000`                                          | HTTP listen port.                                   |
| `--host`        | `0.0.0.0`                                       | HTTP bind address.                                  |
| `--xbox`        | _(none)_                                        | `host:port` of the Xbox device portal, e.g. `192.168.1.100:11443`. Required for `POST /games/<id>/run`. |
| `--scan-dir`    | _(none)_                                        | Extra directory to scan for games (may be passed multiple times). |
| `--scanner`     | `../tools/pe_import_scanner.py`                 | Override path to the scanner script.                |
| `--shims-dir`   | `../shims`                                      | Override the shims/ directory used by the scanner.  |

## Endpoints

| Method | Path                  | Description                                                                 |
|--------|-----------------------|-----------------------------------------------------------------------------|
| GET    | `/health`             | Liveness probe → `{"status":"ok","version":"1.0"}`.                        |
| GET    | `/games`              | List detected games.                                                        |
| GET    | `/games/<id>`         | Single-game detail, including the PE import coverage scan.                 |
| POST   | `/scan`               | Re-scan all configured sources.                                            |
| POST   | `/games/<id>/run`     | Upload the game's `.exe` to the Xbox device portal (stub).                 |

### Example

```bash
$ curl http://localhost:8000/health
{"status":"ok","version":"1.0"}

$ curl http://localhost:8000/games | head
[
  {
    "id": "9f2a1c8e4b7d",
    "name": "Half Sword",
    "exe_path": "/home/z/.steam/steam/steamapps/common/Half Sword/HalfSword.exe",
    "install_dir": "/home/z/.steam/steam/steamapps/common/Half Sword",
    "source": "steam",
    "size_bytes": 8421376
  }
]

$ curl http://localhost:8000/games/9f2a1c8e4b7d
{
  "id": "9f2a1c8e4b7d",
  "name": "Half Sword",
  "exe_path": "...",
  "install_dir": "...",
  "source": "steam",
  "size_bytes": 8421376,
  "coverage": {
    "ok": true,
    "total_files": 14,
    "valid_files": 14,
    "invalid_files": 0,
    "total_imports": 432,
    "total_shimmed": 401,
    "total_missing": 31,
    "coverage_pct": 92.82,
    "uncovered_dlls": ["d3dcompiler_47.dll"]
  }
}

$ curl -X POST http://localhost:8000/scan
{"ok": true, "games": 47}
```

## Game Detection Heuristics

For each scanned folder (e.g. `steamapps/common/Half Sword/`):

1. Match the **folder name** (alphanumeric, case-insensitive) against the
   `.exe` filename — e.g. `Half Sword/HalfSword.exe`.
2. Otherwise, exclude uninstallers (filenames containing `unins` or
   `uninstall`).
3. Pick the **largest remaining `.exe`** by file size.

## Per-Platform Behaviour

| Source       | Windows                                       | Linux                                                     |
|--------------|-----------------------------------------------|-----------------------------------------------------------|
| Steam        | `HKCU\Software\Valve\Steam` → `SteamPath`, parse `steamapps/libraryfolders.vdf`, walk every library's `steamapps/common/*`. | Reads `~/.steam/steam/steamapps/libraryfolders.vdf` (or `~/.local/share/Steam`). |
| Epic         | `C:\Program Files\Epic Games\*`               | _(empty — Epic is Windows-only)_                          |
| GOG          | `C:\GOG Games\*`                              | _(empty)_                                                 |
| Xbox Game Pass | `C:\Program Files\WindowsApps\*` (usually ACL-locked) | _(empty)_                                       |

Use `--scan-dir` to point the agent at any other directory containing game
folders.

## Xbox Push (Stub)

`POST /games/<id>/run` reads the game's `.exe` and uploads it as
`multipart/form-data` to `https://<xbox>/api/files`. The real Xbox Device
Portal uses a different endpoint (`/api/app/packagemanager/package`) and a
specific naming convention — this stub exists so the host-agent interface can
be end-to-end tested before the real WDP wiring lands. Replace
`push_exe_to_xbox()` with the real WDP call when integrating.

## In-Process vs Subprocess Scanner

`GET /games/<id>` runs the PE import scanner on the game's install dir. The
agent first tries to import `pe_import_scanner.build_coverage_report`
in-process (faster — no subprocess overhead); if the import fails it falls
back to running `python3 tools/pe_import_scanner.py --format json` as a
subprocess and parsing the JSON. Either path produces the same `coverage`
field in the response.
