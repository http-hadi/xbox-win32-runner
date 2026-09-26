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


WebGPU / WEBNN (AI MODELS IN THE BROWSER) - new in 1.2.0.0
----------------------------------------------------------
WebGPU (and the WebNN feature names) are now enabled by default in the
Chromium engine. WebView2 does not have edge://flags / chrome://flags -
those internal pages only exist in full browsers - so the equivalent is
done by the app itself via browser launch arguments, set before the first
WebView2 environment is created (WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS).

What to expect per device:
- WINDOWS 10/11 DESKTOP: full hardware-accelerated WebGPU through the
  Evergreen WebView2 runtime (your PC's GPU). Flags used:
  --enable-unsafe-webgpu --enable-features=WebGPU,msWebNN,WebNNAPI
- XBOX DEV MODE: Chromium's WebGPU stack (Dawn) has no Xbox D3D12
  backend, so a hardware WebGPU adapter is impossible today. The app
  therefore ALSO passes --use-webgpu-adapter=swiftshader which guarantees
  a software (CPU) WebGPU adapter: sites like webml.ai/playground will
  detect WebGPU, load their "GPU shader" models and run them - on the
  CPU. Small models (tiny LLMs, Whisper-tiny, embeddings) are usable;
  big ones will be slow. Plus --ignore-gpu-blocklist and
  --enable-unsafe-swiftshader to try hardware compositing/WebGL first
  and fall back to software only if that fails.

CHANGING FLAGS WITHOUT A REBUILD (power users)
----------------------------------------------
If the file  browser-flags.txt  exists in the app's local state folder,
its entire content is used verbatim as the WebView2 browser arguments
(replacing the built-in defaults). On Xbox, browse to
http://<console-ip>:11443 (Device Portal) -> File explorer -> find the
Onitor app's LocalState folder -> create/edit browser-flags.txt, then
restart the app. Example contents to attempt hardware WebGPU instead of
SwiftShader:
    --enable-unsafe-webgpu --ignore-gpu-blocklist
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
