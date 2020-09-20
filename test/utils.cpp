#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <knet/kutils.h>
#include <iostream>

TEST_SUITE_BEGIN("utils");

using namespace knet;

TEST_CASE("time")
{
    auto ts = 1598589292197;
    const auto gmfmt = "gmtime: %Y/%m/%d %H:%M:%S";
    const auto gmstr = "gmtime: 2020/08/28 04:34:52";
    CHECK_EQ(tm2str<64>(ms2tm(ts, false), gmfmt), gmstr);
    std::cout << "localtime:" << tm2str<64>(ms2tm(ts, true)) << std::endl;
    std::cout << "gmtime:" << tm2str<64>(ms2tm(ts, false)) << std::endl;
}

TEST_CASE("rand")
{
    auto low = u32rand();
    auto high = u32rand();
    if (low > high)
        std::swap(low, high);

    for (int i = 0; i < 10; ++i) {
        const auto nounce = u32rand_between(low, high);
        CHECK_GE(nounce, low);
        CHECK_LE(nounce, high);
    }
}

TEST_SUITE_END();
