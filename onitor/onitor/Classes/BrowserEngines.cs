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
                    return WebViewPermissionType.Unspecified;
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
