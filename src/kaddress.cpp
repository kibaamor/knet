#include "../include/kaddress.h"
#include "internal/kinternal.h"
#include <cstring>

namespace knet {

bool address::pton(family_t fa, const std::string& addr, uint16_t port)
{
    memset(&_addr, 0, sizeof(_addr));

    if (family_t::Ipv4 == fa) {
        auto& addr4 = *reinterpret_cast<sockaddr_in*>(_addr);
        addr4.sin_family = AF_INET;
        addr4.sin_port = htons(port);
        auto sa = reinterpret_cast<void*>(&addr4.sin_addr);

        return 1 == ::inet_pton(addr4.sin_family, addr.c_str(), sa);
    } else if (family_t::Ipv6 == fa) {
        auto& addr6 = *reinterpret_cast<sockaddr_in6*>(_addr);
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(port);
        auto sa = reinterpret_cast<void*>(&addr6.sin6_addr);

        return 1 == ::inet_pton(addr6.sin6_family, addr.c_str(), sa);
    } else {
        auto& ad = *reinterpret_cast<sockaddr_storage*>(_addr);
        ad.ss_family = AF_UNSPEC;
    }

    return false;
}

bool address::ntop(std::string& addr, uint16_t& port) const
{
    addr.clear();
    port = 0;

    const auto fa = get_family();
    if (family_t::Ipv4 == fa) {
        char buf[INET_ADDRSTRLEN] = {};
        const auto& addr4 = reinterpret_cast<const sockaddr_in&>(_addr);
        auto csa = static_cast<const void*>(&addr4.sin_addr);
        auto sa = const_cast<void*>(csa);

        if (nullptr == ::inet_ntop(AF_INET, sa, buf, INET_ADDRSTRLEN))
            return false;

        addr = buf;
        port = ntohs(addr4.sin_port);
    } else if (family_t::Ipv6 == fa) {
        char buf[INET6_ADDRSTRLEN] = {};
        const auto& addr6 = reinterpret_cast<const sockaddr_in6&>(_addr);
        auto csa = static_cast<const void*>(&addr6.sin6_addr);
        auto sa = const_cast<void*>(csa);

        if (nullptr == ::inet_ntop(AF_INET6, sa, buf, INET6_ADDRSTRLEN))
            return false;

        addr = buf;
        port = ntohs(addr6.sin6_port);
    } else {
        return false;
    }

    return true;
}

const void* address::get_sockaddr() const
{
    return reinterpret_cast<const void*>(&_addr);
}

int address::get_rawfamily() const
{
    const auto addr = reinterpret_cast<const sockaddr_storage*>(_addr);
    return addr->ss_family;
}

family_t address::get_family() const
{
    switch (get_rawfamily()) {
    case AF_INET:
        return family_t::Ipv4;
    case AF_INET6:
        return family_t::Ipv6;
    default:
        break;
    }
    return family_t::Unknown;
}

int address::get_socklen() const
{
    switch (get_family()) {
    case family_t::Ipv4:
        return sizeof(sockaddr_in);
    case family_t::Ipv6:
        return sizeof(sockaddr_in6);
    default:
        break;
    }
    static_assert(sizeof(_addr) >= sizeof(sockaddr_storage), "");
    return sizeof(_addr);
}

std::string address::to_string() const
{
    std::string addr;
    uint16_t port;
    if (!ntop(addr, port))
        return "ntop() failed!";
    return addr + ":" + std::to_string(port);
}

std::ostream& operator<<(std::ostream& os, const address& addr)
{
    os << addr.to_string();
    return os;
}

} // namespace knet
