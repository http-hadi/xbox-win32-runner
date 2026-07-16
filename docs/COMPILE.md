# Building the Xbox Win32 Runner

This document explains how to compile the Xbox Win32 Runner from source,
produce a sideload-ready `.appx` package, deploy it to an Xbox Series X|S in
Developer Mode, and configure it to launch a target native Win32 game
(Half Sword is the reference target).

The audience is C++/UWP developers who already have an Xbox in Developer
Mode and a Windows 10/11 development PC with Visual Studio 2022 installed.
No prior familiarity with the project's internal architecture is assumed;
for that, see `docs/ARCHITECTURE.md`.

---

## Table of Contents

1.  [Prerequisites](#1-prerequisites)
2.  [Opening the Project](#2-opening-the-project)
3.  [Signing Certificate](#3-signing-certificate)
4.  [C++/WinRT Setup](#4-cwinrt-setup)
5.  [Build for x64 (Test Compile)](#5-build-for-x64-test-compile)
6.  [Build for Xbox](#6-build-for-xbox)
7.  [Creating the .appx Package](#7-creating-the-appx-package)
8.  [Deploy via Xbox Device Portal](#8-deploy-via-xbox-device-portal)
9.  [Testing with test_hello.exe](#9-testing-with-test_helloexe)
10. [Common Compile Errors and Fixes](#10-common-compile-errors-and-fixes)
11. [Run Configuration](#11-run-configuration)

---

## 1. Prerequisites

### 1.1 Operating system

The development PC must run **Windows 10 version 2004 (build 19041) or
later**, or **Windows 11**. Earlier Windows 10 builds do not ship the SDK
that the project targets (`10.0.22621.0`).

### 1.2 Visual Studio 2022

Install **Visual Studio 2022 version 17.0 or later** (Community, Professional,
or Enterprise are all fine). The free Community edition is sufficient.

From the Visual Studio Installer, select the **Workloads** tab and tick:

- **Universal Windows Platform development**
  - In the "Installation details" pane on the right, ensure the following
    optional components are checked:
    - **C++ (v143) Universal Windows Platform tools**
    - **Windows 10 SDK (10.0.22621.0)** or later
- **Desktop development with C++** (provides the MSVC toolchain and ATL/MFC
  headers that the UWP workload references indirectly)
  - On the right, ensure **MSVC v143 - VS 2022 C++ x64/x86 build tools
    (latest)** is checked.

### 1.3 Xbox Live Workload (for the `Xbox` platform)

To build the `Release|Xbox` configuration you must also install the Xbox
Live SDK and platform support:

- In the Visual Studio Installer, click the **Individual components** tab and
  search for **Xbox**. Install:
  - **Xbox Live SDK**
  - **Xbox Development Kit (XDK) Workload Extension** (requires a free
    Microsoft Partner Center enrolment to download — see
    https://developer.microsoft.com/en-us/games/xbox/partner).

If the Xbox platform option is missing from the project's configuration
drop-down after install, see [Common Compile Errors and Fixes](#10-common-compile-errors-and-fixes)
under "Xbox platform not available".

### 1.4 Windows SDK

The project targets **Windows SDK 10.0.22621.0** (Windows 11 22H2). Later
SDKs (e.g. 10.0.22631.0, 10.0.26100.0) also work; the
`WindowsTargetPlatformMinVersion` is fixed at `10.0.22621.0` in the vcxproj,
and the `WindowsTargetPlatformVersion` is set to `10.0` which resolves to
the latest installed SDK at build time.

To verify the installed SDKs, run this from a Developer Command Prompt:

```cmd
dir "%ProgramFiles(x86)%\Windows Kits\10\Include"
```

You should see at least one directory named `10.0.22621.0` or higher.

### 1.5 C++/WinRT

The project uses C++/WinRT for the UWP CoreWindow / ApplicationData /
Windows.Gaming.Input surface that the bridges call into.

- Open `uwp-shell/packages.config` in the project. It declares a dependency
  on **`Microsoft.Windows.CppWinRT` version `2.0.230706.1`**.
- The first build will restore this NuGet package automatically. To
  pre-restore manually, right-click the project in Solution Explorer →
  **Manage NuGet Packages** → confirm the package is listed under
  "Installed".

The `Microsoft.Windows.CppWinRT` package ships `cppwinrt.exe`, which runs at
build time and generates the `winrt/` projection headers from the
`.winmd` metadata of the SDKs you have installed.

### 1.6 Optional: Windows Subsystem for Linux

The repository ships a Linux-side syntax-check harness,
`scripts/check_all.sh`, that compiles every `.cpp` file with
`g++ -std=c++17 -fsyntax-only -DXWR_SYNTAX_CHECK` against a stub
`winheaders/Windows.h`. This is how the project verifies that no source
file has drifted into a syntax error between Visual Studio builds. It is
optional, but useful if you develop on a Linux machine or want to run the
check from a WSL terminal on Windows.

To use it, install any Linux distribution from the Microsoft Store (Ubuntu
22.04 LTS is recommended) and the `g++` package:

```bash
sudo apt-get update && sudo apt-get install -y g++ make
```

No additional libraries are required — the syntax check is self-contained.

### 1.7 Summary checklist

Before you continue, confirm:

- [ ] Visual Studio 2022 17.0+ installed
- [ ] UWP development workload with C++ v143 tools and the 10.0.22621.0 SDK
- [ ] Desktop development with C++ workload with MSVC v143 build tools
- [ ] Xbox Live SDK + XDK workload (for the Xbox platform target)
- [ ] `Microsoft.Windows.CppWinRT` 2.0.230706.1 listed in
  `uwp-shell/packages.config`
- [ ] An Xbox Series X|S in Developer Mode with Device Portal enabled (for
  deployment — see Section 8)

---

## 2. Opening the Project

The repository does **not** ship a Visual Studio `.sln` file. The
`.vcxproj` is intentionally self-contained: opening it directly in Visual
Studio produces a one-project solution. This is the supported entry point.

1. Launch **Visual Studio 2022**.
2. On the start page, click **Open a project or solution**.
3. Browse to the cloned repository and select:

   ```
   xbox-win32-runner/uwp-shell/XboxWin32Runner.vcxproj
   ```

   Do **not** pick `XboxWin32Runner.vcxproj.filters` by accident; that file
   is the filter definition and opening it produces an empty solution.

4. Visual Studio will spend a few seconds restoring the
   `Microsoft.Windows.CppWinRT` NuGet package. Watch the status bar; when
   it reads "Ready", the package is installed.

5. Verify the available configurations: open the configuration drop-down
   on the Standard toolbar. You should see four entries:

   - `Debug|x64`
   - `Release|x64`
   - `Debug|Xbox`
   - `Release|Xbox`

   If `Debug|Xbox` and `Release|Xbox` are missing, the Xbox Live Workload
   extension is not installed correctly — see Section 10.

6. In Solution Explorer, expand the project tree. You should see folders
   for **UWP Shell**, **PeLoader**, **Shims** (with one sub-folder per shim
   module), **D3D11 Bridge**, **Bridges**, and **Assets**. If any folder is
   empty or missing, the `.vcxproj.filters` file is out of sync with the
   `.vcxproj` (this should not happen, but a `git status` will reveal
   local edits).

### 2.1 Why no .sln?

A `.sln` file is a convenience for multi-project solutions. The Xbox Win32
Runner is a single C++ project; adding a `.sln` would just add one more
file to keep in sync with the `.vcxproj` GUID. Opening the `.vcxproj`
directly creates an in-memory solution that behaves identically.

---

## 3. Signing Certificate

Every UWP `.appx` package must be signed. For sideloading onto a developer
Xbox, a self-signed test certificate is sufficient — Microsoft does not
need to trust it. The Xbox trusts whatever certificate you install on it
(see Section 8 for the trust step).

### 3.1 Create the certificate in Visual Studio

1. In Solution Explorer, double-click **Package.appxmanifest**.
   Visual Studio opens the manifest designer.
2. Switch to the **Packaging** tab.
3. Click **Choose Certificate...**.
4. In the "Choose Certificate" dialog, click **Create...**.
5. Fill in the fields:
   - **Certificate name**: e.g. `XboxWin32RunnerCert`
   - **Password**: choose a strong password and remember it
   - **Confirm password**: re-enter
6. Click **OK**. Visual Studio:
   - Generates a `.pfx` file at `uwp-shell/XboxWin32RunnerCert.pfx`.
   - Installs the certificate into your Current User → Trusted People
     store (this is required for the local machine to run the package
     during debugging).
   - Updates `Package.appxmanifest`'s `<Identity Publisher="...">` to
     match the certificate's subject name (e.g.
     `CN=XboxWin32RunnerCert`).
7. Click **OK** to close the "Choose Certificate" dialog.

### 3.2 What changed on disk

After step 6, two files appear in `uwp-shell/`:

- `XboxWin32RunnerCert.pfx` — the certificate + private key. **Do not
  commit this to a public repository.** The project's `.gitignore` should
  exclude `*.pfx`.
- `XboxWin32Runner_TemporaryKey.pfx` — older alias that some VS versions
  create. If you see both, the manifest references whichever one was
  selected in step 3 above; the other can be deleted.

### 3.3 Trust the certificate on the Xbox

The Xbox must trust the same certificate to accept the sideloaded package.
See Section 8.4 for the trust-install step.

### 3.4 Renewing the certificate

Test certificates are valid for one year by default. To renew:

1. Repeat Section 3.1 with a new certificate name
   (e.g. `XboxWin32RunnerCert2025`).
2. Re-install the new `.cer` (public key only) on the Xbox.
3. Uninstall the old `.appx` and deploy the new one.

The old `.appx` will refuse to launch once the old certificate expires.

---

## 4. C++/WinRT Setup

### 4.1 Preprocessor defines

The vcxproj already sets the following preprocessor definitions on every
translation unit:

```
_UNICODE;UNICODE;WINRT_ENABLED;XWR_UWP_BUILD;%(PreprocessorDefinitions)
```

To verify, right-click the project → **Properties** → **Configuration
Properties** → **C/C++** → **Preprocessor** → **Preprocessor Definitions**.
You should see the five entries above. If `WINRT_ENABLED` is missing, add
it manually — without it, the C++/WinRT projection headers fall back to
the ABI-only interface and many `winrt::` types fail to compile.

### 4.2 The cppwinrt.exe build step

The `Microsoft.Windows.CppWinRT` NuGet package injects an MSBuild target
that runs `cppwinrt.exe` at build time. Its output is a folder named
`Generated Files\` inside `uwp-shell\`, containing the projection headers
(`winrt/Windows.Foundation.h`, `winrt/Windows.UI.Core.h`, etc.).

If you do not see a `Generated Files\` folder after the first build, the
NuGet package was not restored. Right-click the project → **Restore NuGet
Packages**, then rebuild.

### 4.3 The forced-include header

Every `.cpp` file in the project gets `shims/CommonPre.h` force-included
via the vcxproj's `<ForcedIncludeFiles>` setting. CommonPre.h:

- Undefines `WIN32_LEAN_AND_MEAN` (we need the full Win32 surface).
- Defines `NOMINMAX` (so `std::min` / `std::max` work).
- Defines `_WIN32_WINNT`, `WINVER`, `NTDDI_VERSION` to `0x0A00`.
- Includes `<Windows.h>`.
- Includes `Win32Types.h` for constants the UWP SDK omits.

Do **not** add `#include <Windows.h>` again at the top of a `.cpp` file —
the force-include already pulled it in. Adding it twice is harmless (header
guards), but unnecessary.

---

## 5. Build for x64 (Test Compile)

Building the x64 desktop configuration verifies that the C++/WinRT
projection is set up, every source file compiles, and the link step
succeeds. It does not produce a runnable app on Xbox, but it catches the
vast majority of compile errors quickly.

1. In the Standard toolbar, set the configuration to **`Debug|x64`**.
2. From the menu: **Build** → **Build Solution** (or press `Ctrl+Shift+B`).
3. Watch the Output window. Expected result:

   ```
   1>------ Build started: Project: XboxWin32Runner, Configuration: Debug x64 ------
   1>Checking whether any NuGet packages need to be restored...
   1>Restoring NuGet packages...
   1>All packages are already installed and there is nothing to restore.
   ...
   1>XboxWin32Runner.vcxproj -> ...\x64\Debug\XboxWin32Runner.exe
   1>XboxWin32Runner.vcxproj : warning APPX0107: ...
   1>Done building project "XboxWin32Runner.vcxproj".
   ========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
   ```

4. Verify the outputs. The build writes:

   ```
   x64\Debug\XboxWin32Runner.exe
   x64\Debug\XboxWin32Runner.pdb
   x64\Debug\Appx\XboxWin32Runner_1.0.0.0_x64_Debug.appx
   x64\Debug\Appx\XboxWin32Runner_1.0.0.0_x64_Debug.appxsym
   ```

   The `.appx` is a sideload-ready package for the desktop (useful for
   debugging the UWP shell on the development PC).

### 5.1 If you see `g++` syntax-check errors in the Output window

You will not. The Linux syntax-check harness (`scripts/check_all.sh`) is
**not** invoked by Visual Studio. The MSVC build is the only build that
runs when you press `Ctrl+Shift+B`. The two checks are independent:

- MSVC (`cl.exe` + `link.exe`) builds the real binary for Windows / Xbox.
- `g++ -fsyntax-only` runs on Linux / WSL and only verifies that each
  `.cpp` file parses against the stub `winheaders/Windows.h`.

If you want to run the g++ check from a WSL terminal on the same machine:

```bash
cd /mnt/c/path/to/xbox-win32-runner
bash scripts/check_all.sh
```

You should see `XX ok, 0 failed` (the count grows as the project adds
more `.cpp` files; at the time of writing it is 34).

---

## 6. Build for Xbox

The `Xbox` platform target produces a package that the Xbox's UWP host
accepts. It differs from the x64 build in two ways:

1. The package identity includes the `Windows.Xbox` target device family.
2. The linker omits desktop-only imports (the Xbox runtime doesn't
   provide them).

### 6.1 Configure remote debugging

If you want to deploy and debug directly from Visual Studio (rather than
sideloading via the Device Portal), set up remote debugging:

1. On the Xbox: **Settings** → **Devices & connections** → **Developer** →
   enable **Device Portal** and note the displayed IP address (e.g.
   `192.168.1.42`) and port (default `11443`).
2. On the Xbox: in the same Developer settings page, enable **Remote
   Debugging**.
3. Back in Visual Studio: right-click the project → **Properties** →
   **Configuration Properties** → **Debugging**.
4. Set the following:
   - **Debugger to launch**: `Remote Debugger`
   - **Remote Server Name**: the Xbox's IP (e.g. `192.168.1.42`)
   - **Authentication Mode**: `Universal (Unencrypted Protocol)` (Xbox
     Dev Mode uses unencrypted remote debugging by default)
5. Click **OK**.

### 6.2 Switch configuration and build

1. Set the configuration drop-down to **`Release|Xbox`**.
2. Build → Build Solution (`Ctrl+Shift+B`).
3. Expected outputs:

   ```
   Xbox\Release\XboxWin32Runner.exe
   Xbox\Release\XboxWin32Runner.pdb
   Xbox\Release\Appx\XboxWin32Runner_1.0.0.0_x64.appx
   Xbox\Release\Appx\XboxWin32Runner_1.0.0.0_x64.appxsym
   ```

   (The `_x64` suffix is the architecture; the Xbox platform still
   produces an x64 binary because the Xbox Series X|S CPU is x86-64.)

4. If the build fails with `APPX1707` or `The .appxmanifest is invalid`,
   see Section 10.

### 6.3 Deploy via F5

If remote debugging is configured, pressing **F5** (Debug → Start
Debugging) will:

1. Build the project.
2. Deploy the `.appx` to the Xbox via the remote debugger channel.
3. Launch the app and attach the Visual Studio debugger.

For sideloading without the debugger attached, see Section 7 and Section 8.

---

## 7. Creating the .appx Package

Visual Studio can also produce a "store-ready" `.appx` (or `.msix`) for
manual sideloading — useful if you want to ship the package to another
developer or archive it.

1. Right-click the project in Solution Explorer → **Publish** →
   **Create App Packages**.
   (On older VS 2022 versions, the menu item is **Store** → **Create App
   Packages...**.)
2. In the wizard:
   - **Choose distribution method**: select **Sideload** (NOT "Microsoft
     Store"). The runner is not a Microsoft Store app.
3. Click **Next**.
4. On the "Signing" page:
   - Select **Use an existing certificate**.
   - Browse to the `.pfx` you created in Section 3.
   - Enter the password.
5. Click **Next**.
6. On the output location page:
   - **Output location**: leave as default
     (`uwp-shell\AppPackages\XboxWin32Runner\`).
   - **Version**: bump if desired (the manifest starts at `1.0.0.0`).
   - **Architectures**: tick **x64** only (no ARM, no x86).
7. Click **Create**. Visual Studio runs MakeAppx and produces:

   ```
   AppPackages/XboxWin32Runner/XboxWin32Runner_1.0.0.0_x64_Test/
       XboxWin32Runner_1.0.0.0_x64.appx
       XboxWin32Runner_1.0.0.0_x64.cer       (public key)
       XboxWin32Runner_1.0.0.0_x64.pfx       (private key, optional)
       Add-AppxDevPackage.bat                (one-click install script)
   ```

8. Note the path. Section 8 deploys from this folder.

### 7.1 What the .appx contains

The `.appx` is a ZIP archive containing:

- `XboxWin32Runner.exe` — the UWP host executable.
- `AppxManifest.xml` — the package manifest (validated against the SDK
  schema).
- `Assets\*.png` — the tile / splash / store logos (currently 1 KB
  placeholders; replace with real art before any public release).
- `resources.pri` — language resource index (auto-generated).
- `[Content_Types].xml` — MIME types for the package parts.
- `AppxBlockMap.xml` + `AppxSignature.p7x` — block hash + signature.

### 7.2 Adding game files to the .appx

The runner expects to read game files from a `C:\Game\` virtual drive that
maps to the UWP local folder — **not** from the package's install folder
(which is read-only on Xbox). You therefore do **not** bundle game files
into the `.appx`. Instead, you push them via the Device Portal's File
Explorer after deployment (Section 11.2).

If you do want to ship a small game bundled with the runner, mark the
files as `<Content Include="..." CopyToOutputDirectory="PreserveNewest" />`
in the vcxproj and add them to the package's `Package.appxmanifest`'s
`<uap:FileTypeAssociation>` if you want file-type activation. For the
Half Sword use case, this is **not** the recommended path — see Section
11.

---

## 8. Deploy via Xbox Device Portal

The Windows Device Portal (WDP) is the simplest deployment path. It runs
on the Xbox over HTTP and lets you drag-drop the `.appx` from a browser.

### 8.1 Enable Device Portal on the Xbox

1. On the Xbox, sign in with the developer account that has Developer Mode
   activated.
2. Open **Settings** → **Devices & connections** → **Developer**.
3. Tick **Developer Mode** (if not already on).
4. Tick **Enable Xbox Device Portal**.
5. Tick **Enable Remote Access to Windows Device Portal**.
6. Note the displayed URL — typically:

   ```
   https://<xbox-ip>:11443
   ```

   e.g. `https://192.168.1.42:11443`.

7. (Optional but recommended) Set a username and password under
   **Authentication**.

### 8.2 Install the certificate on the Xbox

Before the Xbox will accept the `.appx`, it must trust your self-signed
certificate. The easiest way is via the same Device Portal:

1. From your PC's browser, open `https://<xbox-ip>:11443` (accept the
   self-signed cert warning on first visit).
2. Authenticate with the username/password you set in step 7 above.
3. In the WDP UI, navigate to **Network** → **Certificates** (the menu
   item may be labelled **Certmgr** depending on WDP version).
4. Click **Install Certificate** and upload the
   `XboxWin32Runner_1.0.0.0_x64.cer` (the public key only — never upload
   the `.pfx`).
5. Wait for the confirmation toast.

### 8.3 Sideload the .appx

1. Still in the Device Portal, navigate to **Home** → **My games & apps**
   → **Apps**.
2. Either:
   - Drag and drop the `.appx` file onto the drop zone, OR
   - Click **+ Add** and browse to the `.appx`.
3. The WDP uploads the package (a few seconds for ~5 MB), validates the
   signature, installs, and registers the app.
4. On the Xbox home screen, scroll to **My games & apps** → **Apps**.
   You should see **Xbox Win32 Runner**.

### 8.4 Alternative: WinAppDeployCmd

From a Developer Command Prompt on the PC, you can sideload via the
command-line tool:

```cmd
cd C:\Windows\System32
WinAppDeployCmd.exe install -file "C:\path\to\XboxWin32Runner_1.0.0.0_x64.appx" ^
                            -ip   192.168.1.42 ^
                            -pin  <pin>
```

The `<pin>` is displayed on the Xbox's Dev Home screen when you select
"Pair" under the Device Portal settings. The PIN is single-use.

### 8.5 Alternative: PowerShell Add-AppxPackage

From an elevated PowerShell on the Xbox itself (Dev Home → open the
built-in PowerShell session):

```powershell
Add-AppxPackage -Path "D:\XboxWin32Runner_1.0.0.0_x64.appx"
```

This requires the `.appx` to be on a USB drive or copied into the Xbox's
local storage first.

### 8.6 Uninstalling

To remove a previously-installed copy:

- Via WDP: **Home** → **My games & apps** → **Apps** → find the tile →
  press the menu button (≡) → **Uninstall**.
- Via PowerShell on the Xbox:

  ```powershell
  Get-AppxPackage *XboxWin32Runner* | Remove-AppxPackage
  ```

---

## 9. Testing with test_hello.exe

The repository ships a tiny test binary at
`test/SampleGame/SyntheticGame.exe` (built by
`tools/make_synthetic_pe.py`). For real end-to-end smoke testing, you
should also build a one-off `test_hello.exe` that imports only
`kernel32!CreateFileW`, `kernel32!WriteFile`, and `kernel32!CloseHandle`,
and writes the string `Hello from test_hello!\n` to the standard output
handle. This section documents the workflow.

### 9.1 Build test_hello.exe

The simplest approach is to create a one-file C project on the PC:

```c
// test_hello.c
#include <windows.h>

int main(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    const char *msg = "Hello from test_hello!\n";
    DWORD written;
    WriteFile(h, msg, (DWORD)strlen(msg), &written, NULL);
    CloseHandle(h);
    return 0;
}
```

Compile with cl.exe (Developer Command Prompt for VS 2022):

```cmd
cl /nologo /Fe:test_hello.exe test_hello.c
```

The resulting `test_hello.exe` is a real x64 PE with three kernel32
imports — all of which the runner shims.

### 9.2 Wire it into run.config

Edit `uwp-shell/run.config`:

```
exe=C:\test\test_hello.exe
cwd=C:\test\
args=
dll_search_path=C:\test\
```

Rebuild the UWP package (`Release|Xbox`, Build Solution) and redeploy
(Section 8.3).

### 9.3 Run on the Xbox

1. Push `test_hello.exe` to the Xbox's local storage:
   - In WDP → **File Explorer** → navigate to
     `LocalAppData\XboxWin32Runner\LocalState\` → click **Browse** →
     upload `test_hello.exe`. The runner's PathTranslator maps
     `C:\test\` to `LocalState\test\`, so the upload must land inside
     a `test\` subfolder.
2. From the Xbox home screen, launch **Xbox Win32 Runner**.
3. The app starts, the PE loader parses `test_hello.exe`, resolves its
   three imports against the kernel32 shim, calls the entry point, and
   the `WriteFile` call goes to the real Xbox stdout (which is captured
   by the UWP host's ETW logger).
4. In WDP → **ETW Log** → click **Refresh**. You should see a log line
   resembling:

   ```
   [XboxWin32Runner] Hello from test_hello!
   ```

If you don't see the log line:

- Verify `run.config` is included in the `.appx` (it's listed as `<None
  Include="run.config" />` in the vcxproj; MakeAppx packages it
  alongside the exe).
- Verify the `exe=` path uses backslashes (`\`, not `/`).
- Verify `test_hello.exe` was uploaded to the correct subfolder
  (`LocalState\test\test_hello.exe`).

---

## 10. Common Compile Errors and Fixes

This section catalogues the errors that the development team has actually
hit while building the project, with the verified fix for each.

### 10.1 `C2011: 'GUID': 'struct' type redefinition`

**Cause**: `Win32Types.h` and `<guiddef.h>` both define `struct GUID`.

**Fix**:
- Ensure `Win32Types.h` has `#ifndef GUID` guards around its `typedef`.
- Do **not** include `<guiddef.h>` explicitly. `<Windows.h>` already
  pulls it in.
- If the error persists, search the codebase for `#include <guiddef.h>`
  and delete those lines.

### 10.2 `C2065: 'FARPROC': undeclared identifier`

**Cause**: `<Windows.h>` was not included before the shim function pointer
typedef.

**Fix**:
- Confirm that `shims/CommonPre.h` is force-included by the vcxproj
  (Project Properties → C/C++ → Advanced → Forced Include File =
  `CommonPre.h`).
- Alternatively, add `#include <Windows.h>` as the first line of the
  offending `.cpp` file. This is harmless because of the header guard.

### 10.3 `C2664: cannot convert from 'std::nullptr_t' to 'HMODULE'`

**Cause**: In our type system `HMODULE` is a `DWORD` (a 32-bit integer),
not a pointer. `nullptr` does not implicitly convert to integer types.

**Fix**:
- Replace `HMODULE m = nullptr;` with `HMODULE m = static_cast<HMODULE>(0);`
- Replace `return nullptr;` with `return static_cast<HMODULE>(0);`
- For comparison: `if (m == nullptr)` becomes `if (m == 0)` or
  `if (!m)`.

### 10.4 `LNK2019: unresolved external symbol D3D11CreateDevice`

**Cause**: `d3d11.lib` is not in the linker's additional dependencies.

**Fix**:
- Right-click the project → **Properties** → **Configuration Properties**
  → **Linker** → **Input** → **Additional Dependencies**.
- Confirm that `d3d11.lib` is listed. The full list the project requires
  is:

  ```
  kernel32.lib;user32.lib;gdi32.lib;advapi32.lib;shell32.lib;shlwapi.lib;
  ole32.lib;winmm.lib;version.lib;d3d11.lib;dxgi.lib;d2d1.lib;dwrite.lib;
  windowscodecs.lib;xinput.lib;xaudio2.lib;ws2_32.lib;crypt32.lib;
  bcrypt.lib;uuid.lib;%(AdditionalDependencies)
  ```

- The vcxproj already sets this; if it is missing locally, your local
  copy of the vcxproj has been edited. Restore from git.

### 10.5 `APPX1707: The .appxmanifest is invalid`

**Cause**: The XML namespaces in `Package.appxmanifest` don't match the
SDK version, or a referenced schema URL is incorrect.

**Fix**:
- Open `Package.appxmanifest` as XML (right-click → **View Code**).
- Confirm the `xmlns` declarations match:

  ```xml
  xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
  xmlns:mp="http://schemas.microsoft.com/appx/2014/phone/manifest"
  xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
  xmlns:uap3="http://schemas.microsoft.com/appx/manifest/uap/windows10/3"
  xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
  xmlns:xbox="http://schemas.microsoft.com/appx/manifest/xbox/windows10/5"
  IgnorableNamespaces="uap mp rescap uap3 xbox"
  ```

- Remove any `xmlns:` declarations you don't use. Unused namespaces
  trigger the validator.
- Ensure `<Identity Name="Zai.XboxWin32Runner" Publisher="CN=Zai"
  Version="1.0.0.0" />` matches the certificate subject name. If you
  changed the certificate in Section 3, the manifest's `Publisher`
  attribute must match the new CN=.

### 10.6 `LNK2038: mismatch detected for RuntimeLibrary`

**Cause**: One `.cpp` was compiled with `/MD` (multi-threaded DLL CRT)
while another was compiled with `/MDd` (multi-threaded debug DLL CRT), or
a third-party static lib was built with `/MT`.

**Fix**:
- Right-click the project → **Properties** → **Configuration Properties**
  → **C/C++** → **Code Generation** → **Runtime Library**.
- For `Debug` configurations: set to **Multi-threaded Debug DLL
  (`/MDd`)**.
- For `Release` configurations: set to **Multi-threaded DLL (`/MD`)**.
- Every `.cpp` in the project must use the same value. The vcxproj does
  not currently override this per-file, so it should be uniform; if a
  third-party static library forces `/MT`, recompile the library with
  `/MD` or exclude it from the build.

### 10.7 `Xbox platform not available`

**Cause**: The Xbox Live SDK and/or the Xbox Development Kit workload
extension is not installed in Visual Studio.

**Fix**:
- Open the Visual Studio Installer → **Individual components**.
- Search for **Xbox** and install:
  - **Xbox Live SDK**
  - **Xbox Development Kit (XDK) Workload Extension**
- Restart Visual Studio.
- If the workload is installed but the Xbox platform still does not
  appear, ensure the Xbox development tools are registered for the
  current user:

  ```cmd
  "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" /setup
  ```

  (Adjust `Community` to your edition.)

### 10.8 `error MSB8036: The Windows SDK version 10.0.22621.0 was not found`

**Cause**: The Windows SDK 10.0.22621.0 is not installed.

**Fix**:
- Open the Visual Studio Installer → **Individual components**.
- Search for **Windows 10 SDK 10.0.22621** and tick it.
- Click **Modify** to install.
- Re-open Visual Studio.

### 10.9 `cannot open include file 'winrt/Windows.Foundation.h'`

**Cause**: The `cppwinrt.exe` build step did not run, so the
`Generated Files\` folder was not created.

**Fix**:
- Right-click the project → **Restore NuGet Packages**.
- Confirm `packages.config` references
  `Microsoft.Windows.CppWinRT` version `2.0.230706.1`.
- Rebuild. If the `Generated Files\winrt\` folder still does not appear,
  run `cppwinrt.exe` manually:

  ```cmd
  cd uwp-shell
  packages\Microsoft.Windows.CppWinRT.2.0.230706.1\bin\cppwinrt.exe -input local -output Generated Files
  ```

### 10.10 `APPX0501: Validation error. error APPX0501: Element 'Identity'
has a 'ProcessorArchitecture' attribute that is invalid`

**Cause**: The package was built for an architecture the Xbox does not
support (ARM, x86, or `neutral`).

**Fix**:
- Build only for **x64**. In the **Create App Packages** wizard
  (Section 7), uncheck ARM and x86.
- If building from the command line, pass `/p:Platform=x64`.

---

## 11. Run Configuration

The runner reads its target `.exe`, working directory, command-line
arguments, and DLL search path from a single text file, `run.config`,
that ships inside the `.appx` package.

### 11.1 File format

`run.config` is line-oriented, `key=value`. Blank lines and lines
starting with `#` or `;` are ignored. Whitespace around `=` is trimmed.
The recognised keys are:

| Key              | Required | Description                                                  |
|------------------|----------|--------------------------------------------------------------|
| `exe`            | yes      | Absolute path to the game's main `.exe`, inside the virtual `C:\` drive. |
| `cwd`            | yes      | Working directory the game expects, inside the virtual `C:\` drive. |
| `args`           | no       | Command-line arguments appended after the exe name. Space-separated. |
| `dll_search_path`| no       | Semicolon-separated list of additional DLL search directories. |

### 11.2 Example: Half Sword

The shipped `uwp-shell/run.config` is already configured for Half Sword:

```
# Absolute path (inside the UWP virtual C: drive) of the .exe to launch.
exe=C:\Game\Binaries\Win64\HalfSword.exe

# Working directory the .exe expects (used for SetCurrentDirectoryW and as
# the base for relative file paths the game opens).
cwd=C:\Game\

# Command-line arguments appended after the .exe name on the command line
# passed to the PE entry point. Multiple args are space-separated.
args=-dx11 -windowed -resx=1920 -resy=1080

# ;-separated list of additional DLL search directories. The PE loader
# walks these in order when resolving import / delay-load dependencies.
dll_search_path=C:\Game\Binaries\Win64;C:\Game\Engine\Binaries\Win64
```

Notes:

- `-dx11` forces Unreal Engine to use the D3D11 RHI (the project's D3D11
  bridge is the rendering path; D3D12 is not yet wired into the runner's
  swap chain).
- `-windowed` and `-resx`/`-resy` keep the game in windowed mode at
  1080p. The D3D11 bridge creates a single HWND-less swap chain that the
  UWP host renders into; windowed mode avoids the fullscreen-exclusive
  handoff that the Xbox's UWP host does not support.
- `dll_search_path` must list every directory the game's loader will
  need. Unreal Engine ships its own DLLs in `Engine/Binaries/Win64/`;
  include that path even if the game's main `.exe` is in a sibling
  directory.

### 11.3 Pushing game files to the Xbox

The runner's `PathTranslator` maps the virtual `C:\Game\` to the UWP local
folder, which on the Xbox is:

```
LocalAppData\XboxWin32Runner\LocalState\
```

You push the game's install directory into that path via the WDP File
Explorer:

1. On the PC: open WDP → **File Explorer**.
2. Navigate to `LocalAppData\XboxWin32Runner\LocalState\`.
3. Click **Add Folder** (or **Add Directory**).
4. Select the Half Sword install folder from your PC.
5. WDP uploads the entire tree, preserving subdirectory structure. After
   upload, the Xbox's local folder looks like:

   ```
   LocalState\
       Game\
           Binaries\
               Win64\
                   HalfSword.exe
                   ...
           Engine\
               Binaries\
                   Win64\
                       *.dll
               Content\
                   *.pak
           ...
   ```

6. Re-launch the runner. The PE loader will find `HalfSword.exe` at the
   virtual path `C:\Game\Binaries\Win64\HalfSword.exe`.

### 11.4 Adjusting paths for non-Half-Sword games

For a different game, change `exe=`, `cwd=`, and `dll_search_path=` in
`run.config`, rebuild the `.appx`, and redeploy. The runner does not
support runtime configuration changes — the config is baked into the
package because the UWP sandbox makes the package's install folder
read-only.

### 11.5 Logging and diagnostics

The runner writes diagnostic output to the UWP ETW logger. To view it:

1. Open WDP → **ETW Log** (or **Event Viewer** in newer WDP versions).
2. Filter by source `XboxWin32Runner`.
3. Refresh after launch.

If the runner crashes before the message pump starts, the ETW log will
contain a stack trace. Common causes:

- `run.config` not found (the file must be inside the `.appx`; verify
  with `MakeAppx unpack` if needed).
- The target `.exe` does not exist at the virtual path (re-check the
  file upload in Section 11.3).
- The PE loader rejected the binary (see `PeLoader::LastError()` in the
  log).

---

## Appendix A: Project layout

```
xbox-win32-runner/
    pe-loader/             - PeLoader.{h,cpp}: in-process PE loader
    shims/                 - 23 shim modules + ShimRegistry
        CommonPre.h        - Forced-include header
        Win32Types.h       - Supplemental Win32 constants
        ShimRegistry.{h,cpp} - (dll, func) → FARPROC registry
        kernel32/          - Kernel32Shim.cpp, PathTranslator.{h,cpp}
        user32/            - User32Shim.cpp
        gdi32/             - Gdi32Shim.cpp
        advapi32/          - Advapi32Shim.cpp
        shell32/           - Shell32Shim.cpp
        shlwapi/           - ShlwapiShim.cpp
        ole32/             - Ole32Shim.cpp
        winmm/             - WinmmShim.cpp
        version/           - VersionShim.cpp
        pass-through/      - d3d11, d3d12, xinput, xaudio2, ws2_32, misc
        stubs/             - steam, discord, dsound, mfplat, misc
        legacy/            - d3d9, d3d8, ddraw, opengl32
    d3d11-bridge/          - D3D11Bridge.{h,cpp}
    bridges/               - XInput, Keyboard, Audio, Gdi bridges
    uwp-shell/             - main.cpp, App.{h,cpp}, pch.h, run.config,
                             Package.appxmanifest, vcxproj, filters,
                             packages.config, Assets/
    winheaders/            - Stub Windows.h for Linux syntax-check
    tools/                 - pe_import_scanner.py, test_pe_scanner.py,
                             make_synthetic_pe.py
    host-agent/            - host_agent.py (REST broker)
    scripts/               - check_all.sh (g++ syntax-check harness)
    test/                  - Sample synthetic PE files
    docs/                  - This documentation
```

## Appendix B: Build matrix

| Configuration | Platform | Output                                | Use case                                  |
|---------------|----------|---------------------------------------|-------------------------------------------|
| Debug         | x64      | `x64/Debug/XboxWin32Runner.exe`       | Desktop debugging of the UWP shell        |
| Release       | x64      | `x64/Release/XboxWin32Runner.exe`     | Desktop performance test                  |
| Debug         | Xbox     | `Xbox/Debug/XboxWin32Runner.exe`      | Xbox with the VS debugger attached        |
| Release       | Xbox     | `Xbox/Release/XboxWin32Runner.exe`    | Xbox production sideload (recommended)    |

Only the `Release|Xbox` configuration produces a `.appx` that is
recommended for sideloading to a non-development Xbox.

## Appendix C: Quick-reference command list

```cmd
REM Build for x64 desktop test
msbuild uwp-shell\XboxWin32Runner.vcxproj /p:Configuration=Debug /p:Platform=x64

REM Build for Xbox
msbuild uwp-shell\XboxWin32Runner.vcxproj /p:Configuration=Release /p:Platform=Xbox

REM Produce a sideload package (interactive wizard in VS)
REM Right-click project → Publish → Create App Packages → Sideload

REM Sideload from PC command line
WinAppDeployCmd.exe install -file XboxWin32Runner_1.0.0.0_x64.appx -ip <xbox-ip> -pin <pin>

REM Uninstall from the Xbox (PowerShell on the Xbox)
Get-AppxPackage *XboxWin32Runner* | Remove-AppxPackage

REM Run the Linux syntax-check from WSL
bash scripts/check_all.sh
```
