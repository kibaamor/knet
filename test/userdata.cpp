#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <ksocket.h>
#include <cfloat>
#include <iostream>

TEST_SUITE_BEGIN("userdata");

using namespace knet;

TEST_CASE("time")
{
    auto now = now_ms();
    std::cout << "localtime:" << tm2str<>(ms2tm(now, true)) << std::endl;
    std::cout << "gmtime:" << tm2str<>(ms2tm(now, false)) << std::endl;
    std::cout << "localtime:" << tm2str<64>(ms2tm(now, true)) << std::endl;
    std::cout << "gmtime:" << tm2str<64>(ms2tm(now, false)) << std::endl;
    std::cout << tm2str<64>(ms2tm(now, true), "localtime: %Y/%m/%d %H:%M:%S") << std::endl;
    std::cout << tm2str<64>(ms2tm(now, false), "gmtime: %Y/%m/%d %H:%M:%S") << std::endl;
}

TEST_CASE("pointer")
{
#define KNET_TEST_POINTER_HELPER(TYPE, NAME)            \
    SUBCASE(NAME)                                       \
    {                                                   \
        TYPE v;                                         \
        auto pv = &v;                                   \
        userdata ud(pv);                                \
        CHECK(userdata::pointer == ud.type);            \
        CHECK(ud.data.ptr == pv);                       \
    }

    KNET_TEST_POINTER_HELPER(std::vector<int>, "std::vector<int>");
    KNET_TEST_POINTER_HELPER(bool, "bool");
    KNET_TEST_POINTER_HELPER(int8_t, "int8_t");
    KNET_TEST_POINTER_HELPER(uint8_t, "uint8_t");
    KNET_TEST_POINTER_HELPER(int16_t, "int16_t");
    KNET_TEST_POINTER_HELPER(uint16_t, "uint16_t");
    KNET_TEST_POINTER_HELPER(int32_t, "int32_t");
    KNET_TEST_POINTER_HELPER(uint32_t, "uint32_t");
    KNET_TEST_POINTER_HELPER(int64_t, "int64_t");
    KNET_TEST_POINTER_HELPER(uint64_t, "uint64_t");
    KNET_TEST_POINTER_HELPER(float, "float");
    KNET_TEST_POINTER_HELPER(double, "double");
    KNET_TEST_POINTER_HELPER(long double, "long double");
}

TEST_CASE("floatpoint")
{
#define KNET_TEST_FLOATPOINT_HELPER(TYPE, NAME, VALUE)                  \
    SUBCASE(NAME)                                                       \
    {                                                                   \
        TYPE v = VALUE;                                                 \
        userdata ud(v);                                                 \
        CHECK(userdata::floatpoint == ud.type);                         \
        CHECK(ud.data.f64 == static_cast<decltype(ud.data.f64)>(v));    \
    }
    KNET_TEST_FLOATPOINT_HELPER(float, "float", 123.456f);
    KNET_TEST_FLOATPOINT_HELPER(float, "float min", FLT_MIN);
    KNET_TEST_FLOATPOINT_HELPER(float, "float max", FLT_MAX);
    KNET_TEST_FLOATPOINT_HELPER(double, "double", 456.789);
    KNET_TEST_FLOATPOINT_HELPER(double, "double min", DBL_MIN);
    KNET_TEST_FLOATPOINT_HELPER(double, "double max", DBL_MAX);
}

TEST_CASE("integral")
{
#define KNET_TEST_INTERGRAL_HELPER(TYPE, NAME, VALUE)                   \
    SUBCASE(NAME)                                                       \
    {                                                                   \
        TYPE v = VALUE;                                                 \
        userdata ud(v);                                                 \
        CHECK(userdata::integral == ud.type);                           \
        CHECK(ud.data.i64 == static_cast<decltype(ud.data.i64)>(v));    \
    }

    KNET_TEST_INTERGRAL_HELPER(bool, "bool", true);
    KNET_TEST_INTERGRAL_HELPER(bool, "bool", false);
    KNET_TEST_INTERGRAL_HELPER(int8_t, "int8_t", 0x7f);
    KNET_TEST_INTERGRAL_HELPER(uint8_t, "uint8_t", 0xff);
    KNET_TEST_INTERGRAL_HELPER(int16_t, "int16_t", 0x7fff);
    KNET_TEST_INTERGRAL_HELPER(uint16_t, "uint16_t", 0xffff);
    KNET_TEST_INTERGRAL_HELPER(int32_t, "int32_t", 0xffffffff);
    KNET_TEST_INTERGRAL_HELPER(uint32_t, "uint32_t", 0xffffffff);
    KNET_TEST_INTERGRAL_HELPER(int64_t, "int64_t", 0xffffffffffffffff);
    KNET_TEST_INTERGRAL_HELPER(uint64_t, "uint64_t", 0xffffffffffffffff);
}

TEST_SUITE_END();
