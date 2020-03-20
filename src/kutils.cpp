#include "../include/kutils.h"
#include "kinternal.h"

namespace {

class AutoInit {
public:
    AutoInit()
    {
#ifdef _WIN32
        WSADATA wsadata;
        WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
    }
};

AutoInit g_ai;

} // namespace

namespace knet {

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
#else // _WIN32
    timeval now;
    gettimeofday(&now, nullptr);
    return static_cast<int64_t>(now.tv_sec * 1000 + now.tv_usec / 1000);
#endif // _WIN32
}

void sleep_ms(int64_t ms)
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

} // namespace knet
