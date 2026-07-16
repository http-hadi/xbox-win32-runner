// winheaders/Windows.h
// STUB Windows SDK header for g++ -fsyntax-only syntax checking on Linux.
//
// This file mimics enough of <windows.h> so the xbox-win32-runner C++ sources
// compile cleanly with g++ -std=c++17 -fsyntax-only.
// NEVER shipped to Xbox - real UWP SDK headers are used in VS 2022 on Windows.
//
// File structure (order matters!):
//   1) Preprocessor macros (NOMINMAX, _WIN32_WINNT, etc.)
//   2) SAL annotations as no-ops
//   3) Calling-convention keywords as no-ops on Linux
//   4) Scalar types (DWORD, UINT, HANDLE, ...)
//   5) Pointer aliases (LPDWORD, LPBYTE, ...)
//   6) Struct types (OVERLAPPED, MSG, BITMAPINFO, ...)
//   7) Constants (ERROR_*, WM_*, VK_*, ...)
//   8) Function declarations (kernel32, user32, gdi32, ...)
//   9) PE format types (IMAGE_DOS_HEADER, etc.)

#ifndef STUB_WINDOWS_H
#define STUB_WINDOWS_H

// ===========================================================================
// 1) Preprocessor macros
// ===========================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif
#ifndef _WIN32
#define _WIN32 1
#endif
#ifndef _WIN64
#define _WIN64 1
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cstdarg>

// ===========================================================================
// 2) SAL annotations as no-ops
// ===========================================================================
#ifndef _In_
#define _In_
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Inout_
#define _Inout_
#endif
#ifndef _In_opt_
#define _In_opt_
#endif
#ifndef _Out_opt_
#define _Out_opt_
#endif
#ifndef _Inout_opt_
#define _Inout_opt_
#endif
#ifndef _In_z_
#define _In_z_
#endif
#ifndef _Out_z_
#define _Out_z_
#endif
#ifndef _In_reads_
#define _In_reads_(x)
#endif
#ifndef _Out_writes_
#define _Out_writes_(x)
#endif
#ifndef _In_reads_bytes_
#define _In_reads_bytes_(x)
#endif
#ifndef _Out_writes_bytes_
#define _Out_writes_bytes_(x)
#endif
#ifndef _In_reads_bytes_opt_
#define _In_reads_bytes_opt_(x)
#endif
#ifndef _Out_writes_bytes_opt_
#define _Out_writes_bytes_opt_(x)
#endif
#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif
#ifndef _Ret_notnull_
#define _Ret_notnull_
#endif
#ifndef _SUCCESS_
#define _SUCCESS_(x)
#endif
#ifndef _Must_inspect_result_
#define _Must_inspect_result_
#endif
#ifndef _CRT_ALLOCATOR
#define _CRT_ALLOCATOR
#endif
#ifndef _Post_writable_byte_size_
#define _Post_writable_byte_size_(x)
#endif
#ifndef _Post_readable_byte_size_
#define _Post_readable_byte_size_(x)
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif
// NOTE: Do NOT define __in / __out / __inout as macros - they break the
// libstdc++ internal headers which use these names as variable identifiers.

// ===========================================================================
// 3) Calling-convention keywords (no-ops on Linux; g++ doesn't recognize them)
// ===========================================================================
#if !defined(_MSC_VER)
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef __vectorcall
#define __vectorcall
#endif
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef NTAPI
#define NTAPI __stdcall
#endif
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef APIPRIVATE
#define APIPRIVATE __stdcall
#endif
#ifndef PASCAL
#define PASCAL __stdcall
#endif
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE __stdcall
#endif
#ifndef STDAPICALLTYPE
#define STDAPICALLTYPE __stdcall
#endif
#ifndef WINAPIV
#define WINAPIV __cdecl
#endif
#ifndef WINUSERAPI
#define WINUSERAPI
#endif
#ifndef WINBASEAPI
#define WINBASEAPI
#endif
#ifndef WINADVAPI
#define WINADVAPI
#endif
#ifndef WINGDIAPI
#define WINGDIAPI
#endif
#ifndef WINSHELLAPI
#define WINSHELLAPI
#endif
#ifndef WINOLEAPI
#define WINOLEAPI
#endif
#ifndef WINMMAPI
#define WINMMAPI
#endif

// ===========================================================================
// 4) Scalar types
// ===========================================================================
#ifndef VOID
#define VOID void
#endif
typedef void *PVOID, *LPVOID, *LPCVOID;
typedef void VOID_T;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int *PUINT;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned long ULONG;
typedef unsigned long DWORD;
typedef unsigned long DWORD32;
typedef unsigned long long DWORD64;
typedef unsigned long long *PDWORD64;
typedef unsigned long long ULONGLONG, DWORDLONG, *PDWORDLONG;
typedef long long LONGLONG;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef signed long long LONG64;
typedef signed long long INT_PTR;
typedef unsigned long long UINT_PTR;
typedef long long SIZE_T;
typedef long long SSIZE_T;
typedef long long LONG_PTR;
typedef unsigned long long ULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef float FLOAT;

typedef wchar_t WCHAR;
typedef wchar_t *PWCHAR, *LPWCH, *PWCH;
typedef const wchar_t *LPCWCH, *PCWCH;
typedef WCHAR *LPWSTR, *PWSTR, *NWPSTR;
typedef WCHAR *PZZWSTR;
typedef const WCHAR *LPCWSTR, *PCWSTR;
typedef CHAR *LPSTR, *PSTR, *NPSTR;
typedef const CHAR *LPCSTR, *PCSTR;

// Pointer aliases
typedef DWORD *PDWORD, *LPDWORD;
typedef WORD *PWORD, *LPWORD;
typedef BYTE *PBYTE, *LPBYTE;
typedef ULONG *PULONG, *LPULONG;
typedef USHORT *PUSHORT, *LPUSHORT;
typedef UCHAR *PUCHAR, *LPUCHAR;
typedef LONG *PLONG, *LPLONG;
typedef int *PINT, *LPINT;
typedef unsigned int *PUINT;
typedef ULONG_PTR *PULONG_PTR;
typedef double DOUBLE;
typedef DOUBLE DATE;
typedef int BOOL;
typedef BOOL *PBOOL, *LPBOOL;
typedef WCHAR *LPWCH_T;

// TCHAR plumbing
#ifndef _UNICODE
typedef CHAR TCHAR, *PTCHAR;
typedef LPSTR LPTSTR, PTSTR;
typedef LPCSTR LPCTSTR;
#define __TEXT(x) x
#else
typedef WCHAR TCHAR, *PTCHAR;
typedef LPWSTR LPTSTR, PTSTR;
typedef LPCWSTR LPCTSTR;
#define __TEXT(x) L##x
#endif
#ifndef TEXT
#define TEXT(x) __TEXT(x)
#endif

// Booleans
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
// (BOOL already typedef'd above as `int`.)
typedef int WINBOOL;
typedef unsigned char BOOLEAN;
typedef BYTE byte;

// NULL
#ifndef NULL
#ifdef __cplusplus
#define NULL nullptr
#else
#define NULL ((void*)0)
#endif
#endif

// Max path
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// ===========================================================================
// 5) Handle types
// ===========================================================================
#ifndef HANDLE
typedef void *HANDLE;
#endif
typedef HANDLE *PHANDLE, *LPHANDLE;
typedef DWORD HHOOK;
typedef DWORD HGLOBAL;
typedef DWORD HLOCAL;
typedef DWORD HMENU;
typedef DWORD HWND, *PHWND;
typedef DWORD HINSTANCE;
typedef DWORD HMODULE;
typedef DWORD HCURSOR;
typedef DWORD HICON;
typedef DWORD HBRUSH;
typedef DWORD HPEN;
typedef DWORD HFONT;
typedef DWORD HBITMAP;
typedef DWORD HPALETTE;
typedef DWORD HRGN;
typedef DWORD HDC;
typedef DWORD HDC__;
typedef DWORD HGDIOBJ;
typedef DWORD HACCEL;
typedef DWORD HSTRING;
typedef DWORD HDESK;
typedef DWORD HWINSTA;
typedef DWORD HMENU__;
typedef DWORD HMONITOR;
typedef DWORD HRAWINPUT;
typedef DWORD HRSRC;
typedef DWORD HRSRC__;
typedef DWORD HSTR;
typedef DWORD HKEY, *PHKEY;
typedef DWORD SC_HANDLE, *LPSC_HANDLE;
typedef DWORD SC_LOCK;
typedef DWORD SERVICE_STATUS_HANDLE;
typedef DWORD HDEVINFO;
typedef DWORD HCRYPTPROV;
typedef DWORD HCRYPTKEY;
typedef DWORD HCRYPTHASH;
typedef DWORD HMSHANDLE;
typedef DWORD HIMC;
typedef DWORD HIMCC;
typedef DWORD HRAWINPUT__;
typedef DWORD HINTERNET;
typedef DWORD HRSRC;
typedef DWORD HMETAFILE;
typedef DWORD HENHMETAFILE;
typedef DWORD HDROP;
typedef DWORD HKL;            // keyboard layout
typedef DWORD HMIDIOUT;
typedef DWORD HWAVEOUT;
typedef DWORD HMIXER;
typedef DWORD HMMIO;
typedef DWORD HTASK;
typedef DWORD MMVERSION;
typedef char *HPSTR;

// ===========================================================================
// 6) Function pointer typedefs
// ===========================================================================
typedef int (__stdcall *FARPROC)();
typedef int (__stdcall *NEARPROC)();
typedef int (__stdcall *PROC)();

// WPARAM/LPARAM/LRESULT
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;

// ATOM
typedef WORD ATOM;

// ===========================================================================
// 7) Scalar / struct forward declarations
// ===========================================================================
struct IUnknown;
struct IDispatch;
struct ITypeInfo;
struct ITypeLib;
struct IEnumVARIANT;
struct IStream;
struct IMoniker;
struct IBindCtx;
struct IStorage;
struct IClassFactory;
struct IEnumMoniker;
struct IDataObject;
struct IDropSource;
struct IConnectionPoint;
struct IConnectionPointContainer;
struct IEnumConnections;
struct IShellLink;
struct IShellFolder;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;
struct ID3D11Debug;
struct ID3D11InfoQueue;
struct ID3DDeviceContextState;
struct ID3DUserDefinedAnnotation;
struct ID3D12Device;
struct ID3D12CommandQueue;
struct IDXGISwapChain;
struct IDXGIFactory;
struct IDXGIAdapter;
struct ID2D1Factory;
struct ID2D1DCRenderTarget;
struct ID2D1SolidColorBrush;
struct ID2D1Bitmap;
struct ID2D1BitmapBrush;
struct ID2D1LinearGradientBrush;
struct ID2D1RadialGradientBrush;
struct ID2D1Layer;
struct ID2D1Mesh;
struct ID2D1HwndRenderTarget;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct IDWriteTextLayout;
struct IDWriteFontCollection;
struct IDWriteFontFamily;
struct IDWriteFont;
struct IDWriteFontFace;
struct IDWriteGdiInterop;
struct IWICImagingFactory;
struct IWICBitmap;
struct IWICBitmapDecoder;
struct IWICBitmapFrameDecode;
struct IWICFormatConverter;
struct IWICStream;
struct IWICBitmapSource;
struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;
struct SAFEARRAY;

// ===========================================================================
// 8) Common struct types
// ===========================================================================
typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct { DWORD Offset; DWORD OffsetHigh; } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;
    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _SYSTEM_INFO {
    union {
        DWORD dwOemId;
        struct { WORD wProcessorArchitecture; WORD wReserved; } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD_PTR dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

typedef struct _MEMORYSTATUSEX {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _BY_HANDLE_FILE_INFORMATION {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD dwVolumeSerialNumber;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD nNumberOfLinks;
    DWORD nFileIndexHigh;
    DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

typedef struct _WIN32_FIND_DATAW {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    WCHAR cFileName[MAX_PATH];
    WCHAR cAlternateFileName[14];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[MAX_PATH];
    CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

#ifdef _UNICODE
typedef WIN32_FIND_DATAW WIN32_FIND_DATA;
typedef PWIN32_FIND_DATAW PWIN32_FIND_DATA;
typedef LPWIN32_FIND_DATAW LPWIN32_FIND_DATA;
#else
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
typedef PWIN32_FIND_DATAA PWIN32_FIND_DATA;
typedef LPWIN32_FIND_DATAA LPWIN32_FIND_DATA;
#endif

typedef struct _STARTUPINFOW {
    DWORD cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

typedef struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _CRITICAL_SECTION {
    void *DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _MEMORY_BASIC_INFORMATION {
    PVOID  BaseAddress;
    PVOID  AllocationBase;
    DWORD  AllocationProtect;
    SIZE_T RegionSize;
    DWORD  State;
    DWORD  Protect;
    DWORD  Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

// Point/Rect/Size
typedef struct tagPOINT { LONG x; LONG y; } POINT, *PPOINT, *LPPOINT, *NPPOINT;
typedef struct tagRECT { LONG left; LONG top; LONG right; LONG bottom; } RECT, *PRECT, *LPRECT, *NPRECT;
typedef struct tagSIZE { LONG cx; LONG cy; } SIZE, *PSIZE, *LPSIZE;
typedef RECT RECTL, *PRECTL, *LPRECTL;

// LARGE_INTEGER / ULARGE_INTEGER
typedef union _LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; } DUMMYSTRUCTNAME;
    struct { DWORD LowPart; LONG HighPart; } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct { DWORD LowPart; DWORD HighPart; } DUMMYSTRUCTNAME;
    struct { DWORD LowPart; DWORD HighPart; } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

// HRESULT
typedef LONG HRESULT;

// GUID
typedef struct _GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID, *PGUID;
typedef GUID IID, *LPIID;
typedef GUID CLSID, *LPCLSID;
typedef GUID FMTID;
#ifndef REFGUID
#define REFGUID const GUID&
#endif
#ifndef REFIID
#define REFIID const IID&
#endif
#ifndef REFCLSID
#define REFCLSID const CLSID&
#endif

// COM types
typedef wchar_t OLECHAR;
typedef OLECHAR *BSTR;
typedef BSTR *LPBSTR;
typedef LONG SCODE;
typedef struct tagCY { LONGLONG int64; } CY;
// (DATE and DOUBLE already typedef'd above.)
typedef short VARIANT_BOOL;
typedef unsigned short VARTYPE;
typedef OLECHAR **SNB;

// VARIANT (full)
typedef struct tagDEC { BYTE reserved[16]; } DECIMAL;

typedef struct tagVARIANT {
    union {
        struct {
            VARTYPE vt;
            WORD wReserved1;
            WORD wReserved2;
            WORD wReserved3;
            union {
                LONG lVal;
                BYTE bVal;
                SHORT iVal;
                FLOAT fltVal;
                DOUBLE dblVal;
                VARIANT_BOOL boolVal;
                SCODE scode;
                CY cyVal;
                DATE date;
                BSTR bstrVal;
                IUnknown *punkVal;
                IDispatch *pdispVal;
                SAFEARRAY *parray;
                BYTE *pbVal;
                SHORT *piVal;
                LONG *plVal;
                FLOAT *pfltVal;
                DOUBLE *pdblVal;
                VARIANT_BOOL *pboolVal;
                SCODE *pscode;
                CY *pcyVal;
                DATE *pdate;
                BSTR *pbstrVal;
                IUnknown **ppunkVal;
                IDispatch **ppdispVal;
                SAFEARRAY **pparray;
                struct tagVARIANT *pvarVal;
                void *byref;
                CHAR cVal;
                USHORT uiVal;
                ULONG ulVal;
                INT intVal;
                UINT uintVal;
                DECIMAL *pdecVal;
                CHAR *pcVal;
                USHORT *puiVal;
                ULONG *pulVal;
                INT *pintVal;
                UINT *puintVal;
            };
        };
        DECIMAL decVal;
    };
} VARIANT, *LPVARIANT;
typedef VARIANT VARIANTARG, *LPVARIANTARG;

// WNDPROC (after WPARAM/LPARAM)
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);

// Window class structures
typedef struct _WNDCLASSW {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
} WNDCLASSW, *PWNDCLASSW, *NPWNDCLASSW, *LPWNDCLASSW;

typedef struct _WNDCLASSEXW {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
    HICON hIconSm;
} WNDCLASSEXW, *PWNDCLASSEXW, *NPWNDCLASSEXW, *LPWNDCLASSEXW;

// MSG
typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG, *PMSG, *LPMSG, *NPMSG;

// PAINTSTRUCT
typedef struct tagPAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;

// MONITORINFO
typedef struct tagMONITORINFO {
    DWORD cbSize;
    RECT rcMonitor;
    RECT rcWork;
    DWORD dwFlags;
} MONITORINFO, *LPMONITORINFO;

// TPMPARAMS
typedef struct tagTPMPARAMS {
    UINT cbSize;
    RECT rcExclude;
} TPMPARAMS, *LPTPMPARAMS;

// GDI types
typedef DWORD COLORREF;
typedef DWORD *LPCOLORREF;
#ifndef RGB
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif
#ifndef GetRValue
#define GetRValue(rgb) (LOBYTE(rgb))
#define GetGValue(rgb) (LOBYTE(((WORD)(rgb))>>8))
#define GetBValue(rgb) (LOBYTE((rgb)>>16))
#endif
#ifndef LOBYTE
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#endif
#ifndef HIBYTE
#define HIBYTE(w) ((BYTE)(((DWORD_PTR)(w)) >> 8))
#endif
#ifndef LOWORD
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#endif
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l)) >> 16))
#endif
#ifndef LODWORD
#define LODWORD(l) ((DWORD)(((DWORD_PTR)(l)) & 0xffffffff))
#endif
#ifndef HIDWORD
#define HIDWORD(l) ((DWORD)(((DWORD_PTR)(l)) >> 32))
#endif
#ifndef MAKEWORD
#define MAKEWORD(a,b) ((WORD)(((BYTE)(((DWORD_PTR)(a))) | ((WORD)((BYTE)(((DWORD_PTR)(b))))) << 8))
#endif
#ifndef MAKELONG
#define MAKELONG(a,b) ((LONG)(((WORD)(((DWORD_PTR)(a))) | ((DWORD)((WORD)(((DWORD_PTR)(b))))) << 16))
#endif
#ifndef MAKEWPARAM
#define MAKEWPARAM(l,h) ((WPARAM)(DWORD)MAKELONG(l,h))
#endif
#ifndef MAKELPARAM
#define MAKELPARAM(l,h) ((LPARAM)(DWORD)MAKELONG(l,h))
#endif
#ifndef MAKELRESULT
#define MAKELRESULT(l,h) ((LRESULT)(DWORD)MAKELONG(l,h))
#endif

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

typedef struct tagLOGBRUSH {
    UINT lbStyle;
    COLORREF lbColor;
    ULONG_PTR lbHatch;
} LOGBRUSH, *PLOGBRUSH, *LPLOGBRUSH;

typedef struct tagLOGFONTW {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[32];
} LOGFONTW, *PLOGFONTW, *LPLOGFONTW;

typedef struct tagBLENDFUNCTION {
    BYTE BlendOp;
    BYTE BlendFlags;
    BYTE SourceConstantAlpha;
    BYTE AlphaFormat;
} BLENDFUNCTION, *PBLENDFUNCTION;

typedef struct tagXFORM {
    FLOAT eM11;
    FLOAT eM12;
    FLOAT eM21;
    FLOAT eM22;
    FLOAT eDx;
    FLOAT eDy;
} XFORM, *LPXFORM;

typedef struct _RGNDATAHEADER {
    DWORD dwSize;
    DWORD iType;
    DWORD nCount;
    DWORD nRgnSize;
    RECT rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER rdh;
    char Buffer[1];
} RGNDATA, *PRGNDATA, *LPRGNDATA;

typedef struct tagABC {
    int abcA;
    UINT abcB;
    int abcC;
} ABC, *PABC, *LPABC;

typedef struct _GLYPHMETRICS {
    UINT gmBlackBoxX;
    UINT gmBlackBoxY;
    POINT gmCellGlyphOrigin;
    short gmCellIncX;
    short gmCellIncY;
} GLYPHMETRICS, *LPGLYPHMETRICS;

typedef struct tagFIXED {
    WORD fract;
    SHORT value;
} FIXED;

typedef struct _MAT2 {
    FIXED eM11;
    FIXED eM12;
    FIXED eM21;
    FIXED eM22;
} MAT2, *LPMAT2;

typedef struct tagTEXTMETRICW {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    WCHAR tmFirstChar;
    WCHAR tmLastChar;
    WCHAR tmDefaultChar;
    WCHAR tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
} TEXTMETRICW, *PTEXTMETRICW, *LPTEXTMETRICW;

typedef struct tagPOLYTEXTW {
    int x;
    int y;
    UINT n;
    LPCWSTR lpstr;
    UINT uiFlags;
    RECT rcl;
    int *pdx;
} POLYTEXTW, *PPOLYTEXTW, *LPPOLYTEXTW;

typedef int (CALLBACK *FONTENUMPROCW)(const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM);

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagLOGPALETTE {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, *LPLOGPALETTE;

typedef struct tagCOLORADJUSTMENT {
    WORD caSize;
    WORD caFlags;
    WORD caIlluminantIndex;
    WORD caRedGamma;
    WORD caGreenGamma;
    WORD caBlueGamma;
    WORD caReferenceBlack;
    WORD caReferenceWhite;
    SHORT caContrast;
    SHORT caBrightness;
    SHORT caColorfulness;
    SHORT caRedGreenTint;
} COLORADJUSTMENT, *PCOLORADJUSTMENT, *LPCOLORADJUSTMENT;

typedef struct tagENHMETAHEADER {
    DWORD iType;
    DWORD nSize;
    RECTL rclBounds;
    RECTL rclFrame;
    DWORD dSignature;
    DWORD nVersion;
    DWORD nBytes;
    DWORD nRecords;
    WORD nHandles;
    WORD sReserved;
    DWORD nDescription;
    DWORD offDescription;
    DWORD nPalEntries;
    SIZE szlDevice;
    SIZE szlMillimeters;
    DWORD cbPixelFormat;
    DWORD offPixelFormat;
    DWORD bOpenGL;
    SIZE szlMicrometers;
} ENHMETAHEADER, *LPENHMETAHEADER;

// advapi32 types
typedef DWORD REGSAM;
typedef struct _LUID { DWORD LowPart; LONG HighPart; } LUID, *PLUID;
typedef struct _SERVICE_STATUS {
    DWORD dwServiceType;
    DWORD dwCurrentState;
    DWORD dwControlsAccepted;
    DWORD dwWin32ExitCode;
    DWORD dwServiceSpecificExitCode;
    DWORD dwCheckPoint;
    DWORD dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

// shell32 types
typedef ULONGLONG SFGAOF;
typedef WORD FILEOP_FLAGS;

typedef struct _SHELLEXECUTEINFOW {
    DWORD cbSize;
    ULONG fMask;
    HWND hwnd;
    LPCWSTR lpVerb;
    LPCWSTR lpFile;
    LPCWSTR lpParameters;
    LPCWSTR lpDirectory;
    int nShow;
    HINSTANCE hInstApp;
    LPVOID lpIDList;
    LPCWSTR lpClass;
    HKEY hkeyClass;
    DWORD dwHotKey;
    union { HANDLE hIcon; HANDLE hMonitor; } DUMMYUNIONNAME;
    HANDLE hProcess;
} SHELLEXECUTEINFOW, *LPSHELLEXECUTEINFOW;

typedef struct _SHFILEOPSTRUCTW {
    HWND hwnd;
    UINT wFunc;
    LPCWSTR pFrom;
    LPCWSTR pTo;
    FILEOP_FLAGS fFlags;
    BOOL fAnyOperationsAborted;
    LPVOID hNameMappings;
    LPCWSTR lpszProgressTitle;
} SHFILEOPSTRUCTW, *LPSHFILEOPSTRUCTW;

// WinMM types
typedef UINT MMRESULT;
typedef DWORD FOURCC;

typedef struct tWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;

typedef struct wavehdr_tag {
    LPSTR lpData;
    DWORD dwBufferLength;
    DWORD dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD dwFlags;
    DWORD dwLoops;
    struct wavehdr_tag *lpNext;
    DWORD_PTR reserved;
} WAVEHDR, *PWAVEHDR, *LPWAVEHDR;

#ifndef MAXPNAMELEN
#define MAXPNAMELEN 32
#endif
#ifndef MAX_JOYSTICKOEMVXDNAME
#define MAX_JOYSTICKOEMVXDNAME 260
#endif

typedef struct tagWAVEOUTCAPSW {
    WORD wMid;
    WORD wPid;
    MMVERSION vDriverVersion;
    WCHAR szPname[MAXPNAMELEN];
    DWORD dwFormats;
    WORD wChannels;
    WORD wReserved1;
    DWORD dwSupport;
} WAVEOUTCAPSW, *PWAVEOUTCAPSW, *LPWAVEOUTCAPSW;

typedef struct tagMIDIHDR {
    LPSTR lpData;
    DWORD dwBufferLength;
    DWORD dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD dwFlags;
    struct tagMIDIHDR *lpNext;
    DWORD_PTR reserved;
    DWORD dwOffset;
    DWORD_PTR dwReserved[8];
} MIDIHDR, *PMIDIHDR, *LPMIDIHDR;

typedef struct tagTIMECAPS {
    UINT wPeriodMin;
    UINT wPeriodMax;
} TIMECAPS, *LPTIMECAPS;

typedef void (CALLBACK *LPTIMECALLBACK)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

typedef struct _MMIOINFO {
    DWORD dwFlags;
    FOURCC fccIOProc;
    LPVOID pIOProc;
    UINT wErrorRet;
    HTASK hTask;
    LONG cchBuffer;
    HPSTR pchBuffer;
    HPSTR pchNext;
    HPSTR pchEndRead;
    HPSTR pchEndWrite;
    LONG lBufOffset;
    LONG lDiskOffset;
    DWORD adwInfo[4];
    DWORD dwReserved1;
    DWORD dwReserved2;
    HMMIO hmmio;
} MMIOINFO, *PMMIOINFO, *LPMMIOINFO;

typedef struct _MMCKINFO {
    FOURCC ckid;
    FOURCC fccType;
    DWORD cksize;
    DWORD dwDataOffset;
    DWORD dwFlags;
} MMCKINFO, *PMMCKINFO, *LPMMCKINFO;

typedef struct tagJOYCAPSW {
    WORD wMid;
    WORD wPid;
    WCHAR szPname[MAXPNAMELEN];
    UINT wXmin;
    UINT wXmax;
    UINT wYmin;
    UINT wYmax;
    UINT wZmin;
    UINT wZmax;
    UINT wNumButtons;
    UINT wPeriodMin;
    UINT wPeriodMax;
    UINT wRmin;
    UINT wRmax;
    UINT wUmin;
    UINT wUmax;
    UINT wVmin;
    UINT wVmax;
    UINT wCaps;
    UINT wMaxAxes;
    UINT wNumAxes;
    UINT wMaxButtons;
    WCHAR szRegKey[MAXPNAMELEN];
    WCHAR szOEMVxD[MAX_JOYSTICKOEMVXDNAME];
} JOYCAPSW, *PJOYCAPSW, *LPJOYCAPSW;

typedef struct tagJOYINFOEX {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwXpos;
    DWORD dwYpos;
    DWORD dwZpos;
    DWORD dwRpos;
    DWORD dwUpos;
    DWORD dwVpos;
    DWORD dwButtons;
    DWORD dwButtonNumber;
    DWORD dwPOV;
    DWORD dwReserved1;
    DWORD dwReserved2;
} JOYINFOEX, *PJOYINFOEX, *LPJOYINFOEX;

typedef UINT *PUINT_;

// XInput types
typedef struct _XINPUT_GAMEPAD {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION {
    WORD wLeftMotorSpeed;
    WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

typedef struct _XINPUT_CAPABILITIES {
    BYTE Type;
    BYTE SubType;
    WORD Flags;
    XINPUT_GAMEPAD Gamepad;
    XINPUT_VIBRATION Vibration;
} XINPUT_CAPABILITIES, *PXINPUT_CAPABILITIES;

typedef struct _XINPUT_BATTERY_INFORMATION {
    BYTE BatteryType;
    BYTE BatteryLevel;
} XINPUT_BATTERY_INFORMATION, *PXINPUT_BATTERY_INFORMATION;

typedef struct _XINPUT_KEYSTROKE {
    WORD VirtualKey;
    WCHAR Unicode;
    WORD Flags;
    BYTE UserIndex;
    BYTE HidCode;
} XINPUT_KEYSTROKE, *PXINPUT_KEYSTROKE;

// DXGI / D3D types (lightweight stub - real types come from real SDK)
typedef int DXGI_FORMAT;
typedef int DXGI_MODE_SCANLINE_ORDER;
typedef int DXGI_MODE_SCALING;
typedef int D3D11_USAGE;
typedef int D3D11_BIND_FLAG;
typedef int D3D11_CPU_ACCESS_FLAG;
typedef int D3D11_RESOURCE_MISC_FLAG;
typedef int D3D11_TEXTURE2D_MISC_FLAG;

typedef enum D3D_DRIVER_TYPE {
    D3D_DRIVER_TYPE_UNKNOWN = 0,
    D3D_DRIVER_TYPE_HARDWARE = 1,
    D3D_DRIVER_TYPE_REFERENCE = 2,
    D3D_DRIVER_TYPE_NULL = 3,
    D3D_DRIVER_TYPE_WARP = 5,
    D3D_DRIVER_TYPE_SOFTWARE = 6
} D3D_DRIVER_TYPE;

typedef enum D3D_FEATURE_LEVEL {
    D3D_FEATURE_LEVEL_9_1 = 0x9100,
    D3D_FEATURE_LEVEL_9_2 = 0x9200,
    D3D_FEATURE_LEVEL_9_3 = 0x9300,
    D3D_FEATURE_LEVEL_10_0 = 0xa000,
    D3D_FEATURE_LEVEL_10_1 = 0xa100,
    D3D_FEATURE_LEVEL_11_0 = 0xb000,
    D3D_FEATURE_LEVEL_11_1 = 0xb100,
    D3D_FEATURE_LEVEL_12_0 = 0xc000,
    D3D_FEATURE_LEVEL_12_1 = 0xc100
} D3D_FEATURE_LEVEL;

typedef struct DXGI_RATIONAL {
    UINT Numerator;
    UINT Denominator;
} DXGI_RATIONAL;

typedef struct DXGI_SAMPLE_DESC {
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;

typedef struct DXGI_MODE_DESC {
    UINT Width;
    UINT Height;
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
} DXGI_MODE_DESC;

typedef enum DXGI_USAGE {
    DXGI_USAGE_SHADER_INPUT = 0x00000010L,
    DXGI_USAGE_RENDER_TARGET_OUTPUT = 0x00000020L,
    DXGI_USAGE_BACK_BUFFER = 0x00000040L,
    DXGI_USAGE_SHARED = 0x00000080L,
    DXGI_USAGE_DISCARD_ON_PRESENT = 0x00000200L,
    DXGI_USAGE_UNORDERED_ACCESS = 0x00000040L,
} DXGI_USAGE;

typedef enum DXGI_SWAP_EFFECT {
    DXGI_SWAP_EFFECT_DISCARD = 0,
    DXGI_SWAP_EFFECT_SEQUENTIAL = 1,
    DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 3,
    DXGI_SWAP_EFFECT_FLIP_DISCARD = 4
} DXGI_SWAP_EFFECT;

typedef struct DXGI_SWAP_CHAIN_DESC {
    DXGI_MODE_DESC BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    HWND OutputWindow;
    BOOL Windowed;
    DXGI_SWAP_EFFECT SwapEffect;
    UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

typedef UINT32 XAUDIO2_PROCESSOR;

// ===========================================================================
// 9) Constants (selected Win32 constants - see Win32Types.h for more)
// ===========================================================================
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#endif
#ifndef NO_ERROR
#define NO_ERROR 0L
#endif
#ifndef ERROR_INVALID_FUNCTION
#define ERROR_INVALID_FUNCTION 1L
#endif
#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND 2L
#endif
#ifndef ERROR_PATH_NOT_FOUND
#define ERROR_PATH_NOT_FOUND 3L
#endif
#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED 5L
#endif
#ifndef ERROR_INVALID_HANDLE
#define ERROR_INVALID_HANDLE 6L
#endif
#ifndef ERROR_NOT_ENOUGH_MEMORY
#define ERROR_NOT_ENOUGH_MEMORY 8L
#endif
#ifndef ERROR_NO_MORE_FILES
#define ERROR_NO_MORE_FILES 18L
#endif
#ifndef ERROR_SHARING_VIOLATION
#define ERROR_SHARING_VIOLATION 32L
#endif
#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER 87L
#endif
#ifndef ERROR_INSUFFICIENT_BUFFER
#define ERROR_INSUFFICIENT_BUFFER 122L
#endif
#ifndef ERROR_INVALID_NAME
#define ERROR_INVALID_NAME 123L
#endif
#ifndef ERROR_MOD_NOT_FOUND
#define ERROR_MOD_NOT_FOUND 126L
#endif
#ifndef ERROR_PROC_NOT_FOUND
#define ERROR_PROC_NOT_FOUND 127L
#endif
#ifndef ERROR_NOT_SUPPORTED
#define ERROR_NOT_SUPPORTED 50L
#endif
#ifndef ERROR_NOT_IMPLEMENTED
#define ERROR_NOT_IMPLEMENTED 120L
#endif
#ifndef ERROR_BAD_FORMAT
#define ERROR_BAD_FORMAT 11L
#endif
#ifndef ERROR_BAD_EXE_FORMAT
#define ERROR_BAD_EXE_FORMAT 193L
#endif
#ifndef ERROR_EXE_MACHINE_TYPE_MISMATCH
#define ERROR_EXE_MACHINE_TYPE_MISMATCH 216L
#endif
#ifndef ERROR_INVALID_WINDOW_HANDLE
#define ERROR_INVALID_WINDOW_HANDLE 1400L
#endif
#ifndef ERROR_RESOURCE_TYPE_NOT_FOUND
#define ERROR_RESOURCE_TYPE_NOT_FOUND 1813L
#endif
#ifndef ERROR_DLL_INIT_FAILED
#define ERROR_DLL_INIT_FAILED 1114L
#endif
#ifndef ERROR_INVALID_ORDINAL
#define ERROR_INVALID_ORDINAL 182L
#endif
#ifndef ERROR_DEVICE_NOT_CONNECTED
#define ERROR_DEVICE_NOT_CONNECTED 1167L
#endif
#ifndef WAIT_FAILED
#define WAIT_FAILED ((DWORD)0xFFFFFFFF)
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0L
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 258L
#endif
#ifndef WAIT_ABANDONED
#define WAIT_ABANDONED 0x80L
#endif
#ifndef INFINITE
#define INFINITE ((DWORD)0xFFFFFFFF)
#endif
#ifndef STILL_ACTIVE
#define STILL_ACTIVE ((DWORD)259)
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(HANDLE*)(-1))
#endif
#ifndef INVALID_FILE_SIZE
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

// File attributes
#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY 0x00000001
#endif
#ifndef FILE_ATTRIBUTE_HIDDEN
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#endif
#ifndef FILE_ATTRIBUTE_SYSTEM
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif
#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#endif
#ifndef FILE_ATTRIBUTE_TEMPORARY
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#endif
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif
#ifndef FILE_ATTRIBUTE_COMPRESSED
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800
#endif
#ifndef FILE_ATTRIBUTE_OFFLINE
#define FILE_ATTRIBUTE_OFFLINE 0x00001000
#endif
#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#endif
#ifndef FILE_ATTRIBUTE_ENCRYPTED
#define FILE_ATTRIBUTE_ENCRYPTED 0x00004000
#endif

// File access / share / creation
#ifndef GENERIC_READ
#define GENERIC_READ (0x80000000L)
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE (0x40000000L)
#endif
#ifndef GENERIC_EXECUTE
#define GENERIC_EXECUTE (0x20000000L)
#endif
#ifndef GENERIC_ALL
#define GENERIC_ALL (0x10000000L)
#endif
#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ 0x00000001
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE 0x00000002
#endif
#ifndef FILE_SHARE_DELETE
#define FILE_SHARE_DELETE 0x00000004
#endif
#ifndef CREATE_NEW
#define CREATE_NEW 1
#endif
#ifndef CREATE_ALWAYS
#define CREATE_ALWAYS 2
#endif
#ifndef OPEN_EXISTING
#define OPEN_EXISTING 3
#endif
#ifndef OPEN_ALWAYS
#define OPEN_ALWAYS 4
#endif
#ifndef TRUNCATE_EXISTING
#define TRUNCATE_EXISTING 5
#endif
#ifndef FILE_BEGIN
#define FILE_BEGIN 0
#endif
#ifndef FILE_CURRENT
#define FILE_CURRENT 1
#endif
#ifndef FILE_END
#define FILE_END 2
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

// Memory protection
#ifndef PAGE_NOACCESS
#define PAGE_NOACCESS 0x01
#endif
#ifndef PAGE_READONLY
#define PAGE_READONLY 0x02
#endif
#ifndef PAGE_READWRITE
#define PAGE_READWRITE 0x04
#endif
#ifndef PAGE_WRITECOPY
#define PAGE_WRITECOPY 0x08
#endif
#ifndef PAGE_EXECUTE
#define PAGE_EXECUTE 0x10
#endif
#ifndef PAGE_EXECUTE_READ
#define PAGE_EXECUTE_READ 0x20
#endif
#ifndef PAGE_EXECUTE_READWRITE
#define PAGE_EXECUTE_READWRITE 0x40
#endif
#ifndef PAGE_EXECUTE_WRITECOPY
#define PAGE_EXECUTE_WRITECOPY 0x80
#endif
#ifndef PAGE_GUARD
#define PAGE_GUARD 0x100
#endif
#ifndef PAGE_NOCACHE
#define PAGE_NOCACHE 0x200
#endif
#ifndef PAGE_WRITECOMBINE
#define PAGE_WRITECOMBINE 0x400
#endif
#ifndef PAGE_TARGETS_NO_UPDATE
#define PAGE_TARGETS_NO_UPDATE 0x40000000
#endif
#ifndef MEM_COMMIT
#define MEM_COMMIT 0x00001000
#endif
#ifndef MEM_RESERVE
#define MEM_RESERVE 0x00002000
#endif
#ifndef MEM_DECOMMIT
#define MEM_DECOMMIT 0x00004000
#endif
#ifndef MEM_RELEASE
#define MEM_RELEASE 0x00008000
#endif
#ifndef MEM_RESET
#define MEM_RESET 0x00080000
#endif
#ifndef MEM_RESET_UNDO
#define MEM_RESET_UNDO 0x1000000
#endif
#ifndef MEM_FREE
#define MEM_FREE 0x10000
#endif
#ifndef MEM_TOP_DOWN
#define MEM_TOP_DOWN 0x100000
#endif
#ifndef MEM_REPLACE_PLACEHOLDER
#define MEM_REPLACE_PLACEHOLDER 0x00004000
#endif
#ifndef MEM_RESERVE_PLACEHOLDER
#define MEM_RESERVE_PLACEHOLDER 0x00040000
#endif
#ifndef MEM_LARGE_PAGES
#define MEM_LARGE_PAGES 0x20000000
#endif
#ifndef MEM_COALESCE_PLACEHOLDERS
#define MEM_COALESCE_PLACEHOLDERS 0x00000001
#endif

// FormatMessage flags
#ifndef FORMAT_MESSAGE_ALLOCATE_BUFFER
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#endif
#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#endif
#ifndef FORMAT_MESSAGE_FROM_STRING
#define FORMAT_MESSAGE_FROM_STRING 0x00000400
#endif
#ifndef FORMAT_MESSAGE_FROM_HMODULE
#define FORMAT_MESSAGE_FROM_HMODULE 0x00000800
#endif
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#endif
#ifndef FORMAT_MESSAGE_ARGUMENT_ARRAY
#define FORMAT_MESSAGE_ARGUMENT_ARRAY 0x00002000
#endif
#ifndef FORMAT_MESSAGE_MAX_WIDTH_MASK
#define FORMAT_MESSAGE_MAX_WIDTH_MASK 0x000000FF
#endif

// Process / thread
#ifndef CREATE_SUSPENDED
#define CREATE_SUSPENDED 0x00000004
#endif
#ifndef CREATE_NEW_CONSOLE
#define CREATE_NEW_CONSOLE 0x00000010
#endif
#ifndef DETACHED_PROCESS
#define DETACHED_PROCESS 0x00000008
#endif
#ifndef THREAD_PRIORITY_NORMAL
#define THREAD_PRIORITY_NORMAL 0
#endif
#ifndef DLL_PROCESS_ATTACH
#define DLL_PROCESS_ATTACH 1
#endif
#ifndef DLL_THREAD_ATTACH
#define DLL_THREAD_ATTACH 2
#endif
#ifndef DLL_THREAD_DETACH
#define DLL_THREAD_DETACH 3
#endif
#ifndef DLL_PROCESS_DETACH
#define DLL_PROCESS_DETACH 0
#endif

// Console
#ifndef STD_INPUT_HANDLE
#define STD_INPUT_HANDLE ((DWORD)-10)
#endif
#ifndef STD_OUTPUT_HANDLE
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#endif
#ifndef STD_ERROR_HANDLE
#define STD_ERROR_HANDLE ((DWORD)-12)
#endif
#ifndef ENABLE_PROCESSED_INPUT
#define ENABLE_PROCESSED_INPUT 0x0001
#endif
#ifndef ENABLE_LINE_INPUT
#define ENABLE_LINE_INPUT 0x0002
#endif
#ifndef ENABLE_ECHO_INPUT
#define ENABLE_ECHO_INPUT 0x0004
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif
#ifndef ENABLE_PROCESSED_OUTPUT
#define ENABLE_PROCESSED_OUTPUT 0x0001
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// Window messages - basics (more in Win32Types.h)
#ifndef WM_NULL
#define WM_NULL 0x0000
#endif
#ifndef WM_CREATE
#define WM_CREATE 0x0001
#endif
#ifndef WM_DESTROY
#define WM_DESTROY 0x0002
#endif
#ifndef WM_MOVE
#define WM_MOVE 0x0003
#endif
#ifndef WM_SIZE
#define WM_SIZE 0x0005
#endif
#ifndef WM_ACTIVATE
#define WM_ACTIVATE 0x0006
#endif
#ifndef WM_SETFOCUS
#define WM_SETFOCUS 0x0007
#endif
#ifndef WM_KILLFOCUS
#define WM_KILLFOCUS 0x0008
#endif
#ifndef WM_PAINT
#define WM_PAINT 0x000F
#endif
#ifndef WM_CLOSE
#define WM_CLOSE 0x0010
#endif
#ifndef WM_QUIT
#define WM_QUIT 0x0012
#endif
#ifndef WM_ERASEBKGND
#define WM_ERASEBKGND 0x0014
#endif
#ifndef WM_KEYDOWN
#define WM_KEYDOWN 0x0100
#endif
#ifndef WM_KEYUP
#define WM_KEYUP 0x0101
#endif
#ifndef WM_CHAR
#define WM_CHAR 0x0102
#endif
#ifndef WM_SYSKEYDOWN
#define WM_SYSKEYDOWN 0x0104
#endif
#ifndef WM_SYSKEYUP
#define WM_SYSKEYUP 0x0105
#endif
#ifndef WM_MOUSEMOVE
#define WM_MOUSEMOVE 0x0200
#endif
#ifndef WM_LBUTTONDOWN
#define WM_LBUTTONDOWN 0x0201
#endif
#ifndef WM_LBUTTONUP
#define WM_LBUTTONUP 0x0202
#endif
#ifndef WM_RBUTTONDOWN
#define WM_RBUTTONDOWN 0x0204
#endif
#ifndef WM_RBUTTONUP
#define WM_RBUTTONUP 0x0205
#endif
#ifndef WM_TIMER
#define WM_TIMER 0x0113
#endif
#ifndef WM_SETCURSOR
#define WM_SETCURSOR 0x0020
#endif
#ifndef WM_NCHITTEST
#define WM_NCHITTEST 0x0084
#endif

// ShowWindow
#ifndef SW_HIDE
#define SW_HIDE 0
#endif
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
#ifndef SW_SHOWMINIMIZED
#define SW_SHOWMINIMIZED 2
#endif
#ifndef SW_SHOWMAXIMIZED
#define SW_SHOWMAXIMIZED 3
#endif
#ifndef SW_SHOW
#define SW_SHOW 5
#endif
#ifndef SW_MINIMIZE
#define SW_MINIMIZE 6
#endif
#ifndef SW_RESTORE
#define SW_RESTORE 9
#endif

// Window styles
#ifndef WS_OVERLAPPED
#define WS_OVERLAPPED 0x00000000L
#endif
#ifndef WS_POPUP
#define WS_POPUP 0x80000000L
#endif
#ifndef WS_CHILD
#define WS_CHILD 0x40000000L
#endif
#ifndef WS_VISIBLE
#define WS_VISIBLE 0x10000000L
#endif
#ifndef WS_DISABLED
#define WS_DISABLED 0x08000000L
#endif
#ifndef WS_CLIPSIBLINGS
#define WS_CLIPSIBLINGS 0x04000000L
#endif
#ifndef WS_CLIPCHILDREN
#define WS_CLIPCHILDREN 0x02000000L
#endif
#ifndef WS_CAPTION
#define WS_CAPTION 0x00C00000L
#endif
#ifndef WS_BORDER
#define WS_BORDER 0x00800000L
#endif
#ifndef WS_THICKFRAME
#define WS_THICKFRAME 0x00040000L
#endif
#ifndef WS_SYSMENU
#define WS_SYSMENU 0x00080000L
#endif
#ifndef WS_MINIMIZEBOX
#define WS_MINIMIZEBOX 0x00020000L
#endif
#ifndef WS_MAXIMIZEBOX
#define WS_MAXIMIZEBOX 0x00010000L
#endif
#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#endif
#ifndef WS_EX_APPWINDOW
#define WS_EX_APPWINDOW 0x00040000L
#endif
#ifndef WS_EX_TOPMOST
#define WS_EX_TOPMOST 0x00000008L
#endif
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x00080000
#endif

// GDI constants
#ifndef WHITE_BRUSH
#define WHITE_BRUSH 0
#endif
#ifndef LTGRAY_BRUSH
#define LTGRAY_BRUSH 1
#endif
#ifndef GRAY_BRUSH
#define GRAY_BRUSH 2
#endif
#ifndef DKGRAY_BRUSH
#define DKGRAY_BRUSH 3
#endif
#ifndef BLACK_BRUSH
#define BLACK_BRUSH 4
#endif
#ifndef NULL_BRUSH
#define NULL_BRUSH 5
#endif
#ifndef HOLLOW_BRUSH
#define HOLLOW_BRUSH NULL_BRUSH
#endif
#ifndef WHITE_PEN
#define WHITE_PEN 6
#endif
#ifndef BLACK_PEN
#define BLACK_PEN 7
#endif
#ifndef NULL_PEN
#define NULL_PEN 8
#endif
#ifndef SYSTEM_FONT
#define SYSTEM_FONT 13
#endif
#ifndef DEFAULT_PALETTE
#define DEFAULT_PALETTE 15
#endif

#ifndef SRCCOPY
#define SRCCOPY (DWORD)0x00CC0020
#endif
#ifndef SRCPAINT
#define SRCPAINT (DWORD)0x00EE0086
#endif
#ifndef SRCAND
#define SRCAND (DWORD)0x008800C6
#endif
#ifndef SRCINVERT
#define SRCINVERT (DWORD)0x00660046
#endif
#ifndef SRCERASE
#define SRCERASE (DWORD)0x00440328
#endif
#ifndef NOTSRCCOPY
#define NOTSRCCOPY (DWORD)0x00330008
#endif
#ifndef NOTSRCERASE
#define NOTSRCERASE (DWORD)0x001100A6
#endif
#ifndef MERGECOPY
#define MERGECOPY (DWORD)0x00C000CA
#endif
#ifndef MERGEPAINT
#define MERGEPAINT (DWORD)0x00BB0226
#endif
#ifndef PATCOPY
#define PATCOPY (DWORD)0x00F00021
#endif
#ifndef PATPAINT
#define PATPAINT (DWORD)0x00FB0A09
#endif
#ifndef PATINVERT
#define PATINVERT (DWORD)0x005A0049
#endif
#ifndef DSTINVERT
#define DSTINVERT (DWORD)0x00550009
#endif
#ifndef BLACKNESS
#define BLACKNESS (DWORD)0x00000042
#endif
#ifndef WHITENESS
#define WHITENESS (DWORD)0x00FF0062
#endif
#ifndef CAPTUREBLT
#define CAPTUREBLT (DWORD)0x40000000
#endif
#ifndef NOMIRRORBITMAP
#define NOMIRRORBITMAP (DWORD)0x80000000
#endif

// Mapping modes
#ifndef MM_TEXT
#define MM_TEXT 1
#endif
#ifndef MM_LOMETRIC
#define MM_LOMETRIC 2
#endif
#ifndef MM_HIMETRIC
#define MM_HIMETRIC 3
#endif
#ifndef MM_LOENGLISH
#define MM_LOENGLISH 4
#endif
#ifndef MM_HIENGLISH
#define MM_HIENGLISH 5
#endif
#ifndef MM_TWIPS
#define MM_TWIPS 6
#endif
#ifndef MM_ISOTROPIC
#define MM_ISOTROPIC 7
#endif
#ifndef MM_ANISOTROPIC
#define MM_ANISOTROPIC 8
#endif

// Background modes
#ifndef TRANSPARENT
#define TRANSPARENT 1
#endif
#ifndef OPAQUE
#define OPAQUE 2
#endif

// ROP2
#ifndef R2_BLACK
#define R2_BLACK 1
#endif
#ifndef R2_NOP
#define R2_NOP 11
#endif
#ifndef R2_COPYPEN
#define R2_COPYPEN 13
#endif
#ifndef R2_WHITE
#define R2_WHITE 16
#endif

// StretchBlt modes
#ifndef BLACKONWHITE
#define BLACKONWHITE 1
#endif
#ifndef WHITEONBLACK
#define WHITEONBLACK 2
#endif
#ifndef COLORONCOLOR
#define COLORONCOLOR 3
#endif
#ifndef HALFTONE
#define HALFTONE 4
#endif

// Fill modes
#ifndef ALTERNATE
#define ALTERNATE 1
#endif
#ifndef WINDING
#define WINDING 2
#endif

// Text alignment
#ifndef TA_NOUPDATECP
#define TA_NOUPDATECP 0
#endif
#ifndef TA_UPDATECP
#define TA_UPDATECP 1
#endif
#ifndef TA_LEFT
#define TA_LEFT 0
#endif
#ifndef TA_RIGHT
#define TA_RIGHT 2
#endif
#ifndef TA_CENTER
#define TA_CENTER 6
#endif
#ifndef TA_TOP
#define TA_TOP 0
#endif
#ifndef TA_BOTTOM
#define TA_BOTTOM 8
#endif
#ifndef TA_BASELINE
#define TA_BASELINE 24
#endif
#ifndef TA_RTLREADING
#define TA_RTLREADING 256
#endif

// Device caps
#ifndef DRIVERVERSION
#define DRIVERVERSION 0
#endif
#ifndef TECHNOLOGY
#define TECHNOLOGY 2
#endif
#ifndef HORZRES
#define HORZRES 8
#endif
#ifndef VERTRES
#define VERTRES 10
#endif
#ifndef LOGPIXELSX
#define LOGPIXELSX 88
#endif
#ifndef LOGPIXELSY
#define LOGPIXELSY 90
#endif
#ifndef BITSPIXEL
#define BITSPIXEL 12
#endif
#ifndef PLANES
#define PLANES 14
#endif
#ifndef NUMBRUSHES
#define NUMBRUSHES 16
#endif
#ifndef NUMPENS
#define NUMPENS 18
#endif
#ifndef NUMFONTS
#define NUMFONTS 22
#endif
#ifndef ASPECTX
#define ASPECTX 40
#endif
#ifndef ASPECTY
#define ASPECTY 42
#endif
#ifndef ASPECTXY
#define ASPECTXY 44
#endif
#ifndef CLIPCAPS
#define CLIPCAPS 36
#endif
#ifndef SIZEPALETTE
#define SIZEPALETTE 104
#endif
#ifndef RASTERCAPS
#define RASTERCAPS 38
#endif
#ifndef TEXTCAPS
#define TEXTCAPS 34
#endif
#ifndef COLORMGMTCAPS
#define COLORMGMTCAPS 121
#endif
#ifndef PHYSICALWIDTH
#define PHYSICALWIDTH 110
#endif
#ifndef PHYSICALHEIGHT
#define PHYSICALHEIGHT 111
#endif
#ifndef PHYSICALOFFSETX
#define PHYSICALOFFSETX 112
#endif
#ifndef PHYSICALOFFSETY
#define PHYSICALOFFSETY 113
#endif
#ifndef VREFRESH
#define VREFRESH 116
#endif
#ifndef DESKTOPHORZRES
#define DESKTOPHORZRES 118
#endif
#ifndef DESKTOPVERTRES
#define DESKTOPVERTRES 117
#endif
#ifndef BLTALIGNMENT
#define BLTALIGNMENT 119
#endif
#ifndef SHADEBLENDCAPS
#define SHADEBLENDCAPS 120
#endif

// HKEY values
#ifndef HKEY_CLASSES_ROOT
#define HKEY_CLASSES_ROOT ((HKEY)(DWORD_PTR)0x80000000)
#endif
#ifndef HKEY_CURRENT_USER
#define HKEY_CURRENT_USER ((HKEY)(DWORD_PTR)0x80000001)
#endif
#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE ((HKEY)(DWORD_PTR)0x80000002)
#endif
#ifndef HKEY_USERS
#define HKEY_USERS ((HKEY)(DWORD_PTR)0x80000003)
#endif
#ifndef HKEY_CURRENT_CONFIG
#define HKEY_CURRENT_CONFIG ((HKEY)(DWORD_PTR)0x80000005)
#endif

// Registry value types
#ifndef REG_NONE
#define REG_NONE 0
#endif
#ifndef REG_SZ
#define REG_SZ 1
#endif
#ifndef REG_EXPAND_SZ
#define REG_EXPAND_SZ 2
#endif
#ifndef REG_BINARY
#define REG_BINARY 3
#endif
#ifndef REG_DWORD
#define REG_DWORD 4
#endif
#ifndef REG_DWORD_BIG_ENDIAN
#define REG_DWORD_BIG_ENDIAN 5
#endif
#ifndef REG_MULTI_SZ
#define REG_MULTI_SZ 7
#endif
#ifndef REG_QWORD
#define REG_QWORD 11
#endif

// CSIDL
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001A
#endif
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001C
#endif
#ifndef CSIDL_PROGRAM_FILES
#define CSIDL_PROGRAM_FILES 0x0026
#endif
#ifndef CSIDL_PROGRAM_FILES_COMMON
#define CSIDL_PROGRAM_FILES_COMMON 0x002B
#endif
#ifndef CSIDL_WINDOWS
#define CSIDL_WINDOWS 0x0024
#endif
#ifndef CSIDL_SYSTEM
#define CSIDL_SYSTEM 0x0025
#endif
#ifndef CSIDL_PROFILE
#define CSIDL_PROFILE 0x0028
#endif
#ifndef CSIDL_PROGRAMS
#define CSIDL_PROGRAMS 0x0002
#endif
#ifndef CSIDL_STARTUP
#define CSIDL_STARTUP 0x0007
#endif
#ifndef CSIDL_MYDOCUMENTS
#define CSIDL_MYDOCUMENTS 0x000C
#endif
#ifndef CSIDL_MYMUSIC
#define CSIDL_MYMUSIC 0x000D
#endif
#ifndef CSIDL_MYPICTURES
#define CSIDL_MYPICTURES 0x0027
#endif
#ifndef CSIDL_MYVIDEO
#define CSIDL_MYVIDEO 0x000E
#endif

#ifndef KF_FLAG_DEFAULT
#define KF_FLAG_DEFAULT 0
#endif
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
#ifndef SHGFP_TYPE_DEFAULT
#define SHGFP_TYPE_DEFAULT 1
#endif

// HRESULT codes
#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif
#ifndef S_FALSE
#define S_FALSE ((HRESULT)1L)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL ((HRESULT)0x80004001L)
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#endif
#ifndef E_POINTER
#define E_POINTER ((HRESULT)0x80004003L)
#endif
#ifndef E_ABORT
#define E_ABORT ((HRESULT)0x80004004L)
#endif
#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif
#ifndef E_INVALIDARG
#define E_INVALIDARG ((HRESULT)0x80070057L)
#endif
#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif
#ifndef E_UNEXPECTED
#define E_UNEXPECTED ((HRESULT)0x8000FFFFL)
#endif
#ifndef DXGI_ERROR_NOT_FOUND
#define DXGI_ERROR_NOT_FOUND ((HRESULT)0x887A0002L)
#endif
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
#ifndef D3DERR_WASSTILLDRAWING
#define D3DERR_WASSTILLDRAWING ((HRESULT)0x8876086DL)
#endif
#ifndef D3DERR_DEVICEREMOVED
#define D3DERR_DEVICEREMOVED ((HRESULT)0x88760870L)
#endif
#ifndef D3DERR_DEVICEHUNG
#define D3DERR_DEVICEHUNG ((HRESULT)0x88760871L)
#endif

// D3D11 constants
#ifndef D3D11_CREATE_DEVICE_SINGLETHREADED
#define D3D11_CREATE_DEVICE_SINGLETHREADED 0x1
#endif
#ifndef D3D11_CREATE_DEVICE_DEBUG
#define D3D11_CREATE_DEVICE_DEBUG 0x2
#endif
#ifndef D3D11_CREATE_DEVICE_BGRA_SUPPORT
#define D3D11_CREATE_DEVICE_BGRA_SUPPORT 0x20
#endif
#ifndef D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS
#define D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS 0x8
#endif
#ifndef D3D11_SDK_VERSION
#define D3D11_SDK_VERSION 7
#endif

// DXGI format constants (subset)
#ifndef DXGI_FORMAT_UNKNOWN
#define DXGI_FORMAT_UNKNOWN 0
#endif
#ifndef DXGI_FORMAT_R32G32B32A32_FLOAT
#define DXGI_FORMAT_R32G32B32A32_FLOAT 2
#endif
#ifndef DXGI_FORMAT_R8G8B8A8_UNORM
#define DXGI_FORMAT_R8G8B8A8_UNORM 28
#endif
#ifndef DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
#define DXGI_FORMAT_R8G8B8A8_UNORM_SRGB 29
#endif
#ifndef DXGI_FORMAT_B8G8R8A8_UNORM
#define DXGI_FORMAT_B8G8R8A8_UNORM 87
#endif
#ifndef DXGI_FORMAT_D24_UNORM_S8_UINT
#define DXGI_FORMAT_D24_UNORM_S8_UINT 45
#endif
#ifndef DXGI_FORMAT_R16G16B16A16_FLOAT
#define DXGI_FORMAT_R16G16B16A16_FLOAT 10
#endif

// PE format constants
#ifndef IMAGE_DOS_SIGNATURE
#define IMAGE_DOS_SIGNATURE 0x5A4D
#endif
#ifndef IMAGE_NT_SIGNATURE
#define IMAGE_NT_SIGNATURE 0x00004550
#endif
#ifndef IMAGE_NT_OPTIONAL_HDR32_MAGIC
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#endif
#ifndef IMAGE_NT_OPTIONAL_HDR64_MAGIC
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#endif
#ifndef IMAGE_ROM_OPTIONAL_HDR_MAGIC
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC 0x107
#endif
#ifndef IMAGE_SUBSYSTEM_UNKNOWN
#define IMAGE_SUBSYSTEM_UNKNOWN 0
#endif
#ifndef IMAGE_SUBSYSTEM_NATIVE
#define IMAGE_SUBSYSTEM_NATIVE 1
#endif
#ifndef IMAGE_SUBSYSTEM_WINDOWS_GUI
#define IMAGE_SUBSYSTEM_WINDOWS_GUI 2
#endif
#ifndef IMAGE_SUBSYSTEM_WINDOWS_CUI
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3
#endif
#ifndef IMAGE_SUBSYSTEM_XBOX
#define IMAGE_SUBSYSTEM_XBOX 14
#endif

#ifndef IMAGE_DIRECTORY_ENTRY_EXPORT
#define IMAGE_DIRECTORY_ENTRY_EXPORT 0
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_IMPORT
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_RESOURCE
#define IMAGE_DIRECTORY_ENTRY_RESOURCE 2
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_EXCEPTION
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_SECURITY
#define IMAGE_DIRECTORY_ENTRY_SECURITY 4
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_BASERELOC
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_DEBUG
#define IMAGE_DIRECTORY_ENTRY_DEBUG 6
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_ARCHITECTURE
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE 7
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_GLOBALPTR
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR 8
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_TLS
#define IMAGE_DIRECTORY_ENTRY_TLS 9
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG 10
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT 11
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_IAT
#define IMAGE_DIRECTORY_ENTRY_IAT 12
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 13
#endif
#ifndef IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14
#endif

#ifndef IMAGE_SCN_CNT_CODE
#define IMAGE_SCN_CNT_CODE 0x00000020
#endif
#ifndef IMAGE_SCN_CNT_INITIALIZED_DATA
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#endif
#ifndef IMAGE_SCN_CNT_UNINITIALIZED_DATA
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#endif
#ifndef IMAGE_SCN_MEM_DISCARDABLE
#define IMAGE_SCN_MEM_DISCARDABLE 0x02000000
#endif
#ifndef IMAGE_SCN_MEM_EXECUTE
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#endif
#ifndef IMAGE_SCN_MEM_READ
#define IMAGE_SCN_MEM_READ 0x40000000
#endif
#ifndef IMAGE_SCN_MEM_WRITE
#define IMAGE_SCN_MEM_WRITE 0x80000000
#endif

#ifndef IMAGE_REL_BASED_ABSOLUTE
#define IMAGE_REL_BASED_ABSOLUTE 0
#endif
#ifndef IMAGE_REL_BASED_HIGH
#define IMAGE_REL_BASED_HIGH 1
#endif
#ifndef IMAGE_REL_BASED_LOW
#define IMAGE_REL_BASED_LOW 2
#endif
#ifndef IMAGE_REL_BASED_HIGHLOW
#define IMAGE_REL_BASED_HIGHLOW 3
#endif
#ifndef IMAGE_REL_BASED_HIGHADJ
#define IMAGE_REL_BASED_HIGHADJ 4
#endif
#ifndef IMAGE_REL_BASED_DIR64
#define IMAGE_REL_BASED_DIR64 10
#endif

#ifndef IMAGE_FILE_RELOCS_STRIPPED
#define IMAGE_FILE_RELOCS_STRIPPED 0x0001
#endif
#ifndef IMAGE_FILE_EXECUTABLE_IMAGE
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#endif
#ifndef IMAGE_FILE_LARGE_ADDRESS_AWARE
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020
#endif
#ifndef IMAGE_FILE_DEBUG_STRIPPED
#define IMAGE_FILE_DEBUG_STRIPPED 0x0200
#endif
#ifndef IMAGE_FILE_DLL
#define IMAGE_FILE_DLL 0x2000
#endif
#ifndef IMAGE_FILE_MACHINE_UNKNOWN
#define IMAGE_FILE_MACHINE_UNKNOWN 0
#endif
#ifndef IMAGE_FILE_MACHINE_I386
#define IMAGE_FILE_MACHINE_I386 0x014c
#endif
#ifndef IMAGE_FILE_MACHINE_AMD64
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#endif
#ifndef IMAGE_FILE_MACHINE_ARM
#define IMAGE_FILE_MACHINE_ARM 0x01c0
#endif
#ifndef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64 0xAA64
#endif

#ifndef IMAGE_ORDINAL_FLAG64
#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ULL
#endif
#ifndef IMAGE_ORDINAL_FLAG32
#define IMAGE_ORDINAL_FLAG32 0x80000000
#endif
#ifdef _WIN64
#define IMAGE_ORDINAL_FLAG IMAGE_ORDINAL_FLAG64
#else
#define IMAGE_ORDINAL_FLAG IMAGE_ORDINAL_FLAG32
#endif
#ifndef IMAGE_SNAP_BY_ORDINAL64
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) (((Ordinal & IMAGE_ORDINAL_FLAG64) != 0))
#endif
#ifndef IMAGE_SNAP_BY_ORDINAL32
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) (((Ordinal & IMAGE_ORDINAL_FLAG32) != 0))
#endif
#ifndef IMAGE_SNAP_BY_ORDINAL
#define IMAGE_SNAP_BY_ORDINAL(Ordinal) (((Ordinal & IMAGE_ORDINAL_FLAG) != 0))
#endif
#ifndef IMAGE_ORDINAL64
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffff)
#endif
#ifndef IMAGE_ORDINAL32
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#endif
#ifndef IMAGE_ORDINAL
#define IMAGE_ORDINAL(Ordinal) IMAGE_ORDINAL64(Ordinal)
#endif

#ifndef IMAGE_DELAYLOAD_ATTRIBUTE_RVA_BASED
#define IMAGE_DELAYLOAD_ATTRIBUTE_RVA_BASED 0x80000000
#endif

// MMSYS errors
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
#ifndef MMSYSERR_INVALFLAG
#define MMSYSERR_INVALFLAG 10
#endif
#ifndef MMSYSERR_INVALPARAM
#define MMSYSERR_INVALPARAM 11
#endif
#ifndef TIMERR_NOERROR
#define TIMERR_NOERROR 0
#endif
#ifndef TIMERR_NOCANDO
#define TIMERR_NOCANDO 1
#endif

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
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

// XInput constants
#ifndef XINPUT_DEVTYPE_GAMEPAD
#define XINPUT_DEVTYPE_GAMEPAD 0x01
#endif
#ifndef XINPUT_DEVSUBTYPE_GAMEPAD
#define XINPUT_DEVSUBTYPE_GAMEPAD 0x01
#endif
#ifndef XINPUT_GAMEPAD_DPAD_UP
#define XINPUT_GAMEPAD_DPAD_UP 0x0001
#endif
#ifndef XINPUT_GAMEPAD_DPAD_DOWN
#define XINPUT_GAMEPAD_DPAD_DOWN 0x0002
#endif
#ifndef XINPUT_GAMEPAD_DPAD_LEFT
#define XINPUT_GAMEPAD_DPAD_LEFT 0x0004
#endif
#ifndef XINPUT_GAMEPAD_DPAD_RIGHT
#define XINPUT_GAMEPAD_DPAD_RIGHT 0x0008
#endif
#ifndef XINPUT_GAMEPAD_START
#define XINPUT_GAMEPAD_START 0x0010
#endif
#ifndef XINPUT_GAMEPAD_BACK
#define XINPUT_GAMEPAD_BACK 0x0020
#endif
#ifndef XINPUT_GAMEPAD_LEFT_THUMB
#define XINPUT_GAMEPAD_LEFT_THUMB 0x0040
#endif
#ifndef XINPUT_GAMEPAD_RIGHT_THUMB
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080
#endif
#ifndef XINPUT_GAMEPAD_LEFT_SHOULDER
#define XINPUT_GAMEPAD_LEFT_SHOULDER 0x0100
#endif
#ifndef XINPUT_GAMEPAD_RIGHT_SHOULDER
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#endif
#ifndef XINPUT_GAMEPAD_A
#define XINPUT_GAMEPAD_A 0x1000
#endif
#ifndef XINPUT_GAMEPAD_B
#define XINPUT_GAMEPAD_B 0x2000
#endif
#ifndef XINPUT_GAMEPAD_X
#define XINPUT_GAMEPAD_X 0x4000
#endif
#ifndef XINPUT_GAMEPAD_Y
#define XINPUT_GAMEPAD_Y 0x8000
#endif
#ifndef XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 7849
#endif
#ifndef XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#endif
#ifndef XINPUT_GAMEPAD_TRIGGER_THRESHOLD
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 30
#endif

// Virtual key codes (subset)
#ifndef VK_BACK
#define VK_BACK 0x08
#endif
#ifndef VK_TAB
#define VK_TAB 0x09
#endif
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif
#ifndef VK_SHIFT
#define VK_SHIFT 0x10
#endif
#ifndef VK_CONTROL
#define VK_CONTROL 0x11
#endif
#ifndef VK_MENU
#define VK_MENU 0x12
#endif
#ifndef VK_CAPITAL
#define VK_CAPITAL 0x14
#endif
#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif
#ifndef VK_SPACE
#define VK_SPACE 0x20
#endif
#ifndef VK_PRIOR
#define VK_PRIOR 0x21
#endif
#ifndef VK_NEXT
#define VK_NEXT 0x22
#endif
#ifndef VK_END
#define VK_END 0x23
#endif
#ifndef VK_HOME
#define VK_HOME 0x24
#endif
#ifndef VK_LEFT
#define VK_LEFT 0x25
#endif
#ifndef VK_UP
#define VK_UP 0x26
#endif
#ifndef VK_RIGHT
#define VK_RIGHT 0x27
#endif
#ifndef VK_DOWN
#define VK_DOWN 0x28
#endif
#ifndef VK_INSERT
#define VK_INSERT 0x2D
#endif
#ifndef VK_DELETE
#define VK_DELETE 0x2E
#endif
#ifndef VK_LWIN
#define VK_LWIN 0x5B
#endif
#ifndef VK_RWIN
#define VK_RWIN 0x5C
#endif
#ifndef VK_NUMPAD0
#define VK_NUMPAD0 0x60
#endif
#ifndef VK_F1
#define VK_F1 0x70
#endif
#ifndef VK_F2
#define VK_F2 0x71
#endif
#ifndef VK_F3
#define VK_F3 0x72
#endif
#ifndef VK_F4
#define VK_F4 0x73
#endif
#ifndef VK_F5
#define VK_F5 0x74
#endif
#ifndef VK_F6
#define VK_F6 0x75
#endif
#ifndef VK_F7
#define VK_F7 0x76
#endif
#ifndef VK_F8
#define VK_F8 0x77
#endif
#ifndef VK_F9
#define VK_F9 0x78
#endif
#ifndef VK_F10
#define VK_F10 0x79
#endif
#ifndef VK_F11
#define VK_F11 0x7A
#endif
#ifndef VK_F12
#define VK_F12 0x7B
#endif
#ifndef VK_NUMLOCK
#define VK_NUMLOCK 0x90
#endif
#ifndef VK_LSHIFT
#define VK_LSHIFT 0xA0
#endif
#ifndef VK_RSHIFT
#define VK_RSHIFT 0xA1
#endif
#ifndef VK_LCONTROL
#define VK_LCONTROL 0xA2
#endif
#ifndef VK_RCONTROL
#define VK_RCONTROL 0xA3
#endif
#ifndef VK_LMENU
#define VK_LMENU 0xA4
#endif
#ifndef VK_RMENU
#define VK_RMENU 0xA5
#endif

// ===========================================================================
// 10) PE format types
// ===========================================================================
typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD VirtualAddress;
    DWORD Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_DOS_HEADER {
    WORD e_magic;
    WORD e_cblp;
    WORD e_cp;
    WORD e_crlc;
    WORD e_cparhdr;
    WORD e_minalloc;
    WORD e_maxalloc;
    WORD e_ss;
    WORD e_sp;
    WORD e_csum;
    WORD e_ip;
    WORD e_cs;
    WORD e_lfarlc;
    WORD e_ovno;
    WORD e_res[4];
    WORD e_oemid;
    WORD e_oeminfo;
    WORD e_res2[10];
    LONG e_lfanew;
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    WORD Machine;
    WORD NumberOfSections;
    DWORD TimeDateStamp;
    DWORD PointerToSymbolTable;
    DWORD NumberOfSymbols;
    WORD SizeOfOptionalHeader;
    WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    ULONGLONG ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    ULONGLONG SizeOfStackReserve;
    ULONGLONG SizeOfStackCommit;
    ULONGLONG SizeOfHeapReserve;
    ULONGLONG SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_OPTIONAL_HEADER32 {
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    DWORD BaseOfData;
    DWORD ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    DWORD SizeOfStackReserve;
    DWORD SizeOfStackCommit;
    DWORD SizeOfHeapReserve;
    DWORD SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_NT_HEADERS64 {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS32 {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

#ifdef _WIN64
typedef IMAGE_NT_HEADERS64 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;
typedef IMAGE_OPTIONAL_HEADER64 IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER64 PIMAGE_OPTIONAL_HEADER;
#else
typedef IMAGE_NT_HEADERS32 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;
typedef IMAGE_OPTIONAL_HEADER32 IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32 PIMAGE_OPTIONAL_HEADER;
#endif

typedef struct _IMAGE_SECTION_HEADER {
    BYTE Name[8];
    union {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;
    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD NumberOfRelocations;
    WORD NumberOfLinenumbers;
    DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD Characteristics;
        DWORD OriginalFirstThunk;
    } DUMMYUNIONNAME;
    DWORD TimeDateStamp;
    DWORD ForwarderChain;
    DWORD Name;
    DWORD FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_DELAYLOAD_DESCRIPTOR {
    DWORD Attributes;
    DWORD DllNameRVA;
    DWORD ModuleHandleRVA;
    DWORD ImportAddressTableRVA;
    DWORD ImportNameTableRVA;
    DWORD BoundImportAddressTableRVA;
    DWORD UnloadInformationTableRVA;
    DWORD TimeDateStamp;
} IMAGE_DELAYLOAD_DESCRIPTOR, *PIMAGE_DELAYLOAD_DESCRIPTOR;

typedef struct _IMAGE_THUNK_DATA64 {
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
} IMAGE_THUNK_DATA64, *PIMAGE_THUNK_DATA64;

typedef struct _IMAGE_THUNK_DATA32 {
    union {
        DWORD ForwarderString;
        DWORD Function;
        DWORD Ordinal;
        DWORD AddressOfData;
    } u1;
} IMAGE_THUNK_DATA32, *PIMAGE_THUNK_DATA32;

#ifdef _WIN64
typedef IMAGE_THUNK_DATA64 IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA64 PIMAGE_THUNK_DATA;
#else
typedef IMAGE_THUNK_DATA32 IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32 PIMAGE_THUNK_DATA;
#endif

typedef struct _IMAGE_IMPORT_BY_NAME {
    WORD Hint;
    BYTE Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_BASE_RELOCATION {
    DWORD VirtualAddress;
    DWORD SizeOfBlock;
} IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

typedef struct _IMAGE_TLS_DIRECTORY64 {
    ULONGLONG StartAddressOfRawData;
    ULONGLONG EndAddressOfRawData;
    ULONGLONG AddressOfIndex;
    ULONGLONG AddressOfCallBacks;
    DWORD SizeOfZeroFill;
    DWORD Characteristics;
} IMAGE_TLS_DIRECTORY64, *PIMAGE_TLS_DIRECTORY64;

typedef struct _IMAGE_TLS_DIRECTORY32 {
    DWORD StartAddressOfRawData;
    DWORD EndAddressOfRawData;
    DWORD AddressOfIndex;
    DWORD AddressOfCallBacks;
    DWORD SizeOfZeroFill;
    DWORD Characteristics;
} IMAGE_TLS_DIRECTORY32, *PIMAGE_TLS_DIRECTORY32;

#ifdef _WIN64
typedef IMAGE_TLS_DIRECTORY64 IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY64 PIMAGE_TLS_DIRECTORY;
#else
typedef IMAGE_TLS_DIRECTORY32 IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY32 PIMAGE_TLS_DIRECTORY;
#endif

typedef struct _IMAGE_EXPORT_DIRECTORY {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD MajorVersion;
    WORD MinorVersion;
    DWORD Name;
    DWORD Base;
    DWORD NumberOfFunctions;
    DWORD NumberOfNames;
    DWORD AddressOfFunctions;
    DWORD AddressOfNames;
    DWORD AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

// WinSock subset
typedef UINT_PTR SOCKET;
typedef unsigned long u_long;
struct sockaddr;
typedef struct hostent { char *h_name; char **h_aliases; short h_addrtype; short h_length; char **h_addr_list; } HOSTENT;

// Function-pointer typedefs used by kernel32 declarations below.
// Forward-declared so we can use them in the kernel32 function signatures.
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
typedef void (WINAPI *LPOVERLAPPED_COMPLETION_ROUTINE)(DWORD, DWORD, LPVOID);
typedef void *PCONTEXT;
typedef LONG (WINAPI *PVECTORED_EXCEPTION_HANDLER)(void*);
typedef LONG (WINAPI *LPTOP_LEVEL_EXCEPTION_FILTER)(void*);

// Function-pointer typedefs used by user32 declarations below.
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef BOOL (CALLBACK *MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);
typedef BOOL (CALLBACK *GRAYSTRINGPROC)(HDC, LPARAM, int);
typedef const RECT *LPCRECT;

// WinMM pointer aliases
typedef HWAVEOUT *LPHWAVEOUT;
typedef HMIDIOUT *LPHMIDIOUT;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;

// user32 helper structs (declared before user32 function block)
typedef struct tagWINDOWPLACEMENT {
    UINT length;
    UINT flags;
    UINT showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT rcNormalPosition;
} WINDOWPLACEMENT, *PWINDOWPLACEMENT, *LPWINDOWPLACEMENT;
typedef struct tagDRAWTEXTPARAMS {
    UINT cbSize;
    int iTabLength;
    int iLeftMargin;
    int iRightMargin;
    UINT uiLengthDrawn;
} DRAWTEXTPARAMS, *LPDRAWTEXTPARAMS;

// ---------------------------------------------------------------------------
// user32 gap-fill helper types (added by GAP-USER32)
// ---------------------------------------------------------------------------
typedef DWORD HTOUCHINPUT;

typedef LRESULT (CALLBACK *HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);
typedef INT_PTR (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (CALLBACK *DRAWSTATEPROC)(HDC, LPARAM, WPARAM, int, int);
typedef BOOL (CALLBACK *WNDENUMPROC)(HWND hwnd, LPARAM lParam);
typedef BOOL (CALLBACK *PROPENUMPROCA)(HWND, LPCSTR, HANDLE);
typedef BOOL (CALLBACK *PROPENUMPROCW)(HWND, LPCWSTR, HANDLE);

typedef struct tagICONINFO {
    BOOL    fIcon;
    DWORD   xHotspot;
    DWORD   yHotspot;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
} ICONINFO, *PICONINFO;

typedef struct tagFLASHWINFO {
    UINT  cbSize;
    HWND  hwnd;
    DWORD dwFlags;
    UINT  uCount;
    DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;

typedef struct tagTRACKMOUSEEVENT {
    UINT  cbSize;
    DWORD dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

typedef struct tagWINDOWINFO {
    DWORD cbSize;
    RECT  rcWindow;
    RECT  rcClient;
    DWORD dwStyle;
    DWORD dwExStyle;
    DWORD dwWindowStatus;
    UINT  cxWindowBorders;
    UINT  cyWindowBorders;
    ATOM  atomWindowType;
    WORD  wCreatorVersion;
} WINDOWINFO, *PWINDOWINFO, *LPWINDOWINFO;

typedef struct tagMENUITEMINFOW {
    UINT       cbSize;
    UINT       fMask;
    UINT       fType;
    UINT       fState;
    UINT       wID;
    HMENU      hSubMenu;
    HBITMAP    hbmpChecked;
    HBITMAP    hbmpUnchecked;
    ULONG_PTR  dwItemData;
    LPWSTR     dwTypeData;
    UINT       cch;
    HBITMAP    hbmpItem;
} MENUITEMINFOW, *LPMENUITEMINFOW, *LPCMENUITEMINFOW;
typedef MENUITEMINFOW MENUITEMINFO;

typedef struct _DEVMODEW {
    WCHAR dmDeviceName[32];
    WORD  dmSpecVersion;
    WORD  dmDriverVersion;
    WORD  dmSize;
    WORD  dmDriverExtra;
    DWORD dmFields;
    union {
        struct {
            short dmOrientation;
            short dmPaperSize;
            short dmPaperLength;
            short dmPaperWidth;
            short dmScale;
            short dmCopies;
            short dmDefaultSource;
            short dmPrintQuality;
        } DUMMYSTRUCTNAME;
        POINT dmPosition;
        struct {
            DWORD dmDisplayOrientation;
            DWORD dmDisplayFixedOutput;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME;
    short dmColor;
    short dmDuplex;
    short dmYResolution;
    short dmTTOption;
    short dmCollate;
    WCHAR dmFormName[32];
    WORD  dmLogPixels;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
    union {
        DWORD dmDisplayFlags;
        DWORD dmNup;
    } DUMMYUNIONNAME2;
    DWORD dmDisplayFrequency;
    DWORD dmICMMethod;
    DWORD dmICMIntent;
    DWORD dmMediaType;
    DWORD dmDitherType;
    DWORD dmReserved1;
    DWORD dmReserved2;
    DWORD dmPanningWidth;
    DWORD dmPanningHeight;
} DEVMODEW, *PDEVMODEW, *LPDEVMODEW;
typedef DEVMODEW DEVMODE;

typedef struct _DISPLAY_DEVICEW {
    DWORD cb;
    WCHAR DeviceName[32];
    WCHAR DeviceString[128];
    DWORD StateFlags;
    WCHAR DeviceID[128];
    WCHAR DeviceKey[128];
} DISPLAY_DEVICEW, *PDISPLAY_DEVICEW, *LPDISPLAY_DEVICEW;
typedef struct _DISPLAY_DEVICEA {
    DWORD cb;
    CHAR  DeviceName[32];
    CHAR  DeviceString[128];
    DWORD StateFlags;
    CHAR  DeviceID[128];
    CHAR  DeviceKey[128];
} DISPLAY_DEVICEA, *PDISPLAY_DEVICEA, *LPDISPLAY_DEVICEA;

typedef struct tagMONITORINFOEXW {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
    WCHAR szDevice[32];
} MONITORINFOEXW, *LPMONITORINFOEXW;
typedef struct tagMONITORINFOEXA {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
    CHAR  szDevice[32];
} MONITORINFOEXA, *LPMONITORINFOEXA;

typedef struct tagRAWINPUTHEADER {
    DWORD   dwType;
    DWORD   dwSize;
    HANDLE  hDevice;
    WPARAM  wParam;
} RAWINPUTHEADER, *PRAWINPUTHEADER;

typedef struct tagRAWINPUTDEVICE {
    USHORT usUsagePage;
    USHORT usUsage;
    DWORD  dwFlags;
    HWND   hwndTarget;
} RAWINPUTDEVICE, *PRAWINPUTDEVICE, *PCRAWINPUTDEVICE;

typedef struct tagRAWINPUTDEVICELIST {
    HANDLE hDevice;
    DWORD  dwType;
    WCHAR  szName[256];
} RAWINPUTDEVICELIST, *PRAWINPUTDEVICELIST;

typedef struct tagRAWINPUT {
    RAWINPUTHEADER header;
    DWORD          dwPaddingHuge[64];
} RAWINPUT, *PRAWINPUT, *LPRAWINPUT;

typedef struct tagTOUCHINPUT {
    LONG       x;
    LONG       y;
    HANDLE     hSource;
    DWORD      dwID;
    DWORD      dwFlags;
    DWORD      dwMask;
    DWORD      dwTime;
    ULONG_PTR  dwExtraInfo;
    DWORD      cxContact;
    DWORD      cyContact;
} TOUCHINPUT, *PTOUCHINPUT;

typedef struct tagINPUT {
    DWORD type;
    union {
        BYTE padding[64];
    } DUMMYUNIONNAME;
} INPUT, *PINPUT, *LPINPUT;

// USEROBJECT flags struct (for GetUserObjectInformation)
typedef struct tagUSEROBJECTFLAGS {
    BOOL fInherit;
    BOOL fReserved;
    DWORD dwFlags;
} USEROBJECTFLAGS, *PUSEROBJECTFLAGS;

// Display Config (UWP blocks these — declarations exist for syntax only).
typedef enum DISPLAYCONFIG_DEVICE_INFO_TYPE {
    DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME         = 1,
    DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME         = 2,
    DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE = 3,
    DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME        = 4,
    DISPLAYCONFIG_DEVICE_INFO_SET_TARGET_PERSISTENCE  = 5,
    DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_BASE_TYPE    = 6
} DISPLAYCONFIG_DEVICE_INFO_TYPE;

typedef struct DISPLAYCONFIG_DEVICE_INFO_HEADER {
    DISPLAYCONFIG_DEVICE_INFO_TYPE type;
    DWORD                          size;
    LUID                           adapterId;
    UINT32                         id;
} DISPLAYCONFIG_DEVICE_INFO_HEADER;

typedef struct DISPLAYCONFIG_TARGET_DEVICE_NAME {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    DWORD  flags;
    DWORD  outputTechnology;
    UINT16 edidManufactureId;
    UINT16 edidProductCodeId;
    UINT32 connectorInstance;
    WCHAR  monitorFriendlyDeviceName[64];
    WCHAR  monitorDevicePath[128];
} DISPLAYCONFIG_TARGET_DEVICE_NAME;

typedef struct DISPLAYCONFIG_RATIONAL {
    UINT32 Numerator;
    UINT32 Denominator;
} DISPLAYCONFIG_RATIONAL;

typedef struct DISPLAYCONFIG_PATH_SOURCE_INFO {
    LUID   adapterId;
    UINT32 id;
    union {
        UINT32 modeInfoIdx;
        struct {
            UINT32 cloneGroupId       : 16;
            UINT32 sourceModeInfoIdx  : 16;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    DWORD  statusFlags;
} DISPLAYCONFIG_PATH_SOURCE_INFO;

typedef struct DISPLAYCONFIG_PATH_TARGET_INFO {
    LUID   adapterId;
    UINT32 id;
    union {
        UINT32 modeInfoIdx;
        struct {
            UINT32 desktopModeInfoIdx : 16;
            UINT32 cloneGroupId       : 16;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    DWORD outputTechnology;
    DWORD rotation;
    DWORD scaling;
    DISPLAYCONFIG_RATIONAL refreshRate;
    UINT16 scanLineOrdering;
    BOOL targetAvailable;
    DWORD statusFlags;
} DISPLAYCONFIG_PATH_TARGET_INFO;

typedef struct DISPLAYCONFIG_PATH_INFO {
    DISPLAYCONFIG_PATH_SOURCE_INFO sourceInfo;
    DISPLAYCONFIG_PATH_TARGET_INFO targetInfo;
    DWORD flags;
} DISPLAYCONFIG_PATH_INFO;

typedef struct DISPLAYCONFIG_2DREGION {
    UINT32 cx;
    UINT32 cy;
} DISPLAYCONFIG_2DREGION;

typedef struct DISPLAYCONFIG_VIDEO_SIGNAL_INFO {
    UINT64 pixelRate;
    DISPLAYCONFIG_RATIONAL hSyncFreq;
    DISPLAYCONFIG_RATIONAL vSyncFreq;
    DISPLAYCONFIG_2DREGION activeSize;
    DISPLAYCONFIG_2DREGION totalSize;
    UINT32 videoStandard;
    DWORD scanLineOrdering;
} DISPLAYCONFIG_VIDEO_SIGNAL_INFO;

typedef struct DISPLAYCONFIG_TARGET_MODE {
    DISPLAYCONFIG_VIDEO_SIGNAL_INFO targetVideoSignalInfo;
} DISPLAYCONFIG_TARGET_MODE;

typedef struct DISPLAYCONFIG_SOURCE_MODE {
    UINT32 width;
    UINT32 height;
    UINT32 pixelFormat;
    POINT  position;
} DISPLAYCONFIG_SOURCE_MODE;

typedef struct DISPLAYCONFIG_MODE_INFO_UNION {
    DISPLAYCONFIG_TARGET_MODE targetMode;
    DISPLAYCONFIG_SOURCE_MODE sourceMode;
} DISPLAYCONFIG_MODE_INFO_UNION;

typedef struct DISPLAYCONFIG_MODE_INFO {
    DWORD  infoType;
    UINT32 id;
    LUID   adapterId;
    DISPLAYCONFIG_MODE_INFO_UNION modeInfo;
} DISPLAYCONFIG_MODE_INFO;

// ===========================================================================
// 11) Function declarations (kernel32 subset)
// ===========================================================================
extern "C" {
WINBASEAPI HANDLE WINAPI CreateFileW(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
WINBASEAPI HANDLE WINAPI CreateFileA(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
WINBASEAPI BOOL WINAPI CloseHandle(HANDLE);
WINBASEAPI BOOL WINAPI ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
WINBASEAPI BOOL WINAPI WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
WINBASEAPI DWORD WINAPI SetFilePointer(HANDLE, LONG, PLONG, DWORD);
WINBASEAPI BOOL WINAPI SetFilePointerEx(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD);
WINBASEAPI BOOL WINAPI FlushFileBuffers(HANDLE);
WINBASEAPI BOOL WINAPI GetFileSizeEx(HANDLE, PLARGE_INTEGER);
WINBASEAPI DWORD WINAPI GetFileSize(HANDLE, LPDWORD);
WINBASEAPI BOOL WINAPI GetFileAttributesExW(LPCWSTR, int, LPVOID);
WINBASEAPI DWORD WINAPI GetFileAttributesW(LPCWSTR);
WINBASEAPI BOOL WINAPI SetFileAttributesW(LPCWSTR, DWORD);
WINBASEAPI BOOL WINAPI DeleteFileW(LPCWSTR);
WINBASEAPI BOOL WINAPI MoveFileW(LPCWSTR, LPCWSTR);
WINBASEAPI BOOL WINAPI CopyFileW(LPCWSTR, LPCWSTR, BOOL);
WINBASEAPI BOOL WINAPI CreateDirectoryW(LPCWSTR, LPSECURITY_ATTRIBUTES);
WINBASEAPI BOOL WINAPI RemoveDirectoryW(LPCWSTR);
WINBASEAPI HANDLE WINAPI FindFirstFileW(LPCWSTR, LPWIN32_FIND_DATAW);
WINBASEAPI BOOL WINAPI FindNextFileW(HANDLE, LPWIN32_FIND_DATAW);
WINBASEAPI BOOL WINAPI FindClose(HANDLE);
WINBASEAPI LPVOID WINAPI VirtualAlloc(LPVOID, SIZE_T, DWORD, DWORD);
WINBASEAPI LPVOID WINAPI VirtualAllocEx(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
WINBASEAPI BOOL WINAPI VirtualProtect(LPVOID, SIZE_T, DWORD, LPDWORD);
WINBASEAPI BOOL WINAPI VirtualFree(LPVOID, SIZE_T, DWORD);
WINBASEAPI BOOL WINAPI VirtualQuery(LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);
WINBASEAPI HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
WINBASEAPI HANDLE WINAPI CreateEventW(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR);
WINBASEAPI HANDLE WINAPI CreateMutexW(LPSECURITY_ATTRIBUTES, BOOL, LPCWSTR);
WINBASEAPI HANDLE WINAPI CreateSemaphoreW(LPSECURITY_ATTRIBUTES, LONG, LONG, LPCWSTR);
WINBASEAPI BOOL WINAPI SetEvent(HANDLE);
WINBASEAPI BOOL WINAPI ResetEvent(HANDLE);
WINBASEAPI BOOL WINAPI ReleaseMutex(HANDLE);
WINBASEAPI BOOL WINAPI ReleaseSemaphore(HANDLE, LONG, LPLONG);
WINBASEAPI DWORD WINAPI WaitForSingleObject(HANDLE, DWORD);
WINBASEAPI DWORD WINAPI WaitForMultipleObjects(DWORD, const HANDLE*, BOOL, DWORD);
WINBASEAPI void WINAPI Sleep(DWORD);
WINBASEAPI DWORD WINAPI SleepEx(DWORD, BOOL);
WINBASEAPI void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION);
WINBASEAPI void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION);
WINBASEAPI void WINAPI EnterCriticalSection(LPCRITICAL_SECTION);
WINBASEAPI void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION);
WINBASEAPI BOOL WINAPI InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION, DWORD);
WINBASEAPI DWORD WINAPI GetCurrentThreadId();
WINBASEAPI DWORD WINAPI GetCurrentProcessId();
WINBASEAPI HANDLE WINAPI GetCurrentProcess();
WINBASEAPI HANDLE WINAPI GetCurrentThread();
WINBASEAPI DWORD WINAPI GetLastError();
WINBASEAPI void WINAPI SetLastError(DWORD);
WINBASEAPI HMODULE WINAPI GetModuleHandleW(LPCWSTR);
WINBASEAPI HMODULE WINAPI GetModuleHandleA(LPCSTR);
WINBASEAPI FARPROC WINAPI GetProcAddress(HMODULE, LPCSTR);
WINBASEAPI HMODULE WINAPI LoadLibraryW(LPCWSTR);
WINBASEAPI HMODULE WINAPI LoadLibraryExW(LPCWSTR, HANDLE, DWORD);
WINBASEAPI BOOL WINAPI FreeLibrary(HMODULE);
WINBASEAPI DWORD WINAPI GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
WINBASEAPI void WINAPI GetSystemInfo(LPSYSTEM_INFO);
WINBASEAPI BOOL WINAPI GlobalMemoryStatusEx(LPMEMORYSTATUSEX);
WINBASEAPI DWORD WINAPI GetTickCount();
WINBASEAPI ULONGLONG GetTickCount64();
WINBASEAPI BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER*);
WINBASEAPI BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER*);
WINBASEAPI DWORD WINAPI GetEnvironmentVariableW(LPCWSTR, LPWSTR, DWORD);
WINBASEAPI BOOL WINAPI SetEnvironmentVariableW(LPCWSTR, LPCWSTR);
WINBASEAPI LPWSTR WINAPI GetCommandLineW();
WINBASEAPI LPWCH WINAPI GetEnvironmentStringsW();
WINBASEAPI BOOL WINAPI FreeEnvironmentStringsW(LPWCH);
WINBASEAPI DWORD WINAPI GetTempPathW(DWORD, LPWSTR);
WINBASEAPI DWORD WINAPI GetTempFileNameW(LPCWSTR, LPCWSTR, UINT, LPWSTR);
WINBASEAPI DWORD WINAPI FormatMessageW(DWORD, LPCVOID, DWORD, DWORD, LPWSTR, DWORD, va_list*);
WINBASEAPI DWORD WINAPI GetCurrentDirectoryW(DWORD, LPWSTR);
WINBASEAPI BOOL WINAPI SetCurrentDirectoryW(LPCWSTR);
WINBASEAPI DWORD WINAPI GetFullPathNameW(LPCWSTR, DWORD, LPWSTR, LPWSTR*);
WINBASEAPI BOOL WINAPI GetVolumeInformationW(LPCWSTR, LPWSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPWSTR, DWORD);
WINBASEAPI void WINAPI OutputDebugStringW(LPCWSTR);
WINBASEAPI BOOL WINAPI IsDebuggerPresent();
WINBASEAPI void WINAPI DebugBreak();
WINBASEAPI UINT WINAPI GetConsoleCP();
WINBASEAPI UINT WINAPI GetConsoleOutputCP();
WINBASEAPI BOOL WINAPI WriteConsoleW(HANDLE, const void*, DWORD, LPDWORD, LPVOID);
WINBASEAPI BOOL WINAPI AllocConsole();
WINBASEAPI BOOL WINAPI GetConsoleMode(HANDLE, LPDWORD);
WINBASEAPI BOOL WINAPI SetConsoleMode(HANDLE, DWORD);
WINBASEAPI BOOL WINAPI FlushConsoleInputBuffer(HANDLE);
WINBASEAPI HANDLE WINAPI GetStdHandle(DWORD);
WINBASEAPI BOOL WINAPI RtlAddFunctionTable(void*, DWORD, DWORD64);
WINBASEAPI BOOL WINAPI RtlDeleteFunctionTable(void*);
WINBASEAPI BOOL WINAPI DisableThreadLibraryCalls(HMODULE);
WINBASEAPI void* __cdecl _aligned_malloc(size_t, size_t);
WINBASEAPI void __cdecl _aligned_free(void*);
WINBASEAPI void* __cdecl malloc(size_t);
WINBASEAPI void __cdecl free(void*);
WINBASEAPI void* __cdecl calloc(size_t, size_t);
WINBASEAPI void* __cdecl realloc(void*, size_t);
WINBASEAPI void* __cdecl _msize(void*);
WINBASEAPI BOOL WINAPI CreateProcessW(LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
WINBASEAPI BOOL WINAPI GetExitCodeProcess(HANDLE, LPDWORD);
WINBASEAPI BOOL WINAPI GetExitCodeThread(HANDLE, LPDWORD);
WINBASEAPI DWORD WINAPI ResumeThread(HANDLE);
WINBASEAPI DWORD WINAPI SuspendThread(HANDLE);
WINBASEAPI BOOL WINAPI TerminateProcess(HANDLE, UINT);
WINBASEAPI BOOL WINAPI TerminateThread(HANDLE, DWORD);
WINBASEAPI DWORD WINAPI WaitForSingleObjectEx(HANDLE, DWORD, BOOL);
WINBASEAPI DWORD WINAPI WaitForMultipleObjectsEx(DWORD, const HANDLE*, BOOL, DWORD, BOOL);
WINBASEAPI BOOL WINAPI GetOverlappedResult(HANDLE, LPOVERLAPPED, LPDWORD, BOOL);
WINBASEAPI HANDLE WINAPI CreateFileMappingW(HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCWSTR);
WINBASEAPI LPVOID WINAPI MapViewOfFile(HANDLE, DWORD, DWORD, DWORD, SIZE_T);
WINBASEAPI BOOL WINAPI UnmapViewOfFile(LPCVOID);
WINBASEAPI BOOL WINAPI FlushViewOfFile(LPCVOID, SIZE_T);
WINBASEAPI BOOL WINAPI SetEndOfFile(HANDLE);
WINBASEAPI BOOL WINAPI SetFileTime(HANDLE, const FILETIME*, const FILETIME*, const FILETIME*);
WINBASEAPI BOOL WINAPI GetFileTime(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME);
WINBASEAPI BOOL WINAPI SetFileValidData(HANDLE, LONGLONG);
WINBASEAPI BOOL WINAPI GetFileInformationByHandle(HANDLE, LPBY_HANDLE_FILE_INFORMATION);
WINBASEAPI DWORD WINAPI GetLogicalDrives();
WINBASEAPI UINT WINAPI GetDriveTypeW(LPCWSTR);
WINBASEAPI BOOL WINAPI GetDiskFreeSpaceExW(LPCWSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
WINBASEAPI BOOL WINAPI GetVolumePathNameW(LPCWSTR, LPWSTR, DWORD);
WINBASEAPI HANDLE WINAPI CreateIoCompletionPort(HANDLE, HANDLE, ULONG_PTR, DWORD);
WINBASEAPI BOOL WINAPI GetQueuedCompletionStatus(HANDLE, LPDWORD, PULONG_PTR, LPOVERLAPPED*, DWORD);
WINBASEAPI BOOL WINAPI PostQueuedCompletionStatus(HANDLE, DWORD, ULONG_PTR, LPOVERLAPPED);
WINBASEAPI BOOL WINAPI ReadFileEx(HANDLE, LPVOID, DWORD, LPOVERLAPPED, LPOVERLAPPED_COMPLETION_ROUTINE);
WINBASEAPI BOOL WINAPI WriteFileEx(HANDLE, LPCVOID, DWORD, LPOVERLAPPED, LPOVERLAPPED_COMPLETION_ROUTINE);
WINBASEAPI BOOL WINAPI CancelIo(HANDLE);
WINBASEAPI BOOL WINAPI CancelIoEx(HANDLE, LPOVERLAPPED);
WINBASEAPI DWORD WINAPI GetThreadId(HANDLE);
WINBASEAPI DWORD WINAPI GetProcessId(HANDLE);
WINBASEAPI HANDLE WINAPI OpenProcess(DWORD, BOOL, DWORD);
WINBASEAPI HANDLE WINAPI OpenThread(DWORD, BOOL, DWORD);
WINBASEAPI BOOL WINAPI DuplicateHandle(HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD);
WINBASEAPI BOOL WINAPI SetThreadPriority(HANDLE, int);
WINBASEAPI int WINAPI GetThreadPriority(HANDLE);
WINBASEAPI BOOL WINAPI GetProcessTimes(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME);
WINBASEAPI BOOL WINAPI GetThreadTimes(HANDLE, LPFILETIME, LPFILETIME, LPFILETIME, LPFILETIME);
WINBASEAPI DWORD WINAPI TlsAlloc();
WINBASEAPI LPVOID WINAPI TlsGetValue(DWORD);
WINBASEAPI BOOL WINAPI TlsSetValue(DWORD, LPVOID);
WINBASEAPI BOOL WINAPI TlsFree(DWORD);
WINBASEAPI DWORD_PTR WINAPI SetThreadAffinityMask(HANDLE, DWORD_PTR);
WINBASEAPI DWORD_PTR WINAPI SetThreadIdealProcessor(HANDLE, DWORD);
WINBASEAPI HANDLE WINAPI CreateRemoteThread(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
WINBASEAPI BOOL WINAPI GetProcessAffinityMask(HANDLE, PDWORD_PTR, PDWORD_PTR);
WINBASEAPI DWORD_PTR WINAPI GetActiveProcessorCount(WORD);
WINBASEAPI void WINAPI RtlCaptureContext(PCONTEXT);
WINBASEAPI void WINAPI RtlCaptureStackBackTrace(ULONG, ULONG, PVOID*, PULONG);
WINBASEAPI PVOID WINAPI AddVectoredExceptionHandler(ULONG, PVECTORED_EXCEPTION_HANDLER);
WINBASEAPI ULONG WINAPI RemoveVectoredExceptionHandler(PVOID);
WINBASEAPI BOOL WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER);
WINBASEAPI int WINAPI MultiByteToWideChar(UINT, DWORD, LPCSTR, int, LPWSTR, int);
WINBASEAPI int WINAPI WideCharToMultiByte(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
WINBASEAPI LPWSTR WINAPI lstrcpyW(LPWSTR, LPCWSTR);
WINBASEAPI LPWSTR WINAPI lstrcpynW(LPWSTR, LPCWSTR, int);
WINBASEAPI LPWSTR WINAPI lstrcatW(LPWSTR, LPCWSTR);
WINBASEAPI int WINAPI lstrlenW(LPCWSTR);
WINBASEAPI int WINAPI lstrlenA(LPCSTR);
WINBASEAPI HANDLE WINAPI HeapCreate(DWORD, SIZE_T, SIZE_T);
WINBASEAPI BOOL WINAPI HeapDestroy(HANDLE);
WINBASEAPI LPVOID WINAPI GlobalAlloc(UINT, SIZE_T);
WINBASEAPI LPVOID WINAPI GlobalReAlloc(HGLOBAL, SIZE_T, UINT);
WINBASEAPI HGLOBAL WINAPI GlobalFree(HGLOBAL);
WINBASEAPI LPVOID WINAPI GlobalLock(HGLOBAL);
WINBASEAPI BOOL WINAPI GlobalUnlock(HGLOBAL);
WINBASEAPI SIZE_T WINAPI GlobalSize(HGLOBAL);
WINBASEAPI HGLOBAL WINAPI GlobalHandle(LPCVOID);
WINBASEAPI HANDLE WINAPI GetProcessHeap();
WINBASEAPI LPVOID WINAPI HeapAlloc(HANDLE, DWORD, SIZE_T);
WINBASEAPI LPVOID WINAPI HeapReAlloc(HANDLE, DWORD, LPVOID, SIZE_T);
WINBASEAPI BOOL WINAPI HeapFree(HANDLE, DWORD, LPVOID);
WINBASEAPI SIZE_T WINAPI HeapSize(HANDLE, DWORD, LPCVOID);
WINBASEAPI DWORD WINAPI GetTickCount();
WINBASEAPI ULONGLONG GetTickCount64();
}

// Helper types for above - all already forward-declared above the kernel32 block.
// (LPTHREAD_START_ROUTINE, LPOVERLAPPED_COMPLETION_ROUTINE, PCONTEXT,
//  PVECTORED_EXCEPTION_HANDLER, LPTOP_LEVEL_EXCEPTION_FILTER)

// user32 subset
extern "C" {
WINUSERAPI HWND WINAPI CreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
WINUSERAPI HWND WINAPI CreateWindowExA(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
WINUSERAPI BOOL WINAPI DestroyWindow(HWND);
WINUSERAPI BOOL WINAPI ShowWindow(HWND, int);
WINUSERAPI BOOL WINAPI UpdateWindow(HWND);
WINUSERAPI BOOL WINAPI SetWindowPos(HWND, HWND, int, int, int, int, UINT);
WINUSERAPI BOOL WINAPI GetClientRect(HWND, LPRECT);
WINUSERAPI BOOL WINAPI GetWindowRect(HWND, LPRECT);
WINUSERAPI BOOL WINAPI AdjustWindowRect(LPRECT, DWORD, BOOL);
WINUSERAPI BOOL WINAPI AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD);
WINUSERAPI HDC WINAPI GetDC(HWND);
WINUSERAPI int WINAPI ReleaseDC(HWND, HDC);
WINUSERAPI HDC WINAPI BeginPaint(HWND, LPPAINTSTRUCT);
WINUSERAPI BOOL WINAPI EndPaint(HWND, const PAINTSTRUCT*);
WINUSERAPI BOOL WINAPI InvalidateRect(HWND, const RECT*, BOOL);
WINUSERAPI BOOL WINAPI ValidateRect(HWND, const RECT*);
WINUSERAPI BOOL WINAPI ShowCursor(BOOL);
WINUSERAPI BOOL WINAPI SetCursor(HCURSOR);
WINUSERAPI HCURSOR WINAPI LoadCursorW(HINSTANCE, LPCWSTR);
WINUSERAPI HICON WINAPI LoadIconW(HINSTANCE, LPCWSTR);
WINUSERAPI BOOL WINAPI PostMessageW(HWND, UINT, WPARAM, LPARAM);
WINUSERAPI LRESULT WINAPI SendMessageW(HWND, UINT, WPARAM, LPARAM);
WINUSERAPI BOOL WINAPI PeekMessageW(LPMSG, HWND, UINT, UINT, UINT);
WINUSERAPI BOOL WINAPI GetMessageW(LPMSG, HWND, UINT, UINT);
WINUSERAPI BOOL WINAPI TranslateMessage(const MSG*);
WINUSERAPI LRESULT WINAPI DispatchMessageW(const MSG*);
WINUSERAPI LRESULT WINAPI DefWindowProcW(HWND, UINT, WPARAM, LPARAM);
WINUSERAPI ATOM WINAPI RegisterClassW(const WNDCLASSW*);
WINUSERAPI ATOM WINAPI RegisterClassExW(const WNDCLASSEXW*);
WINUSERAPI BOOL WINAPI UnregisterClassW(LPCWSTR, HINSTANCE);
WINUSERAPI HMENU WINAPI CreateMenu();
WINUSERAPI HMENU WINAPI CreatePopupMenu();
WINUSERAPI BOOL WINAPI DestroyMenu(HMENU);
WINUSERAPI BOOL WINAPI AppendMenuW(HMENU, UINT, UINT_PTR, LPCWSTR);
WINUSERAPI BOOL WINAPI InsertMenuW(HMENU, UINT, UINT, UINT_PTR, LPCWSTR);
WINUSERAPI BOOL WINAPI DeleteMenu(HMENU, UINT, UINT);
WINUSERAPI BOOL WINAPI TrackPopupMenuEx(HMENU, UINT, int, int, HWND, LPTPMPARAMS);
WINUSERAPI UINT_PTR WINAPI SetTimer(HWND, UINT_PTR, UINT, TIMERPROC);
WINUSERAPI BOOL WINAPI KillTimer(HWND, UINT_PTR);
WINUSERAPI int WINAPI MessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
WINUSERAPI int WINAPI MessageBoxA(HWND, LPCSTR, LPCSTR, UINT);
WINUSERAPI int WINAPI GetSystemMetrics(int);
WINUSERAPI DWORD WINAPI GetWindowThreadProcessId(HWND, LPDWORD);
WINUSERAPI HWND WINAPI GetForegroundWindow();
WINUSERAPI BOOL WINAPI SetForegroundWindow(HWND);
WINUSERAPI HWND WINAPI GetFocus();
WINUSERAPI BOOL WINAPI SetFocus(HWND);
WINUSERAPI BOOL WINAPI GetCursorPos(LPPOINT);
WINUSERAPI BOOL WINAPI SetCursorPos(int, int);
WINUSERAPI short WINAPI GetAsyncKeyState(int);
WINUSERAPI short WINAPI GetKeyState(int);
WINUSERAPI BOOL WINAPI GetKeyboardState(PBYTE);
WINUSERAPI BOOL WINAPI SetKeyboardState(const BYTE*);
WINUSERAPI int WINAPI ToUnicodeEx(UINT, UINT, const BYTE*, LPWSTR, int, UINT, HKL);
WINUSERAPI int WINAPI ToAsciiEx(UINT, UINT, const BYTE*, LPWORD, UINT, HKL);
WINUSERAPI int WINAPI MapVirtualKeyW(UINT, UINT);
WINUSERAPI int WINAPI MapVirtualKeyA(UINT, UINT);
WINUSERAPI BOOL WINAPI ScreenToClient(HWND, LPPOINT);
WINUSERAPI BOOL WINAPI ClientToScreen(HWND, LPPOINT);
WINUSERAPI BOOL WINAPI WaitMessage();
WINUSERAPI BOOL WINAPI TranslateAcceleratorW(HWND, HACCEL, LPMSG);
WINUSERAPI BOOL WINAPI GetMonitorInfoW(HMONITOR, LPMONITORINFO);
WINUSERAPI HMONITOR MonitorFromWindow(HWND, DWORD);
WINUSERAPI HMONITOR MonitorFromPoint(POINT, DWORD);
WINUSERAPI BOOL WINAPI EnumDisplayMonitors(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
WINUSERAPI int WINAPI GetDpiForWindow(HWND);
WINUSERAPI int WINAPI GetDpiForSystem();
WINUSERAPI BOOL WINAPI SetProcessDPIAware();
WINUSERAPI BOOL WINAPI SetProcessDpiAwareness(int);
WINUSERAPI HWND WINAPI FindWindowW(LPCWSTR, LPCWSTR);
WINUSERAPI HWND WINAPI FindWindowExW(HWND, HWND, LPCWSTR, LPCWSTR);
WINUSERAPI int WINAPI GetWindowTextW(HWND, LPWSTR, int);
WINUSERAPI int WINAPI GetWindowTextLengthW(HWND);
WINUSERAPI BOOL WINAPI SetWindowTextW(HWND, LPCWSTR);
WINUSERAPI LONG WINAPI GetWindowLongW(HWND, int);
WINUSERAPI LONG WINAPI SetWindowLongW(HWND, int, LONG);
WINUSERAPI LONG_PTR WINAPI GetWindowLongPtrW(HWND, int);
WINUSERAPI LONG_PTR WINAPI SetWindowLongPtrW(HWND, int, LONG_PTR);
WINUSERAPI BOOL WINAPI IsWindow(HWND);
WINUSERAPI BOOL WINAPI IsWindowVisible(HWND);
WINUSERAPI BOOL WINAPI IsWindowEnabled(HWND);
WINUSERAPI BOOL WINAPI IsChild(HWND, HWND);
WINUSERAPI HWND WINAPI GetParent(HWND);
WINUSERAPI HWND WINAPI SetParent(HWND, HWND);
WINUSERAPI BOOL WINAPI BringWindowToTop(HWND);
WINUSERAPI BOOL WINAPI RedrawWindow(HWND, const RECT*, HRGN, UINT);
WINUSERAPI HWND WINAPI GetDesktopWindow();
WINUSERAPI HWND WINAPI GetShellWindow();
WINUSERAPI BOOL WINAPI MoveWindow(HWND, int, int, int, int, BOOL);
WINUSERAPI int WINAPI GetKeyNameTextW(LONG, LPWSTR, int);
WINUSERAPI BOOL WINAPI ClipCursor(const RECT*);
WINUSERAPI BOOL WINAPI GetClipCursor(LPRECT);
WINUSERAPI BOOL WINAPI OpenClipboard(HWND);
WINUSERAPI BOOL WINAPI CloseClipboard();
WINUSERAPI BOOL WINAPI EmptyClipboard();
WINUSERAPI HANDLE WINAPI SetClipboardData(UINT, HANDLE);
WINUSERAPI HANDLE WINAPI GetClipboardData(UINT);
WINUSERAPI BOOL WINAPI IsClipboardFormatAvailable(UINT);
WINUSERAPI UINT WINAPI RegisterClipboardFormatW(LPCWSTR);
WINUSERAPI int WINAPI GetClipboardFormatNameW(UINT, LPWSTR, int);
WINUSERAPI int WINAPI CountClipboardFormats();
WINUSERAPI UINT WINAPI EnumClipboardFormats(UINT);
WINUSERAPI BOOL WINAPI MessageBeep(UINT);
WINUSERAPI BOOL WINAPI SystemParametersInfoW(UINT, UINT, PVOID, UINT);
WINUSERAPI void WINAPI PostQuitMessage(int);
WINUSERAPI ATOM WINAPI GlobalAddAtomW(LPCWSTR);
WINUSERAPI ATOM WINAPI GlobalFindAtomW(LPCWSTR);
WINUSERAPI ATOM WINAPI GlobalDeleteAtom(ATOM);
WINUSERAPI BOOL WINAPI GetInputState();
WINUSERAPI DWORD WINAPI GetQueueStatus(UINT);
WINUSERAPI HICON WINAPI LoadImageW(HINSTANCE, LPCWSTR, UINT, int, int, UINT);
WINUSERAPI BOOL WINAPI DrawIconW(HDC, int, int, HICON);
WINUSERAPI BOOL WINAPI DrawIconExW(HDC, int, int, HICON, int, int, UINT, HBRUSH, UINT);
WINUSERAPI BOOL WINAPI DestroyIcon(HICON);
WINUSERAPI BOOL WINAPI DestroyCursor(HCURSOR);
WINUSERAPI BOOL WINAPI SetWindowRgn(HWND, HRGN, BOOL);
WINUSERAPI int WINAPI GetUpdateRgn(HWND, HRGN, BOOL);
WINUSERAPI BOOL WINAPI GetUpdateRect(HWND, LPRECT, BOOL);
WINUSERAPI BOOL WINAPI ScrollWindowEx(HWND, int, int, const RECT*, const RECT*, HRGN, LPRECT, UINT);
WINUSERAPI BOOL WINAPI ShowOwnedPopups(HWND, BOOL);
WINUSERAPI BOOL WINAPI GetWindowPlacement(HWND, WINDOWPLACEMENT*);
WINUSERAPI BOOL WINAPI SetWindowPlacement(HWND, const WINDOWPLACEMENT*);
WINUSERAPI BOOL WINAPI GetClientRect(HWND, LPRECT);
WINUSERAPI int WINAPI FillRect(HDC, const RECT*, HBRUSH);
WINUSERAPI int WINAPI FrameRect(HDC, const RECT*, HBRUSH);
WINUSERAPI int WINAPI DrawTextW(HDC, LPCWSTR, int, LPRECT, UINT);
WINUSERAPI int WINAPI DrawTextExW(HDC, LPWSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);
WINUSERAPI BOOL WINAPI GrayStringW(HDC, HBRUSH, GRAYSTRINGPROC, LPARAM, int, int, int, int, int);
WINUSERAPI BOOL WINAPI TabbedTextOutW(HDC, int, int, LPCWSTR, int, int, const INT*, int);
WINUSERAPI BOOL WINAPI ExtTextOutW(HDC, int, int, UINT, const RECT*, LPCWSTR, UINT, const INT*);

// --- GAP-USER32 additions: 77 missing user32 entry points ---
WINUSERAPI HKL       WINAPI ActivateKeyboardLayout(HKL, UINT);
WINUSERAPI BOOL      WINAPI AddClipboardFormatListener(HWND);
WINUSERAPI BOOL      WINAPI AllowSetForegroundWindow(DWORD);
WINUSERAPI LRESULT   WINAPI CallNextHookEx(HHOOK, int, WPARAM, LPARAM);
WINUSERAPI LONG      WINAPI ChangeDisplaySettingsExW(LPCWSTR, LPDEVMODEW, HWND, DWORD, LPVOID);
WINUSERAPI LONG      WINAPI ChangeDisplaySettingsW(LPDEVMODEW, DWORD);
WINUSERAPI BOOL      WINAPI CloseTouchInputHandle(HTOUCHINPUT);
WINUSERAPI HWND      WINAPI CreateDialogParamW(HINSTANCE, LPCWSTR, HWND, DLGPROC, LPARAM);
WINUSERAPI HICON     WINAPI CreateIconIndirect(PICONINFO);
WINUSERAPI BOOL      WINAPI DestroyAcceleratorTable(HACCEL);
WINUSERAPI VOID      WINAPI DisableProcessWindowsGhosting();
WINUSERAPI LONG      WINAPI DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_HEADER*);
WINUSERAPI BOOL      WINAPI DrawIconEx(HDC, int, int, HICON, int, int, UINT, HBRUSH, UINT);
WINUSERAPI BOOL      WINAPI DrawStateW(HDC, HBRUSH, DRAWSTATEPROC, LPARAM, WPARAM, int, int, int, int, UINT);
WINUSERAPI BOOL      WINAPI EnableMenuItem(HMENU, UINT, UINT);
WINUSERAPI BOOL      WINAPI EnableWindow(HWND, BOOL);
WINUSERAPI BOOL      WINAPI EnumChildWindows(HWND, WNDENUMPROC, LPARAM);
WINUSERAPI BOOL      WINAPI EnumDisplayDevicesA(LPCSTR, DWORD, PDISPLAY_DEVICEA, DWORD);
WINUSERAPI BOOL      WINAPI EnumDisplayDevicesW(LPCWSTR, DWORD, PDISPLAY_DEVICEW, DWORD);
WINUSERAPI BOOL      WINAPI EnumDisplaySettingsW(LPCWSTR, DWORD, PDEVMODEW);
WINUSERAPI HWND      WINAPI FindWindowExA(HWND, HWND, LPCSTR, LPCSTR);
WINUSERAPI BOOL      WINAPI FlashWindowEx(PFLASHWINFO);
WINUSERAPI HWND      WINAPI GetActiveWindow();
WINUSERAPI HWND      WINAPI GetCapture();
WINUSERAPI ULONG_PTR WINAPI GetClassLongPtrW(HWND, int);
WINUSERAPI HDC       WINAPI GetDCEx(HWND, HRGN, DWORD);
WINUSERAPI LONG      WINAPI GetDisplayConfigBufferSizes(UINT32, UINT32*, UINT32*);
WINUSERAPI HWND      WINAPI GetDlgItem(HWND, int);
WINUSERAPI UINT      WINAPI GetDlgItemTextW(HWND, int, LPWSTR, int);
WINUSERAPI HKL       WINAPI GetKeyboardLayout(DWORD);
WINUSERAPI int       WINAPI GetKeyboardLayoutList(int, HKL*);
WINUSERAPI LPARAM    WINAPI GetMessageExtraInfo();
WINUSERAPI BOOL      WINAPI GetMonitorInfoA(HMONITOR, LPMONITORINFO);
WINUSERAPI HWINSTA   WINAPI GetProcessWindowStation();
WINUSERAPI UINT      WINAPI GetRawInputData(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
WINUSERAPI UINT      WINAPI GetRawInputDeviceInfoA(HANDLE, UINT, LPVOID, PINT);
WINUSERAPI UINT      WINAPI GetRawInputDeviceList(PRAWINPUTDEVICELIST, PUINT, UINT);
WINUSERAPI DWORD     WINAPI GetSysColor(int);
WINUSERAPI HBRUSH    WINAPI GetSysColorBrush(int);
WINUSERAPI HMENU     WINAPI GetSystemMenu(HWND, BOOL);
WINUSERAPI HWND      WINAPI GetTopWindow(HWND);
WINUSERAPI BOOL      WINAPI GetTouchInputInfo(HTOUCHINPUT, UINT, PTOUCHINPUT, int);
WINUSERAPI BOOL      WINAPI GetUserObjectInformationW(HWINSTA, int, LPVOID, DWORD, LPDWORD);
WINUSERAPI BOOL      WINAPI GetWindowDisplayAffinity(HWND, DWORD*);
WINUSERAPI BOOL      WINAPI GetWindowInfo(HWND, PWINDOWINFO);
WINUSERAPI BOOL      WINAPI InsertMenuItemW(HMENU, UINT, BOOL, LPCMENUITEMINFOW);
WINUSERAPI BOOL      WINAPI IsDialogMessageW(HWND, LPMSG);
WINUSERAPI BOOL      WINAPI IsIconic(HWND);
WINUSERAPI BOOL      WINAPI IsZoomed(HWND);
WINUSERAPI HACCEL    WINAPI LoadAcceleratorsW(HINSTANCE, LPCWSTR);
WINUSERAPI HBITMAP   WINAPI LoadBitmapW(HINSTANCE, LPCWSTR);
WINUSERAPI HCURSOR   WINAPI LoadCursorFromFileW(LPCWSTR);
WINUSERAPI int       WINAPI MapWindowPoints(HWND, HWND, LPPOINT, UINT);
WINUSERAPI HMONITOR  WINAPI MonitorFromRect(LPCRECT, DWORD);
WINUSERAPI DWORD     WINAPI MsgWaitForMultipleObjects(DWORD, const HANDLE*, BOOL, DWORD, DWORD);
WINUSERAPI DWORD     WINAPI MsgWaitForMultipleObjectsEx(DWORD, const HANDLE*, DWORD, DWORD, DWORD);
WINUSERAPI BOOL      WINAPI PostThreadMessageW(DWORD, UINT, WPARAM, LPARAM);
WINUSERAPI BOOL      WINAPI PtInRect(const RECT*, POINT);
WINUSERAPI LONG      WINAPI QueryDisplayConfig(UINT32, UINT32*, DISPLAYCONFIG_PATH_INFO*, UINT32*, DISPLAYCONFIG_MODE_INFO*, HWND*);
WINUSERAPI BOOL      WINAPI RegisterRawInputDevices(PCRAWINPUTDEVICE, UINT, UINT);
WINUSERAPI BOOL      WINAPI RegisterTouchWindow(HWND, ULONG);
WINUSERAPI UINT      WINAPI RegisterWindowMessageW(LPCWSTR);
WINUSERAPI BOOL      WINAPI ReleaseCapture();
WINUSERAPI UINT      WINAPI SendInput(UINT, LPINPUT, int);
WINUSERAPI HWND      WINAPI SetActiveWindow(HWND);
WINUSERAPI HWND      WINAPI SetCapture(HWND);
WINUSERAPI BOOL      WINAPI SetDlgItemTextW(HWND, int, LPCWSTR);
WINUSERAPI BOOL      WINAPI SetLayeredWindowAttributes(HWND, COLORREF, BYTE, DWORD);
WINUSERAPI BOOL      WINAPI SetWindowDisplayAffinity(HWND, DWORD);
WINUSERAPI HHOOK     WINAPI SetWindowsHookExW(int, HOOKPROC, HINSTANCE, DWORD);
WINUSERAPI BOOL      WINAPI TrackMouseEvent(LPTRACKMOUSEEVENT);
WINUSERAPI BOOL      WINAPI TrackPopupMenu(HMENU, UINT, int, int, int, HWND, const RECT*);
WINUSERAPI BOOL      WINAPI UnhookWindowsHookEx(HHOOK);
WINUSERAPI BOOL      WINAPI UnregisterClassA(LPCSTR, HINSTANCE);
WINUSERAPI DWORD     WINAPI WaitForInputIdle(HANDLE, DWORD);
WINUSERAPI HWND      WINAPI WindowFromPoint(POINT);
WINUSERAPI int       WINAPIV wsprintfW(LPWSTR, LPCWSTR, ...);
WINUSERAPI int       WINAPIV wvsprintfW(LPWSTR, LPCWSTR, va_list);
}

// user32 helper types - all already typedef'd above the user32 function block.

// gdi32 subset
extern "C" {
WINGDIAPI HDC WINAPI CreateCompatibleDC(HDC);
WINGDIAPI BOOL WINAPI DeleteDC(HDC);
WINGDIAPI HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
WINGDIAPI BOOL WINAPI DeleteObject(HGDIOBJ);
WINGDIAPI HGDIOBJ WINAPI GetStockObject(int);
WINGDIAPI HBITMAP WINAPI CreateCompatibleBitmap(HDC, int, int);
WINGDIAPI HBITMAP WINAPI CreateBitmap(int, int, UINT, UINT, const void*);
WINGDIAPI HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO*, UINT, void**, HANDLE, DWORD);
WINGDIAPI HBRUSH WINAPI CreateSolidBrush(COLORREF);
WINGDIAPI HBRUSH WINAPI CreateBrushIndirect(const LOGBRUSH*);
WINGDIAPI HBRUSH WINAPI CreateHatchBrush(int, COLORREF);
WINGDIAPI HBRUSH WINAPI CreatePatternBrush(HBITMAP);
WINGDIAPI HPEN WINAPI CreatePen(int, int, COLORREF);
WINGDIAPI HPEN WINAPI ExtCreatePen(DWORD, DWORD, const LOGBRUSH*, DWORD, const DWORD*);
WINGDIAPI HFONT WINAPI CreateFontIndirectW(const LOGFONTW*);
WINGDIAPI HFONT WINAPI CreateFontW(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR);
WINGDIAPI int WINAPI GetObjectW(HGDIOBJ, int, LPVOID);
WINGDIAPI UINT WINAPI GetDIBColorTable(HDC, UINT, UINT, RGBQUAD*);
WINGDIAPI UINT WINAPI SetDIBColorTable(HDC, UINT, UINT, const RGBQUAD*);
WINGDIAPI BOOL WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
WINGDIAPI BOOL WINAPI StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
WINGDIAPI BOOL WINAPI AlphaBlend(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
WINGDIAPI BOOL WINAPI TransparentBlt(HDC, int, int, int, int, HDC, int, int, int, int, UINT);
WINGDIAPI BOOL WINAPI MaskBlt(HDC, int, int, int, int, HDC, int, int, HBITMAP, int, int, DWORD);
WINGDIAPI BOOL WINAPI PlgBlt(HDC, const POINT*, HDC, int, int, int, int, HBITMAP, int, int);
WINGDIAPI COLORREF WINAPI SetPixel(HDC, int, int, COLORREF);
WINGDIAPI COLORREF WINAPI GetPixel(HDC, int, int);
WINGDIAPI BOOL WINAPI MoveToEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL WINAPI LineTo(HDC, int, int);
WINGDIAPI BOOL WINAPI Rectangle(HDC, int, int, int, int);
WINGDIAPI BOOL WINAPI RoundRect(HDC, int, int, int, int, int, int);
WINGDIAPI BOOL WINAPI Ellipse(HDC, int, int, int, int);
WINGDIAPI BOOL WINAPI Arc(HDC, int, int, int, int, int, int, int, int);
WINGDIAPI BOOL WINAPI Pie(HDC, int, int, int, int, int, int, int, int);
WINGDIAPI BOOL WINAPI Chord(HDC, int, int, int, int, int, int, int, int);
WINGDIAPI BOOL WINAPI Polyline(HDC, const POINT*, int);
WINGDIAPI BOOL WINAPI PolylineTo(HDC, const POINT*, int);
WINGDIAPI BOOL WINAPI Polygon(HDC, const POINT*, int);
WINGDIAPI BOOL WINAPI PolyBezier(HDC, const POINT*, int);
WINGDIAPI BOOL WINAPI PolyBezierTo(HDC, const POINT*, int);
WINGDIAPI BOOL WINAPI InvertRect(HDC, const RECT*);
WINGDIAPI BOOL WINAPI DrawFocusRect(HDC, const RECT*);
WINGDIAPI int WINAPI FillRgn(HDC, HRGN, HBRUSH);
WINGDIAPI int WINAPI FrameRgn(HDC, HRGN, HBRUSH, int, int);
WINGDIAPI int WINAPI PaintRgn(HDC, HRGN);
WINGDIAPI HRGN WINAPI CreateRectRgn(int, int, int, int);
WINGDIAPI HRGN WINAPI CreateRectRgnIndirect(const RECT*);
WINGDIAPI HRGN WINAPI CreateEllipticRgn(int, int, int, int);
WINGDIAPI HRGN WINAPI CreateEllipticRgnIndirect(const RECT*);
WINGDIAPI HRGN WINAPI CreatePolygonRgn(const POINT*, int, int);
WINGDIAPI HRGN WINAPI CreatePolyPolygonRgn(const POINT*, const INT*, int, int);
WINGDIAPI HRGN WINAPI CreateRoundRectRgn(int, int, int, int, int, int);
WINGDIAPI HRGN WINAPI ExtCreateRegion(const XFORM*, DWORD, const RGNDATA*);
WINGDIAPI int WINAPI CombineRgn(HRGN, HRGN, HRGN, int);
WINGDIAPI BOOL WINAPI EqualRgn(HRGN, HRGN);
WINGDIAPI BOOL WINAPI OffsetRgn(HRGN, int, int);
WINGDIAPI BOOL WINAPI PtInRegion(HRGN, int, int);
WINGDIAPI BOOL WINAPI RectInRegion(HRGN, const RECT*);
WINGDIAPI BOOL WINAPI GetRgnBox(HRGN, LPRECT);
WINGDIAPI int WINAPI SelectClipRgn(HDC, HRGN);
WINGDIAPI int WINAPI ExtSelectClipRgn(HDC, HRGN, int);
WINGDIAPI int WINAPI ExcludeClipRect(HDC, int, int, int, int);
WINGDIAPI int WINAPI IntersectClipRect(HDC, int, int, int, int);
WINGDIAPI int WINAPI OffsetClipRgn(HDC, int, int);
WINGDIAPI BOOL WINAPI GetClipBox(HDC, LPRECT);
WINGDIAPI BOOL WINAPI PtVisible(HDC, int, int);
WINGDIAPI BOOL WINAPI RectVisible(HDC, const RECT*);
WINGDIAPI int WINAPI SaveDC(HDC);
WINGDIAPI BOOL WINAPI RestoreDC(HDC, int);
WINGDIAPI int WINAPI GetDeviceCaps(HDC, int);
WINGDIAPI COLORREF WINAPI SetBkColor(HDC, COLORREF);
WINGDIAPI COLORREF WINAPI SetTextColor(HDC, COLORREF);
WINGDIAPI int WINAPI SetBkMode(HDC, int);
WINGDIAPI int WINAPI SetROP2(HDC, int);
WINGDIAPI int WINAPI SetPolyFillMode(HDC, int);
WINGDIAPI int WINAPI SetStretchBltMode(HDC, int);
WINGDIAPI int WINAPI SetMapMode(HDC, int);
WINGDIAPI BOOL WINAPI SetWindowExtEx(HDC, int, int, LPSIZE);
WINGDIAPI BOOL WINAPI SetViewportExtEx(HDC, int, int, LPSIZE);
WINGDIAPI BOOL WINAPI SetWindowOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL WINAPI SetViewportOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL WINAPI OffsetWindowOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL WINAPI OffsetViewportOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL WINAPI ScaleWindowExtEx(HDC, int, int, int, int, LPSIZE);
WINGDIAPI BOOL WINAPI ScaleViewportExtEx(HDC, int, int, int, int, LPSIZE);
WINGDIAPI BOOL WINAPI DPtoLP(HDC, LPPOINT, int);
WINGDIAPI BOOL WINAPI LPtoDP(HDC, LPPOINT, int);
WINGDIAPI HGDIOBJ WINAPI GetCurrentObject(HDC, UINT);
WINGDIAPI COLORREF WINAPI GetBkColor(HDC);
WINGDIAPI COLORREF WINAPI GetTextColor(HDC);
WINGDIAPI int WINAPI GetBkMode(HDC);
WINGDIAPI int WINAPI GetROP2(HDC);
WINGDIAPI BOOL WINAPI TextOutW(HDC, int, int, LPCWSTR, int);
WINGDIAPI BOOL WINAPI GetTextExtentPoint32W(HDC, LPCWSTR, int, LPSIZE);
WINGDIAPI BOOL WINAPI GetTextMetricsW(HDC, LPTEXTMETRICW);
WINGDIAPI BOOL WINAPI GetTextExtentExPointW(HDC, LPCWSTR, int, int, LPINT, LPINT, LPSIZE);
WINGDIAPI DWORD WINAPI GetGlyphOutlineW(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, const MAT2*);
WINGDIAPI UINT WINAPI GetTextAlign(HDC);
WINGDIAPI UINT WINAPI SetTextAlign(HDC, UINT);
WINGDIAPI int WINAPI SetTextCharacterExtra(HDC, int);
WINGDIAPI int WINAPI GetTextCharacterExtra(HDC);
WINGDIAPI BOOL WINAPI GetCharWidth32W(HDC, UINT, UINT, LPINT);
WINGDIAPI BOOL WINAPI GetCharWidthW(HDC, UINT, UINT, LPINT);
WINGDIAPI BOOL WINAPI GetCharABCWidthsW(HDC, UINT, UINT, LPABC);
WINGDIAPI DWORD WINAPI GetFontData(HDC, DWORD, DWORD, LPVOID, DWORD);
WINGDIAPI int WINAPI EnumFontFamiliesExW(HDC, LPLOGFONTW, FONTENUMPROCW, LPARAM, DWORD);
WINGDIAPI HPALETTE WINAPI CreatePalette(const LOGPALETTE*);
WINGDIAPI HPALETTE WINAPI SelectPalette(HDC, HPALETTE, BOOL);
WINGDIAPI UINT WINAPI RealizePalette(HDC);
WINGDIAPI UINT WINAPI SetPaletteEntries(HPALETTE, UINT, UINT, const PALETTEENTRY*);
WINGDIAPI UINT WINAPI GetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY);
WINGDIAPI BOOL WINAPI ResizePalette(HPALETTE, UINT);
WINGDIAPI BOOL WINAPI AnimatePalette(HPALETTE, UINT, UINT, const PALETTEENTRY*);
WINGDIAPI BOOL WINAPI GdiFlush();
WINGDIAPI DWORD WINAPI GdiGetBatchLimit();
WINGDIAPI DWORD WINAPI GdiSetBatchLimit(DWORD);
WINGDIAPI int WINAPI SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT, const void*, const BITMAPINFO*, UINT);
WINGDIAPI int WINAPI StretchDIBits(HDC, int, int, int, int, int, int, int, int, const void*, const BITMAPINFO*, UINT, DWORD);
WINGDIAPI BOOL WINAPI GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT);
}

// advapi32 subset
extern "C" {
WINADVAPI LONG WINAPI RegOpenKeyExW(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
WINADVAPI LONG WINAPI RegOpenKeyW(HKEY, LPCWSTR, PHKEY);
WINADVAPI LONG WINAPI RegCreateKeyExW(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
WINADVAPI LONG WINAPI RegCreateKeyW(HKEY, LPCWSTR, PHKEY);
WINADVAPI LONG WINAPI RegCloseKey(HKEY);
WINADVAPI LONG WINAPI RegSetValueExW(HKEY, LPCWSTR, DWORD, DWORD, const BYTE*, DWORD);
WINADVAPI LONG WINAPI RegQueryValueExW(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
WINADVAPI LONG WINAPI RegDeleteKeyW(HKEY, LPCWSTR);
WINADVAPI LONG WINAPI RegDeleteValueW(HKEY, LPCWSTR);
WINADVAPI LONG WINAPI RegEnumKeyExW(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPWSTR, LPDWORD, PFILETIME);
WINADVAPI LONG WINAPI RegEnumValueW(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
WINADVAPI LONG WINAPI RegFlushKey(HKEY);
WINADVAPI SC_HANDLE WINAPI OpenSCManagerW(LPCWSTR, LPCWSTR, DWORD);
WINADVAPI BOOL WINAPI CloseServiceHandle(SC_HANDLE);
WINADVAPI SC_HANDLE WINAPI CreateServiceW(SC_HANDLE, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD, DWORD, LPCWSTR, LPWSTR, LPDWORD, LPCWSTR, LPCWSTR, LPCWSTR);
WINADVAPI SC_HANDLE WINAPI OpenServiceW(SC_HANDLE, LPCWSTR, DWORD);
WINADVAPI BOOL WINAPI DeleteService(SC_HANDLE);
WINADVAPI BOOL WINAPI StartServiceW(SC_HANDLE, DWORD, LPCWSTR*);
WINADVAPI BOOL WINAPI ControlService(SC_HANDLE, DWORD, LPSERVICE_STATUS);
WINADVAPI BOOL WINAPI QueryServiceStatus(SC_HANDLE, LPSERVICE_STATUS);
WINADVAPI BOOL WINAPI AllocateAndInitializeSid(const void*, BYTE, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, void**);
WINADVAPI void* WINAPI InitializeSecurityDescriptor(void*, DWORD);
WINADVAPI BOOL WINAPI SetSecurityDescriptorDacl(void*, BOOL, void*, BOOL);
WINADVAPI BOOL WINAPI InitializeAcl(void*, DWORD, DWORD);
WINADVAPI BOOL WINAPI AddAccessAllowedAce(void*, DWORD, DWORD, void*);
WINADVAPI BOOL WINAPI GetTokenInformation(HANDLE, int, LPVOID, DWORD, LPDWORD);
WINADVAPI BOOL WINAPI OpenProcessToken(HANDLE, DWORD, PHKEY);
WINADVAPI BOOL WINAPI OpenThreadToken(HANDLE, DWORD, BOOL, PHKEY);
WINADVAPI BOOL WINAPI GetUserNameW(LPWSTR, LPDWORD);
WINADVAPI BOOL WINAPI LookupAccountNameW(LPCWSTR, LPCWSTR, void*, LPDWORD, LPWSTR, LPDWORD, void*);
WINADVAPI BOOL WINAPI AdjustTokenPrivileges(HANDLE, BOOL, void*, DWORD, void*, PDWORD);
WINADVAPI BOOL WINAPI CryptAcquireContextW(HCRYPTPROV*, LPCWSTR, LPCWSTR, DWORD, DWORD);
WINADVAPI BOOL WINAPI CryptReleaseContext(HCRYPTPROV, DWORD);
WINADVAPI BOOL WINAPI CryptGenRandom(HCRYPTPROV, DWORD, BYTE*);
WINADVAPI BOOL WINAPI BCryptOpenAlgorithmProvider(void**, LPCWSTR, LPCWSTR, ULONG);
WINADVAPI BOOL WINAPI BCryptGenRandom(void*, PUCHAR, ULONG, ULONG);
WINADVAPI BOOL WINAPI BCryptCloseAlgorithmProvider(void*, ULONG);
}

// shell32 subset
extern "C" {
WINSHELLAPI HRESULT WINAPI SHGetFolderPathW(HWND, int, HANDLE, DWORD, LPWSTR);
WINSHELLAPI HRESULT WINAPI SHGetKnownFolderPath(const GUID*, DWORD, HANDLE, PWSTR*);
WINSHELLAPI BOOL WINAPI ShellExecuteExW(SHELLEXECUTEINFOW*);
WINSHELLAPI HINSTANCE WINAPI ShellExecuteW(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
WINSHELLAPI int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW);
WINSHELLAPI HRESULT WINAPI SHCreateDirectoryExW(LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES);
WINSHELLAPI DWORD WINAPI SHAutoComplete(HWND, DWORD);
WINSHELLAPI BOOL WINAPI IsUserAnAdmin();
WINSHELLAPI UINT WINAPI DragQueryFileW(HDROP, UINT, LPWSTR, UINT);
WINSHELLAPI BOOL WINAPI DragFinish(HDROP);
WINSHELLAPI void WINAPI SHAddToRecentDocs(UINT, LPCVOID);
WINSHELLAPI BOOL WINAPI SHGetPathFromIDListW(const void*, LPWSTR);
WINSHELLAPI HRESULT WINAPI ILCreateFromPathW(LPCWSTR);
WINSHELLAPI void ILFree(void*);
}

// shlwapi subset
extern "C" {
WINSHELLAPI LPWSTR WINAPI PathAddBackslashW(LPWSTR);
WINSHELLAPI LPWSTR WINAPI PathRemoveBackslashW(LPWSTR);
WINSHELLAPI void WINAPI PathRemoveExtensionW(LPWSTR);
WINSHELLAPI BOOL WINAPI PathRenameExtensionW(LPWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI PathCombineW(LPWSTR, LPCWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI PathAppendW(LPWSTR, LPCWSTR);
WINSHELLAPI BOOL WINAPI PathCanonicalizeW(LPWSTR, LPCWSTR);
WINSHELLAPI BOOL WINAPI PathRelativePathToW(LPWSTR, LPCWSTR, DWORD, LPCWSTR, DWORD);
WINSHELLAPI int WINAPI PathCommonPrefixW(LPCWSTR, LPCWSTR, LPWSTR);
WINSHELLAPI BOOL WINAPI PathIsUNCW(LPCWSTR);
WINSHELLAPI BOOL WINAPI PathIsRelativeW(LPCWSTR);
WINSHELLAPI BOOL WINAPI PathIsRootW(LPCWSTR);
WINSHELLAPI BOOL WINAPI PathIsDirectoryW(LPCWSTR);
WINSHELLAPI BOOL WINAPI PathIsDirectoryEmptyW(LPCWSTR);
WINSHELLAPI BOOL WINAPI PathFileExistsW(LPCWSTR);
WINSHELLAPI BOOL WINAPI PathMatchSpecW(LPCWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI PathGetArgsW(LPCWSTR);
WINSHELLAPI void WINAPI PathStripPathW(LPWSTR);
WINSHELLAPI void WINAPI PathUnquoteSpacesW(LPWSTR);
WINSHELLAPI void WINAPI PathQuoteSpacesW(LPWSTR);
WINSHELLAPI void WINAPI PathRemoveArgsW(LPWSTR);
WINSHELLAPI void WINAPI PathRemoveBlanksW(LPWSTR);
WINSHELLAPI void WINAPI PathRemoveFileSpecW(LPWSTR);
WINSHELLAPI LPCWSTR WINAPI PathFindExtensionW(LPCWSTR);
WINSHELLAPI LPCWSTR WINAPI PathFindFileNameW(LPCWSTR);
WINSHELLAPI int WINAPI PathGetDriveNumberW(LPCWSTR);
WINSHELLAPI int WINAPI StrCmpW(LPCWSTR, LPCWSTR);
WINSHELLAPI int WINAPI StrCmpIW(LPCWSTR, LPCWSTR);
WINSHELLAPI int WINAPI StrCmpNW(LPCWSTR, LPCWSTR, int);
WINSHELLAPI int WINAPI StrCmpNIW(LPCWSTR, LPCWSTR, int);
WINSHELLAPI LPWSTR WINAPI StrCpyW(LPWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrCpyNW(LPWSTR, LPCWSTR, int);
WINSHELLAPI LPWSTR WINAPI StrCatW(LPWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrCatBuffW(LPWSTR, LPCWSTR, int);
WINSHELLAPI int WINAPI StrLenW(LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrStrW(LPWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrStrIW(LPWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrRStrW(LPWSTR, LPCWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrChrW(LPWSTR, WCHAR);
WINSHELLAPI LPWSTR WINAPI StrChrIW(LPWSTR, WCHAR);
WINSHELLAPI LPWSTR WINAPI StrRChrW(LPWSTR, LPCWSTR, WCHAR);
WINSHELLAPI LPWSTR WINAPI StrRChrIW(LPWSTR, LPCWSTR, WCHAR);
WINSHELLAPI int WINAPI StrSpnW(LPCWSTR, LPCWSTR);
WINSHELLAPI int WINAPI StrCSpnW(LPCWSTR, LPCWSTR);
WINSHELLAPI int WINAPI StrCSpnIW(LPCWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrPBrkW(LPCWSTR, LPCWSTR);
WINSHELLAPI LPWSTR WINAPI StrReverseW(LPWSTR);
WINSHELLAPI BOOL WINAPI ChrCmpIW(WCHAR, WCHAR);
WINSHELLAPI COLORREF WINAPI ColorRGBToHLS(COLORREF, LPWORD, LPWORD, LPWORD);
WINSHELLAPI COLORREF WINAPI ColorHLSToRGB(WORD, WORD, WORD);
WINSHELLAPI BOOL WINAPI IsOS(DWORD);
}

// ole32 subset
extern "C" {
WINOLEAPI HRESULT WINAPI CoInitialize(LPVOID);
WINOLEAPI HRESULT WINAPI CoInitializeEx(LPVOID, DWORD);
WINOLEAPI void WINAPI CoUninitialize();
WINOLEAPI HRESULT WINAPI CoCreateInstance(const CLSID&, IUnknown*, DWORD, const IID&, void**);
WINOLEAPI HRESULT WINAPI CoGetClassObject(const CLSID&, DWORD, void*, const IID&, void**);
WINOLEAPI HRESULT WINAPI CoRegisterClassObject(DWORD, IUnknown*, DWORD, DWORD, LPDWORD);
WINOLEAPI HRESULT WINAPI CoRevokeClassObject(DWORD);
WINOLEAPI HRESULT WINAPI CoCreateGuid(GUID*);
WINOLEAPI LPVOID WINAPI CoTaskMemAlloc(SIZE_T);
WINOLEAPI LPVOID WINAPI CoTaskMemRealloc(LPVOID, SIZE_T);
WINOLEAPI void WINAPI CoTaskMemFree(LPVOID);
WINOLEAPI BSTR SysAllocString(const OLECHAR*);
WINOLEAPI BSTR SysAllocStringLen(const OLECHAR*, UINT);
WINOLEAPI BSTR SysAllocStringByteLen(LPCSTR, UINT);
WINOLEAPI void WINAPI SysFreeString(BSTR);
WINOLEAPI UINT WINAPI SysStringLen(BSTR);
WINOLEAPI UINT WINAPI SysStringByteLen(BSTR);
WINOLEAPI HRESULT WINAPI VariantClear(VARIANTARG*);
WINOLEAPI HRESULT WINAPI VariantInit(VARIANTARG*);
WINOLEAPI HRESULT WINAPI VariantCopy(VARIANTARG*, const VARIANTARG*);
WINOLEAPI HRESULT WINAPI VariantChangeType(VARIANTARG*, const VARIANTARG*, USHORT, VARTYPE);
WINOLEAPI HRESULT WINAPI CreateStreamOnHGlobal(HGLOBAL, BOOL, IStream**);
WINOLEAPI HRESULT WINAPI GetHGlobalFromStream(IStream*, HGLOBAL*);
WINOLEAPI HRESULT WINAPI CreateBindCtx(DWORD, IBindCtx**);
WINOLEAPI HRESULT WINAPI MkParseDisplayName(IBindCtx*, LPCWSTR, ULONG*, IMoniker**);
WINOLEAPI HRESULT WINAPI CreateFileMoniker(LPCWSTR, IMoniker**);
WINOLEAPI HRESULT WINAPI BindMoniker(IMoniker*, DWORD, const IID&, void**);
WINOLEAPI HRESULT WINAPI CreateGenericComposite(IMoniker*, IMoniker*, IMoniker**);
WINOLEAPI HRESULT WINAPI OleInitialize(LPVOID);
WINOLEAPI void WINAPI OleUninitialize();
WINOLEAPI HRESULT WINAPI RegisterDragDrop(HWND, IUnknown*);
WINOLEAPI HRESULT WINAPI RevokeDragDrop(HWND);
WINOLEAPI HRESULT WINAPI DoDragDrop(IDataObject*, IDropSource*, DWORD, LPDWORD);
WINOLEAPI HRESULT WINAPI StgCreateDocfile(LPCWSTR, DWORD, DWORD, IStorage**);
WINOLEAPI HRESULT WINAPI StgOpenStorage(LPCWSTR, IStorage*, DWORD, SNB, DWORD, IStorage**);
WINOLEAPI HRESULT WINAPI StgIsStorageFile(LPCWSTR);
}

// winmm subset
extern "C" {
WINMMAPI UINT WINAPI waveOutOpen(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
WINMMAPI UINT WINAPI waveOutClose(HWAVEOUT);
WINMMAPI UINT WINAPI waveOutPrepareHeader(HWAVEOUT, LPWAVEHDR, UINT);
WINMMAPI UINT WINAPI waveOutUnprepareHeader(HWAVEOUT, LPWAVEHDR, UINT);
WINMMAPI UINT WINAPI waveOutWrite(HWAVEOUT, LPWAVEHDR, UINT);
WINMMAPI UINT WINAPI waveOutReset(HWAVEOUT);
WINMMAPI UINT WINAPI waveOutPause(HWAVEOUT);
WINMMAPI UINT WINAPI waveOutRestart(HWAVEOUT);
WINMMAPI UINT WINAPI waveOutGetVolume(HWAVEOUT, LPDWORD);
WINMMAPI UINT WINAPI waveOutSetVolume(HWAVEOUT, DWORD);
WINMMAPI UINT WINAPI waveOutGetDevCapsW(UINT, LPWAVEOUTCAPSW, UINT);
WINMMAPI UINT WINAPI waveOutGetNumDevs();
WINMMAPI UINT WINAPI midiOutOpen(LPHMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD);
WINMMAPI UINT WINAPI midiOutClose(HMIDIOUT);
WINMMAPI UINT WINAPI midiOutShortMsg(HMIDIOUT, DWORD);
WINMMAPI UINT WINAPI midiOutLongMsg(HMIDIOUT, LPMIDIHDR, UINT);
WINMMAPI UINT WINAPI midiOutReset(HMIDIOUT);
WINMMAPI MMRESULT WINAPI timeBeginPeriod(UINT);
WINMMAPI MMRESULT WINAPI timeEndPeriod(UINT);
WINMMAPI MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS, UINT);
WINMMAPI MMRESULT WINAPI timeSetEvent(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT);
WINMMAPI MMRESULT WINAPI timeKillEvent(UINT);
WINMMAPI DWORD WINAPI timeGetTime();
WINMMAPI FOURCC mmioStringToFOURCCW(LPCWSTR, UINT);
WINMMAPI HMMIO mmioOpenW(LPWSTR, LPMMIOINFO, DWORD);
WINMMAPI MMRESULT mmioClose(HMMIO, UINT);
WINMMAPI LONG mmioRead(HMMIO, HPSTR, LONG);
WINMMAPI LONG mmioWrite(HMMIO, const char*, LONG);
WINMMAPI LONG mmioSeek(HMMIO, LONG, int);
WINMMAPI MMRESULT mmioGetInfo(HMMIO, LPMMIOINFO, UINT);
WINMMAPI MMRESULT mmioSetInfo(HMMIO, LPMMIOINFO, UINT);
WINMMAPI MMRESULT mmioCreateChunk(HMMIO, LPMMCKINFO, UINT);
WINMMAPI MMRESULT mmioAscend(HMMIO, LPMMCKINFO, UINT);
WINMMAPI MMRESULT mmioDescend(HMMIO, LPMMCKINFO, const MMCKINFO*, UINT);
WINMMAPI BOOL WINAPI PlaySoundW(LPCWSTR, HMODULE, DWORD);
WINMMAPI UINT WINAPI mciSendStringW(LPCWSTR, LPWSTR, UINT, HWND);
WINMMAPI BOOL WINAPI joyGetPosEx(UINT, LPJOYINFOEX);
WINMMAPI UINT WINAPI joyGetDevCapsW(UINT, LPJOYCAPSW, UINT);
WINMMAPI UINT WINAPI joyGetNumDevs();
}

// version subset
extern "C" {
WINBASEAPI BOOL WINAPI GetFileVersionInfoW(LPCWSTR, DWORD, DWORD, LPVOID);
WINBASEAPI BOOL WINAPI GetFileVersionInfoSizeW(LPCWSTR, LPDWORD);
WINBASEAPI BOOL WINAPI GetFileVersionInfoSizeExW(DWORD, LPCWSTR, LPDWORD);
WINBASEAPI BOOL WINAPI VerQueryValueW(LPCVOID, LPCWSTR, LPVOID*, PUINT);
WINBASEAPI BOOL WINAPI VerInstallFileW(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT);
WINBASEAPI DWORD WINAPI VerLanguageNameW(DWORD, LPWSTR, DWORD);
WINBASEAPI BOOL WINAPI VerFindFileW(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
WINBASEAPI HRESULT WINAPI WinVerifyTrust(HWND, const GUID*, LPVOID);
}

// XInput
extern "C" {
DWORD WINAPI XInputGetState(DWORD, XINPUT_STATE*);
DWORD WINAPI XInputSetState(DWORD, XINPUT_VIBRATION*);
DWORD WINAPI XInputGetCapabilities(DWORD, DWORD, XINPUT_CAPABILITIES*);
DWORD WINAPI XInputEnable(BOOL);
DWORD WINAPI XInputGetBatteryInformation(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);
DWORD WINAPI XInputGetKeystroke(DWORD, DWORD, PXINPUT_KEYSTROKE);
}

// D3D11 (subset)
extern "C" {
HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
                                  const D3D_FEATURE_LEVEL*, UINT, UINT,
                                  ID3D11Device**, D3D_FEATURE_LEVEL*,
                                  ID3D11DeviceContext**);
HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
                                              const D3D_FEATURE_LEVEL*, UINT, UINT,
                                              const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**,
                                              ID3D11Device**, D3D_FEATURE_LEVEL*,
                                              ID3D11DeviceContext**);
}

// D2D / DWrite / WIC
extern "C" {
HRESULT WINAPI D2D1CreateFactory(int, const IID&, const void*, void**);
HRESULT WINAPI DWriteCreateFactory(int, const IID&, IUnknown**);
HRESULT WINAPI WICCreateImagingFactory(UINT, IWICImagingFactory**);
HRESULT WINAPI WICConvertBitmapSource(const GUID&, IWICBitmapSource*, IWICBitmapSource**);
}

// XAudio2
extern "C" {
HRESULT WINAPI XAudio2Create(IXAudio2**, UINT, XAUDIO2_PROCESSOR);
}

// Winsock
extern "C" {
int WINAPI WSAStartup(WORD, void*);
int WINAPI WSACleanup();
SOCKET WINAPI socket(int, int, int);
int WINAPI closesocket(SOCKET);
int WINAPI bind(SOCKET, const void*, int);
int WINAPI listen(SOCKET, int);
SOCKET WINAPI accept(SOCKET, void*, int*);
int WINAPI connect(SOCKET, const void*, int);
int WINAPI send(SOCKET, const char*, int, int);
int WINAPI recv(SOCKET, char*, int, int);
int WINAPI setsockopt(SOCKET, int, int, const char*, int);
int WINAPI getsockopt(SOCKET, int, int, char*, int*);
int WINAPI ioctlsocket(SOCKET, long, u_long*);
int WINAPI gethostname(char*, int);
struct hostent* WINAPI gethostbyname(const char*);
unsigned long WINAPI inet_addr(const char*);
int WINAPI WSAGetLastError();
}

// ===========================================================================
// GAP-WS2_32 additions: Winsock types and functions missing from the
// original Winsock subset above. Added by Task ID GAP-WS2_32.
//
// These mirror the real winsock2.h / ws2tcpip.h declarations so the same
// shim .cpp compiles unchanged against the UWP SDK on Windows. The stub is
// only included on Linux syntax-check builds (guarded by STUB_WINDOWS_H),
// so there is no risk of conflicting with the real SDK on Windows builds.
// ===========================================================================
#ifndef _XWR_WS2_32_GAP_TYPES
#define _XWR_WS2_32_GAP_TYPES

// POSIX-ish length type used by getnameinfo / WSAStringToAddressW / etc.
typedef int socklen_t;

// WSASocketW's GROUP parameter.
typedef unsigned int GROUP;

// Winsock scalar aliases.
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;

// Full definition of struct sockaddr (the stub only forward-declared it).
struct sockaddr {
    short sa_family;
    char  sa_data[14];
};
typedef struct sockaddr SOCKADDR, *PSOCKADDR, *LPSOCKADDR;

struct in_addr {
    unsigned long s_addr;
};
struct sockaddr_in {
    short          sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

// fd_set and struct timeval are intentionally NOT redefined here: on Linux
// syntax-check builds the glibc headers (pulled in transitively by <cstddef>
// / <cwchar>) already define them, and on Windows real builds the UWP SDK's
// winsock2.h provides them. The same applies to the select() function below.

// addrinfo / ADDRINFOW — for getaddrinfo / GetAddrInfoW / freeaddrinfo.
struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    size_t           ai_addrlen;
    char            *ai_canonname;
    struct sockaddr *ai_addr;
    struct addrinfo *ai_next;
};
typedef struct addrinfo ADDRINFOA, *PADDRINFOA;

struct addrinfoW {
    int               ai_flags;
    int               ai_family;
    int               ai_socktype;
    int               ai_protocol;
    size_t            ai_addrlen;
    wchar_t          *ai_canonname;
    struct sockaddr  *ai_addr;
    struct addrinfoW *ai_next;
};
typedef struct addrinfoW ADDRINFOW, *PADDRINFOW;

// WSABUF / WSAOVERLAPPED — for WSARecv / WSASend / WSAIoctl / etc.
typedef struct _WSABUF {
    unsigned long len;
    char         *buf;
} WSABUF, *LPWSABUF;

typedef struct _WSAOVERLAPPED {
    unsigned long Internal;
    unsigned long InternalHigh;
    union {
        struct { unsigned long Offset; unsigned long OffsetHigh; } DUMMYSTRUCTNAME;
        void *Pointer;
    } DUMMYUNIONNAME;
    void *hEvent;
} WSAOVERLAPPED, *LPWSAOVERLAPPED;

typedef void (WINAPI *LPWSAOVERLAPPED_COMPLETION_ROUTINE)(
    DWORD dwError, DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

// WSAEVENT is an opaque HANDLE to a manually-reset event.
typedef void *WSAEVENT;

// WSANETWORKEVENTS — for WSAEnumNetworkEvents.
typedef struct _WSANETWORKEVENTS {
    long lNetworkEvents;
    int  iErrorCode[10];
} WSANETWORKEVENTS, *LPWSANETWORKEVENTS;

// WSAPOLLFD — for WSAPoll.
typedef struct pollfd {
    SOCKET fd;
    short  events;
    short  revents;
} WSAPOLLFD, *PWSAPOLLFD, *LPWSAPOLLFD;

// WSAPROTOCOL_INFOW — opaque to the shims (we never dereference it).
struct _WSAPROTOCOL_INFOW;
typedef struct _WSAPROTOCOL_INFOW WSAPROTOCOL_INFOW, *LPWSAPROTOCOL_INFOW;

#endif  // _XWR_WS2_32_GAP_TYPES

// GAP-WS2_32 function declarations (signatures match the real winsock2.h).
extern "C" {
void           WINAPI WSASetLastError(int);
void           WINAPI freeaddrinfo(struct addrinfo*);
void           WINAPI FreeAddrInfoW(struct addrinfoW*);
int            WINAPI getaddrinfo(const char*, const char*, const struct addrinfo*, struct addrinfo**);
int            WINAPI GetAddrInfoW(const wchar_t*, const wchar_t*, const struct addrinfoW*, struct addrinfoW**);
int            WINAPI getnameinfo(const struct sockaddr*, socklen_t, char*, size_t, char*, size_t, int);
PCSTR          WINAPI inet_ntop(int, PVOID, PSTR, size_t);
PCWSTR         WINAPI InetNtopW(int, PVOID, PWSTR, size_t);
int            WINAPI inet_pton(int, PCSTR, PVOID);

int            WINAPI getpeername(SOCKET, struct sockaddr*, int*);
int            WINAPI getsockname(SOCKET, struct sockaddr*, int*);
u_long         WINAPI htonl(u_long);
u_short        WINAPI htons(u_short);
char*          WINAPI inet_ntoa(struct in_addr);
u_long         WINAPI ntohl(u_long);
u_short        WINAPI ntohs(u_short);
int            WINAPI recvfrom(SOCKET, char*, int, int, struct sockaddr*, int*);
// select() is NOT declared here — glibc's sys/select.h declares it on Linux
// and the UWP SDK's winsock2.h declares it on Windows. Both define fd_set
// and struct timeval too. The shim uses void* parameters and casts.
int            WINAPI sendto(SOCKET, const char*, int, int, const struct sockaddr*, int);
int            WINAPI shutdown(SOCKET, int);

BOOL           WINAPI WSACloseEvent(WSAEVENT);
WSAEVENT       WINAPI WSACreateEvent();
int            WINAPI WSADuplicateSocketW(SOCKET, DWORD, LPWSAPROTOCOL_INFOW);
int            WINAPI WSAEnumNetworkEvents(SOCKET, WSAEVENT, LPWSANETWORKEVENTS);
int            WINAPI WSAEventSelect(SOCKET, WSAEVENT, long);
int            WINAPI WSAIoctl(SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int            WINAPI WSAPoll(LPWSAPOLLFD, ULONG, INT);
int            WINAPI WSARecv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int            WINAPI WSARecvFrom(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, struct sockaddr*, LPINT, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
BOOL           WINAPI WSAResetEvent(WSAEVENT);
int            WINAPI WSASend(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int            WINAPI WSASendTo(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, const struct sockaddr*, int, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
BOOL           WINAPI WSASetEvent(WSAEVENT);
SOCKET         WINAPI WSASocketW(int, int, int, LPWSAPROTOCOL_INFOW, GROUP, DWORD);
DWORD          WINAPI WSAWaitForMultipleEvents(DWORD, const WSAEVENT*, BOOL, DWORD, BOOL);

INT            WINAPI WSAStringToAddressW(LPWSTR, INT, LPWSAPROTOCOL_INFOW, LPSOCKADDR, LPINT);
INT            WINAPI WSAAddressToStringW(struct sockaddr*, DWORD, LPWSAPROTOCOL_INFOW, LPWSTR, LPDWORD);
}

// UWP-specific VirtualAllocFromApp etc.
extern "C" {
WINBASEAPI LPVOID WINAPI VirtualAllocFromApp(LPVOID, SIZE_T, DWORD, DWORD);
WINBASEAPI BOOL WINAPI VirtualProtectFromApp(LPVOID, SIZE_T, DWORD, PDWORD);
WINBASEAPI LPVOID WINAPI VirtualAlloc2FromApp(HANDLE, LPVOID, SIZE_T, DWORD, DWORD, void*, ULONG);
WINBASEAPI HANDLE WINAPI CreateFile2FromAppW(LPCWSTR, DWORD, DWORD, DWORD, const void*);
WINBASEAPI HANDLE WINAPI CreateFileFromAppW(LPCWSTR, DWORD, DWORD, const void*, DWORD, DWORD, HANDLE);
WINBASEAPI BOOL WINAPI GetFileInformationByHandleExFromApp(HANDLE, int, LPVOID, DWORD);
WINBASEAPI BOOL WINAPI SetFileInformationByHandleFromApp(HANDLE, int, LPVOID, DWORD);
WINBASEAPI BOOL WINAPI OpenFileFromAppW(LPCWSTR, void*, UINT);
}

// HSTRING (UWP)
extern "C" {
WINBASEAPI HRESULT WINAPI WindowsCreateString(LPCWSTR, UINT32, HSTRING*);
WINBASEAPI HRESULT WINAPI WindowsCreateStringReference(LPCWSTR, UINT32, void*, HSTRING*);
WINBASEAPI HRESULT WINAPI WindowsDeleteString(HSTRING);
WINBASEAPI LPCWSTR WINAPI WindowsGetStringRawBuffer(HSTRING, UINT32*);
WINBASEAPI UINT32 WINAPI WindowsGetStringLen(HSTRING);
WINBASEAPI BOOL WINAPI WindowsIsStringEmpty(HSTRING);
WINBASEAPI HRESULT WINAPI WindowsDuplicateString(HSTRING, HSTRING*);
}

// Convenience macros
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

#ifdef _UNICODE
#define GetStartupInfo GetStartupInfoW
#else
#define GetStartupInfo GetStartupInfoA
#endif

// Pointer-64
#ifndef POINTER_64
#define POINTER_64 __ptr64
#endif

// getenv (some shims use it for env-based config)
extern "C" char* getenv(const char*);

// C++17 also defines std::min/max even with NOMINMAX; nothing to do.

// ===========================================================================
// 12) Kernel32 gap-fill types & function declarations
//
// Added for shims/kernel32/Kernel32GapFill.cpp. These types and prototypes
// mirror the desktop Win32 SDK so the gap-fill shims can pass through to
// the real Win32 API when running on Xbox with the real SDK. On Linux
// syntax-check builds, these declarations are sufficient for the .cpp to
// pass g++ -fsyntax-only.
// ===========================================================================

// Scalar aliases used by NLS / locale APIs.
#ifndef _XWR_LANGID_DEFINED
typedef WORD LANGID;
#define _XWR_LANGID_DEFINED
#endif
typedef DWORD LCID;
typedef DWORD LCTYPE;
typedef DWORD LGRPID;
typedef DWORD GEOID;
typedef DWORD GEOTYPE;
typedef DWORD EXECUTION_STATE;
typedef ULONGLONG *PULONGLONG;

// Sync primitives treated as opaque structs (we only forward to real APIs).
typedef struct _SRWLOCK { PVOID Ptr; } SRWLOCK, *PSRWLOCK;
typedef struct _CONDITION_VARIABLE { PVOID Ptr; } CONDITION_VARIABLE, *PCONDITION_VARIABLE;
typedef struct _INIT_ONCE { PVOID Ptr; } INIT_ONCE, *PINIT_ONCE;
typedef BOOL (WINAPI *PINIT_ONCE_FN)(PINIT_ONCE, PVOID, PVOID*);
typedef struct _SLIST_ENTRY { struct _SLIST_ENTRY *Next; } SLIST_ENTRY, *PSLIST_ENTRY;
typedef union _SLIST_HEADER { ULONG_PTR Alignment; struct { SLIST_ENTRY Next; WORD Depth; WORD Sequence; } Header; } SLIST_HEADER, *PSLIST_HEADER;

// Console types.
typedef struct _COORD { SHORT X; SHORT Y; } COORD, *PCOORD;
typedef struct _SMALL_RECT { SHORT Left; SHORT Top; SHORT Right; SHORT Bottom; } SMALL_RECT, *PSMALL_RECT;
typedef struct _CONSOLE_SCREEN_BUFFER_INFO { COORD dwSize; COORD dwCursorPosition; WORD wAttributes; SMALL_RECT srWindow; COORD dwMaximumWindowSize; } CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;
typedef struct _CONSOLE_CURSOR_INFO { DWORD dwSize; BOOL bVisible; } CONSOLE_CURSOR_INFO, *PCONSOLE_CURSOR_INFO;
typedef struct _INPUT_RECORD { WORD EventType; union { /* simplified */ UCHAR Padding[32]; } Event; } INPUT_RECORD, *PINPUT_RECORD;
typedef BOOL (WINAPI *PHANDLER_ROUTINE)(DWORD);

// Locale / NLS structs.
typedef struct _NUMBERFMTA { UINT NumDigits; UINT LeadingZero; UINT Grouping; LPSTR lpDecimalSep; LPSTR lpThousandSep; UINT NegativeOrder; } NUMBERFMTA, *LPNUMBERFMTA;
typedef struct _NUMBERFMTW { UINT NumDigits; UINT LeadingZero; UINT Grouping; LPWSTR lpDecimalSep; LPWSTR lpThousandSep; UINT NegativeOrder; } NUMBERFMTW, *LPNUMBERFMTW;
typedef struct _CURRENCYFMTA { UINT NumDigits; UINT LeadingZero; UINT Grouping; LPSTR lpDecimalSep; LPSTR lpThousandSep; UINT NegativeOrder; UINT PositiveOrder; LPSTR lpCurrencySymbol; } CURRENCYFMTA, *LPCURRENCYFMTA;
typedef struct _CURRENCYFMTW { UINT NumDigits; UINT LeadingZero; UINT Grouping; LPWSTR lpDecimalSep; LPWSTR lpThousandSep; UINT NegativeOrder; UINT PositiveOrder; LPWSTR lpCurrencySymbol; } CURRENCYFMTW, *LPCURRENCYFMTW;
typedef struct _cpinfo { UINT MaxCharSize; BYTE DefaultChar[2]; BYTE LeadByte[12]; } CPINFO, *LPCPINFO;
typedef BOOL (WINAPI *LOCALE_ENUMPROCW)(LPWSTR);
typedef BOOL (WINAPI *LOCALE_ENUMPROCA)(LPSTR);
typedef BOOL (WINAPI *CODEPAGE_ENUMPROCW)(LPWSTR);
typedef BOOL (WINAPI *DATEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL (WINAPI *TIMEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL (WINAPI *GEO_ENUMPROC)(GEOID);
typedef void* LPNLSVERINFO;
typedef struct _NLSVERSIONINFOEX { DWORD dwNLSVersionInfoSize; DWORD dwNLSVersion; DWORD dwDefinedVersion; DWORD dwEffectiveId; GUID guidCustomVersion; } NLSVERSIONINFOEX, *LPNLSVERSIONINFOEX;
typedef NLSVERSIONINFOEX NLSVERSIONINFO, *LPNLSVERSIONINFO;

// Time types.
typedef struct _SYSTEMTIME { WORD wYear; WORD wMonth; WORD wDayOfWeek; WORD wDay; WORD wHour; WORD wMinute; WORD wSecond; WORD wMilliseconds; } SYSTEMTIME, *LPSYSTEMTIME;
typedef struct _TIME_ZONE_INFORMATION { LONG Bias; WCHAR StandardName[32]; SYSTEMTIME StandardDate; LONG StandardBias; WCHAR DaylightName[32]; SYSTEMTIME DaylightDate; LONG DaylightBias; } TIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;
typedef struct _DYNAMIC_TIME_ZONE_INFORMATION { LONG Bias; WCHAR StandardName[32]; SYSTEMTIME StandardDate; LONG StandardBias; WCHAR DaylightName[32]; SYSTEMTIME DaylightDate; LONG DaylightBias; WCHAR TimeZoneKeyName[128]; BOOLEAN DynamicDaylightTimeDisabled; } DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;

// System info / version / power.
typedef struct _OSVERSIONINFOEXW { DWORD dwOSVersionInfoSize; DWORD dwMajorVersion; DWORD dwMinorVersion; DWORD dwBuildNumber; DWORD dwPlatformId; WCHAR szCSDVersion[128]; WORD wServicePackMajor; WORD wServicePackMinor; WORD wSuiteMask; BYTE wProductType; BYTE wReserved; } OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW;
typedef OSVERSIONINFOEXW OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW;
typedef struct _SYSTEM_POWER_STATUS { BYTE ACLineStatus; BYTE BatteryFlag; BYTE BatteryLifePercent; BYTE Reserved1; DWORD BatteryLifeTime; DWORD BatteryFullLifeTime; } SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

// NUMA / processor topology.
typedef struct _GROUP_AFFINITY { DWORD_PTR Mask; WORD Group; WORD Reserved[3]; } GROUP_AFFINITY, *PGROUP_AFFINITY;
typedef struct _PROCESSOR_NUMBER { WORD Group; BYTE Number; BYTE Reserved; } PROCESSOR_NUMBER, *PPROCESSOR_NUMBER;
typedef DWORD LOGICAL_PROCESSOR_RELATIONSHIP;
typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION { ULONG_PTR ProcessorMask; LOGICAL_PROCESSOR_RELATIONSHIP Relationship; BYTE _Reserved[24]; } SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;
typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX { LOGICAL_PROCESSOR_RELATIONSHIP Relationship; DWORD Size; BYTE _Reserved[32]; } SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;

// File info / file mapping.
typedef DWORD FILE_INFO_BY_HANDLE_CLASS;
typedef struct _FILE_NAME_INFO { DWORD FileNameLength; WCHAR FileName[1]; } FILE_NAME_INFO;
typedef struct _FILE_IO_PRIORITY_HINT_INFO { ULONG PriorityHint; } FILE_IO_PRIORITY_HINT_INFO;
typedef struct _OVERLAPPED_ENTRY { ULONG_PTR lpCompletionKey; LPOVERLAPPED lpOverlapped; ULONG_PTR Internal; DWORD dwNumberOfBytesTransferred; } OVERLAPPED_ENTRY, *LPOVERLAPPED_ENTRY;
typedef enum _COPYFILE2_MESSAGE_TYPE { COPYFILE2_PROGRESS_NONE_v } COPYFILE2_MESSAGE_TYPE;
typedef struct _COPYFILE2_EXTENDED_PARAMETERS { DWORD dwSize; DWORD dwCopyFlags; BOOL bCancelCopy; PVOID pvExtraContext; PVOID* pfResetProgress; PVOID pfResetProgress2; } COPYFILE2_EXTENDED_PARAMETERS;
typedef struct _CREATEFILE2_EXTENDED_PARAMETERS { DWORD dwSize; DWORD dwFileAttributes; DWORD dwFileFlags; DWORD dwSecurityQosFlags; LPSECURITY_ATTRIBUTES lpSecurityAttributes; HANDLE hTemplateFile; } CREATEFILE2_EXTENDED_PARAMETERS, *LPCREATEFILE2_EXTENDED_PARAMETERS;
typedef DWORD FINDEX_INFO_LEVELS;
typedef DWORD FINDEX_SEARCH_OPS;
typedef void (CALLBACK *PTIMERAPCROUTINE)(LPVOID, DWORD, DWORD);

// Heap / memory resource.
typedef DWORD HEAP_INFORMATION_CLASS;
typedef DWORD MEMORY_RESOURCE_NOTIFICATION_TYPE;
#ifndef FLS_OUT_OF_INDEXES
#define FLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif

// Toolhelp.
typedef struct _PROCESSENTRY32W { DWORD dwSize; DWORD cntUsage; DWORD th32ProcessID; ULONG_PTR th32DefaultHeapID; DWORD th32ModuleID; DWORD cntThreads; DWORD th32ParentProcessID; LONG pcPriClassBase; DWORD dwFlags; WCHAR szExeFile[260]; } PROCESSENTRY32W, *PPROCESSENTRY32W, *LPPROCESSENTRY32W;
typedef struct _PROCESSENTRY32 { DWORD dwSize; DWORD cntUsage; DWORD th32ProcessID; ULONG_PTR th32DefaultHeapID; DWORD th32ModuleID; DWORD cntThreads; DWORD th32ParentProcessID; LONG pcPriClassBase; DWORD dwFlags; CHAR szExeFile[260]; } PROCESSENTRY32, *PPROCESSENTRY32, *LPPROCESSENTRY32;
typedef struct _THREADENTRY32 { DWORD dwSize; DWORD cntUsage; DWORD th32ThreadID; DWORD th32OwnerProcessID; LONG tpBasePri; LONG tpDeltaPri; DWORD dwFlags; } THREADENTRY32, *PTHREADENTRY32, *LPTHREADENTRY32;

// PSAPI.
typedef struct _MODULEINFO { LPVOID lpBaseOfDll; DWORD SizeOfImage; LPVOID EntryPoint; } MODULEINFO, *LPMODULEINFO;
typedef struct _PROCESS_MEMORY_COUNTERS { DWORD cb; DWORD PageFaultCount; SIZE_T PeakWorkingSetSize; SIZE_T WorkingSetSize; SIZE_T QuotaPeakPagedPoolUsage; SIZE_T QuotaPagedPoolUsage; SIZE_T QuotaPeakNonPagedPoolUsage; SIZE_T QuotaNonPagedPoolUsage; SIZE_T PagefileUsage; SIZE_T PeakPagefileUsage; SIZE_T PrivateUsage; } PROCESS_MEMORY_COUNTERS, *PPROCESS_MEMORY_COUNTERS;

// Job objects.
typedef DWORD JOBOBJECTINFOCLASS;

// Exception / RTL.
typedef struct _EXCEPTION_RECORD { DWORD ExceptionCode; DWORD ExceptionFlags; struct _EXCEPTION_RECORD *ExceptionRecord; PVOID ExceptionAddress; DWORD NumberParameters; ULONG_PTR ExceptionInformation[15]; } EXCEPTION_RECORD, *PEXCEPTION_RECORD;
typedef struct _EXCEPTION_POINTERS { PEXCEPTION_RECORD ExceptionRecord; PCONTEXT ContextRecord; } EXCEPTION_POINTERS, *LPEXCEPTION_POINTERS;
typedef enum _EXCEPTION_DISPOSITION { ExceptionContinueSearch, ExceptionContinueExecution, ExceptionNestedException, ExceptionCollidedUnwind } EXCEPTION_DISPOSITION;
typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY { DWORD BeginAddress; DWORD EndAddress; union { DWORD UnwindInfoAddress; DWORD UnwindData; } DUMMYUNIONNAME; } IMAGE_RUNTIME_FUNCTION_ENTRY, *PIMAGE_RUNTIME_FUNCTION_ENTRY;
typedef EXCEPTION_DISPOSITION (WINAPI *PEXCEPTION_ROUTINE)(DWORD64, PIMAGE_RUNTIME_FUNCTION_ENTRY, PCONTEXT, PVOID, PULONG_PTR);
typedef struct _EXCEPTION_HISTORY_TABLE { DWORD Count; ULONG_PTR Index; PEXCEPTION_ROUTINE* Routine; } EXCEPTION_HISTORY_TABLE, *PEXCEPTION_HISTORY_TABLE;

// UNWIND_HISTORY_TABLE — opaque struct (we only pass it by pointer).
struct _UNWIND_HISTORY_TABLE;
typedef struct _UNWIND_HISTORY_TABLE UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

// KNONVOLATILE_CONTEXT_POINTERS — opaque (x64 layout differs by arch; we don't
// actually inspect the fields here — the shim just forwards the pointer).
struct _KNONVOLATILE_CONTEXT_POINTERS;
typedef struct _KNONVOLATILE_CONTEXT_POINTERS KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

// Thread pool (opaque).
typedef struct _TP_WAIT TP_WAIT, *PTP_WAIT;
typedef struct _TP_CALLBACK_INSTANCE TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;
typedef struct _TP_CALLBACK_ENVIRON TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;
typedef void (WINAPI *PTP_WAIT_CALLBACK)(PTP_CALLBACK_INSTANCE, PVOID, TP_WAIT*, DWORD);

// APC / fiber / FLS callbacks.
typedef void (WINAPI *PAPCFUNC)(ULONG_PTR);
typedef void (WINAPI *LPFIBER_START_ROUTINE)(LPVOID);
typedef void (WINAPI *PFLS_CALLBACK_FUNCTION)(PVOID);
typedef DWORD FLS_CALLBACK_FUNCTION;

// Waitable-timer / wait callback.
typedef void (CALLBACK *WAITORTIMERCALLBACK)(PVOID, BOOLEAN);

// Helper used by some locale signatures.
typedef PVOID LPOVERLAPPED_COMPLETION_ROUTINE_PARAM;

// ===========================================================================
// 12b) Kernel32 gap-fill function declarations (alphabetical by name as used).
// ===========================================================================
extern "C" {
// Console
WINBASEAPI BOOL WINAPI AttachConsole(DWORD);
WINBASEAPI BOOL WINAPI FreeConsole();
WINBASEAPI BOOL WINAPI GetConsoleCursorInfo(HANDLE, PCONSOLE_CURSOR_INFO);
WINBASEAPI BOOL WINAPI SetConsoleCursorInfo(HANDLE, const CONSOLE_CURSOR_INFO*);
WINBASEAPI BOOL WINAPI GetConsoleScreenBufferInfo(HANDLE, PCONSOLE_SCREEN_BUFFER_INFO);
WINBASEAPI BOOL WINAPI SetConsoleScreenBufferSize(HANDLE, COORD);
WINBASEAPI HWND WINAPI GetConsoleWindow();
WINBASEAPI BOOL WINAPI GetNumberOfConsoleInputEvents(HANDLE, LPDWORD);
WINBASEAPI BOOL WINAPI FillConsoleOutputAttribute(HANDLE, WORD, DWORD, COORD, LPDWORD);
WINBASEAPI BOOL WINAPI FillConsoleOutputCharacterW(HANDLE, WCHAR, DWORD, COORD, LPDWORD);
WINBASEAPI BOOL WINAPI SetConsoleCursorPosition(HANDLE, COORD);
WINBASEAPI BOOL WINAPI SetConsoleTextAttribute(HANDLE, WORD);
WINBASEAPI BOOL WINAPI SetConsoleTitleA(LPCSTR);
WINBASEAPI BOOL WINAPI SetConsoleTitleW(LPCWSTR);
WINBASEAPI BOOL WINAPI SetConsoleWindowInfo(HANDLE, BOOL, const SMALL_RECT*);
WINBASEAPI BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE, BOOL);
WINBASEAPI BOOL WINAPI ReadConsoleA(HANDLE, LPVOID, DWORD, LPDWORD, LPVOID);
WINBASEAPI BOOL WINAPI ReadConsoleW(HANDLE, LPVOID, DWORD, LPDWORD, LPVOID);
WINBASEAPI BOOL WINAPI ReadConsoleInputW(HANDLE, PINPUT_RECORD, DWORD, LPDWORD);
WINBASEAPI BOOL WINAPI WriteConsoleA(HANDLE, const void*, DWORD, LPDWORD, LPVOID);
WINBASEAPI BOOL WINAPI WriteConsoleInputW(HANDLE, const INPUT_RECORD*, DWORD, LPDWORD);

// SRW / Condition Variable / Critical Section extensions
WINBASEAPI VOID WINAPI InitializeSRWLock(PSRWLOCK);
WINBASEAPI VOID WINAPI AcquireSRWLockExclusive(PSRWLOCK);
WINBASEAPI VOID WINAPI AcquireSRWLockShared(PSRWLOCK);
WINBASEAPI VOID WINAPI ReleaseSRWLockExclusive(PSRWLOCK);
WINBASEAPI VOID WINAPI ReleaseSRWLockShared(PSRWLOCK);
WINBASEAPI BOOLEAN WINAPI TryAcquireSRWLockExclusive(PSRWLOCK);
WINBASEAPI VOID WINAPI InitializeConditionVariable(PCONDITION_VARIABLE);
WINBASEAPI VOID WINAPI WakeConditionVariable(PCONDITION_VARIABLE);
WINBASEAPI VOID WINAPI WakeAllConditionVariable(PCONDITION_VARIABLE);
WINBASEAPI BOOL WINAPI SleepConditionVariableCS(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD);
WINBASEAPI BOOL WINAPI SleepConditionVariableSRW(PCONDITION_VARIABLE, PSRWLOCK, DWORD, ULONG);
WINBASEAPI BOOL WINAPI InitializeCriticalSectionEx(LPCRITICAL_SECTION, DWORD, DWORD);
WINBASEAPI DWORD WINAPI SetCriticalSectionSpinCount(LPCRITICAL_SECTION, DWORD);
WINBASEAPI BOOL WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION);

// SList / InitOnce
WINBASEAPI VOID WINAPI InitializeSListHead(PSLIST_HEADER);
WINBASEAPI PSLIST_ENTRY WINAPI InterlockedFlushSList(PSLIST_HEADER);
WINBASEAPI PSLIST_ENTRY WINAPI InterlockedPopEntrySList(PSLIST_HEADER);
WINBASEAPI PSLIST_ENTRY WINAPI InterlockedPushEntrySList(PSLIST_HEADER, PSLIST_ENTRY);
WINBASEAPI USHORT WINAPI QueryDepthSList(PSLIST_HEADER);
WINBASEAPI BOOL WINAPI InitOnceBeginInitialize(PINIT_ONCE, DWORD, PBOOL, LPVOID*);
WINBASEAPI BOOL WINAPI InitOnceComplete(PINIT_ONCE, DWORD, LPVOID);
WINBASEAPI BOOL WINAPI InitOnceExecuteOnce(PINIT_ONCE, PINIT_ONCE_FN, PVOID, LPVOID*);

// Interlocked (declared as __stdcall here; intrinsics on real SDK)
WINBASEAPI LONG WINAPI InterlockedCompareExchange(LONG volatile*, LONG, LONG);
WINBASEAPI LONG WINAPI InterlockedDecrement(LONG volatile*);
WINBASEAPI LONG WINAPI InterlockedExchange(LONG volatile*, LONG);
WINBASEAPI LONG WINAPI InterlockedIncrement(LONG volatile*);

// TLS / FLS
WINBASEAPI DWORD WINAPI FlsAlloc(PFLS_CALLBACK_FUNCTION);
WINBASEAPI BOOL WINAPI FlsFree(DWORD);
WINBASEAPI PVOID WINAPI FlsGetValue(DWORD);
WINBASEAPI BOOL WINAPI FlsSetValue(DWORD, PVOID);

// Fibers
WINBASEAPI LPVOID WINAPI CreateFiber(SIZE_T, LPFIBER_START_ROUTINE, LPVOID);
WINBASEAPI LPVOID WINAPI CreateFiberEx(SIZE_T, SIZE_T, DWORD, LPFIBER_START_ROUTINE, LPVOID);
WINBASEAPI BOOL WINAPI ConvertFiberToThread();
WINBASEAPI LPVOID WINAPI ConvertThreadToFiber(LPVOID);
WINBASEAPI LPVOID WINAPI ConvertThreadToFiberEx(LPVOID, DWORD);
WINBASEAPI VOID WINAPI DeleteFiber(LPVOID);
WINBASEAPI VOID WINAPI SwitchToFiber(LPVOID);
WINBASEAPI BOOL WINAPI IsThreadAFiber();

// Process / thread info (entries already declared in the kernel32 block above are omitted here)
WINBASEAPI VOID WINAPI ExitProcess(UINT);
WINBASEAPI VOID WINAPI ExitThread(DWORD);
WINBASEAPI BOOL WINAPI CreateProcessA(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
WINBASEAPI VOID WINAPI FreeLibraryAndExitThread(HMODULE, DWORD);
WINBASEAPI DWORD WINAPI GetProcessId(HANDLE);
WINBASEAPI BOOL WINAPI GetProcessHandleCount(HANDLE, PDWORD);
WINBASEAPI DWORD WINAPI GetPriorityClass(HANDLE);
WINBASEAPI BOOL WINAPI QueryFullProcessImageNameW(HANDLE, DWORD, LPWSTR, PDWORD);
WINBASEAPI BOOL WINAPI IsProcessInJob(HANDLE, HANDLE, PBOOL);
WINBASEAPI BOOL WINAPI SetThreadStackGuarantee(PULONG);
WINBASEAPI VOID WINAPI GetCurrentThreadStackLimits(PULONG_PTR, PULONG_PTR);
WINBASEAPI HRESULT WINAPI SetThreadDescription(HANDLE, PCWSTR);
WINBASEAPI DWORD WINAPI GetCurrentProcessorNumber();
WINBASEAPI VOID WINAPI GetCurrentProcessorNumberEx(PPROCESSOR_NUMBER);
WINBASEAPI DWORD WINAPI GetMaximumProcessorCount(WORD);
WINBASEAPI BOOL WINAPI GetNumaHighestNodeNumber(PULONG);
WINBASEAPI BOOL WINAPI GetNumaNodeProcessorMask(UCHAR, PULONGLONG);
WINBASEAPI BOOL WINAPI GetNumaNodeProcessorMaskEx(USHORT, PGROUP_AFFINITY);
WINBASEAPI BOOL WINAPI GetNumaProcessorNode(UCHAR, PUCHAR);
WINBASEAPI BOOL WINAPI GetNumaProcessorNodeEx(PPROCESSOR_NUMBER, PUSHORT);
WINBASEAPI BOOL WINAPI GetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
WINBASEAPI BOOL WINAPI GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
WINBASEAPI BOOL WINAPI SetThreadIdealProcessorEx(HANDLE, PPROCESSOR_NUMBER, PPROCESSOR_NUMBER);
WINBASEAPI BOOL WINAPI GetThreadIdealProcessorEx(HANDLE, PPROCESSOR_NUMBER);
WINBASEAPI BOOL WINAPI SetThreadGroupAffinity(HANDLE, const GROUP_AFFINITY*, PGROUP_AFFINITY);
WINBASEAPI BOOL WINAPI ProcessIdToSessionId(DWORD, PDWORD);
WINBASEAPI DWORD WINAPI QueueUserAPC(PAPCFUNC, HANDLE, ULONG_PTR);
WINBASEAPI BOOL WINAPI QueueUserWorkItem(LPTHREAD_START_ROUTINE, PVOID, ULONG);

// Files / pipes / IO
WINBASEAPI HRESULT WINAPI CopyFile2(LPCWSTR, LPCWSTR, COPYFILE2_EXTENDED_PARAMETERS*);
WINBASEAPI HANDLE WINAPI CreateFile2(LPCWSTR, DWORD, DWORD, DWORD, LPCREATEFILE2_EXTENDED_PARAMETERS);
WINBASEAPI HANDLE WINAPI CreateFileMappingA(HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCSTR);
WINBASEAPI LPVOID WINAPI MapViewOfFileEx(HANDLE, DWORD, DWORD, DWORD, SIZE_T, LPVOID);
WINBASEAPI BOOL WINAPI FlushInstructionCache(HANDLE, LPCVOID, SIZE_T);
WINBASEAPI BOOL WINAPI VirtualUnlock(LPVOID, SIZE_T);
WINBASEAPI BOOL WINAPI GetFileInformationByHandleEx(HANDLE, FILE_INFO_BY_HANDLE_CLASS, LPVOID, DWORD);
WINBASEAPI BOOL WINAPI SetFileInformationByHandle(HANDLE, FILE_INFO_BY_HANDLE_CLASS, LPVOID, DWORD);
WINBASEAPI DWORD WINAPI GetFileAttributesA(LPCSTR);
WINBASEAPI DWORD WINAPI GetFileType(HANDLE);
WINBASEAPI DWORD WINAPI GetFinalPathNameByHandleW(HANDLE, LPWSTR, DWORD, DWORD);
WINBASEAPI BOOL WINAPI SetFileCompletionNotificationModes(HANDLE, UCHAR);
WINBASEAPI HANDLE WINAPI FindFirstFileExA(LPCSTR, FINDEX_INFO_LEVELS, LPVOID, FINDEX_SEARCH_OPS, LPVOID, DWORD);
WINBASEAPI BOOL WINAPI FindNextFileA(HANDLE, LPWIN32_FIND_DATAA);
WINBASEAPI HRSRC WINAPI FindResourceW(HMODULE, LPCWSTR, LPCWSTR);
WINBASEAPI HGLOBAL WINAPI LoadResource(HMODULE, HRSRC);
WINBASEAPI LPVOID WINAPI LockResource(HGLOBAL);
WINBASEAPI DWORD WINAPI SizeofResource(HMODULE, HRSRC);
WINBASEAPI HANDLE WINAPI ReOpenFile(HANDLE, DWORD, DWORD, DWORD);
WINBASEAPI BOOL WINAPI ReplaceFileW(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPVOID, LPVOID);
WINBASEAPI BOOL WINAPI MoveFileExA(LPCSTR, LPCSTR, DWORD);
WINBASEAPI BOOL WINAPI AreFileApisANSI();
WINBASEAPI HANDLE WINAPI CreateNamedPipeW(LPCWSTR, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPSECURITY_ATTRIBUTES);
WINBASEAPI BOOL WINAPI ConnectNamedPipe(HANDLE, LPOVERLAPPED);
WINBASEAPI BOOL WINAPI PeekNamedPipe(HANDLE, LPVOID, DWORD, LPDWORD, LPDWORD, LPDWORD);
WINBASEAPI BOOL WINAPI SetNamedPipeHandleState(HANDLE, LPDWORD, LPDWORD, LPDWORD);
WINBASEAPI BOOL WINAPI GetNamedPipeClientProcessId(HANDLE, PULONG);
WINBASEAPI BOOL WINAPI GetNamedPipeServerProcessId(HANDLE, PULONG);
WINBASEAPI BOOL WINAPI GetNamedPipeHandleStateW(HANDLE, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPWSTR, DWORD);
WINBASEAPI BOOL WINAPI WaitNamedPipeW(LPCWSTR, DWORD);
WINBASEAPI BOOL WINAPI CreatePipe(PHANDLE, PHANDLE, LPSECURITY_ATTRIBUTES, DWORD);
WINBASEAPI BOOL WINAPI DeviceIoControl(HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
WINBASEAPI BOOL WINAPI ReadDirectoryChangesW(HANDLE, LPVOID, DWORD, BOOL, DWORD, LPDWORD, LPOVERLAPPED, LPOVERLAPPED_COMPLETION_ROUTINE);

// IO completion
WINBASEAPI BOOL WINAPI GetQueuedCompletionStatusEx(HANDLE, LPOVERLAPPED_ENTRY, ULONG, PULONG, DWORD, BOOL);
WINBASEAPI BOOL WINAPI CancelSynchronousIo(HANDLE);

// Waitable timers
WINBASEAPI HANDLE WINAPI CreateWaitableTimerA(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR);
WINBASEAPI HANDLE WINAPI CreateWaitableTimerW(LPSECURITY_ATTRIBUTES, BOOL, LPCWSTR);
WINBASEAPI HANDLE WINAPI CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);
WINBASEAPI BOOL WINAPI CancelWaitableTimer(HANDLE);
WINBASEAPI BOOL WINAPI SetWaitableTimer(HANDLE, const LARGE_INTEGER*, LONG, void*, LPVOID, BOOL);

// Thread pool / timer queue
WINBASEAPI PTP_WAIT WINAPI CreateThreadpoolWait(PTP_WAIT_CALLBACK, PVOID, PTP_CALLBACK_ENVIRON);
WINBASEAPI VOID WINAPI SetThreadpoolWait(PTP_WAIT, HANDLE, PFILETIME);
WINBASEAPI VOID WINAPI WaitForThreadpoolWaitCallbacks(PTP_WAIT, BOOL);
WINBASEAPI VOID WINAPI CloseThreadpoolWait(PTP_WAIT);
WINBASEAPI HANDLE WINAPI CreateTimerQueue();
WINBASEAPI BOOL WINAPI CreateTimerQueueTimer(PHANDLE, HANDLE, WAITORTIMERCALLBACK, PVOID, DWORD, DWORD, ULONG);
WINBASEAPI BOOL WINAPI ChangeTimerQueueTimer(HANDLE, HANDLE, ULONG, ULONG);
WINBASEAPI BOOL WINAPI DeleteTimerQueueTimer(HANDLE, HANDLE, HANDLE);
WINBASEAPI BOOL WINAPI RegisterWaitForSingleObject(PHANDLE, HANDLE, WAITORTIMERCALLBACK, PVOID, DWORD, DWORD);
WINBASEAPI BOOL WINAPI UnregisterWait(HANDLE);
WINBASEAPI BOOL WINAPI UnregisterWaitEx(HANDLE, HANDLE);

// Memory resource
WINBASEAPI HANDLE WINAPI CreateMemoryResourceNotification(MEMORY_RESOURCE_NOTIFICATION_TYPE);
WINBASEAPI BOOL WINAPI QueryMemoryResourceNotification(HANDLE, PBOOL);

// Heap
WINBASEAPI BOOL WINAPI HeapSetInformation(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

// Global / local
WINBASEAPI HLOCAL WINAPI LocalAlloc(UINT, SIZE_T);
WINBASEAPI HLOCAL WINAPI LocalFree(HLOCAL);

// Job / toolhelp / PSAPI
WINBASEAPI BOOL WINAPI QueryInformationJobObject(HANDLE, JOBOBJECTINFOCLASS, LPVOID, DWORD, LPDWORD);
WINBASEAPI HANDLE WINAPI CreateJobObjectA(LPSECURITY_ATTRIBUTES, LPCSTR);
WINBASEAPI HANDLE WINAPI CreateJobObjectW(LPSECURITY_ATTRIBUTES, LPCWSTR);
WINBASEAPI BOOL WINAPI AssignProcessToJobObject(HANDLE, HANDLE);
WINBASEAPI BOOL WINAPI SetInformationJobObject(HANDLE, JOBOBJECTINFOCLASS, LPVOID, DWORD);
WINBASEAPI DWORD WINAPI SearchPathW(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPWSTR, LPWSTR*);
WINBASEAPI UINT WINAPI SetHandleCount(UINT);
WINBASEAPI HANDLE WINAPI CreateToolhelp32Snapshot(DWORD, DWORD);
WINBASEAPI BOOL WINAPI Process32First(HANDLE, LPPROCESSENTRY32);
WINBASEAPI BOOL WINAPI Process32FirstW(HANDLE, LPPROCESSENTRY32W);
WINBASEAPI BOOL WINAPI Process32Next(HANDLE, LPPROCESSENTRY32);
WINBASEAPI BOOL WINAPI Process32NextW(HANDLE, LPPROCESSENTRY32W);
WINBASEAPI BOOL WINAPI Thread32First(HANDLE, LPTHREADENTRY32);
WINBASEAPI BOOL WINAPI Thread32Next(HANDLE, LPTHREADENTRY32);
WINBASEAPI BOOL WINAPI K32EnumProcessModules(HANDLE, HMODULE*, DWORD, LPDWORD);
WINBASEAPI BOOL WINAPI K32EnumProcessModulesEx(HANDLE, HMODULE*, DWORD, LPDWORD, DWORD);
WINBASEAPI DWORD WINAPI K32GetModuleFileNameExW(HANDLE, HMODULE, LPWSTR, DWORD);
WINBASEAPI BOOL WINAPI K32GetModuleInformation(HANDLE, HMODULE, LPMODULEINFO, DWORD);
WINBASEAPI BOOL WINAPI K32GetProcessMemoryInfo(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

// Exceptions / RTL
WINBASEAPI VOID WINAPI RaiseException(DWORD, DWORD, DWORD, const ULONG_PTR*);
WINBASEAPI LONG WINAPI UnhandledExceptionFilter(LPEXCEPTION_POINTERS);
WINBASEAPI PIMAGE_RUNTIME_FUNCTION_ENTRY WINAPI RtlLookupFunctionEntry(DWORD64, PDWORD64, PUNWIND_HISTORY_TABLE);
WINBASEAPI PVOID WINAPI RtlPcToFileHeader(PVOID, PVOID*);
WINBASEAPI VOID WINAPI RtlUnwind(PVOID, PVOID, PEXCEPTION_RECORD, PVOID);
WINBASEAPI VOID WINAPI RtlUnwindEx(PVOID, PVOID, PEXCEPTION_RECORD, PVOID, PCONTEXT, PUNWIND_HISTORY_TABLE);
WINBASEAPI PEXCEPTION_ROUTINE WINAPI RtlVirtualUnwind(DWORD, DWORD64, DWORD64, PIMAGE_RUNTIME_FUNCTION_ENTRY, PCONTEXT, PVOID*, PULONG_PTR, PKNONVOLATILE_CONTEXT_POINTERS);

// Loader (extra entries beyond the kernel32 block above)
WINBASEAPI HMODULE WINAPI LoadLibraryA(LPCSTR);
WINBASEAPI HMODULE WINAPI LoadLibraryExA(LPCSTR, HANDLE, DWORD);
WINBASEAPI DWORD WINAPI GetModuleFileNameA(HMODULE, LPSTR, DWORD);
WINBASEAPI BOOL WINAPI GetModuleHandleExA(DWORD, LPCSTR, HMODULE*);
WINBASEAPI BOOL WINAPI GetModuleHandleExW(DWORD, LPCWSTR, HMODULE*);
WINBASEAPI BOOL WINAPI GetDllDirectoryW(DWORD, LPWSTR);
WINBASEAPI BOOL WINAPI SetDllDirectoryW(LPCWSTR);

// System info / locale / time
WINBASEAPI UINT WINAPI GetACP();
WINBASEAPI UINT WINAPI GetOEMCP();
WINBASEAPI BOOL WINAPI GetCPInfo(UINT, LPCPINFO);
WINBASEAPI BOOL WINAPI IsValidCodePage(UINT);
WINBASEAPI BOOL WINAPI GetComputerNameW(LPWSTR, LPDWORD);
WINBASEAPI VOID WINAPI GetStartupInfoW(LPSTARTUPINFOW);
WINBASEAPI LPSTR WINAPI GetCommandLineA();
WINBASEAPI LANGID WINAPI GetSystemDefaultLangID();
WINBASEAPI LANGID WINAPI GetUserDefaultLangID();
WINBASEAPI LCID WINAPI GetUserDefaultLCID();
WINBASEAPI LCID WINAPI GetThreadLocale();
WINBASEAPI int WINAPI GetUserDefaultLocaleName(LPWSTR, int);
WINBASEAPI int WINAPI GetLocaleInfoEx(LPCWSTR, LCTYPE, LPWSTR, int);
WINBASEAPI int WINAPI GetLocaleInfoW(LCID, LCTYPE, LPWSTR, int);
WINBASEAPI int WINAPI LCIDToLocaleName(LCID, LPWSTR, int, DWORD);
WINBASEAPI LCID WINAPI LocaleNameToLCID(LPCWSTR, DWORD);
WINBASEAPI int WINAPI ResolveLocaleName(LPCWSTR, LPWSTR, int);
WINBASEAPI BOOL WINAPI IsValidLocale(LCID, DWORD);
WINBASEAPI BOOL WINAPI EnumSystemLocalesW(LOCALE_ENUMPROCW, DWORD);
WINBASEAPI int WINAPI GetGeoInfoW(GEOID, GEOTYPE, LPWSTR, int, LANGID);
WINBASEAPI GEOID WINAPI GetUserGeoID(GEOTYPE);
WINBASEAPI EXECUTION_STATE WINAPI SetThreadExecutionState(EXECUTION_STATE);
WINBASEAPI int WINAPI CompareStringA(LCID, DWORD, LPCSTR, int, LPCSTR, int);
WINBASEAPI int WINAPI CompareStringW(LCID, DWORD, LPCWSTR, int, LPCWSTR, int);
WINBASEAPI int WINAPI CompareStringEx(LPCWSTR, DWORD, LPCWSTR, int, LPCWSTR, int, LPNLSVERSIONINFOEX, LPVOID, LPARAM);
WINBASEAPI int WINAPI CompareStringOrdinal(LPCWSTR, int, LPCWSTR, int, BOOL);
WINBASEAPI int WINAPI LCMapStringW(LCID, DWORD, LPCWSTR, int, LPWSTR, int);
WINBASEAPI int WINAPI LCMapStringEx(LPCWSTR, DWORD, LPCWSTR, int, LPWSTR, int, LPNLSVERSIONINFOEX, LPVOID, LPARAM);
WINBASEAPI int WINAPI GetDateFormatW(LCID, DWORD, const SYSTEMTIME*, LPCWSTR, LPWSTR, int);
WINBASEAPI int WINAPI GetDateFormatEx(LPCWSTR, DWORD, const SYSTEMTIME*, LPCWSTR, LPWSTR, int, LPCWSTR);
WINBASEAPI int WINAPI GetTimeFormatW(LCID, DWORD, const SYSTEMTIME*, LPCWSTR, LPWSTR, int);
WINBASEAPI int WINAPI GetTimeFormatEx(LPCWSTR, DWORD, const SYSTEMTIME*, LPCWSTR, LPWSTR, int);
WINBASEAPI int WINAPI GetCurrencyFormatEx(LPCWSTR, DWORD, LPCWSTR, const CURRENCYFMTW*, LPWSTR, int);
WINBASEAPI int WINAPI GetNumberFormatEx(LPCWSTR, DWORD, LPCWSTR, const NUMBERFMTW*, LPWSTR, int);
WINBASEAPI BOOL WINAPI GetStringTypeW(DWORD, LPCWSTR, int, LPWORD);

WINBASEAPI VOID WINAPI GetLocalTime(LPSYSTEMTIME);
WINBASEAPI VOID WINAPI GetSystemTime(LPSYSTEMTIME);
WINBASEAPI VOID WINAPI GetSystemTimeAsFileTime(LPFILETIME);
WINBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(LPFILETIME);
WINBASEAPI BOOL WINAPI GetSystemTimeAdjustment(PDWORD, PDWORD, PBOOL);
WINBASEAPI DWORD WINAPI GetTimeZoneInformation(LPTIME_ZONE_INFORMATION);
WINBASEAPI DWORD WINAPI GetDynamicTimeZoneInformation(PDYNAMIC_TIME_ZONE_INFORMATION);
WINBASEAPI BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME*, LPFILETIME);
WINBASEAPI BOOL WINAPI FileTimeToSystemTime(const FILETIME*, LPSYSTEMTIME);
WINBASEAPI BOOL WINAPI LocalFileTimeToFileTime(const FILETIME*, LPFILETIME);
WINBASEAPI BOOL WINAPI SystemTimeToTzSpecificLocalTime(LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);
WINBASEAPI BOOL WINAPI DosDateTimeToFileTime(WORD, WORD, LPFILETIME);
WINBASEAPI LONG WINAPI CompareFileTime(const FILETIME*, const FILETIME*);
WINBASEAPI BOOL WINAPI QueryUnbiasedInterruptTime(PULONGLONG);
WINBASEAPI DWORD WINAPI GetVersion();
WINBASEAPI BOOL WINAPI GetVersionExW(LPOSVERSIONINFOW);
WINBASEAPI BOOL WINAPI VerifyVersionInfoW(LPOSVERSIONINFOEXW, DWORD, DWORDLONG);
WINBASEAPI DWORDLONG WINAPI VerSetConditionMask(DWORDLONG, DWORD, BYTE);

WINBASEAPI DWORD WINAPI GetEnvironmentVariableA(LPCSTR, LPSTR, DWORD);
WINBASEAPI BOOL WINAPI SetEnvironmentVariableA(LPCSTR, LPCSTR);
WINBASEAPI DWORD WINAPI ExpandEnvironmentStringsW(LPCWSTR, LPWSTR, DWORD);
WINBASEAPI DWORD WINAPI GetWindowsDirectoryW(LPWSTR, UINT);
WINBASEAPI UINT WINAPI GetSystemDirectoryA(LPSTR, UINT);
WINBASEAPI UINT WINAPI GetSystemDirectoryW(LPWSTR, UINT);
WINBASEAPI UINT WINAPI GetSystemWow64DirectoryW(LPWSTR, UINT);
WINBASEAPI DWORD WINAPI GetTempPathA(DWORD, LPSTR);
WINBASEAPI DWORD WINAPI GetTempFileNameA(LPCSTR, LPCSTR, UINT, LPSTR);
WINBASEAPI DWORD WINAPI GetLongPathNameW(LPCWSTR, LPWSTR, DWORD);
WINBASEAPI BOOL WINAPI GetVolumePathNameW(LPCWSTR, LPWSTR, DWORD);
WINBASEAPI BOOL WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS);
WINBASEAPI VOID WINAPI GetNativeSystemInfo(LPSYSTEM_INFO);
WINBASEAPI SIZE_T WINAPI GetLargePageMinimum();

// Misc helpers
WINBASEAPI PVOID WINAPI DecodePointer(PVOID);
WINBASEAPI PVOID WINAPI EncodePointer(PVOID);
WINBASEAPI int WINAPI MulDiv(int, int, int);
WINBASEAPI int WINAPI lstrcmpA(LPCSTR, LPCSTR);
WINBASEAPI int WINAPI lstrcmpW(LPCWSTR, LPCWSTR);
WINBASEAPI int WINAPI lstrcmpiW(LPCWSTR, LPCWSTR);
WINBASEAPI DWORD WINAPI GetThreadErrorMode();
WINBASEAPI BOOL WINAPI SetThreadErrorMode(DWORD, LPDWORD);
WINBASEAPI UINT WINAPI SetErrorMode(UINT);
WINBASEAPI BOOL WINAPI SetHandleInformation(HANDLE, DWORD, DWORD);
WINBASEAPI DWORD WINAPI SignalObjectAndWait(HANDLE, HANDLE, DWORD, BOOL);
WINBASEAPI BOOL WINAPI SwitchToThread();
WINBASEAPI BOOL WINAPI IsProcessorFeaturePresent(DWORD);
WINBASEAPI BOOL WINAPI GetThreadContext(HANDLE, PCONTEXT);
WINBASEAPI DWORD WINAPI FormatMessageA(DWORD, LPCVOID, DWORD, DWORD, LPSTR, DWORD, va_list*);
WINBASEAPI BOOL WINAPI WritePrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
WINBASEAPI UINT WINAPI GetPrivateProfileIntW(LPCWSTR, LPCWSTR, INT, LPCWSTR);
WINBASEAPI DWORD WINAPI GetPrivateProfileStringW(LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, DWORD, LPCWSTR);
WINBASEAPI HANDLE WINAPI OpenEventA(DWORD, BOOL, LPCSTR);
WINBASEAPI BOOL WINAPI SetStdHandle(DWORD, HANDLE);
WINBASEAPI BOOL WINAPI GetUserPreferredUILanguages(DWORD, PULONG, PZZWSTR, PULONG);
WINBASEAPI HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCSTR);
WINBASEAPI HANDLE WINAPI CreateEventExA(LPSECURITY_ATTRIBUTES, LPCSTR, DWORD, DWORD);
WINBASEAPI HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR);
WINBASEAPI HANDLE WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES, LONG, LONG, LPCSTR);
WINBASEAPI HANDLE WINAPI CreateSemaphoreExA(LPSECURITY_ATTRIBUTES, LONG, LONG, LPCSTR, DWORD, DWORD);

// (Process / thread / file / heap entries already declared in the kernel32 block above
//  are intentionally not re-declared here to avoid duplicate-declaration noise.)
}

// ===========================================================================
// 13) ADVAPI32 + CRYPT32 gap-fill types & function declarations.
//
// Added by Task ID GAP-ADVAPI32-CRYPT32 to support the 38 missing advapi32
// shims in shims/advapi32/Advapi32GapFill.cpp and the 24 missing crypt32
// shims in shims/pass-through/Crypt32GapFill.cpp. These mirror the desktop
// Win32 SDK so the gap-fill .cpp files compile cleanly under
// g++ -std=c++17 -fsyntax-only on Linux and against the real UWP SDK on
// Windows. Every definition is guarded so it is safe to include alongside
// the real Windows SDK.
// ===========================================================================
#ifndef _XWR_ADVAPI32_CRYPT32_GAP_TYPES
#define _XWR_ADVAPI32_CRYPT32_GAP_TYPES

// advapi32 scalar / handle aliases.
#ifndef ALG_ID_DEFINED
typedef ULONG ALG_ID, *PALG_ID;
#define ALG_ID_DEFINED
#endif
#ifndef REGHANDLE_DEFINED
typedef ULONGLONG REGHANDLE, *PREGHANDLE;
#define REGHANDLE_DEFINED
#endif
#ifndef SECURITY_INFORMATION_DEFINED
typedef DWORD SECURITY_INFORMATION, *PSECURITY_INFORMATION;
#define SECURITY_INFORMATION_DEFINED
#endif

// SID / ACL / security descriptor — opaque at the stub layer.
#ifndef PSID_DEFINED
typedef void *PSID, *PACL;
#define PSID_DEFINED
#endif
#ifndef PSECURITY_DESCRIPTOR_DEFINED
typedef void *PSECURITY_DESCRIPTOR;
#define PSECURITY_DESCRIPTOR_DEFINED
#endif

// LPCGUID / PCGUID — used by EventRegister / EventWriteTransfer.
#ifndef LPCGUID_DEFINED
typedef const GUID *LPCGUID, *PCGUID;
#define LPCGUID_DEFINED
#endif

// Service config (QueryServiceConfigW).
typedef struct _QUERY_SERVICE_CONFIGW {
    DWORD  dwServiceType;
    DWORD  dwStartType;
    DWORD  dwErrorControl;
    LPWSTR lpBinaryPathName;
    LPWSTR lpLoadOrderGroup;
    DWORD  dwTagId;
    LPWSTR lpDependencies;
    LPWSTR lpServiceStartName;
    LPWSTR lpDisplayName;
} QUERY_SERVICE_CONFIGW, *LPQUERY_SERVICE_CONFIGW;

// ETW (EventRegister / EventWriteTransfer / EventSetInformation).
typedef struct _EVENT_DESCRIPTOR {
    USHORT    Id;
    UCHAR     Version;
    UCHAR     Channel;
    UCHAR     Level;
    UCHAR     Opcode;
    USHORT    Task;
    ULONGLONG Keyword;
} EVENT_DESCRIPTOR, *PEVENT_DESCRIPTOR;
typedef const EVENT_DESCRIPTOR *PCEVENT_DESCRIPTOR;

typedef struct _EVENT_DATA_DESCRIPTOR {
    ULONGLONG Ptr;
    ULONG     Size;
    ULONG     Reserved;
} EVENT_DATA_DESCRIPTOR, *PEVENT_DATA_DESCRIPTOR;

typedef struct _EVENT_FILTER_DESCRIPTOR EVENT_FILTER_DESCRIPTOR, *PEVENT_FILTER_DESCRIPTOR;

typedef void (WINAPI *PENABLECALLBACK)(
    LPCGUID SourceId, ULONG IsEnabled,
    UCHAR Level, ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeyword, PEVENT_FILTER_DESCRIPTOR FilterData,
    PVOID CallbackContext);

typedef enum _EVENT_INFO_CLASS {
    EventProviderBinaryTrackInfo = 0,
    EventProviderSetTraits = 1,
    EventProviderUseDescriptorType = 2
} EVENT_INFO_CLASS;

// SE_OBJECT_TYPE — used by SetNamedSecurityInfoW.
typedef enum _SE_OBJECT_TYPE {
    SE_UNKNOWN_OBJECT_TYPE = 0,
    SE_FILE_OBJECT,
    SE_SERVICE,
    SE_PRINTER,
    SE_REGISTRY_KEY,
    SE_LMSHARE,
    SE_KERNEL_OBJECT,
    SE_WINDOW_OBJECT,
    SE_DS_OBJECT,
    SE_DS_OBJECT_ALL,
    SE_PROVIDER_DEFINED_OBJECT,
    SE_WMIGUID_OBJECT,
    SE_REGISTRY_WOW64_32KEY
} SE_OBJECT_TYPE;

// WELL_KNOWN_SID_TYPE — subset, used by CreateWellKnownSid.
typedef enum _WELL_KNOWN_SID_TYPE {
    WinNullSid = 0,
    WinWorldSid = 1,
    WinLocalSid = 2,
    WinCreatorOwnerSid = 3,
    WinCreatorGroupSid = 4,
    WinCreatorOwnerServerSid = 5,
    WinCreatorGroupServerSid = 6,
    WinNtAuthoritySid = 7,
    WinDialupSid = 8,
    WinNetworkSid = 9,
    WinBatchSid = 10,
    WinInteractiveSid = 11,
    WinServiceSid = 12,
    WinAnonymousSid = 13,
    WinProxySid = 14,
    WinEnterpriseControllersSid = 15,
    WinSelfSid = 16,
    WinAuthenticatedUserSid = 17,
    WinRestrictedCodeSid = 18,
    WinTerminalServerSid = 19,
    WinRemoteLogonIdSid = 20,
    WinLogonIdsSid = 21,
    WinLocalSystemSid = 22,
    WinLocalServiceSid = 23,
    WinNetworkServiceSid = 24,
    WinBuiltinDomainSid = 25,
    WinBuiltinAdministratorsSid = 26,
    WinBuiltinUsersSid = 27,
    WinBuiltinGuestsSid = 28
} WELL_KNOWN_SID_TYPE;

// TRUSTEE_W / EXPLICIT_ACCESS_W — opaque to the shim layer.
struct _TRUSTEE_W;
typedef struct _TRUSTEE_W TRUSTEE_W, *PTRUSTEE_W;
struct _EXPLICIT_ACCESS_W;
typedef struct _EXPLICIT_ACCESS_W EXPLICIT_ACCESS_W, *PEXPLICIT_ACCESS_W;

// Registry notify flags (RegNotifyChangeKeyValue) — subset.
#ifndef REG_NOTIFY_CHANGE_NAME
#define REG_NOTIFY_CHANGE_NAME       0x00000001L
#endif
#ifndef REG_NOTIFY_CHANGE_ATTRIBUTES
#define REG_NOTIFY_CHANGE_ATTRIBUTES 0x00000002L
#endif
#ifndef REG_NOTIFY_CHANGE_LAST_SET
#define REG_NOTIFY_CHANGE_LAST_SET   0x00000004L
#endif
#ifndef REG_NOTIFY_CHANGE_SECURITY
#define REG_NOTIFY_CHANGE_SECURITY   0x00000008L
#endif

// RRF_ flags used by RegGetValueW.
#ifndef RRF_RT_REG_NONE
#define RRF_RT_REG_NONE      0x00000001
#endif
#ifndef RRF_RT_REG_SZ
#define RRF_RT_REG_SZ        0x00000002
#endif
#ifndef RRF_RT_REG_EXPAND_SZ
#define RRF_RT_REG_EXPAND_SZ 0x00000004
#endif
#ifndef RRF_RT_REG_BINARY
#define RRF_RT_REG_BINARY    0x00000008
#endif
#ifndef RRF_RT_REG_DWORD
#define RRF_RT_REG_DWORD     0x00000010
#endif
#ifndef RRF_RT_REG_MULTI_SZ
#define RRF_RT_REG_MULTI_SZ  0x00000020
#endif
#ifndef RRF_RT_REG_QWORD
#define RRF_RT_REG_QWORD     0x00000040
#endif
#ifndef RRF_NOEXPAND
#define RRF_NOEXPAND         0x10000000
#endif
#ifndef RRF_ZEROONFAILURE
#define RRF_ZEROONFAILURE    0x20000000
#endif
#ifndef RRF_SUBKEY_WOW6464KEY
#define RRF_SUBKEY_WOW6464KEY 0x00010000
#endif
#ifndef RRF_SUBKEY_WOW6432KEY
#define RRF_SUBKEY_WOW6432KEY 0x00020000
#endif

// advapi32 gap-fill function declarations (signatures match the real SDK).
extern "C" {
// Registry pass-throughs.
WINADVAPI LONG  WINAPI RegGetValueW(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);
WINADVAPI LONG  WINAPI RegQueryInfoKeyW(HKEY, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME);
WINADVAPI LONG  WINAPI RegCreateKeyTransactedW(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD, HANDLE, PVOID);
WINADVAPI LONG  WINAPI RegNotifyChangeKeyValue(HKEY, BOOL, DWORD, HANDLE, BOOL);

// Event log.
WINADVAPI HANDLE WINAPI RegisterEventSourceW(LPCWSTR, LPCWSTR);
WINADVAPI BOOL   WINAPI DeregisterEventSource(HANDLE);
WINADVAPI BOOL   WINAPI ReportEventW(HANDLE, WORD, WORD, DWORD, PSID, WORD, DWORD, LPCWSTR*, LPVOID);

// ETW.
WINADVAPI ULONG  WINAPI EventRegister(LPCGUID, PENABLECALLBACK, PVOID, PREGHANDLE);
WINADVAPI ULONG  WINAPI EventUnregister(REGHANDLE);
WINADVAPI ULONG  WINAPI EventWriteTransfer(REGHANDLE, PCEVENT_DESCRIPTOR, LPCGUID, ULONG, PEVENT_DATA_DESCRIPTOR);
WINADVAPI ULONG  WINAPI EventSetInformation(REGHANDLE, EVENT_INFO_CLASS, PVOID, ULONG);

// Crypto (advapi32 surface).
WINADVAPI BOOL WINAPI CryptCreateHash(HCRYPTPROV, ALG_ID, HCRYPTKEY, DWORD, HCRYPTHASH*);
WINADVAPI BOOL WINAPI CryptDecrypt(HCRYPTKEY, HCRYPTHASH, BOOL, DWORD, BYTE*, DWORD*);
WINADVAPI BOOL WINAPI CryptDestroyHash(HCRYPTHASH);
WINADVAPI BOOL WINAPI CryptDestroyKey(HCRYPTKEY);
WINADVAPI BOOL WINAPI CryptEnumProvidersW(DWORD, DWORD*, DWORD, DWORD*, LPWSTR, DWORD*);
WINADVAPI BOOL WINAPI CryptExportKey(HCRYPTKEY, HCRYPTKEY, DWORD, DWORD, BYTE*, DWORD*);
WINADVAPI BOOL WINAPI CryptGetHashParam(HCRYPTHASH, DWORD, BYTE*, DWORD*, DWORD);
WINADVAPI BOOL WINAPI CryptGetProvParam(HCRYPTPROV, DWORD, BYTE*, DWORD*, DWORD);
WINADVAPI BOOL WINAPI CryptGetUserKey(HCRYPTPROV, DWORD, HCRYPTKEY*);
WINADVAPI BOOL WINAPI CryptHashData(HCRYPTHASH, const BYTE*, DWORD, DWORD);
WINADVAPI BOOL WINAPI CryptSetHashParam(HCRYPTHASH, DWORD, const BYTE*, DWORD);
WINADVAPI BOOL WINAPI CryptSignHashW(HCRYPTHASH, DWORD, LPCWSTR, DWORD, BYTE*, DWORD*);

// Service control.
WINADVAPI BOOL WINAPI ChangeServiceConfigW(SC_HANDLE, DWORD, DWORD, DWORD, LPCWSTR, LPCWSTR, DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR);
WINADVAPI BOOL WINAPI QueryServiceConfigW(SC_HANDLE, LPQUERY_SERVICE_CONFIGW, DWORD, LPDWORD);

// SID / security.
WINADVAPI BOOL WINAPI AllocateLocallyUniqueId(PLUID);
WINADVAPI BOOL WINAPI CheckTokenMembership(HANDLE, PSID, PBOOL);
WINADVAPI BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(LPCWSTR, DWORD, PSECURITY_DESCRIPTOR*, PULONG);
WINADVAPI BOOL WINAPI CreateWellKnownSid(WELL_KNOWN_SID_TYPE, PSID, PSID, DWORD*);
WINADVAPI DWORD WINAPI SetEntriesInAclA(ULONG, void*, PACL, PACL*);
WINADVAPI DWORD WINAPI SetEntriesInAclW(ULONG, PEXPLICIT_ACCESS_W, PACL, PACL*);
WINADVAPI DWORD WINAPI SetNamedSecurityInfoW(LPWSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION, PSID, PSID, PACL, PACL);
WINADVAPI BOOL WINAPI SetSecurityDescriptorGroup(PSECURITY_DESCRIPTOR, PSID, BOOL);
WINADVAPI BOOL WINAPI SetSecurityDescriptorOwner(PSECURITY_DESCRIPTOR, PSID, BOOL);

// Misc.
WINADVAPI BOOL WINAPI DecryptFileW(LPCWSTR, DWORD);
WINADVAPI BOOL WINAPI InitiateSystemShutdownExW(LPWSTR, LPWSTR, DWORD, BOOL, BOOL, DWORD);
WINADVAPI BOOL WINAPI LookupPrivilegeValueW(LPCWSTR, LPCWSTR, PLUID);
WINADVAPI BOOLEAN WINAPI SystemFunction036(PVOID, ULONG);
}

// ---------------------------------------------------------------------------
// crypt32 types
// ---------------------------------------------------------------------------
// All certificate structures are opaque to the stub layer. The shim returns
// FALSE / 0 for every certificate entry so we never dereference them.
struct _CERT_CONTEXT;
typedef struct _CERT_CONTEXT CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;

struct _CERT_CHAIN_CONTEXT;
typedef struct _CERT_CHAIN_CONTEXT CERT_CHAIN_CONTEXT, *PCERT_CHAIN_CONTEXT;
typedef const CERT_CHAIN_CONTEXT *PCCERT_CHAIN_CONTEXT;

typedef void *HCERTSTORE;
typedef void *HCERTCHAINENGINE;

// CRYPT_DATA_BLOB / CRYPT_INTEGER_BLOB — used by PFXExportCertStoreEx.
typedef struct _CRYPTOAPI_BLOB {
    DWORD cbData;
    BYTE *pbData;
} CRYPT_INTEGER_BLOB, CRYPT_DATA_BLOB, *PCRYPT_INTEGER_BLOB, *PCRYPT_DATA_BLOB;

// crypt32 gap-fill function declarations.
extern "C" {
WINADVAPI BOOL              WINAPI CertAddCertificateContextToStore(HCERTSTORE, PCCERT_CONTEXT, DWORD, PCCERT_CONTEXT*);
WINADVAPI BOOL              WINAPI CertCloseStore(HCERTSTORE, DWORD);
WINADVAPI HCERTCHAINENGINE WINAPI CertCreateCertificateChainEngine(const void*);
WINADVAPI PCCERT_CONTEXT    WINAPI CertCreateContext(DWORD, DWORD, const BYTE*, DWORD, DWORD, void*);
WINADVAPI PCCERT_CONTEXT    WINAPI CertDuplicateCertificateContext(PCCERT_CONTEXT);
WINADVAPI PCCERT_CONTEXT    WINAPI CertEnumCertificatesInStore(HCERTSTORE, PCCERT_CONTEXT);
WINADVAPI PCCERT_CONTEXT    WINAPI CertFindCertificateInStore(HCERTSTORE, DWORD, DWORD, DWORD, const void*, PCCERT_CONTEXT);
WINADVAPI void              WINAPI CertFreeCertificateChain(PCCERT_CHAIN_CONTEXT);
WINADVAPI void              WINAPI CertFreeCertificateChainEngine(HCERTCHAINENGINE);
WINADVAPI BOOL              WINAPI CertFreeCertificateContext(PCCERT_CONTEXT);
WINADVAPI BOOL              WINAPI CertGetCertificateChain(HCERTCHAINENGINE, PCCERT_CONTEXT, LPFILETIME, HCERTSTORE, const void*, DWORD, void*, PCCERT_CHAIN_CONTEXT*);
WINADVAPI BOOL              WINAPI CertGetCertificateContextProperty(PCCERT_CONTEXT, DWORD, void*, DWORD*);
WINADVAPI BOOL              WINAPI CertGetEnhancedKeyUsage(PCCERT_CONTEXT, DWORD, void*, DWORD*);
WINADVAPI BOOL              WINAPI CertGetIntendedKeyUsage(PCCERT_CONTEXT, BYTE*, DWORD);
WINADVAPI DWORD             WINAPI CertGetNameStringA(PCCERT_CONTEXT, DWORD, DWORD, void*, LPSTR, DWORD);
WINADVAPI DWORD             WINAPI CertGetNameStringW(PCCERT_CONTEXT, DWORD, DWORD, void*, LPWSTR, DWORD);
WINADVAPI HCERTSTORE        WINAPI CertOpenStore(LPCSTR, DWORD, HCRYPTPROV, DWORD, const void*);
WINADVAPI HCERTSTORE        WINAPI CertOpenSystemStoreA(HCRYPTPROV, LPCSTR);
WINADVAPI HCERTSTORE        WINAPI CertOpenSystemStoreW(HCRYPTPROV, LPCWSTR);
WINADVAPI BOOL              WINAPI CertSetCertificateContextProperty(PCCERT_CONTEXT, DWORD, DWORD, const void*);
WINADVAPI BOOL              WINAPI CertVerifyCertificateChainPolicy(LPCSTR, PCCERT_CHAIN_CONTEXT, void*, void*);
WINADVAPI BOOL              WINAPI CryptAcquireCertificatePrivateKey(PCCERT_CONTEXT, DWORD, void*, void*, DWORD*, BOOL*);
WINADVAPI BOOL              WINAPI CryptHashPublicKeyInfo(HCRYPTPROV, ALG_ID, DWORD, DWORD, const void*, BYTE*, DWORD*);
WINADVAPI BOOL              WINAPI PFXExportCertStoreEx(HCERTSTORE, PCRYPT_DATA_BLOB, LPCWSTR, DWORD, void*);
}

#endif  // _XWR_ADVAPI32_CRYPT32_GAP_TYPES

// ===========================================================================
// 14) MISC SYSTEM DLL gap-fill types & function declarations.
//
// Added by Task ID GAP-MISC-SYSTEM to support the ~200 missing shim
// functions in shims/MiscSystemGapFill.cpp. Covers 33 DLLs (winmm, ole32,
// oleaut32, gdi32, shell32, shlwapi, version, setupapi, dsound, d3d12,
// mfplat, mf, imm32, winhttp, wininet, wintrust, ntdll, dwmapi, cfgmgr32,
// ncrypt, msdmo, propsys, msi, dbghelp, d3dcompiler_47, dxgi, cabinet,
// xaudio2_9redist, mmdevapi, rpcrt4, uxtheme, uiautomationcore, xinput1_4).
// Every definition is guarded so it is safe to include alongside the real
// Windows SDK.
// ===========================================================================
#ifndef _XWR_MISC_SYSTEM_GAP_TYPES
#define _XWR_MISC_SYSTEM_GAP_TYPES

// ----------------------------------------------------------------------------
// NT / kernel types (ntdll pass-throughs)
// ----------------------------------------------------------------------------
#ifndef NTSTATUS
typedef LONG NTSTATUS, *PNTSTATUS;
#endif
#ifndef NTSYSAPI
#define NTSYSAPI
#endif
#ifndef PCWSTR_ALREADY
typedef const wchar_t *PCWSTR;
#define PCWSTR_ALREADY
#endif

// IO_STATUS_BLOCK / PIO_APC_ROUTINE / PIO_STATUS_BLOCK
typedef struct _IO_STATUS_BLOCK {
    union {
        LONG Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
typedef VOID (NTAPI *PIO_APC_ROUTINE)(PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG Reserved);

// UNICODE_STRING
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

// LIST_ENTRY — used inside RTL_CRITICAL_SECTION_DEBUG below.
#ifndef _XWR_LIST_ENTRY_DEFINED
typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;
#define _XWR_LIST_ENTRY_DEFINED
#endif

// PDWORD64 / PSIZE_T — commonly missing from the UWP stub subset.
#ifndef PDWORD64_DEFINED
typedef DWORD64 *PDWORD64;
#define PDWORD64_DEFINED
#endif
#ifndef PSIZE_T_DEFINED
typedef SIZE_T *PSIZE_T;
#define PSIZE_T_DEFINED
#endif

// RTL_CRITICAL_SECTION (real layout — used by RtlIsCriticalSectionLockedByThread)
typedef struct _RTL_CRITICAL_SECTION_DEBUG {
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION* CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Flags;
    DWORD CreatorBackTraceIndexHigh;
    WORD  SpareWORD;
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG;

typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

// RTL_RUN_ONCE / init-fn
typedef struct _RTL_RUN_ONCE {
    PVOID Ptr;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;
typedef BOOLEAN (NTAPI *PRTL_RUN_ONCE_INIT_FN)(PRTL_RUN_ONCE, PVOID, PVOID*);

// RUNTIME_FUNCTION / PRUNTIME_FUNCTION — alias to existing IMAGE_RUNTIME_FUNCTION_ENTRY.
typedef IMAGE_RUNTIME_FUNCTION_ENTRY RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

// UNWIND_HISTORY_TABLE and KNONVOLATILE_CONTEXT_POINTERS are declared earlier
// (next to the other exception/RTL types) so the Rtl* function declarations
// below can reference them.

// Real-context typedefs (real Windows SDK ships CONTEXT in winnt.h; the stub
// uses an opaque void* so the shim only passes it along).
#ifndef XWR_CONTEXT_DEFINED
typedef void *PCONTEXT_FULL;
#define XWR_CONTEXT_DEFINED
#endif

// EXCEPTION_HISTORY_TABLE — needed by RtlUnwindEx's existing declaration.
struct _EXCEPTION_HISTORY_TABLE;
typedef struct _EXCEPTION_HISTORY_TABLE EXCEPTION_HISTORY_TABLE, *PEXCEPTION_HISTORY_TABLE;

// ----------------------------------------------------------------------------
// winmm types (mixer* and waveIn* gap-fill)
// ----------------------------------------------------------------------------
typedef DWORD HWAVEIN;
typedef HWAVEIN *LPHWAVEIN;
typedef DWORD HMIXEROBJ;
typedef HMIXER *LPHMIXER;

// Mixer / waveIn cap structures — opaque stubs (we never read fields).
struct tagMIXERCAPSW;
typedef struct tagMIXERCAPSW MIXERCAPSW, *PMIXERCAPSW, *LPMIXERCAPSW;
struct tagMIXERLINEW;
typedef struct tagMIXERLINEW MIXERLINEW, *PMIXERLINEW, *LPMIXERLINEW;
struct tagMIXERCONTROLDETAILS;
typedef struct tagMIXERCONTROLDETAILS MIXERCONTROLDETAILS, *PMIXERCONTROLDETAILS, *LPMIXERCONTROLDETAILS;
struct tagMIXERLINECONTROLSW;
typedef struct tagMIXERLINECONTROLSW MIXERLINECONTROLSW, *PMIXERLINECONTROLSW, *LPMIXERLINECONTROLSW;
struct tagWAVEINCAPSW;
typedef struct tagWAVEINCAPSW WAVEINCAPSW, *PWAVEINCAPSW, *LPWAVEINCAPSW;
struct tagMMTIME;
typedef struct tagMMTIME MMTIME, *PMMTIME, *LPMMTIME;
typedef UINT *LPUINT;

// ----------------------------------------------------------------------------
// OLE / PROPVARIANT / STGMEDIUM
// ----------------------------------------------------------------------------
#ifndef LPCOLESTR
typedef const OLECHAR *LPCOLESTR;
#endif
#ifndef LPOLESTR
typedef OLECHAR *LPOLESTR;
#endif

// PROPVARIANT — same union layout as VARIANT but with VARTYPE vt and a richer
// value union. For the shim layer we only need an opaque-ish struct (the real
// PropVariantClear/Copy are pass-throughs).
typedef struct tagPROPVARIANT {
    VARTYPE vt;
    WORD    wReserved1;
    WORD    wReserved2;
    WORD    wReserved3;
    union {
        LONG        lVal;
        BYTE        bVal;
        SHORT       iVal;
        FLOAT       fltVal;
        DOUBLE      dblVal;
        VARIANT_BOOL boolVal;
        SCODE       scode;
        CY          cyVal;
        DATE        date;
        BSTR        bstrVal;
        IUnknown   *punkVal;
        IDispatch  *pdispVal;
        SAFEARRAY  *parray;
        void       *byref;
        CHAR        cVal;
        USHORT      uiVal;
        ULONG       ulVal;
        INT         intVal;
        UINT        uintVal;
        GUID       *puuid;
    };
} PROPVARIANT, *LPPROPVARIANT;

// STGMEDIUM — used by ReleaseStgMedium / OleSetClipboard etc.
typedef struct tagSTGMEDIUM {
    DWORD    tymed;
    union {
        HBITMAP     hBitmap;
        HMETAFILE   hMetaFilePict;
        HENHMETAFILE hEnhMetaFile;
        HGLOBAL     hGlobal;
        LPOLESTR    lpszFileName;
        IStream    *pstm;
        IStorage   *pstg;
        PVOID       pUnkForRelease;
    } DUMMYUNIONNAME;
    IUnknown *pUnkForRelease;
} STGMEDIUM, *LPSTGMEDIUM;

// SOLE_AUTHENTICATION_SERVICE — used by CoInitializeSecurity.
typedef struct _SOLE_AUTHENTICATION_SERVICE {
    DWORD   dwAuthnSvc;
    DWORD   dwAuthzSvc;
    LPWSTR  pPrincipalName;
    HRESULT hr;
} SOLE_AUTHENTICATION_SERVICE;

#ifndef RPC_AUTH_IDENTITY_HANDLE
typedef void *RPC_AUTH_IDENTITY_HANDLE;
#endif

// ----------------------------------------------------------------------------
// gdi32 — PIXELFORMATDESCRIPTOR (for ChoosePixelFormat / SetPixelFormat).
// ----------------------------------------------------------------------------
typedef struct tagPIXELFORMATDESCRIPTOR {
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

// ----------------------------------------------------------------------------
// setupapi / cfgmgr32 types
// ----------------------------------------------------------------------------
typedef DWORD CONFIGRET;
typedef DWORD DEVINST, *PDEVINST;
typedef DWORD DEVNODE, *PDEVNODE;

#ifndef CR_SUCCESS
#define CR_SUCCESS 0
#endif
#ifndef CR_FAILURE
#define CR_FAILURE 1
#endif
#ifndef CR_NO_SUCH_DEVNODE
#define CR_NO_SUCH_DEVNODE 3
#endif

// SP_DEVINFO_DATA / SP_DEVICE_INTERFACE_DATA — opaque to the shim layer.
struct _SP_DEVINFO_DATA;
typedef struct _SP_DEVINFO_DATA SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;
struct _SP_DEVICE_INTERFACE_DATA;
typedef struct _SP_DEVICE_INTERFACE_DATA SP_DEVICE_INTERFACE_DATA, *PSP_DEVICE_INTERFACE_DATA;
struct _SP_DEVICE_INTERFACE_DETAIL_DATA_W;
typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA_W SP_DEVICE_INTERFACE_DETAIL_DATA_W, *PSP_DEVICE_INTERFACE_DETAIL_DATA_W;
struct _SP_DEVICE_INTERFACE_DETAIL_DATA_A;
typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA_A SP_DEVICE_INTERFACE_DETAIL_DATA_A, *PSP_DEVICE_INTERFACE_DETAIL_DATA_A;
struct _SP_DRVINFO_DATA_W;
typedef struct _SP_DRVINFO_DATA_W SP_DRVINFO_DATA_W, *PSP_DRVINFO_DATA_W;
struct _SP_DRVINFO_DATA_A;
typedef struct _SP_DRVINFO_DATA_A SP_DRVINFO_DATA_A, *PSP_DRVINFO_DATA_A;

// DEVPROPKEY / DEVPROPTYPE
typedef struct _DEVPROPKEY {
    GUID    fmtid;
    DWORD   pid;
} DEVPROPKEY, *PDEVPROPKEY;
typedef ULONG DEVPROPTYPE, *PDEVPROPTYPE;

// ----------------------------------------------------------------------------
// dwmapi — DWM_TIMING_INFO (opaque).
// ----------------------------------------------------------------------------
struct _DWM_TIMING_INFO;
typedef struct _DWM_TIMING_INFO DWM_TIMING_INFO, *PDWM_TIMING_INFO;

// ----------------------------------------------------------------------------
// ncrypt / msdmo / propsys types
// ----------------------------------------------------------------------------
#ifndef NCRYPT_HANDLE_DEFINED
typedef ULONG_PTR NCRYPT_HANDLE;
typedef ULONG_PTR NCRYPT_PROV_HANDLE;
#define NCRYPT_HANDLE_DEFINED
#endif
typedef LONG SECURITY_STATUS;

// DMO_MEDIA_TYPE — opaque to the shim layer.
struct _DMO_MEDIA_TYPE;
typedef struct _DMO_MEDIA_TYPE DMO_MEDIA_TYPE, *PDMO_MEDIA_TYPE;

// ----------------------------------------------------------------------------
// winhttp / wininet types
// ----------------------------------------------------------------------------
typedef WORD INTERNET_PORT;
#ifndef INTERNET_SCHEME
typedef int INTERNET_SCHEME;
#endif

typedef struct _URL_COMPONENTS {
    DWORD    dwStructSize;
    LPWSTR   lpszScheme;
    DWORD    dwSchemeLength;
    INTERNET_SCHEME nScheme;
    LPWSTR   lpszHostName;
    DWORD    dwHostNameLength;
    INTERNET_PORT nPort;
    LPWSTR   lpszUserName;
    DWORD    dwUserNameLength;
    LPWSTR   lpszPassword;
    DWORD    dwPasswordLength;
    LPWSTR   lpszUrlPath;
    DWORD    dwUrlPathLength;
    LPWSTR   lpszExtraInfo;
    DWORD    dwExtraInfoLength;
} URL_COMPONENTS, *LPURL_COMPONENTS;

typedef struct _WINHTTP_PROXY_INFO {
    DWORD    dwAccessType;
    LPWSTR   lpszProxy;
    LPWSTR   lpszProxyBypass;
} WINHTTP_PROXY_INFO, *LPWINHTTP_PROXY_INFO;

typedef struct _WINHTTP_CURRENT_USER_IE_PROXY_CONFIG {
    BOOL     fAutoDetect;
    LPWSTR   lpszAutoConfigUrl;
    LPWSTR   lpszProxy;
    LPWSTR   lpszProxyBypass;
} WINHTTP_CURRENT_USER_IE_PROXY_CONFIG, *LPWINHTTP_CURRENT_USER_IE_PROXY_CONFIG;

// ----------------------------------------------------------------------------
// wintrust — opaque provider structs.
// ----------------------------------------------------------------------------
typedef DWORD HCATADMIN;
struct _CRYPT_PROVIDER_DATA;
typedef struct _CRYPT_PROVIDER_DATA CRYPT_PROVIDER_DATA, *PCRYPT_PROVIDER_DATA;
struct _CRYPT_PROVIDER_SGNR;
typedef struct _CRYPT_PROVIDER_SGNR CRYPT_PROVIDER_SGNR, *PCRYPT_PROVIDER_SGNR;
struct _CRYPT_PROVIDER_CERT;
typedef struct _CRYPT_PROVIDER_CERT CRYPT_PROVIDER_CERT, *PCRYPT_PROVIDER_CERT;

// ----------------------------------------------------------------------------
// imm32 types
// ----------------------------------------------------------------------------
typedef struct tagCANDIDATEFORM {
    DWORD dwIndex;
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} CANDIDATEFORM, *PCANDIDATEFORM, *LPCANDIDATEFORM;

typedef struct tagCOMPOSITIONFORM {
    DWORD dwStyle;
    POINT ptCurrentPos;
    RECT  rcArea;
} COMPOSITIONFORM, *PCOMPOSITIONFORM, *LPCOMPOSITIONFORM;

// ----------------------------------------------------------------------------
// mfplat / mf types — IMFAsyncCallback forward-declare.
// ----------------------------------------------------------------------------
struct IMFAsyncCallback;
typedef struct IMFAsyncCallback IMFAsyncCallback;

// ----------------------------------------------------------------------------
// dbghelp types
// ----------------------------------------------------------------------------
// ADDRESS64 / ADDRESS_MODE / KDUMP64 — defined up-front so STACKFRAME64 can use them.
#ifndef ADDRESS_MODE_DEFINED
typedef USHORT ADDRESS_MODE;
#define ADDRESS_MODE_DEFINED
#endif
#ifndef ADDRESS64_DEFINED
typedef struct _tagADDRESS64 {
    DWORD64      Offset;
    USHORT       Segment;
    ADDRESS_MODE Mode;
} ADDRESS64, *LPADDRESS64;
#define ADDRESS64_DEFINED
#endif
#ifndef KDUMP64_DEFINED
typedef struct _KDUMP64 {
    DWORD64 Thread;
    DWORD64 ThCallbackStack;
    DWORD64 NextCallback;
    DWORD64 FramePointer;
    DWORD64 KiCallUserMode;
    DWORD64 KeUserCallbackDispatcher;
    DWORD64 SystemRangeStart;
    DWORD64 KiUserExceptionDispatcher;
    DWORD64 StackBase;
    DWORD64 StackLimit;
    DWORD64 Reserved[5];
} KDUMP64, *PKDUMP64;
#define KDUMP64_DEFINED
#endif

typedef struct _STACKFRAME64 {
    ADDRESS64 AddrPC;
    ADDRESS64 AddrReturn;
    ADDRESS64 AddrFrame;
    ADDRESS64 AddrStack;
    ADDRESS64 AddrBStore;
    PVOID     FuncTableEntry;
    DWORD64   Params[4];
    BOOL      Far;
    BOOL      Virtual;
    DWORD     Reserved[3];
    KDUMP64   KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

// dbghelp function-pointer typedefs.
typedef BOOL (WINAPI *PREAD_PROCESS_MEMORY_ROUTINE64)(HANDLE, DWORD64, PVOID, SIZE_T, PSIZE_T);
typedef PVOID (WINAPI *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD64 (WINAPI *PGET_MODULE_BASE_ROUTINE64)(HANDLE, DWORD64);
typedef DWORD (WINAPI *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE, HANDLE, LPADDRESS64);

// ----------------------------------------------------------------------------
// d3d / dxgi / d3dcompiler types
// ----------------------------------------------------------------------------
struct ID3DBlob;
typedef struct ID3DBlob ID3DBlob, *LPD3DBLOB;

// ----------------------------------------------------------------------------
// rpcrt4 / uiautomationcore types
// ----------------------------------------------------------------------------
typedef GUID UUID;
typedef LONG RPC_STATUS;
typedef DWORD PROPERTYID;

// ----------------------------------------------------------------------------
// function declarations
// ----------------------------------------------------------------------------
extern "C" {

// winmm gap-fill (stubs — declared so the shim .cpp compiles).
WINMMAPI UINT WINAPI mixerClose(HMIXER);
WINMMAPI UINT WINAPI mixerGetControlDetailsW(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD);
WINMMAPI UINT WINAPI mixerGetDevCapsW(UINT, LPMIXERCAPSW, UINT);
WINMMAPI UINT WINAPI mixerGetID(HMIXEROBJ, LPUINT, DWORD);
WINMMAPI UINT WINAPI mixerGetLineControlsW(HMIXEROBJ, LPMIXERLINECONTROLSW, DWORD);
WINMMAPI UINT WINAPI mixerGetLineInfoW(HMIXEROBJ, LPMIXERLINEW, DWORD);
WINMMAPI UINT WINAPI mixerGetNumDevs();
WINMMAPI UINT WINAPI mixerOpen(LPHMIXER, UINT, DWORD_PTR, DWORD_PTR, DWORD);
WINMMAPI UINT WINAPI mixerSetControlDetails(HMIXEROBJ, LPMIXERCONTROLDETAILS, DWORD);
WINMMAPI UINT WINAPI waveInAddBuffer(HWAVEIN, LPWAVEHDR, UINT);
WINMMAPI UINT WINAPI waveInClose(HWAVEIN);
WINMMAPI UINT WINAPI waveInGetDevCapsW(UINT, LPWAVEINCAPSW, UINT);
WINMMAPI UINT WINAPI waveInGetErrorTextW(UINT, LPWSTR, UINT);
WINMMAPI UINT WINAPI waveInGetID(HWAVEIN, LPUINT);
WINMMAPI UINT WINAPI waveInGetNumDevs();
WINMMAPI UINT WINAPI waveInGetPosition(HWAVEIN, LPMMTIME, UINT);
WINMMAPI UINT WINAPI waveInMessage(HWAVEIN, UINT, DWORD_PTR, DWORD_PTR);
WINMMAPI UINT WINAPI waveInOpen(LPHWAVEIN, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
WINMMAPI UINT WINAPI waveInPrepareHeader(HWAVEIN, LPWAVEHDR, UINT);
WINMMAPI UINT WINAPI waveInReset(HWAVEIN);
WINMMAPI UINT WINAPI waveInStart(HWAVEIN);
WINMMAPI UINT WINAPI waveInStop(HWAVEIN);
WINMMAPI UINT WINAPI waveInUnprepareHeader(HWAVEIN, LPWAVEHDR, UINT);
WINMMAPI UINT WINAPI waveOutGetErrorTextW(UINT, LPWSTR, UINT);
WINMMAPI UINT WINAPI waveOutGetID(HWAVEOUT, LPUINT);
WINMMAPI UINT WINAPI waveOutGetPosition(HWAVEOUT, LPMMTIME, UINT);
WINMMAPI UINT WINAPI waveOutMessage(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR);

// ole32 gap-fill
WINOLEAPI HRESULT WINAPI CLSIDFromProgID(LPCOLESTR, LPCLSID);
WINOLEAPI HRESULT WINAPI CoGetInterfaceAndReleaseStream(IStream*, REFIID, void**);
WINOLEAPI HRESULT WINAPI CoInitializeSecurity(PSECURITY_DESCRIPTOR, LONG, SOLE_AUTHENTICATION_SERVICE*, void*, DWORD, DWORD, void*, DWORD, void*);
WINOLEAPI HRESULT WINAPI CoMarshalInterThreadInterfaceInStream(REFIID, IUnknown*, IStream**);
WINOLEAPI HRESULT WINAPI CoSetProxyBlanket(IUnknown*, DWORD, DWORD, OLECHAR*, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE, DWORD);
WINOLEAPI HRESULT WINAPI IIDFromString(LPCOLESTR, LPIID);
WINOLEAPI HRESULT WINAPI PropVariantClear(PROPVARIANT*);
WINOLEAPI HRESULT WINAPI PropVariantCopy(PROPVARIANT*, PROPVARIANT*);
WINOLEAPI void    WINAPI ReleaseStgMedium(LPSTGMEDIUM);
WINOLEAPI int     WINAPI StringFromGUID2(REFGUID, LPOLESTR, int);

// gdi32 gap-fill
WINGDIAPI int  WINAPI ChoosePixelFormat(HDC, const PIXELFORMATDESCRIPTOR*);
WINGDIAPI BOOL WINAPI SetPixelFormat(HDC, int, const PIXELFORMATDESCRIPTOR*);
WINGDIAPI BOOL WINAPI SwapBuffers(HDC);

// shell32 gap-fill
WINBASEAPI LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR, int*);

// version gap-fill
WINBASEAPI BOOL WINAPI GetFileVersionInfoExW(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);

// setupapi / cfgmgr32 gap-fill
WINBASEAPI CONFIGRET WINAPI CM_Get_Device_IDW(DEVINST, PWSTR, ULONG, ULONG);
WINBASEAPI BOOL        WINAPI SetupDiEnumDeviceInfo(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
WINBASEAPI HDEVINFO    WINAPI SetupDiGetClassDevsExW(const GUID*, LPCWSTR, HWND, DWORD, HDEVINFO, LPCWSTR, PVOID);
WINBASEAPI BOOL        WINAPI SetupDiGetDeviceInterfaceAlias(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, const GUID*, PSP_DEVICE_INTERFACE_DATA);
WINBASEAPI BOOL        WINAPI SetupDiGetDevicePropertyW(HDEVINFO, PSP_DEVINFO_DATA, const DEVPROPKEY*, DEVPROPTYPE*, PBYTE, DWORD, PDWORD, DWORD);
WINBASEAPI HKEY        WINAPI SetupDiOpenDeviceInterfaceRegKey(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, DWORD, REGSAM);
WINBASEAPI HKEY        WINAPI SetupDiOpenDevRegKey(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);
WINBASEAPI CONFIGRET WINAPI CM_Get_DevNode_PropertyW(DEVINST, const DEVPROPKEY*, DEVPROPTYPE*, PBYTE, PULONG, ULONG);
WINBASEAPI CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST, LPCWSTR, ULONG);

// d3d12 gap-fill (pass-through).
WINBASEAPI HRESULT WINAPI D3D12CoreCreateLayeredDevice(void*, void*);
WINBASEAPI SIZE_T  WINAPI D3D12CoreGetLayeredDeviceSize(const void*);
WINBASEAPI HRESULT WINAPI D3D12CoreRegisterLayers(void*, UINT);
WINBASEAPI HRESULT WINAPI D3D12SerializeVersionedRootSignature(const void*, void**, void**);

// imm32 gap-fill (stubs only).
WINGDIAPI HIMC  WINAPI ImmAssociateContext(HWND, HIMC);
WINGDIAPI HIMC  WINAPI ImmCreateContext();
WINGDIAPI BOOL  WINAPI ImmDestroyContext(HIMC);
WINGDIAPI LONG  WINAPI ImmGetCompositionStringW(HIMC, DWORD, LPVOID, DWORD);
WINGDIAPI HIMC  WINAPI ImmGetContext(HWND);
WINGDIAPI UINT  WINAPI ImmGetDescriptionW(HIMC, LPWSTR, UINT);
WINGDIAPI UINT  WINAPI ImmGetIMEFileNameW(HIMC, LPWSTR, UINT);
WINGDIAPI DWORD WINAPI ImmGetProperty(HKL, DWORD);
WINGDIAPI BOOL  WINAPI ImmNotifyIME(HIMC, DWORD, DWORD, DWORD);
WINGDIAPI BOOL  WINAPI ImmReleaseContext(HWND, HIMC);
WINGDIAPI BOOL  WINAPI ImmSetCandidateWindow(HIMC, LPCANDIDATEFORM);
WINGDIAPI BOOL  WINAPI ImmSetCompositionWindow(HIMC, LPCOMPOSITIONFORM);

// winhttp gap-fill (pass-through).
WINBASEAPI BOOL        WINAPI WinHttpAddRequestHeaders(HINTERNET, LPCWSTR, DWORD, DWORD);
WINBASEAPI BOOL        WINAPI WinHttpCloseHandle(HINTERNET);
WINBASEAPI HINTERNET   WINAPI WinHttpConnect(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
WINBASEAPI BOOL        WINAPI WinHttpCrackUrl(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTS);
WINBASEAPI BOOL        WINAPI WinHttpGetDefaultProxyConfiguration(LPWINHTTP_PROXY_INFO);
WINBASEAPI BOOL        WINAPI WinHttpGetIEProxyConfigForCurrentUser(LPWINHTTP_CURRENT_USER_IE_PROXY_CONFIG);
WINBASEAPI HINTERNET   WINAPI WinHttpOpen(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
WINBASEAPI HINTERNET   WINAPI WinHttpOpenRequest(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
WINBASEAPI BOOL        WINAPI WinHttpReceiveResponse(HINTERNET, LPVOID);
WINBASEAPI BOOL        WINAPI WinHttpSendRequest(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);

// wininet gap-fill (stubs).
WINBASEAPI BOOL      WINAPI HttpAddRequestHeadersW(HINTERNET, LPCWSTR, DWORD, DWORD);
WINBASEAPI HINTERNET WINAPI HttpOpenRequestW(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
WINBASEAPI BOOL      WINAPI HttpQueryInfoW(HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD);
WINBASEAPI BOOL      WINAPI HttpSendRequestW(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD);
WINBASEAPI BOOL      WINAPI InternetCloseHandle(HINTERNET);
WINBASEAPI HINTERNET WINAPI InternetConnectW(HINTERNET, LPCWSTR, INTERNET_PORT, LPCWSTR, LPCWSTR, DWORD, DWORD, DWORD_PTR);
WINBASEAPI BOOL      WINAPI InternetCrackUrlW(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTS);
WINBASEAPI DWORD     WINAPI InternetErrorDlg(HWND, HINTERNET, DWORD, DWORD, LPVOID*);
WINBASEAPI HINTERNET WINAPI InternetOpenW(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
WINBASEAPI BOOL      WINAPI InternetReadFile(HINTERNET, LPVOID, DWORD, LPDWORD);
WINBASEAPI BOOL      WINAPI InternetSetOptionW(HINTERNET, DWORD, LPVOID, DWORD);

// wintrust gap-fill (stubs).
WINBASEAPI BOOL    WINAPI CryptCatAdminCalcHashFromFileHandle(HANDLE, DWORD*, BYTE*, DWORD);
WINBASEAPI void*   WINAPI WTHelperGetProvSignerFromChain(void*, DWORD, BOOL, DWORD);
WINBASEAPI void*   WINAPI WTHelperProvDataFromStateData(void*);

// ntdll gap-fill (pass-through).
NTSYSAPI NTSTATUS NTAPI NtSetInformationThread(HANDLE, ULONG, PVOID, ULONG);
NTSYSAPI NTSTATUS NTAPI NtWriteFile(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
NTSYSAPI VOID     NTAPI RtlInitUnicodeString(PUNICODE_STRING, PCWSTR);
NTSYSAPI BOOLEAN  NTAPI RtlIsCriticalSectionLockedByThread(PRTL_CRITICAL_SECTION);
NTSYSAPI ULONG    NTAPI RtlNtStatusToDosError(NTSTATUS);
NTSYSAPI BOOLEAN  NTAPI RtlRunOnceExecuteOnce(PRTL_RUN_ONCE, PRTL_RUN_ONCE_INIT_FN, PVOID, PVOID*);
NTSYSAPI NTSTATUS NTAPI RtlUTF8ToUnicodeN(PWSTR, ULONG, PULONG, PCSTR, ULONG);

// dwmapi gap-fill (pass-through).
WINBASEAPI HRESULT WINAPI DwmFlush();
WINBASEAPI HRESULT WINAPI DwmGetCompositionTimingInfo(HWND, PDWM_TIMING_INFO);
WINBASEAPI HRESULT WINAPI DwmIsCompositionEnabled(BOOL*);
WINBASEAPI HRESULT WINAPI DwmSetWindowAttribute(HWND, DWORD, LPCVOID, DWORD);

// ncrypt gap-fill (pass-through).
WINBASEAPI SECURITY_STATUS WINAPI NCryptFreeObject(NCRYPT_HANDLE);
WINBASEAPI SECURITY_STATUS WINAPI NCryptGetProperty(NCRYPT_HANDLE, LPCWSTR, PBYTE, DWORD, DWORD*, DWORD);

// msdmo gap-fill (stubs).
WINBASEAPI HRESULT WINAPI MoFreeMediaType(DMO_MEDIA_TYPE*);
WINBASEAPI HRESULT WINAPI MoInitMediaType(DMO_MEDIA_TYPE*, DWORD);

// propsys gap-fill (stub).
WINBASEAPI HRESULT WINAPI PropVariantToBSTR(const PROPVARIANT*, BSTR*);

// dbghelp gap-fill (stubs).
WINBASEAPI PVOID    WINAPI ImageNtHeader(PVOID);
WINBASEAPI BOOL     WINAPI MiniDumpWriteDump(HANDLE, DWORD, HANDLE, DWORD, void*, void*, void*);
WINBASEAPI BOOL     WINAPI StackWalk64(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
WINBASEAPI BOOL     WINAPI SymCleanup(HANDLE);
WINBASEAPI BOOL     WINAPI SymFromAddr(HANDLE, DWORD64, PDWORD64, void*);
WINBASEAPI PVOID    WINAPI SymFunctionTableAccess64(HANDLE, DWORD64);
WINBASEAPI BOOL     WINAPI SymGetLineFromAddr64(HANDLE, DWORD64, PDWORD, void*);
WINBASEAPI DWORD64  WINAPI SymGetModuleBase64(HANDLE, DWORD64);
WINBASEAPI BOOL     WINAPI SymGetModuleInfo64(HANDLE, DWORD64, void*);
WINBASEAPI BOOL     WINAPI SymGetModuleInfoW64(HANDLE, DWORD64, void*);
WINBASEAPI DWORD    WINAPI SymGetOptions();
WINBASEAPI BOOL     WINAPI SymGetSymFromAddr64(HANDLE, DWORD64, PDWORD64, void*);
WINBASEAPI BOOL     WINAPI SymInitialize(HANDLE, LPCSTR, BOOL);
WINBASEAPI BOOL     WINAPI SymInitializeW(HANDLE, LPCWSTR, BOOL);
WINBASEAPI DWORD64  WINAPI SymLoadModuleExW(HANDLE, HANDLE, LPCWSTR, LPCWSTR, DWORD64, DWORD, void*, DWORD);
WINBASEAPI DWORD    WINAPI SymSetOptions(DWORD);
WINBASEAPI BOOL     WINAPI SymSetSearchPathW(HANDLE, LPCWSTR);
WINBASEAPI BOOL     WINAPI SymUnloadModule64(HANDLE, DWORD64);

// d3dcompiler_47 gap-fill (pass-through).
WINBASEAPI HRESULT WINAPI D3DCreateBlob(SIZE_T, ID3DBlob**);

// dxgi gap-fill (pass-through).
WINBASEAPI HRESULT WINAPI CreateDXGIFactory(const IID&, void**);
WINBASEAPI HRESULT WINAPI CreateDXGIFactory1(const IID&, void**);
WINBASEAPI HRESULT WINAPI CreateDXGIFactory2(UINT, const IID&, void**);

// rpcrt4 gap-fill (pass-through).
WINBASEAPI RPC_STATUS WINAPI UuidCreate(UUID*);

// uxtheme gap-fill (stub).
WINBASEAPI HRESULT WINAPI SetWindowTheme(HWND, LPCWSTR, LPCWSTR);

// uiautomationcore gap-fill (stubs).
WINBASEAPI BOOL    WINAPI UiaClientsAreListening();
WINBASEAPI HRESULT WINAPI UiaRaiseAutomationPropertyChangedEvent(void*, PROPERTYID, VARIANT, VARIANT);

// ----------------------------------------------------------------------------
// WER (Windows Error Reporting) — gap-fill declarations.
// Added by Task ID GAP-EXT-MS-WIN. The stub shims in
// shims/ExtMsGapFill.cpp implement these as fake-success stubs because
// (a) WER is unavailable inside an AppContainer UWP game, and
// (b) returning S_OK keeps games that call WerReportCreate / WerReportSubmit
//     at crash time from crashing harder than they already are.
//
// The real SDK signatures (werapi.h) are:
//   HRESULT WerReportCreate(PCWSTR, REFWER_REPORT_INFORMATION, HREPORT*);
//   HRESULT WerReportAddDump(HREPORT, HANDLE, HANDLE, WER_DUMP_TYPE,
//                            PVOID, PVOID, DWORD, DWORD);
//   HRESULT WerReportSetParameter(HREPORT, DWORD, PCWSTR, PCWSTR);
//   HRESULT WerReportSubmit(HREPORT, DWORD, DWORD, PDWORD);
//   HRESULT WerReportCloseHandle(HREPORT);
//
// We use opaque types for the WER_REPORT_INFORMATION pointer and the
// HREPORT handle so the shim layer doesn't need the full struct layout
// (we never dereference them — every stub returns S_OK and optionally
// writes a fake non-null handle so the caller treats it as success).
// ----------------------------------------------------------------------------
#ifndef HREPORT_DEFINED
typedef HANDLE HREPORT, *PHREPORT;
#define HREPORT_DEFINED
#endif
struct _WER_REPORT_INFORMATION;
typedef struct _WER_REPORT_INFORMATION WER_REPORT_INFORMATION, *PWER_REPORT_INFORMATION;

WINBASEAPI HRESULT WINAPI WerReportCreate(PCWSTR, const WER_REPORT_INFORMATION*, HREPORT*);
WINBASEAPI HRESULT WINAPI WerReportAddDump(HREPORT, HANDLE, HANDLE, DWORD, PVOID, PVOID, DWORD, DWORD);
WINBASEAPI HRESULT WINAPI WerReportSetParameter(HREPORT, DWORD, PCWSTR, PCWSTR);
WINBASEAPI HRESULT WINAPI WerReportSubmit(HREPORT, DWORD, DWORD, PDWORD);
WINBASEAPI HRESULT WINAPI WerReportCloseHandle(HREPORT);

// ----------------------------------------------------------------------------
// DXCore — real UWP API for adapter factory creation. Provided by dxcore.lib
// on UWP. The shim in shims/ExtMsGapFill.cpp passes through to the real
// implementation when built with the real SDK; under XWR_SYNTAX_CHECK the
// stub path returns S_OK so g++ -fsyntax-only doesn't need the .lib.
// ----------------------------------------------------------------------------
WINBASEAPI HRESULT WINAPI DXCoreCreateAdapterFactory(const IID&, void**);

// ----------------------------------------------------------------------------
// IPHLPAPI gap-fill (pass-through).
// Real SDK signatures (iphlpapi.h):
//   ULONG WINAPI GetCurrentThreadCompartmentId();
//   DWORD WINAPI SetCurrentThreadCompartmentId(ULONG);
//   ULONG WINAPI GetAdaptersAddresses(ULONG Family, ULONG Flags, PVOID Reserved,
//                                     PIP_ADAPTER_ADDRESSES AdapterAddresses,
//                                     PULONG SizePointer);
//   DWORD WINAPI GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
//   HANDLE WINAPI IcmpCreateFile();
//   BOOL   WINAPI IcmpCloseHandle(HANDLE IcmpHandle);
//   DWORD  WINAPI IcmpSendEcho(HANDLE IcmpHandle, IPAddr DestinationAddress,
//                              LPVOID RequestData, WORD RequestSize,
//                              PIP_OPTION_INFORMATION RequestOptions,
//                              LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
//   NET_IFINDEX WINAPI if_nametoindex(PCSTR InterfaceName);
//
// We use opaque pointer/struct aliases so the shim layer doesn't need the full
// iphlpapi.h struct layouts (IP_ADAPTER_ADDRESSES, IP_ADAPTER_INFO,
// IP_OPTION_INFORMATION) — every pass-through just forwards the pointer
// verbatim. Under XWR_SYNTAX_CHECK we can't link iphlpapi.lib, so the stub
// path returns failure values.
//
// Added by Task ID GAP-GAME-DLL-STUBS.
// ----------------------------------------------------------------------------
#ifndef IP_ADDR_DEFINED
typedef ULONG IP_ADDR, *PIP_ADDR;
#define IP_ADDR_DEFINED
#endif
#ifndef NET_IFINDEX_DEFINED
typedef ULONG NET_IFINDEX;
#define NET_IFINDEX_DEFINED
#endif
struct _IP_ADAPTER_ADDRESSES_LH;
typedef struct _IP_ADAPTER_ADDRESSES_LH IP_ADAPTER_ADDRESSES, *PIP_ADAPTER_ADDRESSES;
struct _IP_ADAPTER_INFO_XP;
typedef struct _IP_ADAPTER_INFO_XP IP_ADAPTER_INFO, *PIP_ADAPTER_INFO;
struct _IP_OPTION_INFORMATION_XP;
typedef struct _IP_OPTION_INFORMATION_XP IP_OPTION_INFORMATION, *PIP_OPTION_INFORMATION;

WINBASEAPI ULONG WINAPI GetCurrentThreadCompartmentId();
WINBASEAPI DWORD WINAPI SetCurrentThreadCompartmentId(ULONG);
WINBASEAPI ULONG WINAPI GetAdaptersAddresses(ULONG, ULONG, PVOID,
                                             PIP_ADAPTER_ADDRESSES, PULONG);
WINBASEAPI DWORD WINAPI GetAdaptersInfo(PIP_ADAPTER_INFO, PULONG);
WINBASEAPI HANDLE WINAPI IcmpCreateFile();
WINBASEAPI BOOL   WINAPI IcmpCloseHandle(HANDLE);
WINBASEAPI DWORD  WINAPI IcmpSendEcho(HANDLE, IP_ADDR, LPVOID, WORD,
                                      PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD);
WINBASEAPI NET_IFINDEX WINAPI if_nametoindex(PCSTR);

}  // extern "C"

// Macros missing from the UWP stub.
#ifndef WINOLEAPI
#define WINOLEAPI
#endif

#endif  // _XWR_MISC_SYSTEM_GAP_TYPES

#endif  // STUB_WINDOWS_H
