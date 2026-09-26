using System;
using System.Linq;
using System.Threading.Tasks;
using UnitedCodebase.Classes;
using UnitedCodebase.WinRT;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.Storage;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.System.Threading;
using Windows.Networking.Connectivity;
using Windows.Web;
using onitor.Classes;
using System.Diagnostics;
using Windows.UI.Popups;
using WebViewComponents;
using SharedLibrary;

namespace Onitor
{
    /// <summary>
    /// Per-tab browser controller. Now engine-agnostic: the actual web content
    /// is rendered by the BrowserView facade (Classes/BrowserView.cs), which
    /// uses Microsoft's WebView2 (Chromium) when available and falls back to
    /// the legacy EdgeHTML WebView otherwise (see Classes/BrowserEngines.cs).
    /// </summary>
    public class WebViewCore
    {
        private BrowserView _webView;
        private string _pageZoom;

        ApplicationDataContainer localSettings = ApplicationData.Current.LocalSettings;

        WebieHandler taskHandler = new WebieHandler();

        public bool IsPageHaveMedia = false;
        public bool IsPageLoaded = false;

        public bool SupportsOnitorTheme = false;

        public Uri URL;

        public static int TotalAdsBlocked;
        public static int CurrentSessionAdsBlocked;

        public WebViewCore()
        {
            _webView = BrowserViewFactory.Create();
            _webView.EngineFailed += (sender, message) =>
            {
                Debug.WriteLine("[Onitor] Engine failure: " + message);
            };

            PageZoom = "100%";

            if (ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 3))
            {
                // Context menu is suppressed on the app level (see MainPage
                // CurrentWebView_ContextRequested) and the engines expose their
                // own handling.
                _webView.ContextFlyout = null;
            }

            _webView.Loaded += WebView_Loaded;
            _webView.NavigationStarting += _webView_NavigationStarting;
            _webView.FrameNavigationStarting += _webView_FrameNavigationStarting;
            _webView.ContentLoading += _webView_ContentLoading;
            _webView.FrameNavigationCompleted += _webView_FrameNavigationCompleted;
            _webView.NavigationCompleted += _webView_NavigationCompleted;
            _webView.ScriptNotify += _webView_ScriptNotify;

            _webView.Settings.IsIndexedDBEnabled = true;

            URL = _webView.Source;

            taskHandler.ReceivedData += TaskHandler_ReceivedData;
        }

        private void _webView_ScriptNotify(object sender, EngineMessageArgs e)
        {
            Debug.WriteLine("Script Notify from _webView: " + e.Value);
        }

        private async void TaskHandler_ReceivedData(string e)
        {
            await ThreadPool.RunAsync((WorkItemHandler) =>
            {
                if (e == "PageHaveMedia")
                {
                    IsPageHaveMedia = true;
                }

                if (e == "SupportingTheme")
                {
                    SupportsOnitorTheme = true;

                    string themeSetting = localSettings.Values["WebViewTheme"].ToString();
                    if (themeSetting == "Default")
                    {
                        var DefaultTheme = new Windows.UI.ViewManagement.UISettings();
                        string uiTheme = DefaultTheme.GetColorValue(Windows.UI.ViewManagement.UIColorType.Background).ToString();
                        if (uiTheme == "#FF000000")
                        {
                            Theme = WebViewTheme.Dark;
                        }
                        else if (uiTheme == "#FFFFFFFF")
                        {
                            Theme = WebViewTheme.Light;
                        }
                    }
                    else if (themeSetting == "Dark")
                    {
                        Theme = WebViewTheme.Dark;
                    }
                    else if (themeSetting == "Light")
                    {
                        Theme = WebViewTheme.Light;
                    }
                }
            });
        }

        private WebViewTheme _theme = WebViewTheme.NotSupported;
        public WebViewTheme Theme
        {
            get
            {
                if (SupportsOnitorTheme)
                {
                    return _theme;
                }
                else
                {
                    _theme = WebViewTheme.NotSupported;
                }

                return _theme;
            }
            set
            {
                _theme = value;

                ChangeOnitorTheme();
            }
        }

        async void ChangeOnitorTheme()
        {
            if (_theme != WebViewTheme.NotSupported)
            {
                if (_theme == WebViewTheme.Dark)
                {
                    string theme = @"
                        var t = document.querySelectorAll('*');
                        for (var i=0; i < t.length; i++) {
                            t[i].setAttribute('onitor-theme', 'dark');
                        }
                        ";

                    await Task.Run(async () =>
                        await _webView.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                            AsyncEngine.ExecuteString(_webView.InvokeScriptAsync("eval", new[] { theme }))
                        )
                    );
                }
                else
                {
                    string theme = @"
                        var t = document.querySelectorAll('*');
                        for (var i=0; i < t.length; i++) {
                            t[i].setAttribute('onitor-theme', 'light');
                        }
                        ";

                    await Task.Run(async () =>
                        await _webView.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
                            AsyncEngine.ExecuteString(_webView.InvokeScriptAsync("eval", new[] { theme }))
                    ));
                }
            }
        }

        private void _webView_FrameNavigationStarting(BrowserView sender, EngineNavStartingArgs args)
        {
            //CurrentSessionAdsBlocked = 0;
            var url = args.Uri;

            var allowed = BlockedDomains.IsUrlAllowed(url);

            // fix whitelist stuff here
            if (!allowed /* && !whitelisted*/)
            {

                //CurrentSessionAdsBlocked++;
                //TotalAdsBlocked++;
                args.Cancel = true;
                //Debug.WriteLine("[BLOCKED] " + url);
                //Debug.WriteLine("Blocked Ads: " + CurrentSessionAdsBlocked);
            }

        }

        string UserSelectedUserAgent { get; set; }
        private void _webView_NavigationStarting(BrowserView sender, EngineNavStartingArgs args)
        {

            //TODO: Fix some pages infinite loading and freezing UI

            if (args.Uri != null)
            {
                // The UA spoofing below rewrites the UA to old Edge/Chrome
                // strings - that was needed on the frozen EdgeHTML engine, but
                // would actively hurt the modern Chromium engine (sites would
                // serve legacy bundles again). It is therefore legacy-only.
                if (!sender.IsChromium)
                {
                    //setting user agent for mobile

                    string DeviceVersion = localSettings.Values["DeviceVersion"].ToString();
                    var result = localSettings.Values["SavedUserAgent"] as string;
                    var predefinedAgent = WhitelistedPages.CheckPageUserAgent(args.Uri.Host);

                    if (predefinedAgent == null || predefinedAgent == "")
                    {
                        predefinedAgent = result;
                    }
                    // Debug.WriteLine("Predefined agent: " + predefinedAgent + "  Domain: " + args.Uri.Host);
                    if (predefinedAgent != null)
                    {
                        if (DeviceVersion == "Mobile")
                        {
                            UserSelectedUserAgent = UserAgent.ModifyUserAgent(false, predefinedAgent);
                        }
                        else
                        {

                            UserSelectedUserAgent = UserAgent.ModifyUserAgent(true, predefinedAgent);

                        }
                    }
                    else
                    {
                        if (DeviceVersion == "Mobile")
                        {
                            // UserAgentManager.ChangeUserAgent(UserAgentManager.DeviceMode.Mobile);
                            if (result != null)
                            {

                                UserSelectedUserAgent = UserAgent.ModifyUserAgent(false, result);
                            }
                            else
                            {
                                UserSelectedUserAgent = UserAgent.ModifyUserAgent(false, "Windows");
                            }

                        }
                        else
                        {
                            if (result != null)
                            {

                                UserSelectedUserAgent = UserAgent.ModifyUserAgent(true, result);
                            }
                            else
                            {
                                UserSelectedUserAgent = UserAgent.ModifyUserAgent(true, "Windows");
                            }
                        }
                    }
                    UserAgent.SetUserAgent(UserSelectedUserAgent);
                }

                // --- YouTube compatibility (legacy engine only) ---------------
                // EdgeHTML cannot run modern youtube.com. Route to the TV UI
                // with a Smart-TV user agent (see Classes/YouTubeCompat.cs).
                // On the Chromium engine YouTube runs natively - do nothing.
                if (!sender.IsChromium && YouTubeCompat.IsYouTubeUrl(args.Uri))
                {
                    BlockedDomains.IsYouTubeContext = true;

                    if (YouTubeCompat.UseInvidiousRedirect())
                    {
                        Uri instance = YouTubeCompat.GetInvidiousInstance();
                        if (instance != null)
                        {
                            Uri invidiousUri = new Uri(instance, args.Uri.PathAndQuery);
                            args.Cancel = true;
                            sender.Source = invidiousUri;
                            URL = invidiousUri;
                            return;
                        }
                    }
                    else
                    {
                        sender.SetUserAgent(YouTubeCompat.TvUserAgent);

                        Uri tvUri;
                        if (YouTubeCompat.TryRewriteToTv(args.Uri, out tvUri))
                        {
                            args.Cancel = true;
                            sender.Source = tvUri;
                            URL = tvUri;
                            return;
                        }
                    }
                }
                else if (!YouTubeCompat.IsYouTubeUrl(args.Uri))
                {
                    BlockedDomains.IsYouTubeContext = false;
                }

                //redirecting to real page
                if (args.Uri.Scheme == "about" && args.Uri.Segments[0] == "home")
                {
                    args.Cancel = true;
                    sender.Source = new Uri("ms-appx-web://71330982-ba82-4d35-b5cb-3488eefb31ed/PagesHTML/Home.html");
                }


                URL = args.Uri;
            }

            IsPageLoaded = false;
            //_webView.AddWebAllowedObject("console", new ConsoleOverride());
            //_webView.AddWebAllowedObject("TaskHandler", taskHandler); //initializing Webie handler
        }

        private void _webView_ContentLoading(BrowserView sender, EngineContentLoadingArgs args)
        {
            if (args.Uri != null)
            {
                URL = args.Uri;
            }
        }

        private async void _webView_FrameNavigationCompleted(BrowserView sender, EngineNavCompletedArgs args)
        {
            IsPageHaveMedia = false;

            if (WhitelistedPages.CheckPageSettings(args.Uri.Host, true, false))
            {
                await sender.InvokeScriptAsync("eval", new string[] { BlockedDomains.XHRBlocking() });

            }
            if (WhitelistedPages.CheckPageSettings(args.Uri.Host, false, true))
            {
                await sender.InvokeScriptAsync("eval", new string[] { BlockedDomains.ADSProtectionScript() });

            }

            // Legacy engine + YouTube: force H.264 streams (Xbox One / phones
            // have no VP9 hardware decode). Chromium negotiates this itself.
            if (!sender.IsChromium && args.Uri != null && YouTubeCompat.IsYouTubeUrl(args.Uri))
            {
                try
                {
                    await sender.InvokeScriptAsync("eval", new string[] { YouTubeCompat.H264ForceScript });
                }
                catch (Exception) { }
            }

            // Chromium engine: bridge window.external.notify() to WebView2's
            // postMessage so pages calling the legacy API still reach
            // ScriptNotify.
            if (sender.IsChromium && args.Uri != null)
            {
                try
                {
                    await sender.InvokeScriptAsync("eval", new string[] { @"
                        (function () {
                          try {
                            if (window.chrome && window.chrome.webview && window.external && !window.external.notify) {
                              window.external.notify = function (m) { window.chrome.webview.postMessage(String(m)); };
                            }
                          } catch (e) { }
                        })();" });
                }
                catch (Exception) { }
            }

        }

        Uri lastPage;
        private async void _webView_NavigationCompleted(BrowserView sender, EngineNavCompletedArgs args)
        {


            IsPageLoaded = true;
            IsPageHaveMedia = false;

            SupportsOnitorTheme = false;

            if (args.Uri != null)
            {
                URL = args.Uri;
            }

            //initializing elements for manipulation
            try
            {
                await sender.InvokeScriptAsync("eval", new[] { "document.body.style.zoom = '" + PageZoom + "';" });
            }
            catch (Exception) { }

            //error pages
            if (!args.IsSuccess)
            {
                if (args.WebErrorStatus == WebErrorStatus.NotFound || args.WebErrorStatus == WebErrorStatus.CannotConnect)
                {
                    if (NetworkInformation.GetInternetConnectionProfile() == null
                        || NetworkInformation.GetInternetConnectionProfile().GetNetworkConnectivityLevel() == NetworkConnectivityLevel.None)
                    {
                        if (sender.IsChromium)
                        {
                            sender.Source = new Uri("ms-appx-web:///PagesHTML/NoInternet.html#" + args.Uri);
                        }
                        else if ((lastPage != null && lastPage == args.Uri) || !ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 6))
                        {
                            await sender.InvokeScriptAsync("eval", new[] { "window.location.replace('ms-appx-web:///PagesHTML/NoInternet.html#' + location.href);" });
                        }
                        else
                        {
                            sender.Source = new Uri("ms-appx-web:///PagesHTML/NoInternet.html#" + args.Uri);
                        }
                    }
                    else if (NetworkInformation.GetInternetConnectionProfile().GetNetworkConnectivityLevel() == NetworkConnectivityLevel.InternetAccess
                        || NetworkInformation.GetInternetConnectionProfile().GetNetworkConnectivityLevel() == NetworkConnectivityLevel.ConstrainedInternetAccess
                        || NetworkInformation.GetInternetConnectionProfile().GetNetworkConnectivityLevel() == NetworkConnectivityLevel.LocalAccess)
                    {
                        if (sender.IsChromium)
                        {
                            sender.Source = new Uri("ms-appx-web:///PagesHTML/NotFound.html#" + args.Uri);
                        }
                        else if ((lastPage != null && lastPage == args.Uri) || !ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 6))
                        {
                            await sender.InvokeScriptAsync("eval", new[] { "window.location.replace('ms-appx-web:///PagesHTML/NotFound.html#' + location.href);" });
                        }
                        else
                        {
                            sender.Source = new Uri("ms-appx-web:///PagesHTML/NotFound.html#" + args.Uri);
                        }
                    }
                }
            }

            lastPage = args.Uri;

        }

        private void WebView_Loaded(object sender, RoutedEventArgs e)
        {
            IsWebViewLoaded = true;
        }

        public bool IsWebViewLoaded { get; private set; } = false;

        public BrowserView WebView
        {
            get
            {
                return _webView;
            }
        }

        public string PageZoom
        {
            set
            {
                _pageZoom = value;
            }
            get
            {
                return _pageZoom;
            }
        }

        public enum WebViewTheme
        {
            NotSupported,
            Light,
            Dark
        }
    }
}
