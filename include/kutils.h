#pragma once
#include "knetfwd.h"
#include <ctime>

namespace knet {

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
    const auto len = ::strftime(buf, sizeof(buf), fmt, &t);
    return len > 0 ? std::string(buf, buf + len) : std::string();
}

} // namespace knet