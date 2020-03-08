#pragma once

#ifdef _WIN32

#ifndef _WIN32_WINNT
# define _WIN32_WINNT   0x0600 // Win7
#endif

#ifndef NOMINMAX
# define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

#include <ws2tcpip.h>
#include <mswsock.h>

#define KNET_USE_IOCP

namespace knet
{
    using sa_family_t = ADDRESS_FAMILY;
    using in_port_t = USHORT;
    using socklen_t = int;
    using rawsocket_t = SOCKET;
    using rawpoller_t = HANDLE;
    using rawpollevent_t = OVERLAPPED_ENTRY;

    constexpr rawpoller_t INVALID_RAWPOLLER = nullptr;
    constexpr rawsocket_t INVALID_RAWSOCKET = INVALID_SOCKET;
    constexpr int RAWSOCKET_ERROR = SOCKET_ERROR;
}

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
# include <sys/epoll.h>
# define KNET_USE_EPOLL
#else
# include <sys/event.h>
# define KNET_USE_KQUEUE
#endif

#define closesocket(s) close(s)

namespace knet
{
    using rawsocket_t = int;
    using rawpoller_t = int;
#ifdef __linux__
    using rawpollevent_t = struct epoll_event;
#else
    using rawpollevent_t = struct kevent;
#endif

    constexpr rawpoller_t INVALID_RAWPOLLER = -1;
    constexpr rawsocket_t INVALID_RAWSOCKET = -1;
    constexpr int RAWSOCKET_ERROR = -1;
}

#endif // _WIN32

#include <cstdint>
#include <string>

#ifdef _DEBUG
# ifdef _MSC_VER
#  include <intrin.h>
#  define kassert(cond) do { if (!(cond)) __debugbreak(); } while (false)
# else
#  include <cassert>
#  define kassert assert
# endif
#else
# define kassert(cond)
#endif

namespace knet
{
    class noncopyable
    {
    protected:
        noncopyable() = default;
        ~noncopyable() = default;

        noncopyable(const noncopyable&) = delete;
        const noncopyable& operator= (const noncopyable&) = delete;
    };


    constexpr int IOCP_PENDING_ACCEPT_NUM = 64;
    constexpr int POLL_EVENT_NUM = 128;
    constexpr int SOCKET_RWBUF_SIZE = 256 * 1024;


    void global_init();

    bool set_rawsocket_bufsize(rawsocket_t rs, int size);

    uint32_t u32rand();
    float f32rand();
    uint32_t u32rand_between(uint32_t low, uint32_t high);
    int32_t s32rand_between(int32_t low, int32_t high);

    int64_t now_ms();
    void sleep_ms(int64_t ms);

    struct tm ms2tm(int64_t ms, bool local);

    template <unsigned int N = 32>
    inline std::string tm2str(const struct tm& t, const char* fmt = "%Y/%m/%d %H:%M:%S")
    {
        char buf[N] = {};
        const auto len = strftime(buf, sizeof(buf), fmt, &t);
        return len > 0 ? std::string(buf, buf + len) : std::string();
    }
}

// config
