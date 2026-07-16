// shims/winmm/WinmmShim.cpp
// Win32 winmm shim layer. timeGetTime uses the real GetTickCount; mmio is
// passed through to the real UWP mmio with path translation; everything
// else (waveOut / midiOut / mciSendString / joyGetPosEx) is stubbed to
// fail gracefully because UWP doesn't expose legacy media APIs.

#include "UwpSdkIncludes.h"


#include <string>
#include <atomic>
#include <mutex>
#include <map>
#include <cstdlib>

#include "../ShimRegistry.h"
#include "../kernel32/PathTranslator.h"

// PATH_MAX isn't guaranteed on MSVC (it's a POSIX/ Linux constant). Use a
// safe local fallback so the mmio scratch buffer isn't sized 0.
#ifndef PATH_MAX
#ifdef MAX_PATH
#define PATH_MAX MAX_PATH
#else
#define PATH_MAX 260
#endif
#endif

#ifndef MAXERRORLENGTH
#define MAXERRORLENGTH 256
#endif
#ifndef MCI_WAIT
#define MCI_WAIT 0x00000002
#endif
#ifndef SND_FILENAME
#define SND_FILENAME 0x00020000
#endif
#ifndef SND_RESOURCE
#define SND_RESOURCE 0x00040004
#endif
#ifndef SND_ASYNC
#define SND_ASYNC 0x0001
#endif
#ifndef SND_SYNC
#define SND_SYNC 0x0000
#endif
#ifndef SND_NODEFAULT
#define SND_NODEFAULT 0x0002
#endif
#ifndef JOYERR_NOERROR
#define JOYERR_NOERROR 0
#endif
#ifndef JOYERR_PARMS
#define JOYERR_PARMS 165
#endif
#ifndef JOYERR_NOCANDO
#define JOYERR_NOCANDO 166
#endif
#ifndef JOYERR_UNPLUGGED
#define JOYERR_UNPLUGGED 167
#endif
#ifndef MMIOERR_CANNOTWRITE
#define MMIOERR_CANNOTWRITE 0x200
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------
extern "C" DWORD __stdcall Shim_timeGetTime() {
    return ::GetTickCount();
}
extern "C" MMRESULT __stdcall Shim_timeBeginPeriod(UINT) { return TIMERR_NOERROR; }
extern "C" MMRESULT __stdcall Shim_timeEndPeriod(UINT) { return TIMERR_NOERROR; }
extern "C" MMRESULT __stdcall Shim_timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
    if (!ptc || cbtc < sizeof(TIMECAPS)) return TIMERR_NOCANDO;
    ptc->wPeriodMin = 1;
    ptc->wPeriodMax = 1000;
    return TIMERR_NOERROR;
}

static std::atomic<uint32_t> g_nextTimerId{0x1000};
static std::mutex& TimerMutex() {
    static std::mutex m;
    return m;
}
static std::map<UINT, bool>& TimerTable() {
    static std::map<UINT, bool> t;
    return t;
}

extern "C" MMRESULT __stdcall Shim_timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK fptc,
                                                 DWORD_PTR dwUser, UINT fuEvent) {
    (void)uDelay; (void)uResolution; (void)fptc; (void)dwUser; (void)fuEvent;
    UINT id = (UINT)g_nextTimerId.fetch_add(1);
    std::lock_guard<std::mutex> lk(TimerMutex());
    TimerTable()[id] = true;
    return (MMRESULT)id;
}
extern "C" MMRESULT __stdcall Shim_timeKillEvent(UINT uTimerID) {
    std::lock_guard<std::mutex> lk(TimerMutex());
    TimerTable().erase(uTimerID);
    return TIMERR_NOERROR;
}

// ---------------------------------------------------------------------------
// waveOut / midiOut — no devices available
// ---------------------------------------------------------------------------
extern "C" UINT __stdcall Shim_waveOutGetNumDevs() { return 0; }
extern "C" UINT __stdcall Shim_waveOutOpen(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD) {
    return MMSYSERR_NODRIVER;
}
extern "C" UINT __stdcall Shim_waveOutClose(HWAVEOUT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutPrepareHeader(HWAVEOUT, LPWAVEHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutUnprepareHeader(HWAVEOUT, LPWAVEHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutWrite(HWAVEOUT, LPWAVEHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutReset(HWAVEOUT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutPause(HWAVEOUT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutRestart(HWAVEOUT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutGetVolume(HWAVEOUT, LPDWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutSetVolume(HWAVEOUT, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_waveOutGetDevCapsW(UINT, LPWAVEOUTCAPSW, UINT) { return MMSYSERR_NODRIVER; }

extern "C" UINT __stdcall Shim_midiOutOpen(LPHMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_midiOutClose(HMIDIOUT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_midiOutShortMsg(HMIDIOUT, DWORD) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_midiOutLongMsg(HMIDIOUT, LPMIDIHDR, UINT) { return MMSYSERR_NODRIVER; }
extern "C" UINT __stdcall Shim_midiOutReset(HMIDIOUT) { return MMSYSERR_NODRIVER; }

// ---------------------------------------------------------------------------
// mmio — pass through with path translation (mmio is allowed in UWP)
// ---------------------------------------------------------------------------
extern "C" FOURCC __stdcall Shim_mmioStringToFOURCCW(LPCWSTR sz, UINT uFlags) {
    return ::mmioStringToFOURCCW(sz, uFlags);
}
extern "C" HMMIO __stdcall Shim_mmioOpenW(LPWSTR pszFileName, LPMMIOINFO lpmmioinfo, DWORD fdwOpen) {
    if (pszFileName && *pszFileName) {
        std::wstring real = PathTranslator::Instance().TranslateToReal(pszFileName);
        // mmioOpenW takes a non-const buffer it can mutate; copy into a local
        // scratch and pass to the real function.
        static thread_local wchar_t scratch[PATH_MAX];
        size_t n = real.size();
        if (n >= PATH_MAX) n = PATH_MAX - 1;
        std::memcpy(scratch, real.c_str(), n * sizeof(wchar_t));
        scratch[n] = L'\0';
        return ::mmioOpenW(scratch, lpmmioinfo, fdwOpen);
    }
    return ::mmioOpenW(pszFileName, lpmmioinfo, fdwOpen);
}
extern "C" MMRESULT __stdcall Shim_mmioClose(HMMIO hmmio, UINT fuClose) { return ::mmioClose(hmmio, fuClose); }
extern "C" LONG __stdcall Shim_mmioRead(HMMIO hmmio, HPSTR pch, LONG cch) { return ::mmioRead(hmmio, pch, cch); }
extern "C" LONG __stdcall Shim_mmioWrite(HMMIO hmmio, const char* pch, LONG cch) { return ::mmioWrite(hmmio, pch, cch); }
extern "C" LONG __stdcall Shim_mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin) { return ::mmioSeek(hmmio, lOffset, iOrigin); }
extern "C" MMRESULT __stdcall Shim_mmioGetInfo(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT fuInfo) {
    return ::mmioGetInfo(hmmio, lpmmioinfo, fuInfo);
}
extern "C" MMRESULT __stdcall Shim_mmioSetInfo(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT fuInfo) {
    return ::mmioSetInfo(hmmio, lpmmioinfo, fuInfo);
}
extern "C" MMRESULT __stdcall Shim_mmioCreateChunk(HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuCreate) {
    return ::mmioCreateChunk(hmmio, pmmcki, fuCreate);
}
extern "C" MMRESULT __stdcall Shim_mmioAscend(HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuAscend) {
    return ::mmioAscend(hmmio, pmmcki, fuAscend);
}
extern "C" MMRESULT __stdcall Shim_mmioDescend(HMMIO hmmio, LPMMCKINFO pmmcki, const MMCKINFO* pmmckiParent, UINT fuDescend) {
    return ::mmioDescend(hmmio, pmmcki, pmmckiParent, fuDescend);
}

// ---------------------------------------------------------------------------
// MCI / PlaySound — no audio devices
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_PlaySoundW(LPCWSTR, HMODULE, DWORD) { return FALSE; }
extern "C" UINT __stdcall Shim_mciSendStringW(LPCWSTR, LPWSTR, UINT, HWND) { return 0; }

// ---------------------------------------------------------------------------
// Joystick — no legacy joystick devices (XInput covers this)
// ---------------------------------------------------------------------------
extern "C" UINT __stdcall Shim_joyGetNumDevs() { return 0; }
extern "C" UINT __stdcall Shim_joyGetDevCapsW(UINT, LPJOYCAPSW, UINT) { return MMSYSERR_NODRIVER; }
extern "C" BOOL __stdcall Shim_joyGetPosEx(UINT, LPJOYINFOEX) { return FALSE; }

}  // namespace xwr

// ===========================================================================
// Registration — also covers "winmm.dll" alias.
// ===========================================================================
REGISTER_SHIM("winmm", "timeGetTime", (FARPROC)&xwr::Shim_timeGetTime);
REGISTER_SHIM("winmm", "timeBeginPeriod", (FARPROC)&xwr::Shim_timeBeginPeriod);
REGISTER_SHIM("winmm", "timeEndPeriod", (FARPROC)&xwr::Shim_timeEndPeriod);
REGISTER_SHIM("winmm", "timeGetDevCaps", (FARPROC)&xwr::Shim_timeGetDevCaps);
REGISTER_SHIM("winmm", "timeSetEvent", (FARPROC)&xwr::Shim_timeSetEvent);
REGISTER_SHIM("winmm", "timeKillEvent", (FARPROC)&xwr::Shim_timeKillEvent);
REGISTER_SHIM("winmm", "waveOutGetNumDevs", (FARPROC)&xwr::Shim_waveOutGetNumDevs);
REGISTER_SHIM("winmm", "waveOutOpen", (FARPROC)&xwr::Shim_waveOutOpen);
REGISTER_SHIM("winmm", "waveOutClose", (FARPROC)&xwr::Shim_waveOutClose);
REGISTER_SHIM("winmm", "waveOutPrepareHeader", (FARPROC)&xwr::Shim_waveOutPrepareHeader);
REGISTER_SHIM("winmm", "waveOutUnprepareHeader", (FARPROC)&xwr::Shim_waveOutUnprepareHeader);
REGISTER_SHIM("winmm", "waveOutWrite", (FARPROC)&xwr::Shim_waveOutWrite);
REGISTER_SHIM("winmm", "waveOutReset", (FARPROC)&xwr::Shim_waveOutReset);
REGISTER_SHIM("winmm", "waveOutPause", (FARPROC)&xwr::Shim_waveOutPause);
REGISTER_SHIM("winmm", "waveOutRestart", (FARPROC)&xwr::Shim_waveOutRestart);
REGISTER_SHIM("winmm", "waveOutGetVolume", (FARPROC)&xwr::Shim_waveOutGetVolume);
REGISTER_SHIM("winmm", "waveOutSetVolume", (FARPROC)&xwr::Shim_waveOutSetVolume);
REGISTER_SHIM("winmm", "waveOutGetDevCapsW", (FARPROC)&xwr::Shim_waveOutGetDevCapsW);
REGISTER_SHIM("winmm", "midiOutOpen", (FARPROC)&xwr::Shim_midiOutOpen);
REGISTER_SHIM("winmm", "midiOutClose", (FARPROC)&xwr::Shim_midiOutClose);
REGISTER_SHIM("winmm", "midiOutShortMsg", (FARPROC)&xwr::Shim_midiOutShortMsg);
REGISTER_SHIM("winmm", "midiOutLongMsg", (FARPROC)&xwr::Shim_midiOutLongMsg);
REGISTER_SHIM("winmm", "midiOutReset", (FARPROC)&xwr::Shim_midiOutReset);
REGISTER_SHIM("winmm", "mmioStringToFOURCCW", (FARPROC)&xwr::Shim_mmioStringToFOURCCW);
REGISTER_SHIM("winmm", "mmioOpenW", (FARPROC)&xwr::Shim_mmioOpenW);
REGISTER_SHIM("winmm", "mmioClose", (FARPROC)&xwr::Shim_mmioClose);
REGISTER_SHIM("winmm", "mmioRead", (FARPROC)&xwr::Shim_mmioRead);
REGISTER_SHIM("winmm", "mmioWrite", (FARPROC)&xwr::Shim_mmioWrite);
REGISTER_SHIM("winmm", "mmioSeek", (FARPROC)&xwr::Shim_mmioSeek);
REGISTER_SHIM("winmm", "mmioGetInfo", (FARPROC)&xwr::Shim_mmioGetInfo);
REGISTER_SHIM("winmm", "mmioSetInfo", (FARPROC)&xwr::Shim_mmioSetInfo);
REGISTER_SHIM("winmm", "mmioCreateChunk", (FARPROC)&xwr::Shim_mmioCreateChunk);
REGISTER_SHIM("winmm", "mmioAscend", (FARPROC)&xwr::Shim_mmioAscend);
REGISTER_SHIM("winmm", "mmioDescend", (FARPROC)&xwr::Shim_mmioDescend);
REGISTER_SHIM("winmm", "PlaySoundW", (FARPROC)&xwr::Shim_PlaySoundW);
REGISTER_SHIM("winmm", "PlaySoundA", (FARPROC)&xwr::Shim_PlaySoundW);
REGISTER_SHIM("winmm", "mciSendStringW", (FARPROC)&xwr::Shim_mciSendStringW);
REGISTER_SHIM("winmm", "mciSendStringA", (FARPROC)&xwr::Shim_mciSendStringW);
REGISTER_SHIM("winmm", "joyGetNumDevs", (FARPROC)&xwr::Shim_joyGetNumDevs);
REGISTER_SHIM("winmm", "joyGetDevCapsW", (FARPROC)&xwr::Shim_joyGetDevCapsW);
REGISTER_SHIM("winmm", "joyGetPosEx", (FARPROC)&xwr::Shim_joyGetPosEx);
