#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <knet/kaddress.h>
#include <iostream>

TEST_SUITE_BEGIN("address");

using namespace knet;

TEST_CASE("ipv4")
{
    const auto fa = family_t::Ipv4;
    const auto ip = "192.168.0.1";
    const uint16_t port = 8888;
    address addr;
    CHECK(addr.pton(fa, ip, port));
    CHECK_EQ(addr.get_family(), fa);

    std::string a;
    uint16_t p;
    CHECK(addr.ntop(a, p));
    CHECK_EQ(ip, a);
    CHECK_EQ(port, p);

    std::cout << fa << ": " << addr << std::endl;
}

TEST_CASE("ipv6")
{
    const auto fa = family_t::Ipv6;
    const auto ip = "fe80::d8bd:224e:a969:1f0d";
    const uint16_t port = 8888;
    address addr;
    CHECK(addr.pton(fa, ip, port));
    CHECK_EQ(addr.get_family(), fa);

    std::string a;
    uint16_t p;
    CHECK(addr.ntop(a, p));
    CHECK_EQ(ip, a);
    CHECK_EQ(port, p);

    std::cout << fa << ": " << addr << std::endl;
}

TEST_SUITE_END();
