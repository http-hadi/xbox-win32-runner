// shims/pass-through/Ws2_32Shim.cpp
// Winsock pass-through shim. All winsock calls forward to the real ws2_32,
// EXCEPT bind() which fails if the requested port < 1024 — UWP blocks low
// ports and the failure would otherwise surface as a generic WSAEACCES
// with no actionable diagnostic.

#include "UwpSdkIncludes.h"


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

// WSASetLastError is declared in ws2_32; we declare it locally since the
// Windows.h stub doesn't include it.
extern "C" void WINAPI WSASetLastError(int);

namespace xwr {

// We need a struct sockaddr_in to peek at the port. The Windows.h stub
// declares `struct sockaddr;` only, so we declare the IPv4 form locally.
namespace {
struct XWR_sockaddr_in {
    short   sin_family;
    uint16_t sin_port;   // network byte order
    char    sin_addr[4];
    char    sin_zero[8];
};
}

extern "C" int __stdcall Shim_WSAStartup(WORD wVersionRequested, void* lpWSAData) {
    return ::WSAStartup(wVersionRequested, lpWSAData);
}
extern "C" int __stdcall Shim_WSACleanup() { return ::WSACleanup(); }
extern "C" SOCKET __stdcall Shim_socket(int af, int type, int protocol) {
    return ::socket(af, type, protocol);
}
extern "C" int __stdcall Shim_closesocket(SOCKET s) { return ::closesocket(s); }

extern "C" int __stdcall Shim_bind(SOCKET s, const void* name, int namelen) {
    if (!name || namelen < (int)sizeof(XWR_sockaddr_in)) {
        ::WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }
    const XWR_sockaddr_in* in = (const XWR_sockaddr_in*)name;
    // sin_port is in network byte order; convert to host.
    uint16_t port = (uint16_t)((in->sin_port >> 8) | (in->sin_port << 8));
    if (port < 1024) {
        ::WSASetLastError(WSAEACCES);
        return SOCKET_ERROR;
    }
    return ::bind(s, name, namelen);
}
extern "C" int __stdcall Shim_listen(SOCKET s, int backlog) { return ::listen(s, backlog); }
extern "C" SOCKET __stdcall Shim_accept(SOCKET s, void* addr, int* addrlen) {
    return ::accept(s, addr, addrlen);
}
extern "C" int __stdcall Shim_connect(SOCKET s, const void* name, int namelen) {
    return ::connect(s, name, namelen);
}
extern "C" int __stdcall Shim_send(SOCKET s, const char* buf, int len, int flags) {
    return ::send(s, buf, len, flags);
}
extern "C" int __stdcall Shim_recv(SOCKET s, char* buf, int len, int flags) {
    return ::recv(s, buf, len, flags);
}
extern "C" int __stdcall Shim_setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen) {
    return ::setsockopt(s, level, optname, optval, optlen);
}
extern "C" int __stdcall Shim_getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen) {
    return ::getsockopt(s, level, optname, optval, optlen);
}
extern "C" int __stdcall Shim_ioctlsocket(SOCKET s, long cmd, u_long* argp) {
    return ::ioctlsocket(s, cmd, argp);
}
extern "C" int __stdcall Shim_gethostname(char* name, int namelen) {
    return ::gethostname(name, namelen);
}
extern "C" struct hostent* __stdcall Shim_gethostbyname(const char* name) {
    return ::gethostbyname(name);
}
extern "C" unsigned long __stdcall Shim_inet_addr(const char* cp) {
    return ::inet_addr(cp);
}
extern "C" int __stdcall Shim_WSAGetLastError() { return ::WSAGetLastError(); }

}  // namespace xwr

// ===========================================================================
// Registration — under "ws2_32".
// ===========================================================================
REGISTER_SHIM("ws2_32", "WSAStartup", (FARPROC)&xwr::Shim_WSAStartup);
REGISTER_SHIM("ws2_32", "WSACleanup", (FARPROC)&xwr::Shim_WSACleanup);
REGISTER_SHIM("ws2_32", "socket", (FARPROC)&xwr::Shim_socket);
REGISTER_SHIM("ws2_32", "closesocket", (FARPROC)&xwr::Shim_closesocket);
REGISTER_SHIM("ws2_32", "bind", (FARPROC)&xwr::Shim_bind);
REGISTER_SHIM("ws2_32", "listen", (FARPROC)&xwr::Shim_listen);
REGISTER_SHIM("ws2_32", "accept", (FARPROC)&xwr::Shim_accept);
REGISTER_SHIM("ws2_32", "connect", (FARPROC)&xwr::Shim_connect);
REGISTER_SHIM("ws2_32", "send", (FARPROC)&xwr::Shim_send);
REGISTER_SHIM("ws2_32", "recv", (FARPROC)&xwr::Shim_recv);
REGISTER_SHIM("ws2_32", "setsockopt", (FARPROC)&xwr::Shim_setsockopt);
REGISTER_SHIM("ws2_32", "getsockopt", (FARPROC)&xwr::Shim_getsockopt);
REGISTER_SHIM("ws2_32", "ioctlsocket", (FARPROC)&xwr::Shim_ioctlsocket);
REGISTER_SHIM("ws2_32", "gethostname", (FARPROC)&xwr::Shim_gethostname);
REGISTER_SHIM("ws2_32", "gethostbyname", (FARPROC)&xwr::Shim_gethostbyname);
REGISTER_SHIM("ws2_32", "inet_addr", (FARPROC)&xwr::Shim_inet_addr);
REGISTER_SHIM("ws2_32", "WSAGetLastError", (FARPROC)&xwr::Shim_WSAGetLastError);
