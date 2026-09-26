using System;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.Web;

namespace onitor.Classes
{
    // ============================================================================
    //  BrowserView - dual-engine web view facade.
    //
    //  Onitor originally used the legacy EdgeHTML WebView control. That engine
    //  is frozen (EdgeHTML 18, 2018) and cannot run modern sites such as
    //  youtube.com. This class exposes the SAME API surface the app used on
    //  Windows.UI.Xaml.Controls.WebView, but renders with either:
    //
    //    * ChromiumEngine - Microsoft.UI.Xaml.Controls.WebView2 (WebView2 /
    //      Chromium), used whenever the OS + WebView2 runtime allow it
    //      (Windows 10 1809+, Xbox One dev mode as UWP, Xbox Series X|S).
    //      See https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/winui2
    //
    //    * LegacyEngine   - the original EdgeHTML WebView, used as fallback
    //      (older systems, runtime missing, or EngineMode == "Legacy").
    //
    //  All call sites that previously held a `WebView` now hold a `BrowserView`
    //  and keep compiling unchanged (Source, Navigate, GoBack, InvokeScriptAsync,
    //  events, ...). Engine differences (JSON-encoded script results,
    //  ms-appx-web:// mapping, user-agent handling, permission model) are
    //  normalised inside the engines.
    // ============================================================================

    /// <summary>Handler signature that keeps a typed sender, like TypedEventHandler.</summary>
    public delegate void EngineEventHandler<TSender, TArgs>(TSender sender, TArgs args);

    public class EngineNavStartingArgs
    {
        public Uri Uri;
        public bool Cancel;
    }

    public class EngineNavCompletedArgs
    {
        public Uri Uri;
        public bool IsSuccess;
        public WebErrorStatus WebErrorStatus = WebErrorStatus.Unknown;
    }

    public class EngineContentLoadingArgs
    {
        public Uri Uri;
    }

    public class EngineUriArgs
    {
        public Uri Uri;
    }

    public class EngineNewWindowArgs
    {
        public Uri Uri;
        public bool Handled;
    }

    public class EngineMessageArgs
    {
        public string Value;
    }

    public class EngineLongRunningScriptArgs
    {
        public TimeSpan ExecutionTime;
        public bool StopPageScriptExecution;
    }

    /// <summary>
    /// Mirrors Windows.UI.Xaml.Controls.WebViewPermissionRequest so the existing
    /// permission dialog flow in MainPage keeps working on both engines.
    /// </summary>
    public class EnginePermissionRequest
    {
        private readonly Action _allow;
        private readonly Action _deny;
        private readonly Action _defer;

        internal EnginePermissionRequest(WebViewPermissionType type, Uri uri, Action allow, Action deny, Action defer)
        {
            PermissionType = type;
            Uri = uri;
            _allow = allow;
            _deny = deny;
            _defer = defer;
        }

        public WebViewPermissionType PermissionType { get; private set; }
        public Uri Uri { get; private set; }

        public void Allow() { if (_allow != null) _allow(); }
        public void Deny() { if (_deny != null) _deny(); }
        public void Defer() { if (_defer != null) _defer(); }
    }

    public class EnginePermissionArgs
    {
        public EnginePermissionRequest PermissionRequest;
    }

    /// <summary>
    /// Mirrors the bits of Windows.UI.Xaml.Controls.WebViewSettings the app uses.
    /// IsJavaScriptEnabled maps to CoreWebView2.Settings.IsScriptEnabled on
    /// Chromium; IsIndexedDBEnabled only exists on the legacy engine (no-op on
    /// Chromium, which manages storage itself).
    /// </summary>
    public class BrowserSettings
    {
        private readonly Action _onChanged;

        internal BrowserSettings(Action onChanged)
        {
            _onChanged = onChanged;
        }

        private bool _isJavaScriptEnabled = true;
        private bool _isIndexedDBEnabled = true;

        public bool IsJavaScriptEnabled
        {
            get { return _isJavaScriptEnabled; }
            set { _isJavaScriptEnabled = value; if (_onChanged != null) _onChanged(); }
        }

        public bool IsIndexedDBEnabled
        {
            get { return _isIndexedDBEnabled; }
            set { _isIndexedDBEnabled = value; if (_onChanged != null) _onChanged(); }
        }
    }

    public class BrowserView : ContentControl
    {
        private readonly IBrowserEngine _engine;

        internal BrowserView(IBrowserEngine engine)
        {
            _engine = engine;
            HorizontalContentAlignment = HorizontalAlignment.Stretch;
            VerticalContentAlignment = VerticalAlignment.Stretch;
            Content = engine.View;

            engine.NavigationStarting += (a) => RaiseNavigationStarting(a);
            engine.NavigationCompleted += (a) => RaiseNavigationCompleted(a);
            engine.ContentLoading += (a) => RaiseContentLoading(a);
            engine.FrameNavigationStarting += (a) => RaiseFrameNavigationStarting(a);
            engine.FrameNavigationCompleted += (a) => RaiseFrameNavigationCompleted(a);
            engine.DOMContentLoaded += (a) => RaiseDOMContentLoaded(a);
            engine.FrameDOMContentLoaded += (a) => RaiseFrameDOMContentLoaded(a);
            engine.NewWindowRequested += (a) => RaiseNewWindowRequested(a);
            engine.PermissionRequested += (a) => RaisePermissionRequested(a);
            engine.ScriptNotify += (a) => RaiseScriptNotify(a);
            engine.UnviewableContentIdentified += (a) => RaiseUnviewableContentIdentified(a);
            engine.LongRunningScriptDetected += (a) => RaiseLongRunningScriptDetected(a);
            engine.ContainsFullScreenElementChanged += (a) => RaiseContainsFullScreenElementChanged(a);
            engine.EngineFailed += (m) => { var h = EngineFailed; if (h != null) h(this, m); };
        }

        // ----- engine information -----

        public bool IsChromium { get { return _engine.IsChromium; } }

        // ----- legacy WebView-compatible surface -----

        public Uri Source
        {
            get { return _engine.Source; }
            set { _engine.Source = value; }
        }

        public string DocumentTitle
        {
            get { return _engine.DocumentTitle; }
        }

        public bool CanGoBack { get { return _engine.CanGoBack; } }
        public bool CanGoForward { get { return _engine.CanGoForward; } }
        public bool ContainsFullScreenElement { get { return _engine.ContainsFullScreenElement; } }

        public BrowserSettings Settings { get { return _engine.Settings; } }

        public void Navigate(Uri source) { _engine.Navigate(source); }
        public void GoBack() { _engine.GoBack(); }
        public void GoForward() { _engine.GoForward(); }
        public void Refresh() { _engine.Refresh(); }
        public void Stop() { _engine.Stop(); }

        /// <summary>
        /// Legacy WebView API. Only scriptName "eval" is supported on the
        /// Chromium engine (that is the only form the app uses).
        /// </summary>
        public IAsyncOperation<string> InvokeScriptAsync(string scriptName, string[] arguments)
        {
            return _engine.InvokeScriptAsync(scriptName, arguments).AsAsyncOperation();
        }

        /// <summary>
        /// Legacy-WebView-only API (WinRT object injection via [AllowForWeb]).
        /// No direct equivalent in WebView2 - it becomes a no-op on Chromium;
        /// the JS console / xevents features therefore only work on the legacy
        /// engine. Chromium's own context menu and dev tools cover the gap.
        /// </summary>
        public void AddWebAllowedObject(string name, object pObject)
        {
            _engine.AddWebAllowedObject(name, pObject);
        }

        /// <summary>
        /// Sets the per-view user agent. Legacy engine: process-wide urlmon
        /// override (original behaviour). Chromium engine: CoreWebView2
        /// Settings.UserAgent (per-view, applied before the next navigation).
        /// </summary>
        public void SetUserAgent(string userAgent)
        {
            _engine.SetUserAgent(userAgent);
        }

        // ----- media / element helpers (previously WebViewExtensions) -----

        public async void PlayMedia()
        {
            string playScript =
                @"
                    if(document.body.getElementsByTagName('video').length > 0)
                    {
                        document.body.getElementsByTagName('video')[0].play();
                    }
                    else if(document.body.getElementsByTagName('audio').length > 0)
                    {
                        document.body.getElementsByTagName('audio')[0].play();
                    }
                ";

            await InvokeScriptAsync("eval", new string[] { playScript });
        }

        public async void PauseMedia()
        {
            string pauseScript =
                @"
                    if(document.body.getElementsByTagName('video').length > 0)
                    {
                        document.body.getElementsByTagName('video')[0].pause();
                    }
                    else if(document.body.getElementsByTagName('audio').length > 0)
                    {
                        document.body.getElementsByTagName('audio')[0].pause();
                    }
                ";

            await InvokeScriptAsync("eval", new string[] { pauseScript });
        }

        public async Task<bool> IsPlayingVideo()
        {
            string scriptJS = await InvokeScriptAsync("eval", new string[] { @"
                if(document.body.getElementsByTagName('video').length > 0) {
                    var video = document.body.getElementsByTagName('video')[0];
                    if(video.currentTime > 0 && !video.paused && !video.ended && video.readyState > 2) { 'true'; };
                }
            " });

            return scriptJS == "true";
        }

        public async Task<bool> IsPlayingAudio()
        {
            string scriptJS = await InvokeScriptAsync("eval", new string[] { @"
                if(document.body.getElementsByTagName('audio').length > 0) {
                    var audio = document.body.getElementsByTagName('audio')[0];
                    if(audio.currentTime > 0 && !audio.paused && !audio.ended && audio.readyState > 2) { 'true'; };
                }
            " });

            return scriptJS == "true";
        }

        public string Domain(string sub)
        {
            string[] subdomain = sub.Split('.');
            string domain = sub;
            if (domain.Contains("."))
            {
                domain = string.Format("{0}.{1}", subdomain[subdomain.Length - 2], subdomain[subdomain.Length - 1]);
            }

            return domain;
        }

        public async Task<bool> IsFocusedElementEditiable()
        {
            string jsEdit = await InvokeScriptAsync("eval", new string[] { @"
                const elem = document.activeElement;
                var textControls = ['text', 'search', 'url'];
                if(elem.tagName === 'TEXTAREA' || (elem.tagName === 'INPUT' && textControls.indexOf(elem.type) != -1))
                {
                    'true';
                }
            " });

            return jsEdit == "true";
        }

        public async void FocusOnPointer(int X, int Y)
        {
            await InvokeScriptAsync("eval", new string[] { @" document.elementFromPoint(" + X + ", " + Y + ").focus(); " });
        }

        public async Task<string> ActiveElement()
        {
            return await InvokeScriptAsync("eval", new string[] { @" document.activeElement " });
        }

        public async Task<string> ActiveElementTagName()
        {
            return await InvokeScriptAsync("eval", new string[] { @" document.activeElement.tagName " });
        }

        public async Task<string> ActiveElementLink()
        {
            if (await ActiveElementTagName() == "A" || await ActiveElementTagName() == "a")
            {
                string link = await InvokeScriptAsync("eval", new string[] { @" document.activeElement.href.toString() " });
                if (link != null && link.Length > 0)
                {
                    return link;
                }
            }
            return null;
        }

        public async Task<string> SelectionText()
        {
            string selectionText;
            if (await IsFocusedElementEditiable())
            {
                selectionText = await InvokeScriptAsync("eval", new string[] { @"
                    var elem = document.activeElement;
                    elem.value.substring(elem.selectionStart, elem.selectionEnd);
                " });
            }
            else
            {
                selectionText = await InvokeScriptAsync("eval", new string[] { @"
                     window.getSelection().toString();
                " });
            }

            return selectionText;
        }

        // ----- events (legacy WebView names) -----

        public event EngineEventHandler<BrowserView, EngineNavStartingArgs> NavigationStarting;
        public event EngineEventHandler<BrowserView, EngineNavCompletedArgs> NavigationCompleted;
        public event EngineEventHandler<BrowserView, EngineContentLoadingArgs> ContentLoading;
        public event EngineEventHandler<BrowserView, EngineNavStartingArgs> FrameNavigationStarting;
        public event EngineEventHandler<BrowserView, EngineNavCompletedArgs> FrameNavigationCompleted;
        public event EngineEventHandler<BrowserView, EngineUriArgs> DOMContentLoaded;
        public event EngineEventHandler<BrowserView, EngineUriArgs> FrameDOMContentLoaded;
        public event EngineEventHandler<BrowserView, EngineNewWindowArgs> NewWindowRequested;
        public event EngineEventHandler<BrowserView, EnginePermissionArgs> PermissionRequested;
        public event EngineEventHandler<object, EngineMessageArgs> ScriptNotify;
        public event EngineEventHandler<BrowserView, EngineUriArgs> UnviewableContentIdentified;
        public event EngineEventHandler<BrowserView, EngineLongRunningScriptArgs> LongRunningScriptDetected;
        public event EngineEventHandler<BrowserView, object> ContainsFullScreenElementChanged;

        /// <summary>Raised when the underlying engine dies unexpectedly (e.g. WebView2 process failure on memory-constrained consoles).</summary>
        public event EngineEventHandler<BrowserView, string> EngineFailed;

        private void RaiseNavigationStarting(EngineNavStartingArgs a) { var h = NavigationStarting; if (h != null) h(this, a); }
        private void RaiseNavigationCompleted(EngineNavCompletedArgs a) { var h = NavigationCompleted; if (h != null) h(this, a); }
        private void RaiseContentLoading(EngineContentLoadingArgs a) { var h = ContentLoading; if (h != null) h(this, a); }
        private void RaiseFrameNavigationStarting(EngineNavStartingArgs a) { var h = FrameNavigationStarting; if (h != null) h(this, a); }
        private void RaiseFrameNavigationCompleted(EngineNavCompletedArgs a) { var h = FrameNavigationCompleted; if (h != null) h(this, a); }
        private void RaiseDOMContentLoaded(EngineUriArgs a) { var h = DOMContentLoaded; if (h != null) h(this, a); }
        private void RaiseFrameDOMContentLoaded(EngineUriArgs a) { var h = FrameDOMContentLoaded; if (h != null) h(this, a); }
        private void RaiseNewWindowRequested(EngineNewWindowArgs a) { var h = NewWindowRequested; if (h != null) h(this, a); }
        private void RaisePermissionRequested(EnginePermissionArgs a) { var h = PermissionRequested; if (h != null) h(this, a); }
        private void RaiseScriptNotify(EngineMessageArgs a) { var h = ScriptNotify; if (h != null) h(this, a); }
        private void RaiseUnviewableContentIdentified(EngineUriArgs a) { var h = UnviewableContentIdentified; if (h != null) h(this, a); }
        private void RaiseLongRunningScriptDetected(EngineLongRunningScriptArgs a) { var h = LongRunningScriptDetected; if (h != null) h(this, a); }
        private void RaiseContainsFullScreenElementChanged(object a) { var h = ContainsFullScreenElementChanged; if (h != null) h(this, a); }
    }
}
