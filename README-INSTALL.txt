Onitor Browser (WebView2 / Chromium dual-engine) - Xbox Dev Mode & Desktop sideload build
=========================================================================================

WHAT'S IN THIS PACKAGE
----------------------
- OnitorBrowser_<version>_x64_Test\   Sideload package folder containing:
    * Onitor_<version>_x64.msixbundle (or .appxbundle)  <- the app itself
    * Dependencies\x64\...            <- framework packages the app needs
    * OnitorBrowser_TemporaryKey.cer  <- signing certificate (public part)
    * Add-AppDevPackage.ps1           <- one-click installer for Windows 10/11 desktop
- OnitorDev.pfx / OnitorDev.cer       <- dev signing cert (password: onitor-dev-2026)
- README-INSTALL.txt                  <- this file

This is a self-signed DEV build. It is NOT store-signed. Windows and Xbox
must be in Developer Mode to install it.


INSTALL ON XBOX ONE / SERIES X|S (DEV MODE)
-------------------------------------------
1. Put the whole package on a USB drive (exFAT/NTFS) or a network share
   reachable from the console.
2. On the console: Dev Home -> My games & apps -> "Add" ->
   navigate to the USB drive.
3. Install the DEPENDENCIES FIRST, one at a time:
       Dependencies\x64\Microsoft.VCLibs.x64.14.00.appx
       Dependencies\x64\Microsoft.UI.Xaml_2.8.7_x64.appx   (and any other
       .appx/.msix files present in Dependencies\x64)
4. Then install the main bundle:
       Onitor_<version>_x64.msixbundle
5. Launch "Onitor Browser" from My games & apps.

Notes for Xbox:
- ENGINE: On Xbox the app tries the Chromium (WebView2) engine first; if the
  console provides no WebView2 runtime it automatically falls back to the
  legacy EdgeHTML engine with YouTube TV compatibility mode (youtube.com/tv
  + H.264 forcing). On Windows 10/11 desktop it uses the Evergreen WebView2
  runtime and youtube.com loads as on a normal Chromium browser.
- INPUT: Mouse emulation is enabled automatically (right stick / cursor).
- If video playback stutters on Xbox, open Settings in the app and set
  Engine Mode = Legacy to use the TV-optimized YouTube interface.


WebGPU / WEBNN (AI MODELS IN THE BROWSER) - improved in 1.4.0.0
----------------------------------------------------------
WebGPU (and the WebNN feature names) are enabled by default in the
Chromium engine. WebView2 does not have edge://flags / chrome://flags -
those internal pages only exist in full browsers - so the equivalent is
done by the app itself via browser launch arguments, set before the first
WebView2 environment is created (WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS).

STATUS BADGE (new in 1.4.0.0): every page now shows a small pill in the
TOP-RIGHT corner telling you at a glance how WebGPU is doing:
  green  "WebGPU: hardware"        - running on the real console/PC GPU
  amber  "WebGPU: software - SwiftShader" - CPU fallback active
  red    "WebGPU: unavailable" / "not enabled"
The dim second line shows the active adapter name and whether WebGL is
hardware or software (a good sanity check for the whole GPU stack).
If a video goes fullscreen the badge hides itself so it never covers
your video.

What to expect per device:
- WINDOWS 10/11 DESKTOP: full hardware-accelerated WebGPU through the
  Evergreen WebView2 runtime (your PC's GPU, D3D12). Flags used:
  --enable-unsafe-webgpu --enable-features=WebGPU,msWebNN,WebNNAPI
- XBOX DEV MODE - automatic hardware ladder (new in 1.4.0.0): the app
  tries REAL-GPU backends in order until one works:
    1) Dawn D3D11 backend (--use-webgpu-adapter=d3d11) - D3D11 is the
       one 3D API fully available to UWP apps on the console (the same
       API the WebGL/ANGLE layer uses), so this is the most promising
       hardware path.
    2) Dawn D3D12 backend (Chromium's Windows default) - historically
       no adapters on Xbox, but newer runtimes / Series X|S may expose
       it, so it gets a chance.
    3) SwiftShader software adapter (--use-webgpu-adapter=swiftshader)
       - guaranteed last resort: models load and run on the CPU.
  At startup the app asks the browser for an adapter; while a hardware
  rung does not deliver a hardware adapter it advances the ladder and
  restarts itself once per rung (max 2 restarts, one time ever - the
  result is remembered in webgpu-mode.txt).
  NOTE: v1.3.0.0 misread the SwiftShader fallback adapter as "hardware"
  (Chromium renamed isFallbackAdapter -> isFallback); 1.4.0.0 checks
  both, which is why sites like webgpucheck.com may now honestly show
  SwiftShader where 1.3.0.0 claimed hardware.

Every launch is logged to  webgpu-status.txt  in the app's LocalState
folder (Xbox Device Portal -> File explorer), telling you exactly which
adapter the browser got, e.g. "Hardware WebGPU adapter ACTIVE" or
"probe result=fallback adapter='SwiftShader DLL'". Chromium's own
diagnostics additionally go to  chromium.log  in the same folder - if
a hardware rung fails, that file contains the Dawn/adapter error.

WebGPU sites to try:
  https://webgpucheck.com/               (quick status check - should
                                          match the badge)
  https://webml.ai/playground          (AI models in the browser)
  https://webgpu.github.io/webgpu-samples/  (samples - the "Hardware Adapter"
     sample shows which adapter is active)

To retry the hardware ladder from scratch: delete webgpu-mode.txt (or
set its content to "auto") in the app's LocalState folder via Device
Portal, then restart the app. To pin a specific backend forever, set
the file to "d3d11" or "d3d12"; to stay on SwiftShader use "auto-cpu".

CHANGING FLAGS WITHOUT A REBUILD (power users)
----------------------------------------------
If the file  browser-flags.txt  exists in the app's local state folder,
its entire content is used verbatim as the WebView2 browser arguments
(replacing the built-in defaults). On Xbox, browse to
http://<console-ip>:11443 (Device Portal) -> File explorer -> find the
Onitor app's LocalState folder -> create/edit browser-flags.txt, then
restart the app. Useful recipes:

  Hardware WebGPU, ladder rung 1 (the new default):
    --enable-unsafe-webgpu --use-webgpu-adapter=d3d11 --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader
  Hardware WebGPU, ladder rung 2 (Chromium Windows default backend):
    --enable-unsafe-webgpu --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader
  Guaranteed software WebGPU (old 1.2.0.0 behaviour):
    --enable-unsafe-webgpu --use-webgpu-adapter=swiftshader --enable-unsafe-swiftshader
  Force WebGPU compatibility profile (lighter feature set, more
  adapters pass validation):
    --enable-unsafe-webgpu --use-webgpu-adapter=d3d11 --force-webgpu-compat --use-angle=d3d11 --ignore-gpu-blocklist
  WebGPU + WebNN feature names:
    --enable-unsafe-webgpu --enable-features=WebGPU,msWebNN,WebNNAPI --use-webgpu-adapter=d3d11 --ignore-gpu-blocklist
Delete the file (or empty it) to return to the built-in defaults.


MORE CPU POWER ON XBOX (new in 1.2.0.0)
---------------------------------------
The package now declares the restricted capability "expandedResources"
(same trick RetroArch uses). Plain UWP apps on Xbox run in a small
shared partition (limited CPU share + very little memory); apps with
this capability get GAME-level resources: 4 exclusive + 2 shared CPU
cores and gigabytes more memory. That is the biggest possible CPU/RAM
budget for the browser and its WebGPU-on-CPU inference.
Caveat: one community report saw WebView2 hard-crash an XBOX ONE when
running with game resources (Series X|S were fine, Series S OOM'd less
than in app mode). If you are on an Xbox One and the console reboots
when launching the browser, tell us and we ship a build without the
capability.

Why the console still won't match a gaming PC: the Xbox Series X|S CPU
is 8x Zen 2 cores at 3.6-3.8 GHz - great multi-core throughput, but
single-core performance is around half of a modern laptop CPU (which is
why benchmark sites rate it below a new Dell Inspiron). Browsing and
video feel fine because that is mostly multi-core work; CPU-based AI
inference is the most demanding thing you can throw at it.


INSTALL ON WINDOWS 10/11 DESKTOP / TABLET
----------------------------------------
1. Copy the OnitorBrowser_<version>_x64_Test folder to the PC.
2. Enable Developer Mode: Settings -> Privacy & security -> For developers
   -> Developer Mode ON.
3. Right-click Add-AppDevPackage.ps1 -> "Run with PowerShell".
   (This installs the certificate and all dependencies automatically.)
   If prompted, allow the cert install.
4. Launch "Onitor Browser" from the Start menu.

Manual desktop install alternative (if the script fails):
- certutil -addstore TrustedPeople OnitorBrowser_TemporaryKey.cer   (admin)
- Then double-click the .msixbundle and click Install.


CERTIFICATE (for reference)
---------------------------
Self-signed, subject CN=Empyreal96 (matches the app identity publisher),
code-signing EKU, RSA-2048/SHA256.
PFX password: onitor-dev-2026
Only needed if you want to re-sign or build the app yourself.


WHAT WAS CHANGED vs original Onitor
-----------------------------------
- Dual-engine browser core: Chromium WebView2 (Microsoft.UI.Xaml 2.8.7 /
  Microsoft.Web.WebView2) when the runtime is available, automatic fallback
  to the legacy EdgeHTML WebView engine otherwise.
- YouTube compatibility layer for the legacy engine: youtube.com/tv
  leanback UI, TV user-agent, H.264 (h264ify-style) forcing.
- Fixed 'lockdown' ReferenceError in the XHR ad-blocker that killed
  YouTube's innertube API.
- Unblocked imasdk.googleapis.com (YouTube IMA SDK) required by the player.
- Fixed early-return bug in WhitelistedPages.
- Xbox mouse emulation enabled (RequiresPointerMode).
- TargetPlatformMinVersion raised to 10.0.17763 (required by WebView2).
- NEW 1.2.0.0: WebGPU enabled in the Chromium engine via
  WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS (no edge://flags in WebView2),
  plus WebNN feature names for Edge/Chromium runtimes.
- NEW 1.3.0.0: HARDWARE WebGPU attempt on Xbox via Dawn's D3D11 backend
  (--use-webgpu-adapter=d3d11, parsed by Chromium's own
  service_utils.cc) with adaptive startup probe: if no GPU adapter is
  available the app auto-falls back to a guaranteed SwiftShader adapter
  (webgpu-mode.txt) and restarts once. All decisions logged to
  webgpu-status.txt. Desktop hardware WebGPU unchanged (D3D12).
- NEW 1.4.0.0:
  * On-page WebGPU STATUS BADGE (top-right pill, every page): green =
    hardware, amber = SwiftShader software, red = unavailable, plus
    adapter name + WebGL hardware/software as a dim second line.
  * Automatic HARDWARE LADDER on Xbox: D3D11 backend -> D3D12 backend
    -> SwiftShader, advancing with one restart per rung until a real
    GPU adapter is found (result remembered in webgpu-mode.txt).
  * Fixed v1.3.0.0 misdetection: Chromium renamed
    GPUAdapter.isFallbackAdapter -> isFallback; the probe now checks
    both, so a SwiftShader fallback adapter is no longer reported as
    hardware (why webgpucheck.com showed SwiftShader before).
  * Chromium diagnostics now go to LocalState\chromium.log
    (--enable-logging --log-file) for Dawn backend error analysis.
