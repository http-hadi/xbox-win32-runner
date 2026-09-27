using System;
using System.Diagnostics;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.Web;
using Mux = Microsoft.UI.Xaml.Controls;
using Core = Microsoft.Web.WebView2.Core;

namespace onitor.Classes
{
    internal interface IBrowserEngine
    {
        FrameworkElement View { get; }
        bool IsChromium { get; }

        Uri Source { get; set; }
        string DocumentTitle { get; }
        bool CanGoBack { get; }
        bool CanGoForward { get; }
        bool ContainsFullScreenElement { get; }
        BrowserSettings Settings { get; }

        void Navigate(Uri source);
        void GoBack();
        void GoForward();
        void Refresh();
        void Stop();
        void AddWebAllowedObject(string name, object pObject);
        void SetUserAgent(string userAgent);
        Task<string> InvokeScriptAsync(string scriptName, string[] arguments);

        event Action<EngineNavStartingArgs> NavigationStarting;
        event Action<EngineNavCompletedArgs> NavigationCompleted;
        event Action<EngineContentLoadingArgs> ContentLoading;
        event Action<EngineNavStartingArgs> FrameNavigationStarting;
        event Action<EngineNavCompletedArgs> FrameNavigationCompleted;
        event Action<EngineUriArgs> DOMContentLoaded;
        event Action<EngineUriArgs> FrameDOMContentLoaded;
        event Action<EngineNewWindowArgs> NewWindowRequested;
        event Action<EnginePermissionArgs> PermissionRequested;
        event Action<EngineMessageArgs> ScriptNotify;
        event Action<EngineUriArgs> UnviewableContentIdentified;
        event Action<EngineLongRunningScriptArgs> LongRunningScriptDetected;
        event Action<object> ContainsFullScreenElementChanged;
        event Action<string> EngineFailed;
    }

    // ============================================================================
    //  Legacy engine - the original EdgeHTML Windows.UI.Xaml.Controls.WebView.
    //  Kept as the fallback for systems where the WebView2 runtime is not
    //  available (pre-1809 desktop, Windows 10 Mobile) and for the
    //  EngineMode == "Legacy" escape hatch.
    // ============================================================================

    internal sealed class LegacyEngine : IBrowserEngine
    {
        private readonly WebView _wv;

        public LegacyEngine()
        {
            _wv = new WebView(WebViewExecutionMode.SeparateThread);

            _wv.NavigationStarting += (s, e) =>
            {
                var a = new EngineNavStartingArgs { Uri = e.Uri, Cancel = e.Cancel };
                var h = NavigationStarting; if (h != null) h(a);
                e.Cancel = a.Cancel;
            };

            _wv.NavigationCompleted += (s, e) =>
            {
                var h = NavigationCompleted; if (h != null) h(new EngineNavCompletedArgs
                {
                    Uri = e.Uri,
                    IsSuccess = e.IsSuccess,
                    WebErrorStatus = e.WebErrorStatus
                });
            };

            _wv.ContentLoading += (s, e) =>
            {
                var h = ContentLoading; if (h != null) h(new EngineContentLoadingArgs { Uri = e.Uri });
            };

            _wv.FrameNavigationStarting += (s, e) =>
            {
                var a = new EngineNavStartingArgs { Uri = e.Uri, Cancel = e.Cancel };
                var h = FrameNavigationStarting; if (h != null) h(a);
                e.Cancel = a.Cancel;
            };

            _wv.FrameNavigationCompleted += (s, e) =>
            {
                var h = FrameNavigationCompleted; if (h != null) h(new EngineNavCompletedArgs
                {
                    Uri = e.Uri,
                    IsSuccess = e.IsSuccess,
                    WebErrorStatus = e.WebErrorStatus
                });
            };

            _wv.DOMContentLoaded += (s, e) =>
            {
                var h = DOMContentLoaded; if (h != null) h(new EngineUriArgs { Uri = e.Uri });
            };

            _wv.FrameDOMContentLoaded += (s, e) =>
            {
                var h = FrameDOMContentLoaded; if (h != null) h(new EngineUriArgs { Uri = e.Uri });
            };

            _wv.NewWindowRequested += (s, e) =>
            {
                var a = new EngineNewWindowArgs { Uri = e.Uri, Handled = e.Handled };
                var h = NewWindowRequested; if (h != null) h(a);
                e.Handled = a.Handled;
            };

            _wv.PermissionRequested += (s, e) =>
            {
                var req = e.PermissionRequest;
                var wrapped = new EnginePermissionRequest(
                    req.PermissionType,
                    req.Uri,
                    () => req.Allow(),
                    () => req.Deny(),
                    () => req.Defer());
                var h = PermissionRequested; if (h != null) h(new EnginePermissionArgs { PermissionRequest = wrapped });
            };

            _wv.ScriptNotify += (s, e) =>
            {
                var h = ScriptNotify; if (h != null) h(new EngineMessageArgs { Value = e.Value });
            };

            _wv.UnviewableContentIdentified += (s, e) =>
            {
                var h = UnviewableContentIdentified; if (h != null) h(new EngineUriArgs { Uri = e.Uri });
            };

            _wv.LongRunningScriptDetected += (s, e) =>
            {
                var h = LongRunningScriptDetected; if (h != null) h(new EngineLongRunningScriptArgs
                {
                    ExecutionTime = e.ExecutionTime,
                    StopPageScriptExecution = e.StopPageScriptExecution
                });
            };

            _wv.ContainsFullScreenElementChanged += (s, e) =>
            {
                var h = ContainsFullScreenElementChanged; if (h != null) h(e);
            };
        }

        public FrameworkElement View { get { return _wv; } }
        public bool IsChromium { get { return false; } }

        public Uri Source { get { return _wv.Source; } set { _wv.Source = value; } }
        public string DocumentTitle { get { return _wv.DocumentTitle; } }
        public bool CanGoBack { get { return _wv.CanGoBack; } }
        public bool CanGoForward { get { return _wv.CanGoForward; } }
        public bool ContainsFullScreenElement { get { return _wv.ContainsFullScreenElement; } }

        public BrowserSettings Settings
        {
            get
            {
                if (_settings == null)
                {
                    _settings = new BrowserSettings(() =>
                    {
                        _wv.Settings.IsJavaScriptEnabled = _settings.IsJavaScriptEnabled;
                        _wv.Settings.IsIndexedDBEnabled = _settings.IsIndexedDBEnabled;
                    });
                }
                return _settings;
            }
        }
        private BrowserSettings _settings;

        public void Navigate(Uri source) { _wv.Navigate(source); }
        public void GoBack() { _wv.GoBack(); }
        public void GoForward() { _wv.GoForward(); }
        public void Refresh() { _wv.Refresh(); }
        public void Stop() { _wv.Stop(); }

        public void AddWebAllowedObject(string name, object pObject)
        {
            _wv.AddWebAllowedObject(name, pObject);
        }

        public void SetUserAgent(string userAgent)
        {
            // Original behaviour: process-wide urlmon override (see UserAgentManager.cs).
            Onitor.UserAgent.SetUserAgent(userAgent);
        }

        public async Task<string> InvokeScriptAsync(string scriptName, string[] arguments)
        {
            return await _wv.InvokeScriptAsync(scriptName, arguments);
        }

        public event Action<EngineNavStartingArgs> NavigationStarting;
        public event Action<EngineNavCompletedArgs> NavigationCompleted;
        public event Action<EngineContentLoadingArgs> ContentLoading;
        public event Action<EngineNavStartingArgs> FrameNavigationStarting;
        public event Action<EngineNavCompletedArgs> FrameNavigationCompleted;
        public event Action<EngineUriArgs> DOMContentLoaded;
        public event Action<EngineUriArgs> FrameDOMContentLoaded;
        public event Action<EngineNewWindowArgs> NewWindowRequested;
        public event Action<EnginePermissionArgs> PermissionRequested;
        public event Action<EngineMessageArgs> ScriptNotify;
        public event Action<EngineUriArgs> UnviewableContentIdentified;
        public event Action<EngineLongRunningScriptArgs> LongRunningScriptDetected;
        public event Action<object> ContainsFullScreenElementChanged;
        public event Action<string> EngineFailed;
    }

    // ============================================================================
    //  Chromium engine - Microsoft.UI.Xaml.Controls.WebView2 (WinUI 2).
    //
    //  Research notes (see worklog):
    //   * Official guide: https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/winui2
    //     ("Platforms: This article applies to Windows and XBOX"), NuGet chain
    //     Microsoft.UI.Xaml 2.8.x -> Microsoft.Web.WebView2.
    //   * Community report (StackOverflow #79799684, Oct 2025): on Xbox One
    //     DEV MODE the WebView2 "works flawlessly as UWP"; it hard-crashes the
    //     console only in RETAIL mode or when the app is deployed "as Game".
    //     Xbox Series X is fine, Series S can hit OOM. Hence: dev mode only,
    //     and the EngineMode == "Legacy" escape hatch exists for trouble.
    //   * Only a subset of APIs is exposed on the control; deeper events live
    //     on CoreWebView2 and are wired after EnsureCoreWebView2Async().
    //   * ExecuteScriptAsync returns JSON-encoded values - normalised here so
    //     the app's "true"/"A"/url comparisons keep working.
    //   * WebView2 cannot load ms-appx-web:// directly, so internal pages are
    //     served through the SetVirtualHostNameToFolderMapping("appassets.local")
    //     trick and translated in both directions.
    // ============================================================================

    internal sealed class ChromiumEngine : IBrowserEngine
    {
        private const string VirtualHost = "appassets.local";
        private const string PkgIdentity = "71330982-ba82-4d35-b5cb-3488eefb31ed"; // matches the ms-appx-web:// URIs the app compares against
        private static readonly Uri HomePageUri = new Uri("ms-appx-web://" + PkgIdentity + "/PagesHTML/Home.html");

        private readonly Mux.WebView2 _wv2;
        private Core.CoreWebView2 _core;
        private bool _coreReady;
        private Uri _pendingUri;
        private string _pendingUserAgent;
        private BrowserSettings _settings;

        // ====================================================================
        // WebGPU / WebNN enablement + Xbox GPU strategy (v1.5.0.0)
        // ====================================================================
        // WebView2 does not expose edge://flags or chrome://flags (internal
        // browser pages are disabled in embedded contexts), so Chromium
        // features have to be turned on through browser launch arguments
        // supplied by the host process. WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS
        // is a documented WebView2 loader variable that is read when the first
        // WebView2 environment of the process is created, so setting it from
        // this static constructor (guaranteed to run before the first engine
        // instance is constructed) covers every tab of the app.
        //
        // Base flags (all platforms):
        //   --enable-unsafe-webgpu : turns on the WebGPU API, required by
        //                            webml.ai / WebLLM / transformers.js etc.
        //   --enable-features=...  : WebGPU (legacy pre-ship feature name,
        //                            ignored by newer runtimes) plus WebNN
        //                            (Edge feature name: msWebNN, Chromium
        //                            prototype name: WebNNAPI) - unknown
        //                            feature names are ignored safely.
        //
        // HARDWARE LADDER (Xbox only, persisted in webgpu-mode.txt):
        //   rung 1  "auto-d3d11" : --use-webgpu-adapter=d3d11 - force the
        //            Dawn D3D11 backend. D3D11 is the one 3D API that IS
        //            fully available to UWP apps on the console (the same
        //            API ANGLE uses for WebGL). Verified against Chromium's
        //            own switch parser: service_utils.cc maps "d3d11" ->
        //            WebGPUAdapterName::kD3D11.
        //   rung 2  "auto-d3d12" : Chromium's default Windows backend (Dawn
        //            D3D12). Historically no adapters were enumerated on
        //            Xbox, but it is cheap to try before giving up.
        //   rung 3  "auto-gles"  : --use-webgpu-adapter=opengles - Dawn's
        //            OpenGLES backend routed through ANGLE's hardware
        //            D3D11 device (the device WebGL already uses). This is
        //            the KEY rung: Chromium's webgpu_decoder_impl.cc forces
        //            the SwiftShader fallback adapter whenever the GPU
        //            feature list marks ACCELERATED_WEBGPU as software (the
        //            Xbox GPU is unknown to it) - for every adapter type
        //            EXCEPT kOpenGLES, which is explicitly exempt from that
        //            forcing. So GLES is the only route that can hand WebGPU
        //            a real console GPU on Xbox today.
        //   rung 4  "auto-cpu"   : --use-webgpu-adapter=swiftshader - last
        //            resort, guarantees a software adapter so WebGPU pages
        //            (webml.ai etc.) always work, just CPU-driven.
        //
        // ADAPTIVE STARTUP: browser flags only apply at process start, so
        // the engine probes the live browser after startup (requestAdapter
        // from the first page, reporting gpu:<adapter>/fallback:<adapter>/
        // null/none/error). While a hardware rung yields no HARDWARE adapter
        // the ladder advances (d3d11 -> d3d12 -> gles -> cpu) and the app
        // restarts itself once per rung, so the best available backend
        // always wins in the end.
        //
        // PROBE MECHANICS (v1.5.1.0 - CRITICAL FIX): WebView2's
        // ExecuteScriptAsync does NOT await Promises - a script whose value
        // is a Promise returns the JSON serialization of the Promise OBJECT,
        // the literal string "{}" (WebView2Feedback issue #2295). The probe
        // used to be exactly such a Promise-returning IIFE, so every probe
        // since v1.3.0.0 returned "{}", matched no outcome branch, and the
        // ladder SILENTLY FROZE on rung 1 - the GLES rung never ran once
        // (webgpu-status.txt: "probe result={} mode=auto-d3d11"). The probe
        // now uses a kick+poll pattern: an idempotent script stores the
        // final adapter verdict in window.__onitorWgpuProbe as a PLAIN
        // STRING, and the host polls that script until the value stops
        // being 'pending'. No Promise ever crosses the boundary, and any
        // unknown probe value is logged instead of being ignored.
        //
        // HONEST SOFTWARE DETECTION (v1.5.0.0): an adapter is software if
        // isFallback/isFallbackAdapter is set OR its description/
        // architecture/vendor matches SwiftShader/llvmpipe/Basic
        // Render/WARP. The name check is required because Chromium reports
        // isFallback=false for an EXPLICITLY requested SwiftShader adapter
        // (rung 4) - v1.4.0.0 relied on the flag alone and therefore showed
        // a green "WebGPU: hardware, adapter: SwiftShader" badge.
        //
        // LADDER GENERATIONS: webgpu-ladder-gen.txt records which ladder
        // generation this console last ran (bump CurrentLadderGeneration
        // whenever a rung is added/moved). On mismatch: consoles in ANY auto
        // state (settled on SwiftShader OR frozen mid-ladder by the broken
        // v1.5.0.0 probe) resume directly at the GLES rung - D3D11/D3D12 are
        // source-verified forced-SwiftShader on Xbox, so nothing worth
        // retrying is skipped; user pins always survive; fresh consoles
        // (v1.5.1.0) also START at the GLES rung.
        //
        // User control via <LocalState>\webgpu-mode.txt (Device Portal):
        //   "auto" (or delete the file) - restart the ladder at rung 1
        //   "d3d11" / "d3d12" / "gles" - pin a backend forever (never
        //   auto-advanced)
        //   "auto-cpu" - stay on SwiftShader
        // Every decision is written to <LocalState>\webgpu-status.txt and
        // Chromium's own diagnostics go to <LocalState>\chromium.log.
        //
        // Power-user override: if <LocalState>\browser-flags.txt exists and
        // is non-empty, its complete content is used verbatim as the browser
        // arguments instead of anything computed here (Device Portal
        // editable, no rebuild needed to experiment with flags).
        //
        // Combined with the "expandedResources" restricted capability in
        // Package.appxmanifest (game-level system resources on Xbox dev
        // mode) the browser gets the largest possible CPU/GPU budget.
        // ====================================================================
        private const string BaseGpuFeatureBrowserArguments =
            "--enable-unsafe-webgpu --enable-features=WebGPU,msWebNN,WebNNAPI";

        private const string XboxD3D11BrowserArguments =
            "--use-webgpu-adapter=d3d11 --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader";

        private const string XboxD3D12BrowserArguments =
            "--use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader";

        // Rung 3: Dawn's OpenGLES backend through ANGLE's hardware D3D11
        // device - the only adapter type EXEMPT from Chromium's
        // force_fallback_adapter software override (webgpu_decoder_impl.cc
        // line ~1231: the forcing applies to every use_webgpu_adapter value
        // except kOpenGLES), hence the best hardware hope on Xbox.
        private const string XboxGlesBrowserArguments =
            "--use-webgpu-adapter=opengles --use-angle=d3d11 --ignore-gpu-blocklist --enable-unsafe-swiftshader";

        private const string XboxCpuBrowserArguments =
            "--use-webgpu-adapter=swiftshader --ignore-gpu-blocklist --enable-unsafe-swiftshader";

        private const string FlagsOverrideFileName = "browser-flags.txt";
        private const string ModeFileName = "webgpu-mode.txt";
        private const string StatusFileName = "webgpu-status.txt";

        // Records which ladder generation ran last. Bump the value whenever
        // the rung set or startup behaviour changes - consoles coming from
        // an older generation (including the probe-frozen gen2 consoles)
        // resume directly at the GLES rung instead of re-running rungs that
        // already failed there.
        private const string LadderGenFileName = "webgpu-ladder-gen.txt";
        private const string CurrentLadderGeneration = "gen3-1.5.1.0";

        /// <summary>
        /// Persisted WebGPU strategy (webgpu-mode.txt). "auto-*" values are
        /// managed by the adaptive ladder; "d3d11"/"d3d12" pin a backend
        /// forever (user choice, never auto-advanced). Unknown/legacy values
        /// (incl. the bare "cpu"/"gpu" written by v1.3.0.0) restart the
        /// ladder so upgraded consoles retry hardware automatically.
        /// </summary>
        private enum WebGpuMode
        {
            FreshAuto,    // no / unknown file -> start at the GLES rung (v1.5.1.0)
            AutoD3D11,    // ladder rung 1 running
            AutoD3D12,    // ladder rung 2 running
            AutoGles,     // ladder rung 3 running (GLES via ANGLE)
            AutoCpu,      // ladder settled on SwiftShader
            PinnedD3D11,  // user pinned rung 1 - never auto-advance
            PinnedD3D12,  // user pinned rung 2 - never auto-advance
            PinnedGles    // user pinned rung 3 - never auto-advance
        }

        // --------------------------------------------------------------------
        // On-page WebGPU status badge (v1.4.0.0). Injected into every page
        // BEFORE its own scripts run (AddScriptToExecuteOnDocumentCreated),
        // top frame only. Shows at a glance in the TOP-RIGHT CORNER:
        //   green  "WebGPU: hardware \u2713"        - running on the console GPU
        //   amber  "WebGPU: software \u00b7 SwiftShader" - CPU fallback active
        //   red    "WebGPU: unavailable" / "not enabled"
        // plus the WebGL renderer as a dim second line (hardware vs software
        // GL is the key diagnostic for whether the console GPU stack works
        // at all). Pure CSSOM styling (element.style properties, no style
        // attributes or <style> tags) so strict-CSP sites cannot block it;
        // pointer-events: none so it never intercepts input; hides itself
        // while a video is fullscreen.
        // --------------------------------------------------------------------
        private const string WebGpuBadgeScript = @"
(function () {
  'use strict';
  try {
    if (window.top !== window.self) return;
    if (window.__onitorWgpuBadge) return;
    window.__onitorWgpuBadge = true;

    var box = document.createElement('div');
    var s = box.style;
    s.position = 'fixed';
    s.top = '8px';
    s.right = '8px';
    s.zIndex = '2147483647';
    s.padding = '5px 10px';
    s.borderRadius = '9px';
    s.font = '600 12px/1.45 system-ui, sans-serif';
    s.color = '#e8eaf0';
    s.background = 'rgba(18,20,26,0.82)';
    s.border = '1px solid rgba(255,255,255,0.25)';
    s.pointerEvents = 'none';
    s.whiteSpace = 'nowrap';
    s.textShadow = '0 1px 2px rgba(0,0,0,0.85)';
    s.maxWidth = '48vw';
    s.overflow = 'hidden';
    s.textOverflow = 'ellipsis';

    var l1 = document.createElement('div');
    l1.textContent = 'WebGPU: checking...';
    var l2 = document.createElement('div');
    var s2 = l2.style;
    s2.fontSize = '10px';
    s2.fontWeight = '400';
    s2.opacity = '0.72';
    s2.whiteSpace = 'nowrap';
    s2.overflow = 'hidden';
    s2.textOverflow = 'ellipsis';
    box.appendChild(l1);
    box.appendChild(l2);

    function mount() {
      (document.body || document.documentElement).appendChild(box);
    }
    if (document.body) { mount(); }
    else { document.addEventListener('DOMContentLoaded', mount, { once: true }); }

    function paint(state, text, sub) {
      l1.textContent = text;
      l2.textContent = sub || '';
      var c = state === 'ok' ? '#34d27b' : (state === 'soft' ? '#f5b93d' : (state === 'bad' ? '#ff6b6b' : '#9aa3af'));
      s.borderColor = c;
      l1.style.color = c;
    }

    function webglLine() {
      try {
        var cv = document.createElement('canvas');
        var gl = cv.getContext('webgl2') || cv.getContext('webgl');
        if (!gl) return 'WebGL: unavailable';
        var r = '';
        try {
          var ext = gl.getExtension('WEBGL_debug_renderer_info');
          if (ext) r = String(gl.getParameter(ext.UNMASKED_RENDERER_WEBGL) || '');
        } catch (e) {}
        if (!r) return 'WebGL: active (renderer info masked)';
        var soft = /swiftshader|software|llvmpipe|basic render/i.test(r);
        return 'WebGL: ' + (soft ? 'software' : 'hardware') + ' \u00b7 ' + r;
      } catch (e) { return ''; }
    }

    if (!navigator.gpu) {
      paint('bad', 'WebGPU: not enabled', webglLine());
      return;
    }

    var gl2 = webglLine();
    navigator.gpu.requestAdapter().then(function (a) {
      if (!a) { paint('bad', 'WebGPU: unavailable', gl2); return; }
      var info = {};
      try { info = a.info || {}; } catch (e) {}
      var d = '';
      try { d = info.description || info.architecture || ''; } catch (e) {}
      if (!d) {
        try {
          if (typeof a.requestAdapterInfo === 'function') {
            var ri = a.requestAdapterInfo();
            if (ri) d = ri.description || ri.architecture || '';
          }
        } catch (e) {}
      }
      var fb = false;
      try { fb = !!(a.isFallback || a.isFallbackAdapter); } catch (e) {}
      // Software is ALSO detected by NAME: an explicitly requested
      // SwiftShader adapter (ladder rung 4) reports isFallback=false, so the
      // flag alone lied in v1.4.0.0 (green badge with 'adapter: SwiftShader').
      var hay = d + ' ' + (info.vendor || '') + ' ' + (info.architecture || '');
      var soft = fb || /swiftshader|software|llvmpipe|basic render|warp/i.test(hay);
      if (d) d = String(d);
      if (d.length > 46) d = d.slice(0, 45) + '\u2026';
      if (soft) {
        paint('soft', 'WebGPU: software' + (/swiftshader/i.test(hay) ? ' \u00b7 SwiftShader' : ''),
          (d ? 'adapter: ' + d : '') + (gl2 ? (d ? ' \u00b7 ' : '') + gl2 : ''));
      } else {
        paint('ok', 'WebGPU: hardware \u2713',
          (d ? 'adapter: ' + d : 'adapter: GPU') + (gl2 ? ' \u00b7 ' + gl2 : ''));
      }
    }, function () {
      paint('bad', 'WebGPU: error', gl2);
    });

    document.addEventListener('fullscreenchange', function () {
      box.style.display = document.fullscreenElement ? 'none' : '';
    });
  } catch (e) { /* never break the page */ }
})();";

        private static bool _webgpuProbeStarted;
        private static bool _restartedForFallback;

        static ChromiumEngine()
        {
            try
            {
                string flags = ComputeBrowserArguments();

                string existing = Environment.GetEnvironmentVariable(
                    "WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS");
                if (string.IsNullOrEmpty(existing))
                {
                    Environment.SetEnvironmentVariable(
                        "WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", flags);
                }
                else if (existing.IndexOf("--enable-unsafe-webgpu", StringComparison.OrdinalIgnoreCase) < 0)
                {
                    // Keep whatever the environment already forced, just make
                    // sure the GPU feature flags are present as well.
                    Environment.SetEnvironmentVariable(
                        "WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS",
                        existing + " " + flags);
                }
                Debug.WriteLine("[Onitor] WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS = " +
                    Environment.GetEnvironmentVariable("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS"));
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] Failed to set WebGPU browser arguments: " + ex.Message);
            }
        }

        private static string ComputeBrowserArguments()
        {
            // 1) Power-user override file wins if present.
            try
            {
                string overridePath = System.IO.Path.Combine(
                    Windows.Storage.ApplicationData.Current.LocalFolder.Path,
                    FlagsOverrideFileName);
                if (System.IO.File.Exists(overridePath))
                {
                    string custom = System.IO.File.ReadAllText(overridePath);
                    if (!string.IsNullOrWhiteSpace(custom))
                    {
                        Debug.WriteLine("[Onitor] Custom browser flags loaded from " + overridePath);
                        return custom.Trim();
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] Could not read " + FlagsOverrideFileName + ": " + ex.Message);
            }

            // 2) Computed defaults: hardware WebGPU ladder on Xbox, plain
            //    hardware WebGPU everywhere else.
            string args = BaseGpuFeatureBrowserArguments;
            if (IsXboxDevice())
            {
                WebGpuMode mode = ReadWebGpuMode();
                if (mode == WebGpuMode.FreshAuto)
                {
                    // Fresh ladder (v1.5.1.0): start DIRECTLY at the GLES
                    // rung and persist it so the startup probe knows which
                    // backend this process is running. D3D11 and D3D12 are
                    // skipped: on Xbox Chromium's webgpu_decoder_impl.cc
                    // force-replaces both with SwiftShader (unknown GPU ->
                    // software-marked feature list -> force_fallback_adapter
                    // for everything except kOpenGLES), and with the probe
                    // now actually working the first restart-worthy backend
                    // is GLES anyway.
                    mode = WebGpuMode.AutoGles;
                    TrySetWebGpuMode("auto-gles");
                }

                if (mode == WebGpuMode.AutoCpu)
                {
                    args += " " + XboxCpuBrowserArguments;
                }
                else if (mode == WebGpuMode.AutoGles || mode == WebGpuMode.PinnedGles)
                {
                    args += " " + XboxGlesBrowserArguments;
                }
                else if (mode == WebGpuMode.AutoD3D12 || mode == WebGpuMode.PinnedD3D12)
                {
                    args += " " + XboxD3D12BrowserArguments;
                }
                else // AutoD3D11 / PinnedD3D11
                {
                    args += " " + XboxD3D11BrowserArguments;
                }

                // Chromium debug log -> <LocalState>\chromium.log (readable
                // through the Xbox Device Portal file explorer). Captures
                // Dawn backend / adapter-initialisation errors, the fastest
                // way to diagnose why a ladder rung failed.
                try
                {
                    args += " --enable-logging --log-file=" + LocalStatePath("chromium.log");
                }
                catch (Exception) { /* keep the flags without logging */ }
            }
            return args;
        }

        private static bool IsXboxDevice()
        {
            try
            {
                return Windows.System.Profile.AnalyticsInfo.VersionInfo.DeviceFamily == "Windows.Xbox";
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] DeviceFamily detection failed: " + ex.Message);
                return false;
            }
        }

        private static string LocalStatePath(string fileName)
        {
            return System.IO.Path.Combine(
                Windows.Storage.ApplicationData.Current.LocalFolder.Path, fileName);
        }

        /// <summary>
        /// Resolves the effective mode for THIS process: raw file value,
        /// adjusted for ladder generation changes (see ReadWebGpuModeFile).
        /// Called from the static constructor (flag computation) and from the
        /// startup probe - both get the same answer.
        /// </summary>
        private static WebGpuMode ReadWebGpuMode()
        {
            WebGpuMode mode = ReadWebGpuModeFile();
            if (!LadderGenerationIsCurrent())
            {
                if (mode == WebGpuMode.AutoCpu || mode == WebGpuMode.AutoD3D11 ||
                    mode == WebGpuMode.AutoD3D12 || mode == WebGpuMode.AutoGles)
                {
                    // Previous generation in ANY auto state - settled on
                    // SwiftShader, or frozen mid-ladder by the broken v1.5.0.0
                    // probe ("probe result={}" matched no branch, so the
                    // ladder never advanced) - resumes directly at the GLES
                    // rung. D3D11/D3D12 are known forced-SwiftShader on Xbox;
                    // user pins always survive below.
                    mode = WebGpuMode.AutoGles;
                    TrySetWebGpuMode("auto-gles");
                }
                else if (mode != WebGpuMode.PinnedD3D11 &&
                         mode != WebGpuMode.PinnedD3D12 &&
                         mode != WebGpuMode.PinnedGles)
                {
                    // Never started: start at the GLES rung (user pins
                    // survive; FreshAuto is converted in
                    // ComputeBrowserArguments).
                    mode = WebGpuMode.FreshAuto;
                }
                TryWriteLadderGeneration();
            }
            return mode;
        }

        private static WebGpuMode ReadWebGpuModeFile()
        {
            try
            {
                string path = LocalStatePath(ModeFileName);
                if (System.IO.File.Exists(path))
                {
                    string mode = System.IO.File.ReadAllText(path).Trim();
                    if (string.Equals(mode, "auto-d3d11", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.AutoD3D11;
                    if (string.Equals(mode, "auto-d3d12", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.AutoD3D12;
                    if (string.Equals(mode, "auto-gles", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.AutoGles;
                    if (string.Equals(mode, "auto-cpu", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.AutoCpu;
                    if (string.Equals(mode, "d3d11", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.PinnedD3D11;
                    if (string.Equals(mode, "d3d12", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.PinnedD3D12;
                    if (string.Equals(mode, "gles", StringComparison.OrdinalIgnoreCase)) return WebGpuMode.PinnedGles;
                    // Anything else - "auto", legacy v1.3.0.0 values ("gpu" /
                    // bare "cpu"), garbage - starts a FRESH ladder so every
                    // upgraded console retries hardware automatically.
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] Could not read " + ModeFileName + ": " + ex.Message);
            }
            return WebGpuMode.FreshAuto;
        }

        private static bool LadderGenerationIsCurrent()
        {
            try
            {
                string path = LocalStatePath(LadderGenFileName);
                if (System.IO.File.Exists(path))
                {
                    string gen = System.IO.File.ReadAllText(path).Trim();
                    return string.Equals(gen, CurrentLadderGeneration, StringComparison.OrdinalIgnoreCase);
                }
            }
            catch (Exception) { }
            return false;
        }

        private static void TryWriteLadderGeneration()
        {
            try
            {
                System.IO.File.WriteAllText(LocalStatePath(LadderGenFileName), CurrentLadderGeneration);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] Could not write " + LadderGenFileName + ": " + ex.Message);
            }
        }

        /// <summary>
        /// Persist a ladder value and verify it landed (read-back). The
        /// verification matters: the ladder only restarts the app after a
        /// CONFIRMED mode change, which rules out restart loops if the file
        /// system misbehaves.
        /// </summary>
        private static bool TrySetWebGpuMode(string value)
        {
            try
            {
                System.IO.File.WriteAllText(LocalStatePath(ModeFileName), value);
                string readBack = System.IO.File.ReadAllText(LocalStatePath(ModeFileName)).Trim();
                return string.Equals(readBack, value, StringComparison.OrdinalIgnoreCase);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] Could not write " + ModeFileName + ": " + ex.Message);
                return false;
            }
        }

        private static string ModeName(WebGpuMode mode)
        {
            switch (mode)
            {
                case WebGpuMode.AutoD3D11: return "auto-d3d11";
                case WebGpuMode.AutoD3D12: return "auto-d3d12";
                case WebGpuMode.AutoGles: return "auto-gles";
                case WebGpuMode.AutoCpu: return "auto-cpu";
                case WebGpuMode.PinnedD3D11: return "d3d11 (pinned)";
                case WebGpuMode.PinnedD3D12: return "d3d12 (pinned)";
                case WebGpuMode.PinnedGles: return "gles (pinned)";
                default: return "fresh-auto";
            }
        }

        private static void AppendWebGpuStatus(string line)
        {
            try
            {
                string path = LocalStatePath(StatusFileName);
                string stamp = DateTimeOffset.Now.ToString("yyyy-MM-dd HH:mm:ss");
                string entry = "[" + stamp + "] " + line + "\r\n";

                // Read-modify-write keeps us on the safest UWP file API
                // (File.ReadAllText/WriteAllText) and lets us cap the log.
                string existing = string.Empty;
                if (System.IO.File.Exists(path))
                {
                    existing = System.IO.File.ReadAllText(path);
                }
                if (existing != null && existing.Length > 8192)
                {
                    // Keep only the most recent quarter of the log.
                    existing = existing.Substring(existing.Length - 4096);
                    int cut = existing.IndexOf('\n');
                    if (cut >= 0 && cut + 1 <= existing.Length)
                    {
                        existing = existing.Substring(cut + 1);
                    }
                }
                System.IO.File.WriteAllText(path, existing + entry);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] Could not write " + StatusFileName + ": " + ex.Message);
            }
        }

        public ChromiumEngine()
        {
            _wv2 = new Mux.WebView2();
            _settings = new BrowserSettings(ApplySettings);
            InitializeAsync();
        }

        private async void InitializeAsync()
        {
            try
            {
                await _wv2.EnsureCoreWebView2Async();
                _core = _wv2.CoreWebView2;
                WireCore();
                _coreReady = true;

                // Local app pages (Home.html / error pages) through a virtual host,
                // because WebView2 cannot navigate to ms-appx-web:// directly.
                try
                {
                    _core.SetVirtualHostNameToFolderMapping(
                        VirtualHost,
                        Package.Current.InstalledLocation.Path,
                        Core.CoreWebView2HostResourceAccessKind.Allow);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("[Onitor] SetVirtualHostNameToFolderMapping failed: " + ex.Message);
                }

                // WebGPU status badge (v1.4.0.0): the small top-right pill
                // on every page (see WebGpuBadgeScript). Registered per tab
                // BEFORE any page script runs.
                try
                {
                    await _core.AddScriptToExecuteOnDocumentCreatedAsync(WebGpuBadgeScript);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("[Onitor] Could not install WebGPU badge script: " + ex.Message);
                }

                ApplySettings();
                if (!string.IsNullOrEmpty(_pendingUserAgent))
                {
                    _core.Settings.UserAgent = _pendingUserAgent;
                }

                if (_pendingUri != null)
                {
                    Uri uri = _pendingUri;
                    _pendingUri = null;
                    _wv2.Source = ToEngine(uri);
                }

                // Once per process: verify which WebGPU adapter the browser
                // actually got and adapt (see ProbeWebGpuAndAdaptAsync).
                ProbeWebGpuAndAdaptAsync(_wv2);
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] WebView2 initialization failed: " + ex.Message);
                var h = EngineFailed; if (h != null) h("WebView2 initialization failed: " + ex.Message);
            }
        }

        private void WireCore()
        {
            _core.NavigationStarting += (s, e) =>
            {
                var a = new EngineNavStartingArgs { Uri = FromEngine(e.Uri), Cancel = e.Cancel };
                var h = NavigationStarting; if (h != null) h(a);
                e.Cancel = a.Cancel;
            };

            _core.NavigationCompleted += (s, e) =>
            {
                var h = NavigationCompleted; if (h != null) h(new EngineNavCompletedArgs
                {
                    Uri = CurrentUri(),
                    IsSuccess = e.IsSuccess,
                    WebErrorStatus = MapErrorStatus(e.WebErrorStatus)
                });
            };

            _core.ContentLoading += (s, e) =>
            {
                var h = ContentLoading; if (h != null) h(new EngineContentLoadingArgs { Uri = CurrentUri() });
            };

            _core.FrameNavigationStarting += (s, e) =>
            {
                var a = new EngineNavStartingArgs { Uri = FromEngine(e.Uri), Cancel = e.Cancel };
                var h = FrameNavigationStarting; if (h != null) h(a);
                e.Cancel = a.Cancel;
            };

            _core.FrameNavigationCompleted += (s, e) =>
            {
                var h = FrameNavigationCompleted; if (h != null) h(new EngineNavCompletedArgs
                {
                    Uri = CurrentUri(),
                    IsSuccess = e.IsSuccess,
                    WebErrorStatus = MapErrorStatus(e.WebErrorStatus)
                });
            };

            _core.DOMContentLoaded += (s, e) =>
            {
                var h = DOMContentLoaded; if (h != null) h(new EngineUriArgs { Uri = CurrentUri() });
            };

            _core.NewWindowRequested += (s, e) =>
            {
                var a = new EngineNewWindowArgs { Uri = FromEngine(e.Uri), Handled = e.Handled };
                var h = NewWindowRequested; if (h != null) h(a);
                e.Handled = a.Handled;
            };

            _core.PermissionRequested += (s, e) =>
            {
                EnginePermissionRequest wrapped = new EnginePermissionRequest(
                    MapPermissionKind(e.PermissionKind),
                    CurrentUri(),
                    () => { e.State = Core.CoreWebView2PermissionState.Allow; e.Handled = true; },
                    () => { e.State = Core.CoreWebView2PermissionState.Deny; e.Handled = true; },
                    () => { /* leave unhandled -> default behaviour */ });
                var h = PermissionRequested; if (h != null) h(new EnginePermissionArgs { PermissionRequest = wrapped });
            };

            _core.WebMessageReceived += (s, e) =>
            {
                try
                {
                    var h = ScriptNotify; if (h != null) h(new EngineMessageArgs { Value = e.TryGetWebMessageAsString() });
                }
                catch (Exception) { /* message was JSON, not a string */ }
            };

            _core.ContainsFullScreenElementChanged += (s, e) =>
            {
                var h = ContainsFullScreenElementChanged; if (h != null) h(e);
            };

            _core.ProcessFailed += (s, e) =>
            {
                Debug.WriteLine("[Onitor] WebView2 process failed.");
                var h = EngineFailed; if (h != null) h("The WebView2 browser process stopped unexpectedly. Close and reopen this tab.");
            };

            // WebView2 exposes frame DOMContentLoaded only per-frame; the app only
            // logs this event, so it is approximated with the main DOM event.
            _core.DOMContentLoaded += (s, e) =>
            {
                var h = FrameDOMContentLoaded; if (h != null) h(new EngineUriArgs { Uri = CurrentUri() });
            };
        }

        private void ApplySettings()
        {
            if (!_coreReady) return;
            try
            {
                _core.Settings.IsScriptEnabled = _settings.IsJavaScriptEnabled;
                _core.Settings.IsWebMessageEnabled = true; // ScriptNotify bridge
                _core.Settings.AreDevToolsEnabled = true;
            }
            catch (Exception ex)
            {
                Debug.WriteLine("[Onitor] ApplySettings failed: " + ex.Message);
            }
        }

        // ----- URL translation (ms-appx-web:// <-> https://appassets.local) -----

        private Uri ToEngine(Uri uri)
        {
            if (uri == null) return null;

            // The app's internal "about:home" scheme is intercepted before
            // Chromium ever sees it (it is not a real Chrome about: page).
            if (uri.Scheme == "about" && uri.AbsolutePath != null && uri.AbsolutePath.IndexOf("home", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                uri = HomePageUri;
            }

            string s = uri.AbsoluteUri;
            if (s.StartsWith("ms-appx-web://", StringComparison.OrdinalIgnoreCase))
            {
                int authorityStart = "ms-appx-web://".Length;
                int pathStart = s.IndexOf('/', authorityStart);
                if (pathStart < 0)
                {
                    return new Uri("https://" + VirtualHost + "/");
                }
                return new Uri("https://" + VirtualHost + s.Substring(pathStart));
            }
            return uri;
        }

        private Uri FromEngine(Uri uri)
        {
            if (uri == null) return null;
            string s = uri.AbsoluteUri;
            if (s.StartsWith("https://" + VirtualHost + "/", StringComparison.OrdinalIgnoreCase))
            {
                return new Uri("ms-appx-web://" + PkgIdentity + s.Substring(("https://" + VirtualHost).Length));
            }
            return uri;
        }

        // WinRT WebView2 exposes URIs as strings (CoreWebView2.Source, event args).
        private Uri FromEngine(string uri)
        {
            if (string.IsNullOrEmpty(uri)) return null;
            try { return FromEngine(new Uri(uri)); }
            catch (Exception) { return null; }
        }

        private Uri CurrentUri()
        {
            if (_coreReady)
            {
                try { return FromEngine(_core.Source); }
                catch (Exception) { }
            }
            return Source;
        }

        // --------------------------------------------------------------------
        // Adaptive WebGPU startup (v1.4.0.0 ladder, probe fixed in v1.5.1.0).
        // Browser flags are fixed for the life of the process, so we verify
        // what the browser actually ended up with by asking the page for an
        // adapter. Probe outcomes:
        //   "gpu:<desc>"      -> hardware adapter active - ladder done, the
        //                        current rung is sticky for future launches.
        //   "fallback:<desc>" -> adapter active but a CPU fallback
        //                        (SwiftShader). If we are still on a hardware
        //                        rung of the ladder, ADVANCE and restart once.
        //   "null"            -> no adapter at all - same ladder advance.
        //   "none"            -> navigator.gpu missing: flags were not applied
        //                        at all (env var blocked?) - logged only.
        //   "error"           -> requestAdapter rejected - logged, retried
        //                        next launch.
        //   "inconclusive"    -> adapter request did not settle within the
        //                        polling window (or an unexpected value came
        //                        back) - retried next launch.
        // Restarts happen at most ONCE per process and only after the mode
        // file change has been VERIFIED (TrySetWebGpuMode read-back), so the
        // ladder can never restart-loop.
        //
        // WHY KICK+POLL (the v1.5.1.0 fix): WebView2's ExecuteScriptAsync
        // does NOT await Promises. A script whose value is a Promise returns
        // the JSON serialization of the Promise OBJECT itself - the literal
        // string "{}" (WebView2Feedback issue #2295). The probe used to
        // return requestAdapter().then(...), i.e. a Promise, so every probe
        // from v1.3.0.0 to v1.5.0.0 came back as "{}", matched no outcome
        // branch, and the ladder silently froze on rung 1 - the GLES rung
        // never ran once (user's webgpu-status.txt: "probe result={}
        // mode=auto-d3d11"). The probe now works differently: one idempotent
        // script starts requestAdapter AT MOST once per page and stores the
        // verdict in window.__onitorWgpuProbe as a PLAIN STRING; the host
        // executes that same script repeatedly (it doubles as the poll) and
        // reads the string. No Promise ever crosses the ExecuteScriptAsync
        // boundary, and the script re-kicks itself after a navigation
        // (fresh window global), so a racing navigation cannot kill it.
        // --------------------------------------------------------------------
        private static async void ProbeWebGpuAndAdaptAsync(Mux.WebView2 view)
        {
            if (_webgpuProbeStarted) return;
            _webgpuProbeStarted = true;
            try
            {
                await Task.Delay(2500); // let the first page settle

                // GPUAdapter.isFallbackAdapter was renamed to isFallback
                // around Chrome 119 - check BOTH. Additionally an
                // EXPLICITLY requested SwiftShader adapter (rung 4) reports
                // isFallback=false, so software is also detected by adapter
                // NAME (SwiftShader/llvmpipe/Basic Render/WARP) - otherwise a
                // forced SwiftShader adapter is misreported as hardware
                // (the v1.4.0.0 badge bug).
                string kickPollJs =
                    "(function(){" +
                    "  try {" +
                    "    if (!navigator.gpu) { window.__onitorWgpuProbe = 'none'; return 'none'; }" +
                    "    var cur = window.__onitorWgpuProbe;" +
                    "    if (typeof cur === 'string' && cur.length > 0) return cur;" +
                    "    window.__onitorWgpuProbe = 'pending';" +
                    "    navigator.gpu.requestAdapter().then(function(a){" +
                    "      try {" +
                    "        if (!a) { window.__onitorWgpuProbe = 'null'; return; }" +
                    "        var fb=false; try { fb = !!(a.isFallback || a.isFallbackAdapter); } catch(e){}" +
                    "        var d='',ar='',vn='';" +
                    "        try { var i=a.info||{}; d=i.description||''; ar=i.architecture||''; vn=i.vendor||'';" +
                    "          if(!d && !ar && !vn && typeof a.requestAdapterInfo==='function'){ var ri=a.requestAdapterInfo(); if(ri){ d=ri.description||''; ar=ri.architecture||''; vn=ri.vendor||''; } } } catch(e){}" +
                    "        var v=(ar?ar+' ':'')+vn;" +
                    "        var soft = fb || /swiftshader|software|llvmpipe|basic render|warp/i.test(d+' '+v);" +
                    "        window.__onitorWgpuProbe = (soft?'fallback:':'gpu:') + String(d||v).slice(0,90);" +
                    "      } catch(e){ window.__onitorWgpuProbe = 'error'; }" +
                    "    }, function(){ window.__onitorWgpuProbe = 'error'; });" +
                    "    return 'pending';" +
                    "  } catch (e) { return 'error'; }" +
                    "})()";

                // Poll the kick/poll script: first call starts the adapter
                // request (returns 'pending'), later calls just read the
                // global. ~10.8s total window for the GPU process to
                // enumerate adapters (first-ever GLES/ANGLE init can be
                // slow), then we give up as inconclusive and try again on
                // the next launch - the mode file is NOT advanced in that
                // case, so nothing is skipped.
                string result = null;
                const int MaxPollAttempts = 18;
                for (int attempt = 0; attempt < MaxPollAttempts && result == null; attempt++)
                {
                    if (attempt > 0) await Task.Delay(600);
                    try
                    {
                        string raw = await view.ExecuteScriptAsync(kickPollJs);
                        string current = NormalizeScriptResult(raw);
                        if (string.IsNullOrEmpty(current)) continue; // global not set yet
                        if (current == "pending") continue;          // request still in flight
                        result = current;
                    }
                    catch (Exception)
                    {
                        // Navigation raced with this poll - the next attempt
                        // re-kicks on whatever page the tab is on then.
                    }
                }

                string kind;
                string desc;
                if (result == null)
                {
                    kind = "inconclusive";
                    desc = string.Empty;
                }
                else
                {
                    kind = result;
                    desc = string.Empty;
                    int split = kind.IndexOf(':');
                    if (split > 0)
                    {
                        desc = kind.Substring(split + 1);
                        kind = kind.Substring(0, split);
                    }
                }
                if (kind.Length == 0) kind = "inconclusive";

                // Runtime identification (v1.5.1.0): the exact WebGPU
                // fallback logic in webgpu_decoder_impl.cc depends on the
                // Chromium version, so every probe line now records the
                // browser's Chromium version from the user agent.
                string chromiumVersion = string.Empty;
                try
                {
                    string ua = NormalizeScriptResult(
                        await view.ExecuteScriptAsync("navigator.userAgent")) ?? string.Empty;
                    int c = ua.IndexOf("Chrome/", StringComparison.OrdinalIgnoreCase);
                    if (c >= 0)
                    {
                        System.Text.StringBuilder v = new System.Text.StringBuilder();
                        for (int i = c + 7; i < ua.Length; i++)
                        {
                            char ch = ua[i];
                            if ((ch >= '0' && ch <= '9') || ch == '.') { v.Append(ch); }
                            else break;
                        }
                        string vs = v.ToString().TrimEnd('.');
                        if (vs.Length > 0) chromiumVersion = " chromium=" + vs;
                    }
                }
                catch (Exception) { }

                WebGpuMode mode = ReadWebGpuMode();
                if (mode == WebGpuMode.FreshAuto) mode = WebGpuMode.AutoGles; // fresh starts at the GLES rung (v1.5.1.0)
                bool isXbox = IsXboxDevice();

                Debug.WriteLine("[Onitor] WebGPU probe: " + kind + " ('" + desc + "') mode=" + ModeName(mode));
                AppendWebGpuStatus("probe result=" + kind +
                    (desc.Length > 0 ? " adapter='" + desc + "'" : string.Empty) +
                    " mode=" + ModeName(mode) + " xbox=" + isXbox + chromiumVersion);

                if (kind == "gpu")
                {
                    AppendWebGpuStatus("Hardware WebGPU adapter ACTIVE ('" + desc + "') - running on the console GPU.");
                    return;
                }

                if (kind == "fallback" || kind == "null")
                {
                    // This launch's backend did not produce a hardware
                    // adapter. Advance the ladder once per rung.
                    if (isXbox && !_restartedForFallback)
                    {
                        string next = null;
                        if (mode == WebGpuMode.AutoD3D11) next = "auto-d3d12";
                        else if (mode == WebGpuMode.AutoD3D12) next = "auto-gles";
                        else if (mode == WebGpuMode.AutoGles) next = "auto-cpu";

                        if (next != null && TrySetWebGpuMode(next))
                        {
                            _restartedForFallback = true;
                            AppendWebGpuStatus("No hardware adapter in " + ModeName(mode) +
                                " - advancing ladder to " + next + " and restarting once.");
                            await Windows.ApplicationModel.Core.CoreApplication.RequestRestartAsync("webgpu-ladder");
                            return;
                        }
                    }

                    if (mode == WebGpuMode.AutoCpu)
                    {
                        AppendWebGpuStatus(kind == "fallback"
                            ? "SwiftShader software adapter active (expected in auto-cpu mode)."
                            : "auto-cpu mode still has no adapter - check browser-flags.txt and chromium.log.");
                    }
                    else
                    {
                        AppendWebGpuStatus("WebGPU is on a software adapter or unavailable (mode=" + ModeName(mode) +
                            "). Set webgpu-mode.txt to 'auto' to retry the hardware ladder.");
                    }
                    return;
                }

                if (kind == "none")
                {
                    AppendWebGpuStatus("navigator.gpu is missing - WebGPU flags were not applied (WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS blocked?).");
                }
                else if (kind == "error" || kind == "inconclusive")
                {
                    AppendWebGpuStatus("probe " + kind + " (adapter request failed or timed out) - will retry next launch.");
                }
                else
                {
                    // v1.5.1.0 guard: an unexpected probe value must NEVER
                    // be silently ignored - a script returning something we
                    // did not anticipate is exactly how the "{}" Promise
                    // serialization froze the ladder for three versions.
                    AppendWebGpuStatus("probe returned unexpected value '" + kind +
                        "' - treated as inconclusive, will retry next launch.");
                }
            }
            catch (Exception ex)
            {
                // Probe races with navigation are harmless - retried next launch.
                Debug.WriteLine("[Onitor] WebGPU probe skipped: " + ex.Message);
            }
        }

        // ----- enum mapping -----

        private static WebViewPermissionType MapPermissionKind(Core.CoreWebView2PermissionKind kind)
        {
            switch (kind)
            {
                case Core.CoreWebView2PermissionKind.Geolocation:
                    return WebViewPermissionType.Geolocation;
                case Core.CoreWebView2PermissionKind.Microphone:
                case Core.CoreWebView2PermissionKind.Camera:
                    return WebViewPermissionType.Media;
                case Core.CoreWebView2PermissionKind.Notifications:
                    return WebViewPermissionType.WebNotifications;
                default:
                    // WebViewPermissionType has no Unspecified/Other member
                    // (real members: Geolocation, ImmersiveView, Media, PointerLock,
                    // Screen, UnlimitedIndexedDBQuota, WebNotifications). Screen has
                    // no case in the app's permission switch, so unmapped WebView2
                    // permission kinds fall through to WebView2's default handling.
                    return WebViewPermissionType.Screen;
            }
        }

        private static WebErrorStatus MapErrorStatus(Core.CoreWebView2WebErrorStatus status)
        {
            switch (status)
            {
                case Core.CoreWebView2WebErrorStatus.CannotConnect: return WebErrorStatus.CannotConnect;
                case Core.CoreWebView2WebErrorStatus.CertificateCommonNameIsIncorrect: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.CertificateExpired: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.CertificateIsInvalid: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.ConnectionAborted: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.ConnectionReset: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.Disconnected: return WebErrorStatus.Disconnected;
                case Core.CoreWebView2WebErrorStatus.HostNameNotResolved: return WebErrorStatus.HostNameNotResolved;
                case Core.CoreWebView2WebErrorStatus.OperationCanceled: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.RedirectFailed: return WebErrorStatus.Unknown;
                case Core.CoreWebView2WebErrorStatus.Timeout: return WebErrorStatus.Timeout;
                case Core.CoreWebView2WebErrorStatus.UnexpectedError: return WebErrorStatus.Unknown;
                default: return WebErrorStatus.Unknown;
            }
        }

        /// <summary>
        /// WebView2's ExecuteScriptAsync returns JSON-encoded results
        /// ("true", "\"A\"", "null") while EdgeHTML's InvokeScriptAsync returned
        /// raw strings. Normalise so existing comparisons keep working.
        /// </summary>
        private static string NormalizeScriptResult(string raw)
        {
            if (string.IsNullOrEmpty(raw)) return raw;
            if (raw == "null" || raw == "undefined") return null;
            if (raw.Length >= 2 && raw[0] == '"' && raw[raw.Length - 1] == '"')
            {
                try
                {
                    return Newtonsoft.Json.JsonConvert.DeserializeObject<string>(raw);
                }
                catch (Exception)
                {
                    return raw;
                }
            }
            return raw;
        }

        // ----- IBrowserEngine -----

        public FrameworkElement View { get { return _wv2; } }
        public bool IsChromium { get { return true; } }

        public Uri Source
        {
            get
            {
                if (_coreReady)
                {
                    try { return FromEngine(_core.Source); }
                    catch (Exception) { }
                }
                return _pendingUri;
            }
            set
            {
                Uri engineUri = ToEngine(value);
                if (!_coreReady)
                {
                    _pendingUri = value; // applied after CoreWebView2 + settings are ready
                    return;
                }
                _wv2.Source = engineUri;
            }
        }

        public string DocumentTitle
        {
            get { return _coreReady ? _core.DocumentTitle : string.Empty; }
        }

        public bool CanGoBack { get { return _coreReady && _wv2.CanGoBack; } }
        public bool CanGoForward { get { return _coreReady && _wv2.CanGoForward; } }
        public bool ContainsFullScreenElement { get { return _coreReady && _core.ContainsFullScreenElement; } }

        public BrowserSettings Settings { get { return _settings; } }

        public void Navigate(Uri source)
        {
            Source = source;
        }

        public void GoBack() { if (_coreReady) _wv2.GoBack(); }
        public void GoForward() { if (_coreReady) _wv2.GoForward(); }
        public void Refresh() { if (_coreReady) _wv2.Reload(); }
        public void Stop() { if (_coreReady) _core.Stop(); }

        public void AddWebAllowedObject(string name, object pObject)
        {
            // EdgeHTML-only API. There is no direct WebView2 equivalent in the
            // WinUI 2 control (AddHostObjectToScript is not projected for UWP),
            // so the JS console / xevents features degrade to Chromium's own
            // context menu + DevTools instead.
            Debug.WriteLine("[Onitor] AddWebAllowedObject('" + name + "') ignored on the Chromium engine.");
        }

        public void SetUserAgent(string userAgent)
        {
            if (_coreReady)
            {
                _core.Settings.UserAgent = userAgent;
            }
            else
            {
                _pendingUserAgent = userAgent; // applied before the first navigation completes init
            }
        }

        public async Task<string> InvokeScriptAsync(string scriptName, string[] arguments)
        {
            if (!_coreReady) return null;
            if (scriptName != "eval" || arguments == null || arguments.Length == 0)
            {
                Debug.WriteLine("[Onitor] ChromiumEngine only supports InvokeScriptAsync(\"eval\", [script]).");
                return null;
            }
            string raw = await _wv2.ExecuteScriptAsync(arguments[0]);
            return NormalizeScriptResult(raw);
        }

        public event Action<EngineNavStartingArgs> NavigationStarting;
        public event Action<EngineNavCompletedArgs> NavigationCompleted;
        public event Action<EngineContentLoadingArgs> ContentLoading;
        public event Action<EngineNavStartingArgs> FrameNavigationStarting;
        public event Action<EngineNavCompletedArgs> FrameNavigationCompleted;
        public event Action<EngineUriArgs> DOMContentLoaded;
        public event Action<EngineUriArgs> FrameDOMContentLoaded;
        public event Action<EngineNewWindowArgs> NewWindowRequested;
        public event Action<EnginePermissionArgs> PermissionRequested;
        public event Action<EngineMessageArgs> ScriptNotify;
        public event Action<EngineUriArgs> UnviewableContentIdentified;
        public event Action<EngineLongRunningScriptArgs> LongRunningScriptDetected;
        public event Action<object> ContainsFullScreenElementChanged;
        public event Action<string> EngineFailed;
    }

    /// <summary>
    /// Chooses the engine per app settings and platform capabilities.
    /// EngineMode: "Auto" (default, prefer WebView2) or "Legacy" (force EdgeHTML).
    /// </summary>
    internal static class BrowserViewFactory
    {
        public static BrowserView Create()
        {
            string mode = GlobalLocalSettings.EngineMode;
            bool modernOs = ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 7); // 1809+

            if (!string.Equals(mode, "Legacy", StringComparison.OrdinalIgnoreCase) && modernOs)
            {
                ChromiumEngine chromium = new ChromiumEngine();
                return new BrowserView(chromium);
            }

            return new BrowserView(new LegacyEngine());
        }
    }
}
