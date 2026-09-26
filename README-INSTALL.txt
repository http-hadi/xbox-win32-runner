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
done by the app itself via browser launch arguments
(--enable-unsafe-webgpu --enable-features=WebGPU,msWebNN,WebNNAPI).
Sites like webml.ai/playground, webllm.ai or any WebGPU demo should now
detect navigator.gpu and load their "GPU shader" models.

What to expect per device:
- WINDOWS 10/11 DESKTOP: full hardware-accelerated WebGPU through the
  Evergreen WebView2 runtime (your PC's GPU).
- XBOX DEV MODE: Microsoft's WebGPU stack (Dawn) does not provide hardware
  D3D12 adapters on Xbox, so Chromium falls back to a software (SwiftShader)
  adapter. WebGPU apps and AI model loaders will WORK, but inference runs
  on the CPU - expect small models to load and respond slowly rather than
  at GPU speed. This is a platform limitation of WebView2 on Xbox, not
  something the app can switch on.
  Quick check: open https://webgpu.github.io/webgpu-samples/ - samples
  should render (software-rendered on Xbox).


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
