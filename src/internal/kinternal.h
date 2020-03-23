#pragma once
#include "../include/knetfwd.h"
#include <cstring>

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Win7
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <ws2tcpip.h>
#include <mswsock.h>
#include <intrin.h>

namespace knet {

using sa_family_t = ADDRESS_FAMILY;
using in_port_t = USHORT;
using socklen_t = int;

constexpr rawsocket_t INVALID_RAWSOCKET = INVALID_SOCKET;
constexpr int RAWSOCKET_ERROR = SOCKET_ERROR;

} // namespace knet

#else

#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef __linux__
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif

#define __debugbreak __builtin_trap

namespace knet {

constexpr rawsocket_t INVALID_RAWSOCKET = -1;
constexpr int RAWSOCKET_ERROR = -1;

} // namespace knet

#endif // _WIN32

#ifdef KNET_DEBUG
#define kassert(cond)       \
    do {                    \
        if (!(cond))        \
            __debugbreak(); \
    } while (false)
#else
#define kassert(cond)
#endif

namespace knet {

constexpr int IOCP_PENDING_ACCEPT_NUM = 64;
constexpr int POLL_EVENT_NUM = 128;
constexpr int SOCKET_RWBUF_SIZE = 256 * 1024;

void on_fatal_error(int err, const char* apiname);

bool set_rawsocket_bufsize(rawsocket_t rs, size_t size);

rawsocket_t create_rawsocket(int domain, bool nonblock);
void close_rawsocket(rawsocket_t& rs);

bool set_rawsocket_opt(rawsocket_t rs, int level, int optname,
    const void* optval, socklen_t optlen);

#ifndef _WIN32
bool set_rawsocket_nonblock(rawsocket_t rs);
bool set_rawsocket_cloexec(rawsocket_t rs);
#endif

#ifdef KNET_DEBUG
void kdebug_impl(const std::string& log, const char* file, int line);
#define kdebug(log) kdebug_impl(log, __FILE__, __LINE__)
#else
#define kdebug(log)
#endif

} // namespace knet
