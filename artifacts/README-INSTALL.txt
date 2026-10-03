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


WebGPU / WEBNN - CURRENT STATE (1.6.1.0)
----------------------------------------
HARDWARE WEBGPU IS WORKING on Xbox dev-mode consoles: green badge,
adapter 'microsoft', Dawn's native D3D12 backend on the real console
GPU, hardware WebGL. This is the result of the 1.6.0.0 rebuild (full
story below). If your badge is green: nothing to do, enjoy the GPU.

ONE KNOWN LIMITATION - COMPUTE PIPELINES (honest status):
Rendering pipelines, textures, buffers and canvas rendering all run on
the real GPU. WebGPU COMPUTE dispatches, however, crash the console's
UWP D3D12 driver with DXGI_ERROR_DRIVER_INTERNAL_ERROR (device removed;
Chromium then restarts its GPU process and rendering continues). This
is deterministic across sessions (verified 2026-09-28: on
webgpucheck.com every render test passes and the GPU is listed
natively; the compute-pipeline test reports the mapAsync exception).
The crash is inside the precompiled WebView2 runtime + console driver,
i.e. below anything app flags control. Tested and ruled out so far:
  * --disable-dawn-features=use_dxc (force the old FXC compiler):
    kills the WHOLE GPU stack on the console - WebGPU and WebGL both
    become unavailable (the FXC/d3dcompiler path cannot initialize
    inside the UWP container). Dead end, do not use.
  * --enable-dawn-features=d3d12_dont_use_shader_model_66_or_higher:
    compute still crashes - the shader model is not the trigger.
Remaining untested candidates (root-signature 1.0, workgroup-access
decomposition, HLSL 2018 codegen) can be tried with one paste - see
the "Compute-crash experiment" recipe in the flags section below.

XBOX DEV MODE - automatic hardware ladder (1.6.1.0 order, gen5):
  1) Dawn D3D12 backend (Chromium's Windows default, no
     --use-webgpu-adapter switch, blocklist bypassed) - the console's
     native GPU API. Works: green badge.
  2) SwiftShader software adapter - guaranteed last resort.
The 1.6.0.0 d3d11 and gles rungs were PROVEN to always end on
SwiftShader on Xbox (the console's D3D11 is the D3D11On12 layer -
Dawn's D3D11 backend cannot create a device on it; the OpenGLES
backend is compiled out of the win-UWP WebView2 runtime) and were
removed from the auto-ladder. Their pin values still work for
diagnostics and now say so in webgpu-status.txt.

WebGPU / WEBNN (AI MODELS IN THE BROWSER) - rebuilt in 1.6.0.0
----------------------------------------------------------
WebGPU (and the WebNN feature names) are enabled by default in the
Chromium engine. WebView2 does not have edge://flags / chrome://flags -
those internal pages only exist in full browsers - so the equivalent is
done by the app itself via browser launch arguments, set before the first
WebView2 environment is created (WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS).

THE 1.6.0.0 REBUILD - why every previous version ended on SwiftShader:
Your 1.5.1.0 debug upload (webgpu-status.txt + chromium.log) allowed the
WHOLE fallback chain to be verified against the exact engine source on
your console (Chromium 150.0.7871.212, Dawn pin d089fc91). Three facts
fell out of it:
1. The GLES rung could never work: Dawn's build config
   (dawn_features.gni, "Disables OpenGLES when compiling for UWP")
   compiles the OpenGLES backend OUT of the Windows-UWP WebView2 runtime
   your Xbox uses. Enumerating GLES adapters returns nothing, and
   Chromium's WebGPU decoder then silently falls through to its tail
   fallback: Vulkan + forceFallbackAdapter = SwiftShader. That is the
   'swiftshader google' adapter every probe saw.
2. The D3D11/D3D12 rungs never actually ran with a working probe: the
   1.5.1.0 ladder skipped them entirely (they were written off based on
   the same fallback-forcing that the flags below now bypass), and
   before that the broken probe froze the ladder before reaching them.
3. The forcing itself is avoidable: --ignore-gpu-blocklist skips the
   entire GPU blocklist in the BROWSER process (gpu_util.cc), so
   ACCELERATED_WEBGPU stays "Enabled" instead of "software" and the
   force-to-SwiftShader override never engages. --enable-unsafe-webgpu
   (already present since 1.3.0.0) additionally disables the adapter-
   level blocklist. Both switches reach the GPU process inside the
   serialized --gpu-preferences blob, immune to GPU-process switch
   filtering. With the forcing gone, Chromium's Windows DEFAULT adapter
   request is a REAL hardware enumeration on the D3D12 backend - the
   console's native GPU API, zero translation layers.

What this means for YOUR console after installing 1.6.0.0:
- First launch: rung 1 = Dawn D3D12, native console GPU, no forced
  fallback (first time this configuration has EVER run - it was skipped
  by every previous version).
- Green badge -> real console-GPU WebGPU, done.
- Amber -> one restart onto rung 2 (Dawn D3D11, the same D3D11 layer
  your WebGL already uses), then rung 3 (GLES, diagnostics only), then
  the guaranteed SwiftShader rung.
- chromium.log now records INFO-level messages too (--log-level=0), so
  if everything still lands on SwiftShader the log will contain the
  Dawn adapter-enumeration lines that say exactly why - send
  webgpu-status.txt + chromium.log again.

History - the critical 1.5.1.0 probe fix (kept for reference):
The app verifies at startup which adapter the browser actually got by
running a small script that calls navigator.gpu.requestAdapter(). Up to
1.5.0.0 that script returned its answer as a JavaScript Promise - but
WebView2's ExecuteScriptAsync does NOT wait for Promises; it returns the
JSON serialization of the Promise object itself, the literal string "{}"
(Microsoft's documented behaviour, WebView2Feedback #2295). "{}" matched
none of the probe's outcome branches, so the whole hardware ladder was
SILENTLY FROZEN on its first rung (D3D11) since 1.3.0.0 - the OpenGLES
rung added in 1.5.0.0 was never actually attempted (the tell-tale line
in webgpu-status.txt was "probe result={} mode=auto-d3d11"). 1.5.1.0
replaces the mechanism: the script stores its verdict in a page global
as a plain string and the app polls for it - no Promise crosses the
boundary, any unexpected value is logged instead of ignored, and every
probe line now also records the browser's Chromium version.

What this means for YOUR console after installing 1.5.1.0 (superseded by
1.6.0.0 above - kept for reference):
- The very first launch runs the OpenGLES rung (--use-webgpu-adapter=
  opengles, routed through ANGLE's hardware D3D11 device - the same
  device your WebGL already uses, as the badge's "WebGL: hardware"
  second line proves).
- Green badge right away -> real console-GPU WebGPU, done.
- If GLES cannot deliver a hardware adapter, the app advances to the
  guaranteed SwiftShader rung and restarts itself ONCE. From then on
  WebGPU pages work on the CPU as before.
- webgpu-status.txt now contains the REAL per-rung verdicts (adapter
  names!) and chromium.log the Dawn backend errors - if the badge is
  still amber, send both files again: they now say exactly WHY.

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
- XBOX DEV MODE - automatic hardware ladder (1.6.1.0 order): the app
  tries REAL-GPU backends until one works:
    1) Dawn D3D12 backend (no --use-webgpu-adapter switch = Chromium's
       Windows default) - the console's NATIVE GPU API, zero translation
       layers, run with the blocklist bypass so no forced fallback can
       engage. First time this configuration has ever run on the console
       (previous versions skipped it / their probe was broken).
    2) SwiftShader software adapter (--use-webgpu-adapter=swiftshader)
       - guaranteed last resort: models load and run on the CPU.
  (1.6.0.0 briefly had d3d11 and gles as rungs 2-3; both were proven to
  always end on SwiftShader on Xbox - see the 1.6.1.0 notes above.)
  At startup the app asks the browser for an adapter (the fixed
  kick+poll probe); while a hardware rung does not deliver a hardware
  adapter it advances the ladder and restarts itself once per rung
  (result remembered in webgpu-mode.txt). Consoles coming from older
  versions restart from rung 1 - the old D3D11/D3D12 rungs never ran
  with a working probe, so nothing already-failed is re-run.
  HONEST DETECTION (1.5.0.0): an adapter is counted as software when
  its name/description says SwiftShader/llvmpipe/Basic Render/WARP,
  not only when the fallback flag is set. v1.4.0.0 relied on that flag
  alone - and Chromium reports isFallback=false for an explicitly
  requested SwiftShader adapter - which is why v1.4.0.0 showed a
  green "WebGPU: hardware, adapter: SwiftShader" badge while
  webgpucheck.com correctly reported the CPU fallback.

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
the file to "d3d11", "d3d12" or "gles"; to stay on SwiftShader use
"auto-cpu".

CHANGING FLAGS WITHOUT A REBUILD (power users)
----------------------------------------------
If the file  browser-flags.txt  exists in the app's local state folder,
its entire content is used as the WebView2 browser arguments
(replacing the built-in defaults). On Xbox, browse to
http://<console-ip>:11443 (Device Portal) -> File explorer -> find the
Onitor app's LocalState folder -> create/edit browser-flags.txt, then
restart the app.

1.6.1.0 improvements to override sessions:
- chromium.log IS captured (the logging flags are appended to your
  line automatically) - in 1.6.0.0 override sessions logged nothing.
- webgpu-status.txt records a "browser-flags.txt override ACTIVE"
  line each launch, so experiments are always traceable.
- Pasted/wrapped lines are whitespace-normalized automatically (raw
  line breaks used to silently break flag parsing).
Useful recipes:

  Hardware WebGPU, D3D12 native (the default rung 1 - no
  --use-webgpu-adapter switch, Chromium's Windows default backend,
  blocklist bypassed so no forced SwiftShader fallback):
    --enable-unsafe-webgpu --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader
  Compute-crash experiment (ALL remaining Dawn compute-path toggles
  at once: shader model capped below 6.6, workgroup-access
  decomposition ON, root signature forced to 1.0, HLSL 2018 codegen).
  If the webgpucheck.com compute test PASSES with this line, tell us -
  the winning toggle gets baked into the next build:
    --enable-unsafe-webgpu --enable-features=WebGPU,msWebNN,WebNNAPI --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader --enable-dawn-features=d3d12_dont_use_shader_model_66_or_higher,d3d12_decompose_workgroup_access --disable-dawn-features=d3d12_use_root_signature_version_1_1,d3d12_use_hlsl_2021
  Hardware WebGPU, D3D11 backend (known SwiftShader trap on Xbox -
  diagnostics only, console D3D11 is the 11on12 layer):
    --enable-unsafe-webgpu --use-webgpu-adapter=d3d11 --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader
  Guaranteed software WebGPU (old 1.2.0.0 behaviour):
    --enable-unsafe-webgpu --use-webgpu-adapter=swiftshader --enable-unsafe-swiftshader
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
- NEW 1.5.0.0:
  * New ladder rung 3: Dawn OpenGLES backend through ANGLE's hardware
    D3D11 device (--use-webgpu-adapter=opengles). Key insight from
    Chromium source: kOpenGLES is the ONLY adapter type exempt from
    the force-SwiftShader-fallback override that kicks in whenever the
    GPU feature list marks WebGPU as software - which it does for the
    unknown Xbox GPU. This makes GLES the primary real-GPU candidate
    on the console.
  * HONEST software detection: adapters are now also classified as
    software by NAME (SwiftShader/llvmpipe/Basic Render/WARP), fixing
    the v1.4.0.0 green "hardware, adapter: SwiftShader" badge (an
    explicitly forced SwiftShader adapter reports isFallback=false).
  * Ladder generations (webgpu-ladder-gen.txt): consoles that already
    settled on SwiftShader resume directly at newly added rungs
    instead of re-running already-failed ones.
  * Mode file gains "auto-gles" rung value and "gles" pin value.
- NEW 1.5.1.0 (critical):
  * STARTUP PROBE FIXED. The probe used to return its adapter verdict
    as a JavaScript Promise, but WebView2's ExecuteScriptAsync does not
    await Promises - it returned the literal string "{}", which matched
    no outcome branch, silently freezing the hardware ladder on its
    first rung (D3D11) in every version since 1.3.0.0. The OpenGLES
    rung added in 1.5.0.0 was therefore never actually attempted. The
    probe now uses a kick+poll pattern (verdict stored as a plain
    string in a page global, polled by the host), unexpected values are
    logged instead of ignored, and every probe line records the
    browser's Chromium version.
  * Fresh and upgraded consoles start DIRECTLY at the OpenGLES rung
    (D3D11/D3D12 are force-replaced by SwiftShader on Xbox anyway;
    user pins always survive).
  * Result: first launch after updating = the real GLES hardware
    attempt. Green badge -> done. Amber -> one automatic restart onto
    guaranteed SwiftShader, and webgpu-status.txt/chromium.log now
    contain the actual reason.
- NEW 1.6.0.0 (root cause found + hardware ladder rebuilt):
  * Full fallback chain verified against the exact engine source running
    on the console (Chromium 150.0.7871.212, Dawn d089fc91). Root cause
    of the permanent SwiftShader: every rung's preferred backend found
    nothing and fell through to the decoder's tail fallback
    (Vulkan + forceFallbackAdapter = SwiftShader, the only Vulkan
    adapter on the win-UWP WebView2 runtime).
  * The OpenGLES rung is dead BY BUILD on Xbox: Dawn's dawn_features.gni
    compiles the OpenGLES backend out of Windows-UWP WebView2 runtimes.
    Demoted to a diagnostics-only rung.
  * New rung order (gen4): D3D12 (native, default backend, blocklist
    bypassed so the forced fallback never engages) -> D3D11 (the D3D11
    layer hardware WebGL already uses) -> GLES (diagnostics) -> CPU.
    The D3D12/D3D11 rungs never ran with a working probe in any older
    version, so every console - including ones settled on auto-cpu -
    restarts the ladder from rung 1.
  * chromium.log upgraded to INFO level (--log-level=0): if a rung still
    fails, the log now contains the Dawn adapter-enumeration lines that
    say exactly why.
  * Mode file unchanged: "auto" restarts the ladder at rung 1, pins
    ("d3d11"/"d3d12"/"gles") and "auto-cpu" behave as before.
- NEW 1.6.1.0 (instrumentation + honest compute status):
  * COMPUTE PIPELINE LIMITATION documented (see the 1.6.1.0 section at
    the top): render WebGPU is hardware; compute dispatches crash the
    console's UWP D3D12 driver (DXGI_ERROR_DRIVER_INTERNAL_ERROR). The
    two Dawn compiler levers were tested via browser-flags.txt and
    ruled out: use_dxc off kills the whole GPU stack (FXC cannot
    initialize in the UWP container), the shader-model-6.6 cap does
    not stop the crash.
  * browser-flags.txt override sessions now capture chromium.log and
    write a "browser-flags.txt override ACTIVE" line to
    webgpu-status.txt (in 1.6.0.0 they were a forensic blind spot:
    no log, no status trace), and pasted flags are whitespace-
    normalized so wrapped lines cannot silently break parsing.
  * Ladder collapsed to gen5 (d3d12 -> cpu): the d3d11 rung always
    ended on SwiftShader on Xbox (console D3D11 = D3D11On12 layer,
    Dawn D3D11 cannot create a device on it - reproduced twice) and
    the gles backend is compiled out of the runtime; both rungs only
    ever burned two restarts on the way to the same result. Pinning
    "d3d11"/"gles" still works for diagnostics and webgpu-status.txt
    now explains the trap when a pinned rung falls back.
