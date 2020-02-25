#include "../include/knetfwd.h"
#include <random>
#include <ctime>
#ifndef _WIN32
#include <sys/time.h>
#endif


namespace
{
    std::default_random_engine& get_random_engine()
    {
        static thread_local std::default_random_engine re{ std::random_device()() };
        return re;
    }
}

namespace knet
{
    void global_init() noexcept
    {
#ifdef _WIN32
        WSADATA wsadata;
        (void)WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
        get_random_engine().seed(static_cast<uint32_t>(time(nullptr)));
    }

    void set_rawsocket_sndrcvbufsize(rawsocket_t rawsocket, int size)
    {
        auto optval = reinterpret_cast<const char*>(&size);
        setsockopt(rawsocket, SOL_SOCKET, SO_RCVBUF, optval, sizeof(size));
        setsockopt(rawsocket, SOL_SOCKET, SO_SNDBUF, optval, sizeof(size));
    }

    uint32_t u32rand() noexcept
    {
        return get_random_engine()();
    }

    float f32rand() noexcept
    {
        std::uniform_real_distribution<float> distribution(0.f, 1.f);
        return distribution(get_random_engine());
    }

    uint32_t u32rand_between(uint32_t low, uint32_t high) noexcept
    {
        std::uniform_int_distribution<uint32_t> distribution(low, high);
        return distribution(get_random_engine());
    }

    int32_t s32rand_between(int32_t low, int32_t high) noexcept
    {
        std::uniform_int_distribution<int32_t> distribution(low, high);
        return distribution(get_random_engine());
    }

    int64_t now_ms() noexcept
    {
#ifdef _WIN32
        static thread_local FILETIME ft;
        static thread_local ULARGE_INTEGER ui;
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8                   0x0602
#endif
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        GetSystemTimePreciseAsFileTime(&ft);
#else
        GetSystemTimeAsFileTime(&ft);
#endif
        ui.LowPart = ft.dwLowDateTime;
        ui.HighPart = ft.dwHighDateTime;
        return static_cast<int64_t>((ui.QuadPart - 116444736000000000) / 10000);
#else // _WIN32
        timeval now;
        gettimeofday(&now, nullptr);
        return static_cast<int64_t>(now.tv_sec * 1000 + now.tv_usec / 1000);
#endif // _WIN32
    }

    void sleep_ms(int64_t ms) noexcept
    {
#ifdef _WIN32
        Sleep(static_cast<DWORD>(ms));
#else
        usleep(static_cast<useconds_t>(ms * 1000));
#endif
    }

    struct tm ms2tm(int64_t ms, bool local)
    {
        time_t tt = ms / 1000;
        struct tm t;
#ifdef _WIN32
        local ? localtime_s(&t, &tt) : gmtime_s(&t, &tt);
#else
        local ? localtime_r(&tt, &t) : gmtime_r(&tt, &t);
#endif
        return t;
    }
}
