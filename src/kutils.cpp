#include "../include/knet/kutils.h"
#include "internal/kplatform.h"
#ifndef _WIN32
#include <sys/resource.h>
#include <sys/signal.h>
#endif
#include <random>

namespace {

class AutoInit {
public:
    AutoInit()
    {
        init_netlib();
        setup_rlimit();
        setup_signal();
    }

    void init_netlib()
    {
#ifdef _WIN32
        WSADATA wsadata;
        WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif // _WIN32
    }

    void setup_rlimit()
    {
#ifndef _WIN32
        struct rlimit rt = {};
        if (!getrlimit(RLIMIT_NOFILE, &rt)) {
            rt.rlim_cur = rt.rlim_max;

#ifdef __APPLE__
            if (rt.rlim_cur > OPEN_MAX) {
                rt.rlim_cur = OPEN_MAX;
            }
#endif // __APPLE__

            setrlimit(RLIMIT_NOFILE, &rt);
        }
#endif // !_WIN32
    }

    void setup_signal()
    {
#ifndef _WIN32
        signal(SIGPIPE, SIG_IGN);
#endif // !_WIN32
    }
}; // namespace

AutoInit g_ai;

std::default_random_engine& get_random_engine()
{
    static thread_local std::default_random_engine re { std::random_device()() };
    return re;
}

} // namespace

namespace knet {

uint32_t u32rand()
{
    return get_random_engine()();
}

float f32rand()
{
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(get_random_engine());
}

uint32_t u32rand_between(uint32_t low, uint32_t high)
{
    std::uniform_int_distribution<uint32_t> dist(low, high);
    return dist(get_random_engine());
}

int32_t s32rand_between(int32_t low, int32_t high)
{
    std::uniform_int_distribution<int32_t> dist(low, high);
    return dist(get_random_engine());
}

int64_t now_ms()
{
#ifdef _WIN32
    static thread_local FILETIME ft;
    static thread_local ULARGE_INTEGER ui;

#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    GetSystemTimePreciseAsFileTime(&ft);
#else
    GetSystemTimeAsFileTime(&ft);
#endif

    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    return static_cast<int64_t>((ui.QuadPart - 116444736000000000) / 10000);

#else // !_WIN32

    timeval now;
    gettimeofday(&now, nullptr);
    return static_cast<int64_t>(now.tv_sec * 1000 + now.tv_usec / 1000);

#endif // _WIN32
}

void sleep_ms(int64_t ms)
{
#ifdef _WIN32
    Sleep(static_cast<DWORD>(ms));
#else // !_WIN32
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms - ts.tv_sec * 1000) * 1000ul * 1000ul;
    nanosleep(&ts, nullptr);
#endif // _WIN32
}

struct tm ms2tm(int64_t ms, bool local)
{
    time_t tt = ms / 1000;
    struct tm t;
#ifdef _WIN32
    local ? localtime_s(&t, &tt) : gmtime_s(&t, &tt);
#else // !_WIN32
    local ? localtime_r(&tt, &t) : gmtime_r(&tt, &t);
#endif // _WIN32
    return t;
}

} // namespace knet
