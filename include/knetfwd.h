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
    using rawsocket_t = SOCKET;
    using poller_t = HANDLE;
    using pollevent_t = OVERLAPPED_ENTRY;

    constexpr poller_t INVALID_POLLER = nullptr;
    constexpr rawsocket_t INVALID_RAWSOCKET = INVALID_SOCKET;
    constexpr int RAWSOCKET_ERROR = SOCKET_ERROR;
}

#else

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

//#define KNET_USE_EPOLL

#define closesocket(s) close(s)

namespace knet
{
    using rawsocket_t = int;
    using poller_t = int;
    using pollevent_t = epoll_event;

    constexpr poller_t INVALID_POLLER = -1;
    constexpr rawsocket_t INVALID_RAWSOCKET = -1;
    constexpr int RAWSOCKET_ERROR = -1;
}

#endif // _WIN32

#include <cstdint>
#include <string>

#ifdef _DEBUG
# ifdef _MSC_VER
#  define kassert(cond) do { if (!(cond)) __asm { int 3 }; } while (false)
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
        constexpr noncopyable() = default;
        ~noncopyable() = default;

        noncopyable(const noncopyable&) = delete;
        const noncopyable& operator= (const noncopyable&) = delete;
    };


    using socketid_t = int32_t;

    constexpr int IOCP_PENDING_ACCEPT_NUM = 64;
    constexpr int POLL_EVENT_NUM = 64;
    constexpr int SOCKET_SNDRCVBUF_SIZE = 64 * 1024;
    constexpr int SOCKET_RWBUF_SIZE = 256 * 1024;
    constexpr socketid_t INVALID_SOCKETID = 0;


    void global_init() noexcept;

    void set_rawsocket_sndrcvbufsize(rawsocket_t rawsocket, int size);

    uint32_t u32rand() noexcept;
    float f32rand() noexcept;
    uint32_t u32rand_between(uint32_t low, uint32_t high) noexcept;
    int32_t s32rand_between(int32_t low, int32_t high) noexcept;

    int64_t now_ms() noexcept;
    void sleep_ms(int64_t ms) noexcept;

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
#define KNET_GRACEFUL_CLOSE_SOCKET
#define KNET_REUSE_ADDR
