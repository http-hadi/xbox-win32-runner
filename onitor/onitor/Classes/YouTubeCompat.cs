using System;
using Windows.Storage;

namespace onitor.Classes
{
    /// <summary>
    /// YouTube compatibility for the legacy EdgeHTML engine (fallback path).
    ///
    /// Modern youtube.com serves ES2019+ JavaScript that EdgeHTML 18 cannot
    /// even parse, and YouTube has not shipped a legacy bundle since Edge
    /// Legacy was retired in 2021. On the Chromium engine this class is never
    /// used - native YouTube simply works. On the EdgeHTML fallback we instead:
    ///
    ///   1. Rewrite youtube.com URLs to the TV ("leanback") interface
    ///      (www.youtube.com/tv), which is built for underpowered TV browsers
    ///      and remote/gamepad navigation (ideal on Xbox).
    ///   2. Spoof a Smart-TV user agent so YouTube serves the TV bundle.
    ///   3. Force H.264 video (block VP8/VP9/AV1 and >30fps) because Xbox One /
    ///      phone hardware has no VP9 decode - h264ify-style, see the MIT
    ///      licensed ClassesJS/h264ify originally bundled with this app.
    ///
    /// Optional Invidious fallback: set the local setting "InvidiousInstance"
    /// (e.g. "https://inv.example.com") and "YouTubeMode" = "invidious" to be
    /// redirected to a self-hosted server-rendered instance instead.
    /// </summary>
    internal static class YouTubeCompat
    {
        /// <summary>Smart-TV user agent that receives the leanback TV bundle.</summary>
        public const string TvUserAgent =
            "Mozilla/5.0 (SMART-TV; LINUX; Tizen 7.0) AppleWebKit/537.36 (KHTML, like Gecko) Version/7.0 TV Safari/537.36";

        /// <summary>
        /// Forces H.264 / caps 30fps by making VP8/VP9/AV1 mime types report as
        /// unsupported (h264ify technique, (c) 2015 erkserkserks, MIT).
        /// Injected on every YouTube page load on the legacy engine.
        /// </summary>
        public const string H264ForceScript = @"
            (function () {
              var BLOCKED = ['webm', 'vp8', 'vp9', 'av01', 'av1'];
              function wrapper(orig) {
                return function (t) {
                  if (t === undefined || t === null) return '';
                  t = String(t);
                  for (var i = 0; i < BLOCKED.length; i++) { if (t.indexOf(BLOCKED[i]) !== -1) return ''; }
                  var m = /framerate=(\d+)/.exec(t);
                  if (m && parseInt(m[1], 10) > 30) return '';
                  try { return orig(t); } catch (e) { return ''; }
                };
              }
              try {
                var v = document.createElement('video');
                var proto = Object.getPrototypeOf(v);
                if (proto && proto.canPlayType) proto.canPlayType = wrapper(proto.canPlayType.bind(v));
              } catch (e) { }
              try {
                var mse = window.MediaSource;
                if (mse && mse.isTypeSupported) mse.isTypeSupported = wrapper(mse.isTypeSupported.bind(mse));
              } catch (e) { }
            })();";

        public static bool IsYouTubeUrl(Uri url)
        {
            if (url == null || url.Host == null) return false;
            string host = url.Host.ToLowerInvariant();
            return host == "youtube.com" || host == "www.youtube.com" || host == "m.youtube.com"
                || host == "music.youtube.com" || host == "youtube-nocookie.com" || host == "www.youtube-nocookie.com"
                || host == "youtu.be" || host == "www.youtu.be";
        }

        /// <summary>True when the user configured an Invidious instance redirect.</summary>
        public static bool UseInvidiousRedirect()
        {
            try
            {
                var settings = ApplicationData.Current.LocalSettings;
                string mode = settings.Values["YouTubeMode"] as string;
                string instance = settings.Values["InvidiousInstance"] as string;
                return string.Equals(mode, "invidious", StringComparison.OrdinalIgnoreCase)
                    && !string.IsNullOrEmpty(instance);
            }
            catch (Exception) { return false; }
        }

        public static Uri GetInvidiousInstance()
        {
            try
            {
                string instance = ApplicationData.Current.LocalSettings.Values["InvidiousInstance"] as string;
                if (!string.IsNullOrEmpty(instance))
                {
                    return new Uri(instance.TrimEnd('/') + "/");
                }
            }
            catch (Exception) { }
            return null;
        }

        /// <summary>
        /// Rewrites any YouTube URL to the TV interface. Returns false when the
        /// URL is already a /tv URL (idempotent) or not a YouTube URL.
        /// watch / shorts / live / embed / youtu.be all map to /tv#/watch?v=ID.
        /// </summary>
        public static bool TryRewriteToTv(Uri url, out Uri tvUrl)
        {
            tvUrl = null;
            if (!IsYouTubeUrl(url)) return false;

            string path = url.AbsolutePath;
            if (path.StartsWith("/tv", StringComparison.OrdinalIgnoreCase))
            {
                return false; // already the TV UI - let it load
            }

            string videoId = null;

            if (url.Host.EndsWith("youtu.be", StringComparison.OrdinalIgnoreCase))
            {
                if (url.Segments.Length > 1)
                {
                    videoId = url.Segments[1].TrimEnd('/');
                }
            }
            else if (path.StartsWith("/watch", StringComparison.OrdinalIgnoreCase))
            {
                videoId = GetQueryParam(url, "v");
            }
            else if (path.StartsWith("/shorts/") || path.StartsWith("/live/") || path.StartsWith("/embed/") || path.StartsWith("/v/"))
            {
                if (url.Segments.Length > 2)
                {
                    videoId = url.Segments[2].TrimEnd('/');
                }
            }
            else if (path.StartsWith("/redirect", StringComparison.OrdinalIgnoreCase))
            {
                // YouTube redirect wrapper: /redirect?q=<encoded target>
                string target = GetQueryParam(url, "q");
                if (!string.IsNullOrEmpty(target) && Uri.IsWellFormedUriString(target, UriKind.Absolute))
                {
                    Uri inner = new Uri(target);
                    if (IsYouTubeUrl(inner))
                    {
                        return TryRewriteToTv(inner, out tvUrl);
                    }
                }
            }

            string fragment = string.IsNullOrEmpty(videoId)
                ? string.Empty
                : "#/watch?v=" + Uri.EscapeDataString(videoId);

            tvUrl = new Uri("https://www.youtube.com/tv" + fragment);
            return true;
        }

        private static string GetQueryParam(Uri url, string key)
        {
            string query = url.Query == null ? string.Empty : url.Query.TrimStart('?');
            if (query.Length == 0) return null;
            string[] pairs = query.Split('&');
            for (int i = 0; i < pairs.Length; i++)
            {
                string[] kv = pairs[i].Split('=');
                if (kv.Length >= 1 && string.Equals(kv[0], key, StringComparison.OrdinalIgnoreCase))
                {
                    return kv.Length > 1 ? Uri.UnescapeDataString(kv[1]) : string.Empty;
                }
            }
            return null;
        }
    }
}
