// shims/pass-through/Ws2_32GapFill.cpp
// Gap-fill shims for the 56 WS2_32.dll (Winsock) entries reported missing by
// the coverage gap scan. Breakdown of the 56 gaps:
//
//   - 23 named function imports
//       freeaddrinfo, freeaddrinfow, getaddrinfo, getaddrinfow, getnameinfo,
//       inet_ntop, inet_pton, inetntopw, wsacloseevent, wsacreateevent,
//       wsaduplicatesocketw, wsaenumnetworkevents, wsaeventselect, wsaioctl,
//       wsapoll, wsarecv, wsarecvfrom, wsaresetevent, wsasend, wsasendto,
//       wsasetevent, wsasocketw, wsawaitformultipleevents.
//
//   - 33 ordinal imports
//       ordinal:1..23, ordinal:51, ordinal:52, ordinal:55, ordinal:56,
//       ordinal:57, ordinal:111, ordinal:112, ordinal:115, ordinal:116,
//       ordinal:151.
//
// All shims pass through to the real UWP Winsock API. For functions that
// Ws2_32Shim.cpp already wraps (accept, bind, send, recv, etc.), we reuse
// those existing Shim_* definitions for the ordinal by-name registrations
// (forward-declared below) so we don't emit duplicate symbols.
//
// Per the ws2_32 export convention every ordinal import is registered BOTH
// by name (e.g. "accept") AND by ordinal (e.g. "ordinal:1"), so a game that
// imports the function by either form resolves against the shim registry.
// The 23 named gaps are registered by name only.
//
// REGISTER_SHIM count: 23 (named gaps) + 33 (ordinals by name) + 33 (ordinals
// by ordinal) = 89.
//
// Added by Task ID GAP-WS2_32.

#include "UwpSdkIncludes.h"

// ---------------------------------------------------------------------------
// MSVC: winsock2.h + ws2tcpip.h MUST be visible here even if some other TU
// earlier didn't include them. They declare the SOCKET type and the winsock
// API signatures that the gap-fill shims below forward to.
// ---------------------------------------------------------------------------
#ifdef _MSC_VER
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#endif

#include <cstdint>
#include <cstring>

#include "../ShimRegistry.h"

// Winsock error constants the UWP subset omits.
#ifndef WSAEACCES
#define WSAEACCES 10013
#endif
#ifndef WSAEINVAL
#define WSAEINVAL 10022
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (SOCKET)(~0)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

namespace xwr {

// ===========================================================================
// Forward declarations of shim functions already defined in Ws2_32Shim.cpp.
// We reuse them for the ordinal by-name registrations below to avoid emitting
// duplicate symbol definitions.
// ===========================================================================
extern "C" int                   __stdcall Shim_WSAStartup(WORD, void*);
extern "C" int                   __stdcall Shim_WSACleanup();
extern "C" SOCKET                __stdcall Shim_socket(int, int, int);
extern "C" int                   __stdcall Shim_closesocket(SOCKET);
extern "C" int                   __stdcall Shim_bind(SOCKET, const void*, int);
extern "C" int                   __stdcall Shim_listen(SOCKET, int);
extern "C" SOCKET                __stdcall Shim_accept(SOCKET, void*, int*);
extern "C" int                   __stdcall Shim_connect(SOCKET, const void*, int);
extern "C" int                   __stdcall Shim_send(SOCKET, const char*, int, int);
extern "C" int                   __stdcall Shim_recv(SOCKET, char*, int, int);
extern "C" int                   __stdcall Shim_setsockopt(SOCKET, int, int, const char*, int);
extern "C" int                   __stdcall Shim_getsockopt(SOCKET, int, int, char*, int*);
extern "C" int                   __stdcall Shim_ioctlsocket(SOCKET, long, u_long*);
extern "C" int                   __stdcall Shim_gethostname(char*, int);
extern "C" struct hostent*       __stdcall Shim_gethostbyname(const char*);
extern "C" unsigned long         __stdcall Shim_inet_addr(const char*);
extern "C" int                   __stdcall Shim_WSAGetLastError();

// ===========================================================================
// New shim implementations — pass-through to the real ws2_32 API.
// ===========================================================================

// --- Address resolution (8 named gaps) ---
extern "C" void __stdcall Shim_freeaddrinfo(struct addrinfo* p) { ::freeaddrinfo(p); }
extern "C" void __stdcall Shim_freeaddrinfow(struct addrinfoW* p) { ::FreeAddrInfoW(p); }
extern "C" int  __stdcall Shim_getaddrinfo(const char* node, const char* service,
                                           const struct addrinfo* hints, struct addrinfo** res) {
    return ::getaddrinfo(node, service, hints, res);
}
extern "C" int  __stdcall Shim_getaddrinfow(const wchar_t* node, const wchar_t* service,
                                            const struct addrinfoW* hints, struct addrinfoW** res) {
    return ::GetAddrInfoW(node, service, hints, res);
}
extern "C" int  __stdcall Shim_getnameinfo(const struct sockaddr* sa, socklen_t salen,
                                           char* host, size_t hostlen,
                                           char* serv, size_t servlen, int flags) {
    return ::getnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
}
extern "C" const char* __stdcall Shim_inet_ntop(int af, void* src, char* dst, size_t size) {
    return ::inet_ntop(af, src, dst, size);
}
extern "C" int __stdcall Shim_inet_pton(int af, const char* src, void* dst) {
    return ::inet_pton(af, src, dst);
}
extern "C" const wchar_t* __stdcall Shim_inetntopw(int af, void* src, wchar_t* dst, size_t size) {
    return ::InetNtopW(af, src, dst, size);
}

// --- Socket primitives (11) — new ordinals not already in Ws2_32Shim.cpp ---
extern "C" int     __stdcall Shim_getpeername(SOCKET s, struct sockaddr* name, int* namelen) {
    return ::getpeername(s, name, namelen);
}
extern "C" int     __stdcall Shim_getsockname(SOCKET s, struct sockaddr* name, int* namelen) {
    return ::getsockname(s, name, namelen);
}
extern "C" u_long  __stdcall Shim_htonl(u_long hostlong) { return ::htonl(hostlong); }
extern "C" u_short __stdcall Shim_htons(u_short hostshort) { return ::htons(hostshort); }
extern "C" char*   __stdcall Shim_inet_ntoa(struct in_addr in) { return ::inet_ntoa(in); }
extern "C" u_long  __stdcall Shim_ntohl(u_long netlong) { return ::ntohl(netlong); }
extern "C" u_short __stdcall Shim_ntohs(u_short netshort) { return ::ntohs(netshort); }
extern "C" int     __stdcall Shim_recvfrom(SOCKET s, char* buf, int len, int flags,
                                           struct sockaddr* from, int* fromlen) {
    return ::recvfrom(s, buf, len, flags, from, fromlen);
}
extern "C" int     __stdcall Shim_sendto(SOCKET s, const char* buf, int len, int flags,
                                         const struct sockaddr* to, int tolen) {
    return ::sendto(s, buf, len, flags, to, tolen);
}
extern "C" int     __stdcall Shim_shutdown(SOCKET s, int how) { return ::shutdown(s, how); }

// select() — on real Windows, fd_set and timeval come from <winsock2.h>.
// On Linux syntax-check, they come from glibc's sys/select.h.
// We use void* casts so this compiles on both without redefining fd_set.
#ifdef _MSC_VER
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
extern "C" int __stdcall Shim_select(int nfds, void* readfds, void* writefds,
                                     void* exceptfds, void* timeout) {
#ifdef _MSC_VER
    return ::select(nfds,
                    reinterpret_cast<fd_set*>(readfds),
                    reinterpret_cast<fd_set*>(writefds),
                    reinterpret_cast<fd_set*>(exceptfds),
                    reinterpret_cast<struct timeval*>(timeout));
#else
    // Linux syntax-check: ::select is declared by glibc; cast to void*
    (void)nfds; (void)readfds; (void)writefds; (void)exceptfds; (void)timeout;
    return 0;
#endif
}

// --- WSA* events / overlapped I/O (15 named gaps, incl. 3 also ordinals) ---
extern "C" BOOL     __stdcall Shim_WSACloseEvent(WSAEVENT hEvent) { return ::WSACloseEvent(hEvent); }
extern "C" WSAEVENT __stdcall Shim_WSACreateEvent() { return ::WSACreateEvent(); }
extern "C" int      __stdcall Shim_WSADuplicateSocketW(SOCKET s, DWORD dwProcessId,
                                                       LPWSAPROTOCOL_INFOW lpProtocolInfo) {
    return ::WSADuplicateSocketW(s, dwProcessId, lpProtocolInfo);
}
extern "C" int      __stdcall Shim_WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEventObject,
                                                        LPWSANETWORKEVENTS lpNetworkEvents) {
    return ::WSAEnumNetworkEvents(s, hEventObject, lpNetworkEvents);
}
extern "C" int      __stdcall Shim_WSAEventSelect(SOCKET s, WSAEVENT hEventObject, long lNetworkEvents) {
    return ::WSAEventSelect(s, hEventObject, lNetworkEvents);
}
extern "C" int      __stdcall Shim_WSAIoctl(SOCKET s, DWORD dwIoControlCode,
                                            void* lpvInBuffer, DWORD cbInBuffer,
                                            void* lpvOutBuffer, DWORD cbOutBuffer,
                                            DWORD* lpcbBytesReturned,
                                            LPWSAOVERLAPPED lpOverlapped,
                                            LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    return ::WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer,
                      lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
}
extern "C" int      __stdcall Shim_WSAPoll(LPWSAPOLLFD fdarray, ULONG nfds, INT timeout) {
    return ::WSAPoll(fdarray, nfds, timeout);
}
extern "C" int      __stdcall Shim_WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                                           DWORD* lpNumberOfBytesRecvd, DWORD* lpFlags,
                                           LPWSAOVERLAPPED lpOverlapped,
                                           LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    return ::WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags,
                     lpOverlapped, lpCompletionRoutine);
}
extern "C" int      __stdcall Shim_WSARecvFrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                                               DWORD* lpNumberOfBytesRecvd, DWORD* lpFlags,
                                               struct sockaddr* lpFrom, int* lpFromlen,
                                               LPWSAOVERLAPPED lpOverlapped,
                                               LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    return ::WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags,
                         lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
}
extern "C" BOOL     __stdcall Shim_WSAResetEvent(WSAEVENT hEvent) { return ::WSAResetEvent(hEvent); }
extern "C" int      __stdcall Shim_WSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                                           DWORD* lpNumberOfBytesSent, DWORD dwFlags,
                                           LPWSAOVERLAPPED lpOverlapped,
                                           LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    return ::WSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags,
                     lpOverlapped, lpCompletionRoutine);
}
extern "C" int      __stdcall Shim_WSASendTo(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
                                             DWORD* lpNumberOfBytesSent, DWORD dwFlags,
                                             const struct sockaddr* lpTo, int iTolen,
                                             LPWSAOVERLAPPED lpOverlapped,
                                             LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) {
    return ::WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen,
                       lpOverlapped, lpCompletionRoutine);
}
extern "C" BOOL     __stdcall Shim_WSASetEvent(WSAEVENT hEvent) { return ::WSASetEvent(hEvent); }
extern "C" SOCKET   __stdcall Shim_WSASocketW(int af, int type, int protocol,
                                              LPWSAPROTOCOL_INFOW lpProtocolInfo, GROUP g, DWORD dwFlags) {
    return ::WSASocketW(af, type, protocol, lpProtocolInfo, g, dwFlags);
}
extern "C" DWORD    __stdcall Shim_WSAWaitForMultipleEvents(DWORD cEvents, const WSAEVENT* lphEvents,
                                                            BOOL fWaitAll, DWORD dwTimeout, BOOL fAlertable) {
    return ::WSAWaitForMultipleEvents(cEvents, lphEvents, fWaitAll, dwTimeout, fAlertable);
}

// --- WSA* address/string conversion (2) — ordinal-only new ---
extern "C" INT __stdcall Shim_WSAStringToAddressW(LPWSTR AddressString, INT AddressFamily,
                                                  LPWSAPROTOCOL_INFOW lpProtocolInfo,
                                                  LPSOCKADDR Address, LPINT lpAddressLength) {
    return ::WSAStringToAddressW(AddressString, AddressFamily, lpProtocolInfo, Address, lpAddressLength);
}
extern "C" INT __stdcall Shim_WSAAddressToStringW(struct sockaddr* lpsaAddress, DWORD dwAddressLength,
                                                  LPWSAPROTOCOL_INFOW lpProtocolInfo,
                                                  LPWSTR lpszAddressString,
                                                  LPDWORD lpdwAddressStringLength) {
    return ::WSAAddressToStringW(lpsaAddress, dwAddressLength, lpProtocolInfo,
                                 lpszAddressString, lpdwAddressStringLength);
}

}  // namespace xwr

// ===========================================================================
// Registration — 89 REGISTER_SHIM entries total:
//   - 23 named-gap by-name registrations
//   - 33 ordinal by-name registrations
//   - 33 ordinal by-ordinal registrations
// All entries target the "ws2_32" module.
// ===========================================================================

// ---------------------------------------------------------------------------
// (1) Named gaps (23 by-name registrations)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ws2_32", "freeaddrinfo",             (FARPROC)&xwr::Shim_freeaddrinfo);
REGISTER_SHIM("ws2_32", "freeaddrinfow",            (FARPROC)&xwr::Shim_freeaddrinfow);
REGISTER_SHIM("ws2_32", "getaddrinfo",              (FARPROC)&xwr::Shim_getaddrinfo);
REGISTER_SHIM("ws2_32", "getaddrinfow",             (FARPROC)&xwr::Shim_getaddrinfow);
REGISTER_SHIM("ws2_32", "getnameinfo",              (FARPROC)&xwr::Shim_getnameinfo);
REGISTER_SHIM("ws2_32", "inet_ntop",                (FARPROC)&xwr::Shim_inet_ntop);
REGISTER_SHIM("ws2_32", "inet_pton",                (FARPROC)&xwr::Shim_inet_pton);
REGISTER_SHIM("ws2_32", "inetntopw",                (FARPROC)&xwr::Shim_inetntopw);
REGISTER_SHIM("ws2_32", "wsacloseevent",            (FARPROC)&xwr::Shim_WSACloseEvent);
REGISTER_SHIM("ws2_32", "wsacreateevent",           (FARPROC)&xwr::Shim_WSACreateEvent);
REGISTER_SHIM("ws2_32", "wsaduplicatesocketw",      (FARPROC)&xwr::Shim_WSADuplicateSocketW);
REGISTER_SHIM("ws2_32", "wsaenumnetworkevents",     (FARPROC)&xwr::Shim_WSAEnumNetworkEvents);
REGISTER_SHIM("ws2_32", "wsaeventselect",           (FARPROC)&xwr::Shim_WSAEventSelect);
REGISTER_SHIM("ws2_32", "wsaioctl",                 (FARPROC)&xwr::Shim_WSAIoctl);
REGISTER_SHIM("ws2_32", "wsapoll",                  (FARPROC)&xwr::Shim_WSAPoll);
REGISTER_SHIM("ws2_32", "wsarecv",                  (FARPROC)&xwr::Shim_WSARecv);
REGISTER_SHIM("ws2_32", "wsarecvfrom",              (FARPROC)&xwr::Shim_WSARecvFrom);
REGISTER_SHIM("ws2_32", "wsaresetevent",            (FARPROC)&xwr::Shim_WSAResetEvent);
REGISTER_SHIM("ws2_32", "wsasend",                  (FARPROC)&xwr::Shim_WSASend);
REGISTER_SHIM("ws2_32", "wsasendto",                (FARPROC)&xwr::Shim_WSASendTo);
REGISTER_SHIM("ws2_32", "wsasetevent",              (FARPROC)&xwr::Shim_WSASetEvent);
REGISTER_SHIM("ws2_32", "wsasocketw",               (FARPROC)&xwr::Shim_WSASocketW);
REGISTER_SHIM("ws2_32", "wsawaitformultipleevents", (FARPROC)&xwr::Shim_WSAWaitForMultipleEvents);

// ---------------------------------------------------------------------------
// (2) Ordinals: by-name registrations (33)
// For functions already shimmed by Ws2_32Shim.cpp (accept, bind, send, recv,
// etc.) we reuse the existing Shim_* definitions. For the rest we use the
// new Shim_* functions defined in this file.
// ---------------------------------------------------------------------------
REGISTER_SHIM("ws2_32", "accept",                   (FARPROC)&xwr::Shim_accept);
REGISTER_SHIM("ws2_32", "bind",                     (FARPROC)&xwr::Shim_bind);
REGISTER_SHIM("ws2_32", "closesocket",              (FARPROC)&xwr::Shim_closesocket);
REGISTER_SHIM("ws2_32", "connect",                  (FARPROC)&xwr::Shim_connect);
REGISTER_SHIM("ws2_32", "getpeername",              (FARPROC)&xwr::Shim_getpeername);
REGISTER_SHIM("ws2_32", "getsockname",              (FARPROC)&xwr::Shim_getsockname);
REGISTER_SHIM("ws2_32", "getsockopt",               (FARPROC)&xwr::Shim_getsockopt);
REGISTER_SHIM("ws2_32", "htonl",                    (FARPROC)&xwr::Shim_htonl);
REGISTER_SHIM("ws2_32", "htons",                    (FARPROC)&xwr::Shim_htons);
REGISTER_SHIM("ws2_32", "ioctlsocket",              (FARPROC)&xwr::Shim_ioctlsocket);
REGISTER_SHIM("ws2_32", "inet_addr",                (FARPROC)&xwr::Shim_inet_addr);
REGISTER_SHIM("ws2_32", "inet_ntoa",                (FARPROC)&xwr::Shim_inet_ntoa);
REGISTER_SHIM("ws2_32", "listen",                   (FARPROC)&xwr::Shim_listen);
REGISTER_SHIM("ws2_32", "ntohl",                    (FARPROC)&xwr::Shim_ntohl);
REGISTER_SHIM("ws2_32", "ntohs",                    (FARPROC)&xwr::Shim_ntohs);
REGISTER_SHIM("ws2_32", "recv",                     (FARPROC)&xwr::Shim_recv);
REGISTER_SHIM("ws2_32", "recvfrom",                 (FARPROC)&xwr::Shim_recvfrom);
REGISTER_SHIM("ws2_32", "select",                   (FARPROC)&xwr::Shim_select);
REGISTER_SHIM("ws2_32", "send",                     (FARPROC)&xwr::Shim_send);
REGISTER_SHIM("ws2_32", "sendto",                   (FARPROC)&xwr::Shim_sendto);
REGISTER_SHIM("ws2_32", "setsockopt",               (FARPROC)&xwr::Shim_setsockopt);
REGISTER_SHIM("ws2_32", "shutdown",                 (FARPROC)&xwr::Shim_shutdown);
REGISTER_SHIM("ws2_32", "socket",                   (FARPROC)&xwr::Shim_socket);
REGISTER_SHIM("ws2_32", "gethostname",              (FARPROC)&xwr::Shim_gethostname);
REGISTER_SHIM("ws2_32", "gethostbyname",            (FARPROC)&xwr::Shim_gethostbyname);
REGISTER_SHIM("ws2_32", "WSAStartup",               (FARPROC)&xwr::Shim_WSAStartup);
REGISTER_SHIM("ws2_32", "WSACleanup",               (FARPROC)&xwr::Shim_WSACleanup);
REGISTER_SHIM("ws2_32", "WSAGetLastError",          (FARPROC)&xwr::Shim_WSAGetLastError);
REGISTER_SHIM("ws2_32", "WSARecv",                  (FARPROC)&xwr::Shim_WSARecv);
REGISTER_SHIM("ws2_32", "WSASend",                  (FARPROC)&xwr::Shim_WSASend);
REGISTER_SHIM("ws2_32", "WSAStringToAddressW",      (FARPROC)&xwr::Shim_WSAStringToAddressW);
REGISTER_SHIM("ws2_32", "WSAAddressToStringW",      (FARPROC)&xwr::Shim_WSAAddressToStringW);
REGISTER_SHIM("ws2_32", "WSADuplicateSocketW",      (FARPROC)&xwr::Shim_WSADuplicateSocketW);

// ---------------------------------------------------------------------------
// (3) Ordinals: by-ordinal registrations (33)
// ws2_32.dll exports these entry points by ordinal; a game's IAT may import
// either "accept" or "ordinal:1". Registering both forms ensures the PE
// loader resolves either import style against the shim registry.
// ---------------------------------------------------------------------------
REGISTER_SHIM("ws2_32", "ordinal:1",                (FARPROC)&xwr::Shim_accept);
REGISTER_SHIM("ws2_32", "ordinal:2",                (FARPROC)&xwr::Shim_bind);
REGISTER_SHIM("ws2_32", "ordinal:3",                (FARPROC)&xwr::Shim_closesocket);
REGISTER_SHIM("ws2_32", "ordinal:4",                (FARPROC)&xwr::Shim_connect);
REGISTER_SHIM("ws2_32", "ordinal:5",                (FARPROC)&xwr::Shim_getpeername);
REGISTER_SHIM("ws2_32", "ordinal:6",                (FARPROC)&xwr::Shim_getsockname);
REGISTER_SHIM("ws2_32", "ordinal:7",                (FARPROC)&xwr::Shim_getsockopt);
REGISTER_SHIM("ws2_32", "ordinal:8",                (FARPROC)&xwr::Shim_htonl);
REGISTER_SHIM("ws2_32", "ordinal:9",                (FARPROC)&xwr::Shim_htons);
REGISTER_SHIM("ws2_32", "ordinal:10",               (FARPROC)&xwr::Shim_ioctlsocket);
REGISTER_SHIM("ws2_32", "ordinal:11",               (FARPROC)&xwr::Shim_inet_addr);
REGISTER_SHIM("ws2_32", "ordinal:12",               (FARPROC)&xwr::Shim_inet_ntoa);
REGISTER_SHIM("ws2_32", "ordinal:13",               (FARPROC)&xwr::Shim_listen);
REGISTER_SHIM("ws2_32", "ordinal:14",               (FARPROC)&xwr::Shim_ntohl);
REGISTER_SHIM("ws2_32", "ordinal:15",               (FARPROC)&xwr::Shim_ntohs);
REGISTER_SHIM("ws2_32", "ordinal:16",               (FARPROC)&xwr::Shim_recv);
REGISTER_SHIM("ws2_32", "ordinal:17",               (FARPROC)&xwr::Shim_recvfrom);
REGISTER_SHIM("ws2_32", "ordinal:18",               (FARPROC)&xwr::Shim_select);
REGISTER_SHIM("ws2_32", "ordinal:19",               (FARPROC)&xwr::Shim_send);
REGISTER_SHIM("ws2_32", "ordinal:20",               (FARPROC)&xwr::Shim_sendto);
REGISTER_SHIM("ws2_32", "ordinal:21",               (FARPROC)&xwr::Shim_setsockopt);
REGISTER_SHIM("ws2_32", "ordinal:22",               (FARPROC)&xwr::Shim_shutdown);
REGISTER_SHIM("ws2_32", "ordinal:23",               (FARPROC)&xwr::Shim_socket);
REGISTER_SHIM("ws2_32", "ordinal:51",               (FARPROC)&xwr::Shim_gethostname);
REGISTER_SHIM("ws2_32", "ordinal:52",               (FARPROC)&xwr::Shim_gethostbyname);
REGISTER_SHIM("ws2_32", "ordinal:55",               (FARPROC)&xwr::Shim_WSAStartup);
REGISTER_SHIM("ws2_32", "ordinal:56",               (FARPROC)&xwr::Shim_WSACleanup);
REGISTER_SHIM("ws2_32", "ordinal:57",               (FARPROC)&xwr::Shim_WSAGetLastError);
REGISTER_SHIM("ws2_32", "ordinal:111",              (FARPROC)&xwr::Shim_WSARecv);
REGISTER_SHIM("ws2_32", "ordinal:112",              (FARPROC)&xwr::Shim_WSASend);
REGISTER_SHIM("ws2_32", "ordinal:115",              (FARPROC)&xwr::Shim_WSAStringToAddressW);
REGISTER_SHIM("ws2_32", "ordinal:116",              (FARPROC)&xwr::Shim_WSAAddressToStringW);
REGISTER_SHIM("ws2_32", "ordinal:151",              (FARPROC)&xwr::Shim_WSADuplicateSocketW);
