// shims/Win32Types.h
// Supplemental Win32 types & constants for the Xbox Win32 Runner.
//
// The UWP SDK provides MOST types but is missing:
//   - Some legacy constants (e.g. certain D3D_OK / MMSYSERR_* / WM_*)
//   - Some legacy handle typedefs (SC_HANDLE was already in Windows.h; here we add
//     HDEVINFO, HMSHANDLE, MMRESULT, etc.)
//   - Some macros (e.g. MAKEINTRESOURCE for resources)
//
// Every definition below is wrapped in #ifndef so the file is safe to include
// alongside the real Windows SDK. Never redefine a type the SDK already provides.
//
// This file is included transitively by CommonPre.h (forced include).

#ifndef XWR_WIN32_TYPES_H
#define XWR_WIN32_TYPES_H

// ----------------------------------------------------------------------------
// Resource macro (frequently missing in UWP SDK)
// ----------------------------------------------------------------------------
#ifndef MAKEINTRESOURCEA
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#endif
#ifndef MAKEINTRESOURCEW
#define MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#endif
#ifdef _UNICODE
#define MAKEINTRESOURCE MAKEINTRESOURCEW
#else
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#endif

// ----------------------------------------------------------------------------
// Direct3D status codes (some missing from UWP SDK if you don't include d3d11.h)
// ----------------------------------------------------------------------------
#ifndef D3D_OK
#define D3D_OK ((HRESULT)0L)
#endif
#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL ((HRESULT)0x8876086CL)
#endif
#ifndef D3DERR_OUTOFVIDEOMEMORY
#define D3DERR_OUTOFVIDEOMEMORY ((HRESULT)0x8876017CL)
#endif
#ifndef D3DERR_NOTAVAILABLE
#define D3DERR_NOTAVAILABLE ((HRESULT)0x8876086AL)
#endif
#ifndef D3DERR_DEVICELOST
#define D3DERR_DEVICELOST ((HRESULT)0x88760868L)
#endif
#ifndef D3DERR_DEVICENOTRESET
#define D3DERR_DEVICENOTRESET ((HRESULT)0x88760869L)
#endif
#ifndef D3DERR_DRIVERINVALIDCALL
#define D3DERR_DRIVERINVALIDCALL ((HRESULT)0x8876086BL)
#endif
#ifndef D3DERR_WASSTILLDRAWING
#define D3DERR_WASSTILLDRAWING ((HRESULT)0x8876086DL)
#endif
#ifndef D3DERR_DEVICEREMOVED
#define D3DERR_DEVICEREMOVED ((HRESULT)0x88760870L)
#endif
#ifndef D3DERR_DEVICEHUNG
#define D3DERR_DEVICEHUNG ((HRESULT)0x88760871L)
#endif

// ----------------------------------------------------------------------------
// DirectSound error codes
// ----------------------------------------------------------------------------
#ifndef DS_OK
#define DS_OK ((HRESULT)0L)
#endif
#ifndef DSERR_NODRIVER
#define DSERR_NODRIVER ((HRESULT)MAKE_HRESULT_CUST(1, 0x878, 120))
#endif
#ifndef DSERR_GENERIC
#define DSERR_GENERIC ((HRESULT)E_FAIL)
#endif
#ifndef DSERR_INVALIDPARAM
#define DSERR_INVALIDPARAM ((HRESULT)E_INVALIDARG)
#endif
#ifndef DSERR_UNSUPPORTED
#define DSERR_UNSUPPORTED ((HRESULT)E_NOTIMPL)
#endif
#ifndef DSBPLAY_LOOPING
#define DSBPLAY_LOOPING 0x00000001
#endif

// Helper macro for custom HRESULTs (only if not already defined)
#ifndef MAKE_HRESULT_CUST
#define MAKE_HRESULT_CUST(sev, fac, code) \
    ((HRESULT)(((unsigned long)(sev) << 31) | ((unsigned long)(fac) << 16) | ((unsigned long)(code))))
#endif

// ----------------------------------------------------------------------------
// WinMM error codes
// ----------------------------------------------------------------------------
#ifndef MMSYSERR_NOERROR
#define MMSYSERR_NOERROR 0
#endif
#ifndef MMSYSERR_ERROR
#define MMSYSERR_ERROR 1
#endif
#ifndef MMSYSERR_BADDEVICEID
#define MMSYSERR_BADDEVICEID 2
#endif
#ifndef MMSYSERR_NOTENABLED
#define MMSYSERR_NOTENABLED 3
#endif
#ifndef MMSYSERR_ALLOCATED
#define MMSYSERR_ALLOCATED 4
#endif
#ifndef MMSYSERR_INVALHANDLE
#define MMSYSERR_INVALHANDLE 5
#endif
#ifndef MMSYSERR_NODRIVER
#define MMSYSERR_NODRIVER 6
#endif
#ifndef MMSYSERR_NOMEM
#define MMSYSERR_NOMEM 7
#endif
#ifndef MMSYSERR_NOTSUPPORTED
#define MMSYSERR_NOTSUPPORTED 8
#endif
#ifndef MMSYSERR_BADERRNUM
#define MMSYSERR_BADERRNUM 9
#endif
#ifndef MMSYSERR_INVALFLAG
#define MMSYSERR_INVALFLAG 10
#endif
#ifndef MMSYSERR_INVALPARAM
#define MMSYSERR_INVALPARAM 11
#endif
#ifndef MMSYSERR_HANDLEBUSY
#define MMSYSERR_HANDLEBUSY 12
#endif
#ifndef MMSYSERR_INVALIDALIAS
#define MMSYSERR_INVALIDALIAS 13
#endif
#ifndef MMSYSERR_BADDB
#define MMSYSERR_BADDB 14
#endif
#ifndef MMSYSERR_KEYNOTFOUND
#define MMSYSERR_KEYNOTFOUND 15
#endif
#ifndef MMSYSERR_READERROR
#define MMSYSERR_READERROR 16
#endif
#ifndef MMSYSERR_WRITEERROR
#define MMSYSERR_WRITEERROR 17
#endif
#ifndef MMSYSERR_DELETEERROR
#define MMSYSERR_DELETEERROR 18
#endif
#ifndef MMSYSERR_VALNOTFOUND
#define MMSYSERR_VALNOTFOUND 19
#endif
#ifndef MMSYSERR_NODRIVERCB
#define MMSYSERR_NODRIVERCB 20
#endif
#ifndef MMSYSERR_LASTERROR
#define MMSYSERR_LASTERROR 20
#endif
#ifndef TIMERR_NOERROR
#define TIMERR_NOERROR 0
#endif
#ifndef TIMERR_NOCANDO
#define TIMERR_NOCANDO 1
#endif
#ifndef TIMERR_STRUCT
#define TIMERR_STRUCT 2
#endif

// WAV/midi format tags
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif
#ifndef WAVE_FORMAT_ADPCM
#define WAVE_FORMAT_ADPCM 2
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 3
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#endif
#ifndef WAVE_MAPPER
#define WAVE_MAPPER ((UINT)-1)
#endif
#ifndef MIDI_MAPPER
#define MIDI_MAPPER ((UINT)-1)
#endif

// mmio flags
#ifndef MMIO_READ
#define MMIO_READ 0x00000000
#endif
#ifndef MMIO_WRITE
#define MMIO_WRITE 0x00000001
#endif
#ifndef MMIO_READWRITE
#define MMIO_READWRITE 0x00000002
#endif
#ifndef MMIO_CREATE
#define MMIO_CREATE 0x00001000
#endif
#ifndef MMIO_ALLOCBUF
#define MMIO_ALLOCBUF 0x00010000
#endif
#ifndef MMIO_EXIST
#define MMIO_EXIST 0x00004000
#endif
#ifndef MMIO_DEFAULT
#define MMIO_DEFAULT 0
#endif
#ifndef MMIOM_READ
#define MMIOM_READ 0
#endif
#ifndef MMIOM_WRITE
#define MMIOM_WRITE 1
#endif
#ifndef MMIOM_SEEK
#define MMIOM_SEEK 2
#endif
#ifndef MMIO_TOUPPER
#define MMIO_TOUPPER 0x0010
#endif
#ifndef MMIO_FINDCHUNK
#define MMIO_FINDCHUNK 0x0010
#endif
#ifndef MMIO_FINDRIFF
#define MMIO_FINDRIFF 0x0020
#endif
#ifndef MMIO_FINDLIST
#define MMIO_FINDLIST 0x0040
#endif

// CALL_WINDOW_PROC style
#ifndef FO_MOVE
#define FO_MOVE 0x0001
#endif
#ifndef FO_COPY
#define FO_COPY 0x0002
#endif
#ifndef FO_DELETE
#define FO_DELETE 0x0003
#endif
#ifndef FO_RENAME
#define FO_RENAME 0x0004
#endif

// ----------------------------------------------------------------------------
// MessageBox flags (some missing from UWP SDK user32)
// ----------------------------------------------------------------------------
#ifndef MB_OK
#define MB_OK 0x00000000L
#endif
#ifndef MB_OKCANCEL
#define MB_OKCANCEL 0x00000001L
#endif
#ifndef MB_ABORTRETRYIGNORE
#define MB_ABORTRETRYIGNORE 0x00000002L
#endif
#ifndef MB_YESNOCANCEL
#define MB_YESNOCANCEL 0x00000003L
#endif
#ifndef MB_YESNO
#define MB_YESNO 0x00000004L
#endif
#ifndef MB_RETRYCANCEL
#define MB_RETRYCANCEL 0x00000005L
#endif
#ifndef MB_ICONHAND
#define MB_ICONHAND 0x00000010L
#endif
#ifndef MB_ICONQUESTION
#define MB_ICONQUESTION 0x00000020L
#endif
#ifndef MB_ICONEXCLAMATION
#define MB_ICONEXCLAMATION 0x00000030L
#endif
#ifndef MB_ICONASTERISK
#define MB_ICONASTERISK 0x00000040L
#endif
#ifndef MB_ICONWARNING
#define MB_ICONWARNING MB_ICONEXCLAMATION
#endif
#ifndef MB_ICONERROR
#define MB_ICONERROR MB_ICONHAND
#endif
#ifndef MB_ICONINFORMATION
#define MB_ICONINFORMATION MB_ICONASTERISK
#endif
#ifndef MB_ICONSTOP
#define MB_ICONSTOP MB_ICONHAND
#endif
#ifndef MB_DEFBUTTON1
#define MB_DEFBUTTON1 0x00000000L
#endif
#ifndef MB_DEFBUTTON2
#define MB_DEFBUTTON2 0x00000100L
#endif
#ifndef MB_DEFBUTTON3
#define MB_DEFBUTTON3 0x00000200L
#endif
#ifndef MB_DEFBUTTON4
#define MB_DEFBUTTON4 0x00000300L
#endif
#ifndef MB_SYSTEMMODAL
#define MB_SYSTEMMODAL 0x00001000L
#endif
#ifndef MB_TASKMODAL
#define MB_TASKMODAL 0x00002000L
#endif
#ifndef MB_TOPMOST
#define MB_TOPMOST 0x00040000L
#endif
#ifndef MB_RIGHT
#define MB_RIGHT 0x00080000L
#endif
#ifndef MB_RTLREADING
#define MB_RTLREADING 0x00100000L
#endif
#ifndef MB_SETFOREGROUND
#define MB_SETFOREGROUND 0x00010000L
#endif

// MessageBox return values
#ifndef IDOK
#define IDOK 1
#endif
#ifndef IDCANCEL
#define IDCANCEL 2
#endif
#ifndef IDABORT
#define IDABORT 3
#endif
#ifndef IDRETRY
#define IDRETRY 4
#endif
#ifndef IDIGNORE
#define IDIGNORE 5
#endif
#ifndef IDYES
#define IDYES 6
#endif
#ifndef IDNO
#define IDNO 7
#endif
#ifndef IDTRYAGAIN
#define IDTRYAGAIN 10
#endif
#ifndef IDCONTINUE
#define IDCONTINUE 11
#endif
#ifndef IDCLOSE
#define IDCLOSE 8
#endif
#ifndef IDHELP
#define IDHELP 9
#endif

// ----------------------------------------------------------------------------
// Window message constants - additional ones (the basics are in Windows.h)
// ----------------------------------------------------------------------------
#ifndef WM_ACTIVATEAPP
#define WM_ACTIVATEAPP 0x001C
#endif
#ifndef WM_FONTCHANGE
#define WM_FONTCHANGE 0x001D
#endif
#ifndef WM_TIMECHANGE
#define WM_TIMECHANGE 0x001E
#endif
#ifndef WM_CANCELMODE
#define WM_CANCELMODE 0x001F
#endif
#ifndef WM_DRAWITEM
#define WM_DRAWITEM 0x002B
#endif
#ifndef WM_MEASUREITEM
#define WM_MEASUREITEM 0x002C
#endif
#ifndef WM_DELETEITEM
#define WM_DELETEITEM 0x002D
#endif
#ifndef WM_VKEYTOITEM
#define WM_VKEYTOITEM 0x002E
#endif
#ifndef WM_CHARTOITEM
#define WM_CHARTOITEM 0x002F
#endif
#ifndef WM_SETFONT
#define WM_SETFONT 0x0030
#endif
#ifndef WM_GETFONT
#define WM_GETFONT 0x0031
#endif
#ifndef WM_SETHOTKEY
#define WM_SETHOTKEY 0x0032
#endif
#ifndef WM_GETHOTKEY
#define WM_GETHOTKEY 0x0033
#endif
#ifndef WM_QUERYDRAGICON
#define WM_QUERYDRAGICON 0x0037
#endif
#ifndef WM_COMPAREITEM
#define WM_COMPAREITEM 0x0039
#endif
#ifndef WM_COMPACTING
#define WM_COMPACTING 0x0041
#endif
#ifndef WM_COMMNOTIFY
#define WM_COMMNOTIFY 0x0044
#endif
#ifndef WM_WINDOWPOSCHANGING
#define WM_WINDOWPOSCHANGING 0x0046
#endif
#ifndef WM_WINDOWPOSCHANGED
#define WM_WINDOWPOSCHANGED 0x0047
#endif
#ifndef WM_POWER
#define WM_POWER 0x0048
#endif
#ifndef WM_COPYDATA
#define WM_COPYDATA 0x004A
#endif
#ifndef WM_CANCELJOURNAL
#define WM_CANCELJOURNAL 0x004B
#endif
#ifndef WM_INPUT
#define WM_INPUT 0x00FF
#endif
#ifndef WM_KEYFIRST
#define WM_KEYFIRST 0x0100
#endif
#ifndef WM_KEYLAST
#define WM_KEYLAST 0x0109
#endif
#ifndef WM_INITDIALOG
#define WM_INITDIALOG 0x0110
#endif
#ifndef WM_COMMAND
#define WM_COMMAND 0x0111
#endif
#ifndef WM_SYSCOMMAND
#define WM_SYSCOMMAND 0x0112
#endif
#ifndef WM_HSCROLL
#define WM_HSCROLL 0x0114
#endif
#ifndef WM_VSCROLL
#define WM_VSCROLL 0x0115
#endif
#ifndef WM_INITMENU
#define WM_INITMENU 0x0116
#endif
#ifndef WM_INITMENUPOPUP
#define WM_INITMENUPOPUP 0x0117
#endif
#ifndef WM_MENUSELECT
#define WM_MENUSELECT 0x011F
#endif
#ifndef WM_MENUCHAR
#define WM_MENUCHAR 0x0120
#endif
#ifndef WM_ENTERIDLE
#define WM_ENTERIDLE 0x0121
#endif
#ifndef WM_MENURBUTTONUP
#define WM_MENURBUTTONUP 0x0122
#endif
#ifndef WM_MENUDRAG
#define WM_MENUDRAG 0x0123
#endif
#ifndef WM_MENUGETOBJECT
#define WM_MENUGETOBJECT 0x0124
#endif
#ifndef WM_UNINITMENUPOPUP
#define WM_UNINITMENUPOPUP 0x0125
#endif
#ifndef WM_MENUCOMMAND
#define WM_MENUCOMMAND 0x0126
#endif
#ifndef WM_CHANGEUISTATE
#define WM_CHANGEUISTATE 0x0127
#endif
#ifndef WM_UPDATEUISTATE
#define WM_UPDATEUISTATE 0x0128
#endif
#ifndef WM_QUERYUISTATE
#define WM_QUERYUISTATE 0x0129
#endif
#ifndef WM_CTLCOLORMSGBOX
#define WM_CTLCOLORMSGBOX 0x0132
#endif
#ifndef WM_CTLCOLOREDIT
#define WM_CTLCOLOREDIT 0x0133
#endif
#ifndef WM_CTLCOLORLISTBOX
#define WM_CTLCOLORLISTBOX 0x0134
#endif
#ifndef WM_CTLCOLORBTN
#define WM_CTLCOLORBTN 0x0135
#endif
#ifndef WM_CTLCOLORDLG
#define WM_CTLCOLORDLG 0x0136
#endif
#ifndef WM_CTLCOLORSCROLLBAR
#define WM_CTLCOLORSCROLLBAR 0x0137
#endif
#ifndef WM_CTLCOLORSTATIC
#define WM_CTLCOLORSTATIC 0x0138
#endif
#ifndef WM_MOUSEFIRST
#define WM_MOUSEFIRST 0x0200
#endif
#ifndef WM_MOUSELAST
#define WM_MOUSELAST 0x020E
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WM_PARENTNOTIFY
#define WM_PARENTNOTIFY 0x0210
#endif
#ifndef WM_ENTERMENULOOP
#define WM_ENTERMENULOOP 0x0211
#endif
#ifndef WM_EXITMENULOOP
#define WM_EXITMENULOOP 0x0212
#endif
#ifndef WM_NEXTMENU
#define WM_NEXTMENU 0x0213
#endif
#ifndef WM_SIZING
#define WM_SIZING 0x0214
#endif
#ifndef WM_CAPTURECHANGED
#define WM_CAPTURECHANGED 0x0215
#endif
#ifndef WM_MOVING
#define WM_MOVING 0x0216
#endif
#ifndef WM_POWERBROADCAST
#define WM_POWERBROADCAST 0x0218
#endif
#ifndef WM_DEVICECHANGE
#define WM_DEVICECHANGE 0x0219
#endif
#ifndef WM_MDICREATE
#define WM_MDICREATE 0x0220
#endif
#ifndef WM_MDIDESTROY
#define WM_MDIDESTROY 0x0221
#endif
#ifndef WM_MDIACTIVATE
#define WM_MDIACTIVATE 0x0222
#endif
#ifndef WM_MDIRESTORE
#define WM_MDIRESTORE 0x0223
#endif
#ifndef WM_MDINEXT
#define WM_MDINEXT 0x0224
#endif
#ifndef WM_MDIMAXIMIZE
#define WM_MDIMAXIMIZE 0x0225
#endif
#ifndef WM_MDITILE
#define WM_MDITILE 0x0226
#endif
#ifndef WM_MDICASCADE
#define WM_MDICASCADE 0x0227
#endif
#ifndef WM_MDIICONARRANGE
#define WM_MDIICONARRANGE 0x0228
#endif
#ifndef WM_MDIGETACTIVE
#define WM_MDIGETACTIVE 0x0229
#endif
#ifndef WM_MDISETMENU
#define WM_MDISETMENU 0x0230
#endif
#ifndef WM_ENTERSIZEMOVE
#define WM_ENTERSIZEMOVE 0x0231
#endif
#ifndef WM_EXITSIZEMOVE
#define WM_EXITSIZEMOVE 0x0232
#endif
#ifndef WM_DROPFILES
#define WM_DROPFILES 0x0233
#endif
#ifndef WM_MDIREFRESHMENU
#define WM_MDIREFRESHMENU 0x0234
#endif
#ifndef WM_IME_SETCONTEXT
#define WM_IME_SETCONTEXT 0x0281
#endif
#ifndef WM_IME_NOTIFY
#define WM_IME_NOTIFY 0x0282
#endif
#ifndef WM_IME_CONTROL
#define WM_IME_CONTROL 0x0283
#endif
#ifndef WM_IME_COMPOSITIONFULL
#define WM_IME_COMPOSITIONFULL 0x0284
#endif
#ifndef WM_IME_SELECT
#define WM_IME_SELECT 0x0285
#endif
#ifndef WM_IME_CHAR
#define WM_IME_CHAR 0x0286
#endif
#ifndef WM_IME_REQUEST
#define WM_IME_REQUEST 0x0288
#endif
#ifndef WM_IME_KEYDOWN
#define WM_IME_KEYDOWN 0x0290
#endif
#ifndef WM_IME_KEYUP
#define WM_IME_KEYUP 0x0291
#endif
#ifndef WM_NCMOUSEMOVE
#define WM_NCMOUSEMOVE 0x00A0
#endif
#ifndef WM_NCLBUTTONDOWN
#define WM_NCLBUTTONDOWN 0x00A1
#endif
#ifndef WM_NCLBUTTONUP
#define WM_NCLBUTTONUP 0x00A2
#endif
#ifndef WM_NCLBUTTONDBLCLK
#define WM_NCLBUTTONDBLCLK 0x00A3
#endif
#ifndef WM_NCRBUTTONDOWN
#define WM_NCRBUTTONDOWN 0x00A4
#endif
#ifndef WM_NCRBUTTONUP
#define WM_NCRBUTTONUP 0x00A5
#endif
#ifndef WM_NCRBUTTONDBLCLK
#define WM_NCRBUTTONDBLCLK 0x00A6
#endif
#ifndef WM_NCMBUTTONDOWN
#define WM_NCMBUTTONDOWN 0x00A7
#endif
#ifndef WM_NCMBUTTONUP
#define WM_NCMBUTTONUP 0x00A8
#endif
#ifndef WM_NCMBUTTONDBLCLK
#define WM_NCMBUTTONDBLCLK 0x00A9
#endif
#ifndef WM_NCXBUTTONDOWN
#define WM_NCXBUTTONDOWN 0x00AB
#endif
#ifndef WM_NCXBUTTONUP
#define WM_NCXBUTTONUP 0x00AC
#endif
#ifndef WM_NCXBUTTONDBLCLK
#define WM_NCXBUTTONDBLCLK 0x00AD
#endif
#ifndef WM_INPUT_DEVICE_CHANGE
#define WM_INPUT_DEVICE_CHANGE 0x00FE
#endif
#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif
#ifndef WM_HOTKEY
#define WM_HOTKEY 0x0312
#endif
#ifndef WM_POINTERUPDATE
#define WM_POINTERUPDATE 0x0245
#endif
#ifndef WM_POINTERDOWN
#define WM_POINTERDOWN 0x0246
#endif
#ifndef WM_POINTERUP
#define WM_POINTERUP 0x0247
#endif

// ----------------------------------------------------------------------------
// Window hit-test codes (HT*)
// ----------------------------------------------------------------------------
#ifndef HTERROR
#define HTERROR (-2)
#endif
#ifndef HTTRANSPARENT
#define HTTRANSPARENT (-1)
#endif
#ifndef HTNOWHERE
#define HTNOWHERE 0
#endif
#ifndef HTCLIENT
#define HTCLIENT 1
#endif
#ifndef HTCAPTION
#define HTCAPTION 2
#endif
#ifndef HTSYSMENU
#define HTSYSMENU 3
#endif
#ifndef HTGROWBOX
#define HTGROWBOX 4
#endif
#ifndef HTSIZE
#define HTSIZE HTGROWBOX
#endif
#ifndef HTMENU
#define HTMENU 5
#endif
#ifndef HTHSCROLL
#define HTHSCROLL 6
#endif
#ifndef HTVSCROLL
#define HTVSCROLL 7
#endif
#ifndef HTMINBUTTON
#define HTMINBUTTON 8
#endif
#ifndef HTMAXBUTTON
#define HTMAXBUTTON 9
#endif
#ifndef HTLEFT
#define HTLEFT 10
#endif
#ifndef HTRIGHT
#define HTRIGHT 11
#endif
#ifndef HTTOP
#define HTTOP 12
#endif
#ifndef HTTOPLEFT
#define HTTOPLEFT 13
#endif
#ifndef HTTOPRIGHT
#define HTTOPRIGHT 14
#endif
#ifndef HTBOTTOM
#define HTBOTTOM 15
#endif
#ifndef HTBOTTOMLEFT
#define HTBOTTOMLEFT 16
#endif
#ifndef HTBOTTOMRIGHT
#define HTBOTTOMRIGHT 17
#endif
#ifndef HTBORDER
#define HTBORDER 18
#endif
#ifndef HTREDUCE
#define HTREDUCE HTMINBUTTON
#endif
#ifndef HTZOOM
#define HTZOOM HTMAXBUTTON
#endif
#ifndef HTSIZEFIRST
#define HTSIZEFIRST HTLEFT
#endif
#ifndef HTSIZELAST
#define HTSIZELAST HTBOTTOMRIGHT
#endif
#ifndef HTOBJECT
#define HTOBJECT 19
#endif
#ifndef HTCLOSE
#define HTCLOSE 20
#endif
#ifndef HTHELP
#define HTHELP 21
#endif

// ----------------------------------------------------------------------------
// WM_SIZE wParam values
// ----------------------------------------------------------------------------
#ifndef SIZE_RESTORED
#define SIZE_RESTORED 0
#endif
#ifndef SIZE_MINIMIZED
#define SIZE_MINIMIZED 1
#endif
#ifndef SIZE_MAXIMIZED
#define SIZE_MAXIMIZED 2
#endif
#ifndef SIZE_MAXSHOW
#define SIZE_MAXSHOW 3
#endif
#ifndef SIZE_MAXHIDE
#define SIZE_MAXHIDE 4
#endif

// WM_ACTIVATE wParam lo-word values
#ifndef WA_INACTIVE
#define WA_INACTIVE 0
#endif
#ifndef WA_ACTIVE
#define WA_ACTIVE 1
#endif
#ifndef WA_CLICKACTIVE
#define WA_CLICKACTIVE 2
#endif

// ----------------------------------------------------------------------------
// Hot key modifiers
// ----------------------------------------------------------------------------
#ifndef HOTKEYF_SHIFT
#define HOTKEYF_SHIFT 0x01
#endif
#ifndef HOTKEYF_CONTROL
#define HOTKEYF_CONTROL 0x02
#endif
#ifndef HOTKEYF_ALT
#define HOTKEYF_ALT 0x04
#endif
#ifndef HOTKEYF_EXT
#define HOTKEYF_EXT 0x08
#endif

// ----------------------------------------------------------------------------
// Drag-drop formats
// ----------------------------------------------------------------------------
#ifndef CF_TEXT
#define CF_TEXT 1
#endif
#ifndef CF_BITMAP
#define CF_BITMAP 2
#endif
#ifndef CF_METAFILEPICT
#define CF_METAFILEPICT 3
#endif
#ifndef CF_SYLK
#define CF_SYLK 4
#endif
#ifndef CF_DIF
#define CF_DIF 5
#endif
#ifndef CF_TIFF
#define CF_TIFF 6
#endif
#ifndef CF_OEMTEXT
#define CF_OEMTEXT 7
#endif
#ifndef CF_DIB
#define CF_DIB 8
#endif
#ifndef CF_PALETTE
#define CF_PALETTE 9
#endif
#ifndef CF_PENDATA
#define CF_PENDATA 10
#endif
#ifndef CF_RIFF
#define CF_RIFF 11
#endif
#ifndef CF_WAVE
#define CF_WAVE 12
#endif
#ifndef CF_UNICODETEXT
#define CF_UNICODETEXT 13
#endif
#ifndef CF_ENHMETAFILE
#define CF_ENHMETAFILE 14
#endif
#ifndef CF_HDROP
#define CF_HDROP 15
#endif
#ifndef CF_LOCALE
#define CF_LOCALE 16
#endif
#ifndef CF_DIBV5
#define CF_DIBV5 17
#endif

// ----------------------------------------------------------------------------
// Menu flags
// ----------------------------------------------------------------------------
#ifndef MF_INSERT
#define MF_INSERT 0x00000000L
#endif
#ifndef MF_CHANGE
#define MF_CHANGE 0x00000080L
#endif
#ifndef MF_APPEND
#define MF_APPEND 0x00000100L
#endif
#ifndef MF_DELETE
#define MF_DELETE 0x00000200L
#endif
#ifndef MF_REMOVE
#define MF_REMOVE 0x00001000L
#endif
#ifndef MF_BYCOMMAND
#define MF_BYCOMMAND 0x00000000L
#endif
#ifndef MF_BYPOSITION
#define MF_BYPOSITION 0x00000400L
#endif
#ifndef MF_SEPARATOR
#define MF_SEPARATOR 0x00000800L
#endif
#ifndef MF_ENABLED
#define MF_ENABLED 0x00000000L
#endif
#ifndef MF_GRAYED
#define MF_GRAYED 0x00000001L
#endif
#ifndef MF_DISABLED
#define MF_DISABLED 0x00000002L
#endif
#ifndef MF_UNCHECKED
#define MF_UNCHECKED 0x00000000L
#endif
#ifndef MF_CHECKED
#define MF_CHECKED 0x00000008L
#endif
#ifndef MF_USECHECKBITMAPS
#define MF_USECHECKBITMAPS 0x00000200L
#endif
#ifndef MF_STRING
#define MF_STRING 0x00000000L
#endif
#ifndef MF_BITMAP
#define MF_BITMAP 0x00000004L
#endif
#ifndef MF_OWNERDRAW
#define MF_OWNERDRAW 0x00000100L
#endif
#ifndef MF_POPUP
#define MF_POPUP 0x00000010L
#endif
#ifndef MF_MENUBARBREAK
#define MF_MENUBARBREAK 0x00000020L
#endif
#ifndef MF_MENUBREAK
#define MF_MENUBREAK 0x00000040L
#endif
#ifndef MF_UNHILITE
#define MF_UNHILITE 0x00000000L
#endif
#ifndef MF_HILITE
#define MF_HILITE 0x00000080L
#endif
#ifndef MF_DEFAULT
#define MF_DEFAULT 0x00001000L
#endif
#ifndef MF_SYSMENU
#define MF_SYSMENU 0x00002000L
#endif
#ifndef MF_HELP
#define MF_HELP 0x00004000L
#endif
#ifndef MF_RIGHTJUSTIFY
#define MF_RIGHTJUSTIFY 0x00004000L
#endif
#ifndef MF_MOUSESELECT
#define MF_MOUSESELECT 0x00008000L
#endif
#ifndef MF_END
#define MF_END 0x00000080L
#endif

// TrackPopupMenu flags
#ifndef TPM_LEFTBUTTON
#define TPM_LEFTBUTTON 0x0000L
#endif
#ifndef TPM_RIGHTBUTTON
#define TPM_RIGHTBUTTON 0x0002L
#endif
#ifndef TPM_LEFTALIGN
#define TPM_LEFTALIGN 0x0000L
#endif
#ifndef TPM_CENTERALIGN
#define TPM_CENTERALIGN 0x0004L
#endif
#ifndef TPM_RIGHTALIGN
#define TPM_RIGHTALIGN 0x0008L
#endif
#ifndef TPM_TOPALIGN
#define TPM_TOPALIGN 0x0000L
#endif
#ifndef TPM_VCENTERALIGN
#define TPM_VCENTERALIGN 0x0010L
#endif
#ifndef TPM_BOTTOMALIGN
#define TPM_BOTTOMALIGN 0x0020L
#endif
#ifndef TPM_HORIZONTAL
#define TPM_HORIZONTAL 0x0000L
#endif
#ifndef TPM_VERTICAL
#define TPM_VERTICAL 0x0040L
#endif
#ifndef TPM_NONOTIFY
#define TPM_NONOTIFY 0x0080L
#endif
#ifndef TPM_RETURNCMD
#define TPM_RETURNCMD 0x0100L
#endif
#ifndef TPM_RECURSE
#define TPM_RECURSE 0x0001L
#endif
#ifndef TPM_HORPOSANIMATION
#define TPM_HORPOSANIMATION 0x0400L
#endif
#ifndef TPM_HORNEGANIMATION
#define TPM_HORNEGANIMATION 0x0800L
#endif
#ifndef TPM_VERPOSANIMATION
#define TPM_VERPOSANIMATION 0x1000L
#endif
#ifndef TPM_VERNEGANIMATION
#define TPM_VERNEGANIMATION 0x2000L
#endif
#ifndef TPM_NOANIMATION
#define TPM_NOANIMATION 0x4000L
#endif
#ifndef TPM_LAYOUTRTL
#define TPM_LAYOUTRTL 0x8000L
#endif
#ifndef TPM_WORKAREA
#define TPM_WORKAREA 0x10000L
#endif

// ----------------------------------------------------------------------------
// Help constants
// ----------------------------------------------------------------------------
#ifndef HELP_CONTEXT
#define HELP_CONTEXT 0x0001L
#endif
#ifndef HELP_QUIT
#define HELP_QUIT 0x0002L
#endif
#ifndef HELP_INDEX
#define HELP_INDEX 0x0003L
#endif
#ifndef HELP_CONTENTS
#define HELP_CONTENTS 0x0003L
#endif
#ifndef HELP_HELPONHELP
#define HELP_HELPONHELP 0x0004L
#endif
#ifndef HELP_SETINDEX
#define HELP_SETINDEX 0x0005L
#endif
#ifndef HELP_SETCONTENTS
#define HELP_SETCONTENTS 0x0005L
#endif
#ifndef HELP_CONTEXTPOPUP
#define HELP_CONTEXTPOPUP 0x0008L
#endif
#ifndef HELP_FINDER
#define HELP_FINDER 0x000BL
#endif
#ifndef HELP_KEY
#define HELP_KEY 0x0101L
#endif
#ifndef HELP_COMMAND
#define HELP_COMMAND 0x0102L
#endif
#ifndef HELP_PARTIALKEY
#define HELP_PARTIALKEY 0x0105L
#endif
#ifndef HELP_MULTIKEY
#define HELP_MULTIKEY 0x0201L
#endif
#ifndef HELP_SETWINPOS
#define HELP_SETWINPOS 0x0203L
#endif
#ifndef HELP_CONTEXTMENU
#define HELP_CONTEXTMENU 0x000AL
#endif
#ifndef HELP_FINDER_
#define HELP_FINDER_ 0x000BL
#endif
#ifndef HELP_WM_HELP
#define HELP_WM_HELP 0x000CL
#endif
#ifndef HELP_TCARD
#define HELP_TCARD 0x8000L
#endif

// ----------------------------------------------------------------------------
// Console handlers / control events
// ----------------------------------------------------------------------------
#ifndef CTRL_C_EVENT
#define CTRL_C_EVENT 0
#endif
#ifndef CTRL_BREAK_EVENT
#define CTRL_BREAK_EVENT 1
#endif
#ifndef CTRL_CLOSE_EVENT
#define CTRL_CLOSE_EVENT 2
#endif
#ifndef CTRL_LOGOFF_EVENT
#define CTRL_LOGOFF_EVENT 5
#endif
#ifndef CTRL_SHUTDOWN_EVENT
#define CTRL_SHUTDOWN_EVENT 6
#endif

// ----------------------------------------------------------------------------
// Token information classes
// ----------------------------------------------------------------------------
#ifndef TokenUser
#define TokenUser 1
#endif
#ifndef TokenGroups
#define TokenGroups 2
#endif
#ifndef TokenPrivileges
#define TokenPrivileges 3
#endif
#ifndef TokenOwner
#define TokenOwner 4
#endif
#ifndef TokenPrimaryGroup
#define TokenPrimaryGroup 5
#endif
#ifndef TokenDefaultDacl
#define TokenDefaultDacl 6
#endif
#ifndef TokenSource
#define TokenSource 7
#endif
#ifndef TokenType
#define TokenType 8
#endif
#ifndef TokenImpersonationLevel
#define TokenImpersonationLevel 9
#endif
#ifndef TokenStatistics
#define TokenStatistics 10
#endif
#ifndef TokenRestrictedSids
#define TokenRestrictedSids 11
#endif
#ifndef TokenSessionId
#define TokenSessionId 12
#endif
#ifndef TokenGroupsAndPrivileges
#define TokenGroupsAndPrivileges 13
#endif
#ifndef TokenSessionReference
#define TokenSessionReference 14
#endif
#ifndef TokenSandBoxInert
#define TokenSandBoxInert 15
#endif
#ifndef TokenAuditPolicy
#define TokenAuditPolicy 16
#endif
#ifndef TokenOrigin
#define TokenOrigin 17
#endif
#ifndef TokenElevationType
#define TokenElevationType 18
#endif
#ifndef TokenLinkedToken
#define TokenLinkedToken 19
#endif
#ifndef TokenElevation
#define TokenElevation 20
#endif
#ifndef TokenHasRestrictions
#define TokenHasRestrictions 21
#endif
#ifndef TokenAccessInformation
#define TokenAccessInformation 22
#endif
#ifndef TokenVirtualizationAllowed
#define TokenVirtualizationAllowed 23
#endif
#ifndef TokenVirtualizationEnabled
#define TokenVirtualizationEnabled 24
#endif
#ifndef TokenIntegrityLevel
#define TokenIntegrityLevel 25
#endif
#ifndef TokenUIAccess
#define TokenUIAccess 26
#endif
#ifndef TokenMandatoryPolicy
#define TokenMandatoryPolicy 27
#endif
#ifndef TokenLogonSid
#define TokenLogonSid 28
#endif

// ----------------------------------------------------------------------------
// Service control / state constants
// ----------------------------------------------------------------------------
#ifndef SERVICE_CONTROL_STOP
#define SERVICE_CONTROL_STOP 0x00000001
#endif
#ifndef SERVICE_CONTROL_PAUSE
#define SERVICE_CONTROL_PAUSE 0x00000002
#endif
#ifndef SERVICE_CONTROL_CONTINUE
#define SERVICE_CONTROL_CONTINUE 0x00000003
#endif
#ifndef SERVICE_CONTROL_INTERROGATE
#define SERVICE_CONTROL_INTERROGATE 0x00000004
#endif
#ifndef SERVICE_CONTROL_SHUTDOWN
#define SERVICE_CONTROL_SHUTDOWN 0x00000005
#endif
#ifndef SERVICE_CONTROL_PRESHUTDOWN
#define SERVICE_CONTROL_PRESHUTDOWN 0x0000000F
#endif
#ifndef SERVICE_STOPPED
#define SERVICE_STOPPED 0x00000001
#endif
#ifndef SERVICE_START_PENDING
#define SERVICE_START_PENDING 0x00000002
#endif
#ifndef SERVICE_STOP_PENDING
#define SERVICE_STOP_PENDING 0x00000003
#endif
#ifndef SERVICE_RUNNING
#define SERVICE_RUNNING 0x00000004
#endif
#ifndef SERVICE_CONTINUE_PENDING
#define SERVICE_CONTINUE_PENDING 0x00000005
#endif
#ifndef SERVICE_PAUSE_PENDING
#define SERVICE_PAUSE_PENDING 0x00000006
#endif
#ifndef SERVICE_PAUSED
#define SERVICE_PAUSED 0x00000007
#endif
#ifndef SERVICE_KERNEL_DRIVER
#define SERVICE_KERNEL_DRIVER 0x00000001
#endif
#ifndef SERVICE_FILE_SYSTEM_DRIVER
#define SERVICE_FILE_SYSTEM_DRIVER 0x00000002
#endif
#ifndef SERVICE_WIN32_OWN_PROCESS
#define SERVICE_WIN32_OWN_PROCESS 0x00000010
#endif
#ifndef SERVICE_WIN32_SHARE_PROCESS
#define SERVICE_WIN32_SHARE_PROCESS 0x00000020
#endif
#ifndef SERVICE_BOOT_START
#define SERVICE_BOOT_START 0x00000000
#endif
#ifndef SERVICE_SYSTEM_START
#define SERVICE_SYSTEM_START 0x00000001
#endif
#ifndef SERVICE_AUTO_START
#define SERVICE_AUTO_START 0x00000002
#endif
#ifndef SERVICE_DEMAND_START
#define SERVICE_DEMAND_START 0x00000003
#endif
#ifndef SERVICE_DISABLED
#define SERVICE_DISABLED 0x00000004
#endif
#ifndef SERVICE_ERROR_IGNORE
#define SERVICE_ERROR_IGNORE 0x00000000
#endif
#ifndef SERVICE_ERROR_NORMAL
#define SERVICE_ERROR_NORMAL 0x00000001
#endif
#ifndef SERVICE_ERROR_SEVERE
#define SERVICE_ERROR_SEVERE 0x00000002
#endif
#ifndef SERVICE_ERROR_CRITICAL
#define SERVICE_ERROR_CRITICAL 0x00000003
#endif

// SC_MANAGER access masks
#ifndef SC_MANAGER_CONNECT
#define SC_MANAGER_CONNECT 0x0001
#endif
#ifndef SC_MANAGER_CREATE_SERVICE
#define SC_MANAGER_CREATE_SERVICE 0x0002
#endif
#ifndef SC_MANAGER_ENUMERATE_SERVICE
#define SC_MANAGER_ENUMERATE_SERVICE 0x0004
#endif
#ifndef SC_MANAGER_LOCK
#define SC_MANAGER_LOCK 0x0008
#endif
#ifndef SC_MANAGER_QUERY_LOCK_STATUS
#define SC_MANAGER_QUERY_LOCK_STATUS 0x0010
#endif
#ifndef SC_MANAGER_MODIFY_BOOT_CONFIG
#define SC_MANAGER_MODIFY_BOOT_CONFIG 0x0020
#endif
#ifndef SC_MANAGER_ALL_ACCESS
#define SC_MANAGER_ALL_ACCESS 0xF003F
#endif

// SERVICE access masks
#ifndef SERVICE_QUERY_CONFIG
#define SERVICE_QUERY_CONFIG 0x0001
#endif
#ifndef SERVICE_CHANGE_CONFIG
#define SERVICE_CHANGE_CONFIG 0x0002
#endif
#ifndef SERVICE_QUERY_STATUS
#define SERVICE_QUERY_STATUS 0x0004
#endif
#ifndef SERVICE_ENUMERATE_DEPENDENTS
#define SERVICE_ENUMERATE_DEPENDENTS 0x0008
#endif
#ifndef SERVICE_START
#define SERVICE_START 0x0010
#endif
#ifndef SERVICE_STOP
#define SERVICE_STOP 0x0020
#endif
#ifndef SERVICE_PAUSE_CONTINUE
#define SERVICE_PAUSE_CONTINUE 0x0040
#endif
#ifndef SERVICE_INTERROGATE
#define SERVICE_INTERROGATE 0x0080
#endif
#ifndef SERVICE_USER_DEFINED_CONTROL
#define SERVICE_USER_DEFINED_CONTROL 0x0100
#endif
#ifndef SERVICE_ALL_ACCESS
#define SERVICE_ALL_ACCESS 0xF01FF
#endif

// ----------------------------------------------------------------------------
// Window placement / Z-order constants
// ----------------------------------------------------------------------------
#ifndef HWND_TOP
#define HWND_TOP ((HWND)0)
#endif
#ifndef HWND_BOTTOM
#define HWND_BOTTOM ((HWND)1)
#endif
#ifndef HWND_TOPMOST
#define HWND_TOPMOST ((HWND)-1)
#endif
#ifndef HWND_NOTOPMOST
#define HWND_NOTOPMOST ((HWND)-2)
#endif
#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif

// SetWindowPos flags
#ifndef SWP_NOSIZE
#define SWP_NOSIZE 0x0001
#endif
#ifndef SWP_NOMOVE
#define SWP_NOMOVE 0x0002
#endif
#ifndef SWP_NOZORDER
#define SWP_NOZORDER 0x0004
#endif
#ifndef SWP_NOREDRAW
#define SWP_NOREDRAW 0x0008
#endif
#ifndef SWP_NOACTIVATE
#define SWP_NOACTIVATE 0x0010
#endif
#ifndef SWP_FRAMECHANGED
#define SWP_FRAMECHANGED 0x0020
#endif
#ifndef SWP_SHOWWINDOW
#define SWP_SHOWWINDOW 0x0040
#endif
#ifndef SWP_HIDEWINDOW
#define SWP_HIDEWINDOW 0x0080
#endif
#ifndef SWP_NOCOPYBITS
#define SWP_NOCOPYBITS 0x0100
#endif
#ifndef SWP_NOOWNERZORDER
#define SWP_NOOWNERZORDER 0x0200
#endif
#ifndef SWP_NOSENDCHANGING
#define SWP_NOSENDCHANGING 0x0400
#endif
#ifndef SWP_DRAWFRAME
#define SWP_DRAWFRAME SWP_FRAMECHANGED
#endif
#ifndef SWP_NOREPOSITION
#define SWP_NOREPOSITION SWP_NOOWNERZORDER
#endif
#ifndef SWP_DEFERERASE
#define SWP_DEFERERASE 0x2000
#endif
#ifndef SWP_ASYNCWINDOWPOS
#define SWP_ASYNCWINDOWPOS 0x4000
#endif

// RedrawWindow flags
#ifndef RDW_INVALIDATE
#define RDW_INVALIDATE 0x0001
#endif
#ifndef RDW_INTERNALPAINT
#define RDW_INTERNALPAINT 0x0002
#endif
#ifndef RDW_ERASE
#define RDW_ERASE 0x0004
#endif
#ifndef RDW_VALIDATE
#define RDW_VALIDATE 0x0008
#endif
#ifndef RDW_NOINTERNALPAINT
#define RDW_NOINTERNALPAINT 0x0010
#endif
#ifndef RDW_NOERASE
#define RDW_NOERASE 0x0020
#endif
#ifndef RDW_NOCHILDREN
#define RDW_NOCHILDREN 0x0040
#endif
#ifndef RDW_ALLCHILDREN
#define RDW_ALLCHILDREN 0x0080
#endif
#ifndef RDW_UPDATENOW
#define RDW_UPDATENOW 0x0100
#endif
#ifndef RDW_ERASENOW
#define RDW_ERASENOW 0x0200
#endif
#ifndef RDW_FRAME
#define RDW_FRAME 0x0400
#endif
#ifndef RDW_NOFRAME
#define RDW_NOFRAME 0x0800
#endif

// ----------------------------------------------------------------------------
// DrawText format flags
// ----------------------------------------------------------------------------
#ifndef DT_TOP
#define DT_TOP 0x00000000
#endif
#ifndef DT_LEFT
#define DT_LEFT 0x00000000
#endif
#ifndef DT_CENTER
#define DT_CENTER 0x00000001
#endif
#ifndef DT_RIGHT
#define DT_RIGHT 0x00000002
#endif
#ifndef DT_VCENTER
#define DT_VCENTER 0x00000004
#endif
#ifndef DT_BOTTOM
#define DT_BOTTOM 0x00000008
#endif
#ifndef DT_WORDBREAK
#define DT_WORDBREAK 0x00000010
#endif
#ifndef DT_SINGLELINE
#define DT_SINGLELINE 0x00000020
#endif
#ifndef DT_EXPANDTABS
#define DT_EXPANDTABS 0x00000040
#endif
#ifndef DT_TABSTOP
#define DT_TABSTOP 0x00000080
#endif
#ifndef DT_NOCLIP
#define DT_NOCLIP 0x00000100
#endif
#ifndef DT_EXTERNALLEADING
#define DT_EXTERNALLEADING 0x00000200
#endif
#ifndef DT_CALCRECT
#define DT_CALCRECT 0x00000400
#endif
#ifndef DT_NOPREFIX
#define DT_NOPREFIX 0x00000800
#endif
#ifndef DT_INTERNAL
#define DT_INTERNAL 0x00001000
#endif
#ifndef DT_EDITCONTROL
#define DT_EDITCONTROL 0x00002000
#endif
#ifndef DT_PATH_ELLIPSIS
#define DT_PATH_ELLIPSIS 0x00004000
#endif
#ifndef DT_END_ELLIPSIS
#define DT_END_ELLIPSIS 0x00008000
#endif
#ifndef DT_MODIFYSTRING
#define DT_MODIFYSTRING 0x00010000
#endif
#ifndef DT_RTLREADING
#define DT_RTLREADING 0x00020000
#endif
#ifndef DT_WORD_ELLIPSIS
#define DT_WORD_ELLIPSIS 0x00040000
#endif
#ifndef DT_NOFULLWIDTHCHARBREAK
#define DT_NOFULLWIDTHCHARBREAK 0x00080000
#endif
#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX 0x00100000
#endif
#ifndef DT_PREFIXONLY
#define DT_PREFIXONLY 0x00200000
#endif

// ----------------------------------------------------------------------------
// ShowWindow enums (additional)
// ----------------------------------------------------------------------------
#ifndef SW_FORCEMINIMIZE
#define SW_FORCEMINIMIZE 11
#endif
#ifndef SW_SHOWNA
#define SW_SHOWNA 8
#endif
#ifndef SW_SHOWNOACTIVATE
#define SW_SHOWNOACTIVATE 4
#endif
#ifndef SW_SHOWDEFAULT
#define SW_SHOWDEFAULT 10
#endif
#ifndef SW_SHOWMINNOACTIVE
#define SW_SHOWMINNOACTIVE 7
#endif

// ----------------------------------------------------------------------------
// Queue status masks
// ----------------------------------------------------------------------------
#ifndef QS_KEY
#define QS_KEY 0x0001
#endif
#ifndef QS_MOUSEMOVE
#define QS_MOUSEMOVE 0x0002
#endif
#ifndef QS_MOUSEBUTTON
#define QS_MOUSEBUTTON 0x0004
#endif
#ifndef QS_POSTMESSAGE
#define QS_POSTMESSAGE 0x0008
#endif
#ifndef QS_TIMER
#define QS_TIMER 0x0010
#endif
#ifndef QS_PAINT
#define QS_PAINT 0x0020
#endif
#ifndef QS_SENDMESSAGE
#define QS_SENDMESSAGE 0x0040
#endif
#ifndef QS_HOTKEY
#define QS_HOTKEY 0x0080
#endif
#ifndef QS_ALLPOSTMESSAGE
#define QS_ALLPOSTMESSAGE 0x0100
#endif
#ifndef QS_RAWINPUT
#define QS_RAWINPUT 0x0400
#endif
#ifndef QS_TOUCH
#define QS_TOUCH 0x0800
#endif
#ifndef QS_POINTER
#define QS_POINTER 0x1000
#endif
#ifndef QS_MOUSE
#define QS_MOUSE (QS_MOUSEMOVE | QS_MOUSEBUTTON)
#endif
#ifndef QS_INPUT
#define QS_INPUT (QS_MOUSE | QS_KEY | QS_RAWINPUT | QS_TOUCH | QS_POINTER)
#endif
#ifndef QS_ALLEVENTS
#define QS_ALLEVENTS (QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_HOTKEY)
#endif
#ifndef QS_ALLINPUT
#define QS_ALLINPUT (QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_HOTKEY | QS_SENDMESSAGE)
#endif

// PeekMessage flags
#ifndef PM_NOREMOVE
#define PM_NOREMOVE 0x0000
#endif
#ifndef PM_REMOVE
#define PM_REMOVE 0x0001
#endif
#ifndef PM_NOYIELD
#define PM_NOYIELD 0x0002
#endif
#ifndef PM_QS_INPUT
#define PM_QS_INPUT (QS_INPUT << 16)
#endif
#ifndef PM_QS_POSTMESSAGE
#define PM_QS_POSTMESSAGE ((QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER) << 16)
#endif
#ifndef PM_QS_PAINT
#define PM_QS_PAINT (QS_PAINT << 16)
#endif
#ifndef PM_QS_SENDMESSAGE
#define PM_QS_SENDMESSAGE (QS_SENDMESSAGE << 16)
#endif

// ----------------------------------------------------------------------------
// GetWindow / GW_*
// ----------------------------------------------------------------------------
#ifndef GW_HWNDFIRST
#define GW_HWNDFIRST 0
#endif
#ifndef GW_HWNDLAST
#define GW_HWNDLAST 1
#endif
#ifndef GW_HWNDNEXT
#define GW_HWNDNEXT 2
#endif
#ifndef GW_HWNDPREV
#define GW_HWNDPREV 3
#endif
#ifndef GW_OWNER
#define GW_OWNER 4
#endif
#ifndef GW_CHILD
#define GW_CHILD 5
#endif
#ifndef GW_ENABLEDPOPUP
#define GW_ENABLEDPOPUP 6
#endif

// GetWindowLong offsets
#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif
#ifndef GWL_HINSTANCE
#define GWL_HINSTANCE (-6)
#endif
#ifndef GWL_HWNDPARENT
#define GWL_HWNDPARENT (-8)
#endif
#ifndef GWL_STYLE
#define GWL_STYLE (-16)
#endif
#ifndef GWL_EXSTYLE
#define GWL_EXSTYLE (-20)
#endif
#ifndef GWL_ID
#define GWL_ID (-12)
#endif
#ifndef GWL_WNDPROC
#define GWL_WNDPROC (-4)
#endif

// ----------------------------------------------------------------------------
// COM init flags
// ----------------------------------------------------------------------------
#ifndef COINIT_APARTMENTTHREADED
#define COINIT_APARTMENTTHREADED 0x2
#endif
#ifndef COINIT_MULTITHREADED
#define COINIT_MULTITHREADED 0x0
#endif
#ifndef COINIT_DISABLE_OLE1DDE
#define COINIT_DISABLE_OLE1DDE 0x4
#endif
#ifndef COINIT_SPEED_OVER_MEMORY
#define COINIT_SPEED_OVER_MEMORY 0x8
#endif

// REGCLS flags
#ifndef REGCLS_SINGLEUSE
#define REGCLS_SINGLEUSE 0
#endif
#ifndef REGCLS_MULTIPLEUSE
#define REGCLS_MULTIPLEUSE 1
#endif
#ifndef REGCLS_MULTI_SEPARATE
#define REGCLS_MULTI_SEPARATE 2
#endif
#ifndef REGCLS_SUSPENDED
#define REGCLS_SUSPENDED 4
#endif
#ifndef REGCLS_SURROGATE
#define REGCLS_SURROGATE 8
#endif
#ifndef REGCLS_AGILE
#define REGCLS_AGILE 0x10
#endif

// CLSCTX flags
#ifndef CLSCTX_INPROC_SERVER
#define CLSCTX_INPROC_SERVER 0x1
#endif
#ifndef CLSCTX_INPROC_HANDLER
#define CLSCTX_INPROC_HANDLER 0x2
#endif
#ifndef CLSCTX_LOCAL_SERVER
#define CLSCTX_LOCAL_SERVER 0x4
#endif
#ifndef CLSCTX_INPROC_SERVER16
#define CLSCTX_INPROC_SERVER16 0x8
#endif
#ifndef CLSCTX_REMOTE_SERVER
#define CLSCTX_REMOTE_SERVER 0x10
#endif
#ifndef CLSCTX_INPROC_HANDLER16
#define CLSCTX_INPROC_HANDLER16 0x20
#endif
#ifndef CLSCTX_RESERVED1
#define CLSCTX_RESERVED1 0x40
#endif
#ifndef CLSCTX_RESERVED2
#define CLSCTX_RESERVED2 0x80
#endif
#ifndef CLSCTX_RESERVED3
#define CLSCTX_RESERVED3 0x100
#endif
#ifndef CLSCTX_RESERVED4
#define CLSCTX_RESERVED4 0x200
#endif
#ifndef CLSCTX_NO_CODE_DOWNLOAD
#define CLSCTX_NO_CODE_DOWNLOAD 0x400
#endif
#ifndef CLSCTX_NO_CUSTOM_MARSHALING
#define CLSCTX_NO_CUSTOM_MARSHALING 0x1000
#endif
#ifndef CLSCTX_ENABLE_CODE_DOWNLOAD
#define CLSCTX_ENABLE_CODE_DOWNLOAD 0x2000
#endif
#ifndef CLSCTX_NO_FAILURE_LOG
#define CLSCTX_NO_FAILURE_LOG 0x4000
#endif
#ifndef CLSCTX_DISABLE_AAA
#define CLSCTX_DISABLE_AAA 0x8000
#endif
#ifndef CLSCTX_ENABLE_AAA
#define CLSCTX_ENABLE_AAA 0x10000
#endif
#ifndef CLSCTX_FROM_DEFAULT_CONTEXT
#define CLSCTX_FROM_DEFAULT_CONTEXT 0x20000
#endif
#ifndef CLSCTX_ACTIVATE_X86_SERVER
#define CLSCTX_ACTIVATE_X86_SERVER 0x40000
#endif
#ifndef CLSCTX_ACTIVATE_32_BIT_SERVER
#define CLSCTX_ACTIVATE_32_BIT_SERVER CLSCTX_ACTIVATE_X86_SERVER
#endif
#ifndef CLSCTX_ACTIVATE_64_BIT_SERVER
#define CLSCTX_ACTIVATE_64_BIT_SERVER 0x80000
#endif
#ifndef CLSCTX_ENABLE_CLOAKING
#define CLSCTX_ENABLE_CLOAKING 0x100000
#endif
#ifndef CLSCTX_APPCONTAINER
#define CLSCTX_APPCONTAINER 0x400000
#endif
#ifndef CLSCTX_ACTIVATE_AAA_AS_IU
#define CLSCTX_ACTIVATE_AAA_AS_IU 0x800000
#endif
#ifndef CLSCTX_PS_DLL
#define CLSCTX_PS_DLL 0x80000000
#endif
#ifndef CLSCTX_ALL
#define CLSCTX_ALL (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER)
#endif
#ifndef CLSCTX_SERVER
#define CLSCTX_SERVER (CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER)
#endif

// ----------------------------------------------------------------------------
// Variants (subset)
// ----------------------------------------------------------------------------
#ifndef VT_EMPTY
#define VT_EMPTY 0
#endif
#ifndef VT_NULL
#define VT_NULL 1
#endif
#ifndef VT_I2
#define VT_I2 2
#endif
#ifndef VT_I4
#define VT_I4 3
#endif
#ifndef VT_R4
#define VT_R4 4
#endif
#ifndef VT_R8
#define VT_R8 5
#endif
#ifndef VT_CY
#define VT_CY 6
#endif
#ifndef VT_DATE
#define VT_DATE 7
#endif
#ifndef VT_BSTR
#define VT_BSTR 8
#endif
#ifndef VT_DISPATCH
#define VT_DISPATCH 9
#endif
#ifndef VT_ERROR
#define VT_ERROR 10
#endif
#ifndef VT_BOOL
#define VT_BOOL 11
#endif
#ifndef VT_VARIANT
#define VT_VARIANT 12
#endif
#ifndef VT_UNKNOWN
#define VT_UNKNOWN 13
#endif
#ifndef VT_DECIMAL
#define VT_DECIMAL 14
#endif
#ifndef VT_I1
#define VT_I1 16
#endif
#ifndef VT_UI1
#define VT_UI1 17
#endif
#ifndef VT_UI2
#define VT_UI2 18
#endif
#ifndef VT_UI4
#define VT_UI4 19
#endif
#ifndef VT_I8
#define VT_I8 20
#endif
#ifndef VT_UI8
#define VT_UI8 21
#endif
#ifndef VT_INT
#define VT_INT 22
#endif
#ifndef VT_UINT
#define VT_UINT 23
#endif
#ifndef VT_VOID
#define VT_VOID 24
#endif
#ifndef VT_HRESULT
#define VT_HRESULT 25
#endif
#ifndef VT_PTR
#define VT_PTR 26
#endif
#ifndef VT_SAFEARRAY
#define VT_SAFEARRAY 27
#endif
#ifndef VT_CARRAY
#define VT_CARRAY 28
#endif
#ifndef VT_USERDEFINED
#define VT_USERDEFINED 29
#endif
#ifndef VT_LPSTR
#define VT_LPSTR 30
#endif
#ifndef VT_LPWSTR
#define VT_LPWSTR 31
#endif
#ifndef VT_RECORD
#define VT_RECORD 36
#endif
#ifndef VT_INT_PTR
#define VT_INT_PTR 37
#endif
#ifndef VT_UINT_PTR
#define VT_UINT_PTR 38
#endif
#ifndef VT_FILETIME
#define VT_FILETIME 64
#endif
#ifndef VT_BLOB
#define VT_BLOB 65
#endif
#ifndef VT_STREAM
#define VT_STREAM 66
#endif
#ifndef VT_STORAGE
#define VT_STORAGE 67
#endif
#ifndef VT_STREAMED_OBJECT
#define VT_STREAMED_OBJECT 68
#endif
#ifndef VT_STORED_OBJECT
#define VT_STORED_OBJECT 69
#endif
#ifndef VT_BLOB_OBJECT
#define VT_BLOB_OBJECT 70
#endif
#ifndef VT_CF
#define VT_CF 71
#endif
#ifndef VT_CLSID
#define VT_CLSID 72
#endif
#ifndef VT_VERSIONED_STREAM
#define VT_VERSIONED_STREAM 73
#endif
#ifndef VT_BSTR_BLOB
#define VT_BSTR_BLOB 0xfff
#endif
#ifndef VT_VECTOR
#define VT_VECTOR 0x1000
#endif
#ifndef VT_ARRAY
#define VT_ARRAY 0x2000
#endif
#ifndef VT_BYREF
#define VT_BYREF 0x4000
#endif
#ifndef VT_RESERVED
#define VT_RESERVED 0x8000
#endif
#ifndef VT_ILLEGAL
#define VT_ILLEGAL 0xffff
#endif
#ifndef VT_ILLEGALMASKED
#define VT_ILLEGALMASKED 0xfff
#endif
#ifndef VT_TYPEMASK
#define VT_TYPEMASK 0xfff
#endif

// Variant change type flags
#ifndef VARIANT_NOVALUEPROP
#define VARIANT_NOVALUEPROP 0x01
#endif
#ifndef VARIANT_ALPHABOOL
#define VARIANT_ALPHABOOL 0x02
#endif
#ifndef VARIANT_NOUSEROVERRIDE
#define VARIANT_NOUSEROVERRIDE 0x04
#endif
#ifndef VARIANT_CALEXTRA
#define VARIANT_CALEXTRA 0x80
#endif
#ifndef VARIANT_LOCALBOOL
#define VARIANT_LOCALBOOL 0x10
#endif

// ----------------------------------------------------------------------------
// Path manipulation constants used by shlwapi/PathFileExists etc.
// ----------------------------------------------------------------------------
#ifndef FILE_ATTRIBUTE_DEVICE
#define FILE_ATTRIBUTE_DEVICE 0x00000040
#endif
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif
#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#endif
#ifndef FILE_ATTRIBUTE_ENCRYPTED
#define FILE_ATTRIBUTE_ENCRYPTED 0x00004000
#endif

// STGM flags
#ifndef STGM_DIRECT
#define STGM_DIRECT 0x00000000L
#endif
#ifndef STGM_TRANSACTED
#define STGM_TRANSACTED 0x00010000L
#endif
#ifndef STGM_READ
#define STGM_READ 0x00000000L
#endif
#ifndef STGM_WRITE
#define STGM_WRITE 0x00000001L
#endif
#ifndef STGM_READWRITE
#define STGM_READWRITE 0x00000002L
#endif
#ifndef STGM_SHARE_DENY_NONE
#define STGM_SHARE_DENY_NONE 0x00000040L
#endif
#ifndef STGM_SHARE_DENY_READ
#define STGM_SHARE_DENY_READ 0x00000030L
#endif
#ifndef STGM_SHARE_DENY_WRITE
#define STGM_SHARE_DENY_WRITE 0x00000020L
#endif
#ifndef STGM_SHARE_EXCLUSIVE
#define STGM_SHARE_EXCLUSIVE 0x00000010L
#endif
#ifndef STGM_CREATE
#define STGM_CREATE 0x00001000L
#endif
#ifndef STGM_CONVERT
#define STGM_CONVERT 0x00020000L
#endif
#ifndef STGM_FAILIFTHERE
#define STGM_FAILIFTHERE 0x00000000L
#endif
#ifndef STGM_DELETEONRELEASE
#define STGM_DELETEONRELEASE 0x04000000L
#endif

// ----------------------------------------------------------------------------
// Steam / Discord return codes (for stub shims)
// ----------------------------------------------------------------------------
#ifndef STEAM_API_OK
#define STEAM_API_OK true
#endif
#ifndef STEAM_API_FAIL
#define STEAM_API_FAIL false
#endif

// ----------------------------------------------------------------------------
// EXE / DLL function name decorations
// ----------------------------------------------------------------------------
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

// ----------------------------------------------------------------------------
// Property key struct (used by some shell32 paths)
// ----------------------------------------------------------------------------
// The real SDK may guard this with _PROPERTYKEY_DEFINED (wtypes.h) or
// _TAGPROPERTYKEY_DEFINED (some older SDKs / fwddecl). Use BOTH guards so we
// never collide with the SDK's own definition (C2011 redefinition errors).
#if !defined(_PROPERTYKEY_DEFINED) && !defined(_TAGPROPERTYKEY_DEFINED)
typedef struct _tagpropertykey {
    GUID fmtid;
    DWORD pid;
} PROPERTYKEY;
#define _PROPERTYKEY_DEFINED
#define _TAGPROPERTYKEY_DEFINED
#endif

// ----------------------------------------------------------------------------
// Win32 locale / system metrics: locale IDs
// ----------------------------------------------------------------------------
#ifndef LOCALE_USER_DEFAULT
#define LOCALE_USER_DEFAULT 0x0400
#endif
#ifndef LOCALE_SYSTEM_DEFAULT
#define LOCALE_SYSTEM_DEFAULT 0x0800
#endif
#ifndef LOCALE_NEUTRAL
#define LOCALE_NEUTRAL 0x0000
#endif
#ifndef LOCALE_INVARIANT
#define LOCALE_INVARIANT 0x007F
#endif

// CompareString etc. flags
#ifndef NORM_IGNORECASE
#define NORM_IGNORECASE 0x00000001
#endif
#ifndef NORM_IGNORENONSPACE
#define NORM_IGNORENONSPACE 0x00000002
#endif
#ifndef NORM_IGNORESYMBOLS
#define NORM_IGNORESYMBOLS 0x00000004
#endif
#ifndef NORM_IGNOREKANATYPE
#define NORM_IGNOREKANATYPE 0x00010000
#endif
#ifndef NORM_IGNOREWIDTH
#define NORM_IGNOREWIDTH 0x00020000
#endif

// LCID helpers
#ifndef MAKELCID
#define MAKELCID(lgid, srtid) ((DWORD)((((DWORD)((WORD)(srtid))) << 16) | ((DWORD)((WORD)(lgid))))
#endif
#ifndef LANGIDFROMLCID
#define LANGIDFROMLCID(lcid) ((WORD)(lcid))
#endif

// ----------------------------------------------------------------------------
// System colors (GetSysColor indices)
// ----------------------------------------------------------------------------
#ifndef COLOR_SCROLLBAR
#define COLOR_SCROLLBAR 0
#endif
#ifndef COLOR_BACKGROUND
#define COLOR_BACKGROUND 1
#endif
#ifndef COLOR_ACTIVECAPTION
#define COLOR_ACTIVECAPTION 2
#endif
#ifndef COLOR_INACTIVECAPTION
#define COLOR_INACTIVECAPTION 3
#endif
#ifndef COLOR_MENU
#define COLOR_MENU 4
#endif
#ifndef COLOR_WINDOW
#define COLOR_WINDOW 5
#endif
#ifndef COLOR_WINDOWFRAME
#define COLOR_WINDOWFRAME 6
#endif
#ifndef COLOR_MENUTEXT
#define COLOR_MENUTEXT 7
#endif
#ifndef COLOR_WINDOWTEXT
#define COLOR_WINDOWTEXT 8
#endif
#ifndef COLOR_CAPTIONTEXT
#define COLOR_CAPTIONTEXT 9
#endif
#ifndef COLOR_ACTIVEBORDER
#define COLOR_ACTIVEBORDER 10
#endif
#ifndef COLOR_INACTIVEBORDER
#define COLOR_INACTIVEBORDER 11
#endif
#ifndef COLOR_APPWORKSPACE
#define COLOR_APPWORKSPACE 12
#endif
#ifndef COLOR_HIGHLIGHT
#define COLOR_HIGHLIGHT 13
#endif
#ifndef COLOR_HIGHLIGHTTEXT
#define COLOR_HIGHLIGHTTEXT 14
#endif
#ifndef COLOR_BTNFACE
#define COLOR_BTNFACE 15
#endif
#ifndef COLOR_BTNSHADOW
#define COLOR_BTNSHADOW 16
#endif
#ifndef COLOR_GRAYTEXT
#define COLOR_GRAYTEXT 17
#endif
#ifndef COLOR_BTNTEXT
#define COLOR_BTNTEXT 18
#endif
#ifndef COLOR_INACTIVECAPTIONTEXT
#define COLOR_INACTIVECAPTIONTEXT 19
#endif
#ifndef COLOR_BTNHIGHLIGHT
#define COLOR_BTNHIGHLIGHT 20
#endif
#ifndef COLOR_3DDKSHADOW
#define COLOR_3DDKSHADOW 21
#endif
#ifndef COLOR_3DLIGHT
#define COLOR_3DLIGHT 22
#endif
#ifndef COLOR_INFOTEXT
#define COLOR_INFOTEXT 23
#endif
#ifndef COLOR_INFOBK
#define COLOR_INFOBK 24
#endif
#ifndef COLOR_DESKTOP
#define COLOR_DESKTOP COLOR_BACKGROUND
#endif
#ifndef COLOR_3DFACE
#define COLOR_3DFACE COLOR_BTNFACE
#endif
#ifndef COLOR_3DSHADOW
#define COLOR_3DSHADOW COLOR_BTNSHADOW
#endif
#ifndef COLOR_3DHIGHLIGHT
#define COLOR_3DHIGHLIGHT COLOR_BTNHIGHLIGHT
#endif
#ifndef COLOR_3DHILIGHT
#define COLOR_3DHILIGHT COLOR_BTNHIGHLIGHT
#endif
#ifndef COLOR_BTNHILIGHT
#define COLOR_BTNHILIGHT COLOR_BTNHIGHLIGHT
#endif

// ----------------------------------------------------------------------------
// Common control window classes
// ----------------------------------------------------------------------------
#ifndef WC_LISTVIEWA
#define WC_LISTVIEWA "SysListView32"
#endif
#ifndef WC_LISTVIEWW
#define WC_LISTVIEWW L"SysListView32"
#endif
#ifdef _UNICODE
#define WC_LISTVIEW WC_LISTVIEWW
#else
#define WC_LISTVIEW WC_LISTVIEWA
#endif

// ----------------------------------------------------------------------------
// Notification constants
// ----------------------------------------------------------------------------
#ifndef NIIF_NONE
#define NIIF_NONE 0x00000000
#endif
#ifndef NIIF_INFO
#define NIIF_INFO 0x00000001
#endif
#ifndef NIIF_WARNING
#define NIIF_WARNING 0x00000002
#endif
#ifndef NIIF_ERROR
#define NIIF_ERROR 0x00000003
#endif

// ----------------------------------------------------------------------------
// Open File / Save File dialog flags (used by shell32 stubs)
// ----------------------------------------------------------------------------
#ifndef OFN_READONLY
#define OFN_READONLY 0x00000001
#endif
#ifndef OFN_OVERWRITEPROMPT
#define OFN_OVERWRITEPROMPT 0x00000002
#endif
#ifndef OFN_HIDEREADONLY
#define OFN_HIDEREADONLY 0x00000004
#endif
#ifndef OFN_NOCHANGEDIR
#define OFN_NOCHANGEDIR 0x00000008
#endif
#ifndef OFN_SHOWHELP
#define OFN_SHOWHELP 0x00000010
#endif
#ifndef OFN_ENABLEHOOK
#define OFN_ENABLEHOOK 0x00000020
#endif
#ifndef OFN_ENABLETEMPLATE
#define OFN_ENABLETEMPLATE 0x00000040
#endif
#ifndef OFN_ENABLETEMPLATEHANDLE
#define OFN_ENABLETEMPLATEHANDLE 0x00000080
#endif
#ifndef OFN_NOVALIDATE
#define OFN_NOVALIDATE 0x00000100
#endif
#ifndef OFN_ALLOWMULTISELECT
#define OFN_ALLOWMULTISELECT 0x00000200
#endif
#ifndef OFN_EXTENSIONDIFFERENT
#define OFN_EXTENSIONDIFFERENT 0x00000400
#endif
#ifndef OFN_PATHMUSTEXIST
#define OFN_PATHMUSTEXIST 0x00000800
#endif
#ifndef OFN_FILEMUSTEXIST
#define OFN_FILEMUSTEXIST 0x00001000
#endif
#ifndef OFN_CREATEPROMPT
#define OFN_CREATEPROMPT 0x00002000
#endif
#ifndef OFN_SHAREAWARE
#define OFN_SHAREAWARE 0x00004000
#endif
#ifndef OFN_NOREADONLYRETURN
#define OFN_NOREADONLYRETURN 0x00008000
#endif
#ifndef OFN_NOTESTFILECREATE
#define OFN_NOTESTFILECREATE 0x00010000
#endif
#ifndef OFN_NONETWORKBUTTON
#define OFN_NONETWORKBUTTON 0x00020000
#endif
#ifndef OFN_NOLONGNAMES
#define OFN_NOLONGNAMES 0x00040000
#endif
#ifndef OFN_EXPLORER
#define OFN_EXPLORER 0x00080000
#endif
#ifndef OFN_NODEREFERENCELINKS
#define OFN_NODEREFERENCELINKS 0x00100000
#endif
#ifndef OFN_LONGNAMES
#define OFN_LONGNAMES 0x00200000
#endif

// ----------------------------------------------------------------------------
// Misc process / token rights
// ----------------------------------------------------------------------------
#ifndef TOKEN_ASSIGN_PRIMARY
#define TOKEN_ASSIGN_PRIMARY (0x0001)
#endif
#ifndef TOKEN_DUPLICATE
#define TOKEN_DUPLICATE (0x0002)
#endif
#ifndef TOKEN_IMPERSONATE
#define TOKEN_IMPERSONATE (0x0004)
#endif
#ifndef TOKEN_QUERY
#define TOKEN_QUERY (0x0008)
#endif
#ifndef TOKEN_QUERY_SOURCE
#define TOKEN_QUERY_SOURCE (0x0010)
#endif
#ifndef TOKEN_ADJUST_PRIVILEGES
#define TOKEN_ADJUST_PRIVILEGES (0x0020)
#endif
#ifndef TOKEN_ADJUST_GROUPS
#define TOKEN_ADJUST_GROUPS (0x0040)
#endif
#ifndef TOKEN_ADJUST_DEFAULT
#define TOKEN_ADJUST_DEFAULT (0x0080)
#endif
#ifndef TOKEN_ADJUST_SESSIONID
#define TOKEN_ADJUST_SESSIONID (0x0100)
#endif
#ifndef TOKEN_ALL_ACCESS
#define TOKEN_ALL_ACCESS (0xF01FF)
#endif

// PROCESS/DREAD/QUERY_*
#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE (0x0001)
#endif
#ifndef PROCESS_CREATE_THREAD
#define PROCESS_CREATE_THREAD (0x0002)
#endif
#ifndef PROCESS_SET_SESSIONID
#define PROCESS_SET_SESSIONID (0x0004)
#endif
#ifndef PROCESS_VM_OPERATION
#define PROCESS_VM_OPERATION (0x0008)
#endif
#ifndef PROCESS_VM_READ
#define PROCESS_VM_READ (0x0010)
#endif
#ifndef PROCESS_VM_WRITE
#define PROCESS_VM_WRITE (0x0020)
#endif
#ifndef PROCESS_DUP_HANDLE
#define PROCESS_DUP_HANDLE (0x0040)
#endif
#ifndef PROCESS_CREATE_PROCESS
#define PROCESS_CREATE_PROCESS (0x0080)
#endif
#ifndef PROCESS_SET_QUOTA
#define PROCESS_SET_QUOTA (0x0100)
#endif
#ifndef PROCESS_SET_INFORMATION
#define PROCESS_SET_INFORMATION (0x0200)
#endif
#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION (0x0400)
#endif
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME (0x0800)
#endif
#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION (0x1000)
#endif
#ifndef PROCESS_ALL_ACCESS
#define PROCESS_ALL_ACCESS (0x1F0FFF)
#endif

// THREAD_*
#ifndef THREAD_TERMINATE
#define THREAD_TERMINATE (0x0001)
#endif
#ifndef THREAD_SUSPEND_RESUME
#define THREAD_SUSPEND_RESUME (0x0002)
#endif
#ifndef THREAD_GET_CONTEXT
#define THREAD_GET_CONTEXT (0x0008)
#endif
#ifndef THREAD_SET_CONTEXT
#define THREAD_SET_CONTEXT (0x0010)
#endif
#ifndef THREAD_SET_INFORMATION
#define THREAD_SET_INFORMATION (0x0020)
#endif
#ifndef THREAD_QUERY_INFORMATION
#define THREAD_QUERY_INFORMATION (0x0040)
#endif
#ifndef THREAD_SET_THREAD_TOKEN
#define THREAD_SET_THREAD_TOKEN (0x0080)
#endif
#ifndef THREAD_IMPERSONATE
#define THREAD_IMPERSONATE (0x0100)
#endif
#ifndef THREAD_DIRECT_IMPERSONATION
#define THREAD_DIRECT_IMPERSONATION (0x0200)
#endif
#ifndef THREAD_SET_LIMITED_INFORMATION
#define THREAD_SET_LIMITED_INFORMATION (0x0400)
#endif
#ifndef THREAD_QUERY_LIMITED_INFORMATION
#define THREAD_QUERY_LIMITED_INFORMATION (0x0800)
#endif
#ifndef THREAD_ALL_ACCESS
#define THREAD_ALL_ACCESS (0x1F03FF)
#endif

// ----------------------------------------------------------------------------
// CreateProcess / GetStartupInfo dwFlags
// ----------------------------------------------------------------------------
#ifndef STARTF_USESHOWWINDOW
#define STARTF_USESHOWWINDOW 0x00000001
#endif
#ifndef STARTF_USESIZE
#define STARTF_USESIZE 0x00000002
#endif
#ifndef STARTF_USEPOSITION
#define STARTF_USEPOSITION 0x00000004
#endif
#ifndef STARTF_USECOUNTCHARS
#define STARTF_USECOUNTCHARS 0x00000008
#endif
#ifndef STARTF_USEFILLATTRIBUTE
#define STARTF_USEFILLATTRIBUTE 0x00000010
#endif
#ifndef STARTF_RUNFULLSCREEN
#define STARTF_RUNFULLSCREEN 0x00000020
#endif
#ifndef STARTF_FORCEONFEEDBACK
#define STARTF_FORCEONFEEDBACK 0x00000040
#endif
#ifndef STARTF_FORCEOFFFEEDBACK
#define STARTF_FORCEOFFFEEDBACK 0x00000080
#endif
#ifndef STARTF_USESTDHANDLES
#define STARTF_USESTDHANDLES 0x00000100
#endif
#ifndef STARTF_USEHOTKEY
#define STARTF_USEHOTKEY 0x00000200
#endif
#ifndef STARTF_TITLEISLINKNAME
#define STARTF_TITLEISLINKNAME 0x00000800
#endif
#ifndef STARTF_TITLEISAPPID
#define STARTF_TITLEISAPPID 0x00001000
#endif
#ifndef STARTF_PREVENTPINNING
#define STARTF_PREVENTPINNING 0x00002000
#endif
#ifndef STARTF_UNTRUSTEDSOURCE
#define STARTF_UNTRUSTEDSOURCE 0x00008000
#endif

// FormatMessage flags (additional)
#ifndef FORMAT_MESSAGE_MAX_WIDTH_MASK
#define FORMAT_MESSAGE_MAX_WIDTH_MASK 0x000000FF
#endif

// File flags
#ifndef FILE_FLAG_WRITE_THROUGH
#define FILE_FLAG_WRITE_THROUGH 0x80000000
#endif
#ifndef FILE_FLAG_OVERLAPPED
#define FILE_FLAG_OVERLAPPED 0x40000000
#endif
#ifndef FILE_FLAG_NO_BUFFERING
#define FILE_FLAG_NO_BUFFERING 0x20000000
#endif
#ifndef FILE_FLAG_RANDOM_ACCESS
#define FILE_FLAG_RANDOM_ACCESS 0x10000000
#endif
#ifndef FILE_FLAG_SEQUENTIAL_SCAN
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#endif
#ifndef FILE_FLAG_DELETE_ON_CLOSE
#define FILE_FLAG_DELETE_ON_CLOSE 0x04000000
#endif
#ifndef FILE_FLAG_BACKUP_SEMANTICS
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000
#endif
#ifndef FILE_FLAG_POSIX_SEMANTICS
#define FILE_FLAG_POSIX_SEMANTICS 0x01000000
#endif
#ifndef FILE_FLAG_OPEN_REPARSE_POINT
#define FILE_FLAG_OPEN_REPARSE_POINT 0x00200000
#endif
#ifndef FILE_FLAG_OPEN_NO_RECALL
#define FILE_FLAG_OPEN_NO_RECALL 0x00100000
#endif
#ifndef FILE_FLAG_FIRST_PIPE_INSTANCE
#define FILE_FLAG_FIRST_PIPE_INSTANCE 0x00080000
#endif

// ----------------------------------------------------------------------------
// Heap flags
// ----------------------------------------------------------------------------
#ifndef HEAP_NO_SERIALIZE
#define HEAP_NO_SERIALIZE 0x00000001
#endif
#ifndef HEAP_GENERATE_EXCEPTIONS
#define HEAP_GENERATE_EXCEPTIONS 0x00000004
#endif
#ifndef HEAP_ZERO_MEMORY
#define HEAP_ZERO_MEMORY 0x00000008
#endif

// ----------------------------------------------------------------------------
// Wait / timer / fiber consts (used by some shims)
// ----------------------------------------------------------------------------
#ifndef WT_EXECUTEDEFAULT
#define WT_EXECUTEDEFAULT 0x00000000
#endif
#ifndef WT_EXECUTEINIOTHREAD
#define WT_EXECUTEINIOTHREAD 0x00000001
#endif
#ifndef WT_EXECUTEINPERSISTENTTHREAD
#define WT_EXECUTEINPERSISTENTTHREAD 0x00000080
#endif
#ifndef WT_EXECUTEINWAITTHREAD
#define WT_EXECUTEINWAITTHREAD 0x00000004
#endif
#ifndef WT_EXECUTEONLYONCE
#define WT_EXECUTEONLYONCE 0x00000008
#endif
#ifndef WT_EXECUTELONGFUNCTION
#define WT_EXECUTELONGFUNCTION 0x00000010
#endif
#ifndef WT_EXECUTEINTIMERTHREAD
#define WT_EXECUTEINTIMERTHREAD 0x00000020
#endif
#ifndef WT_EXECUTEINUIAPI
#define WT_EXECUTEINUIAPI 0x00000100
#endif
#ifndef WT_EXECUTEDELETEWAIT
#define WT_EXECUTEDELETEWAIT 0x00000040
#endif

// Fiber/XState consts
#ifndef FIBER_FLAG_FLOAT_SWITCH
#define FIBER_FLAG_FLOAT_SWITCH 0x00000001
#endif

#endif  // XWR_WIN32_TYPES_H
