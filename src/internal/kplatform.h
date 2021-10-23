#pragma once
#include "../../include/knet/knetfwd.h"

//-------------------------------------------------------------------------------------
// platform compat

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
#include <type_traits>

namespace knet {

using sa_family_t = ADDRESS_FAMILY;
using socklen_t = int;

constexpr rawsocket_t INVALID_RAWSOCKET = INVALID_SOCKET;
constexpr int RAWSOCKET_ERROR = SOCKET_ERROR;

static_assert(std::is_same<rawsocket_t, SOCKET>::value, "rawsocket_t and SOCKET must be the same type");

} // namespace knet

#else // !_WIN32

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define __debugbreak() __builtin_trap()
#define WSAGetLastError() errno

#ifdef __APPLE__
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

// https://android.googlesource.com/platform/system/core.git/+/refs/heads/master/libutils/include/utils/Compat.h
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({         \
    decltype (exp) _rc;                    \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif

namespace knet {

constexpr rawsocket_t INVALID_RAWSOCKET = -1;
constexpr int RAWSOCKET_ERROR = -1;

} // namespace knet

#endif // _WIN32

//-------------------------------------------------------------------------------------
// common headers
#include <string.h>
#include <array>

//-------------------------------------------------------------------------------------
// macros

// kassert macro
#ifdef KNET_ENABLE_ASSERT

#define kassert(cond)       \
    do {                    \
        if (!(cond)) {      \
            __debugbreak(); \
        }                   \
    } while (false)

#else // !KNET_ENABLE_ASSERT

#define kassert(cond)

#endif // KNET_ENABLE_ASSERT

// kdebug macro
#ifdef KNET_ENABLE_DEBUG_LOG

#define kdebug(log) kdebug_impl(log, __FILE__, __LINE__)

namespace knet {
inline void kdebug_impl(const char* log, const char* file, int line)
{
    const auto en = WSAGetLastError();
    std::cerr << log << " errno:" << en << " file:" << file << " line:" << line << "\n";
}
} // namespace knet

#else // !KNET_ENABLE_DEBUG_LOG

#define kdebug(log) // nothing

#endif // KNET_ENABLE_DEBUG_LOG
