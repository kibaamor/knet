#include "../include/knet/kaddress.h"
#include "internal/kinternal.h"
#include <cstring>

namespace knet {

bool address::resolve_all(const std::string& node_name, const std::string& service_name,
    family_t fa, std::vector<address>& addrs)
{
    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_family = get_rawfamily_by_family(fa);

    auto nn = node_name.c_str();
    if (node_name.empty()) {
        nn = nullptr;
        hints.ai_flags = AI_PASSIVE;
    }

    struct addrinfo* result = nullptr;
    do {
        const auto ret = ::getaddrinfo(nn, service_name.c_str(), &hints, &result);
        if (ret == EAI_AGAIN) {
            continue;
        } else if (0 != ret) {
            return false;
        }
    } while (false);

    for (auto rp = result; nullptr != rp; rp = rp->ai_next) {
        if (sizeof(address::_addr) >= rp->ai_addrlen) {
            addrs.emplace_back();
            ::memcpy(addrs.back()._addr, rp->ai_addr, rp->ai_addrlen);
        }
    }

    ::freeaddrinfo(result);

    return true;
}

bool address::resolve_one(const std::string& node_name, const std::string& service_name,
    family_t fa, address& addr)
{
    std::vector<address> addrs;
    if (!resolve_all(node_name, service_name, fa, addrs) || addrs.empty()) {
        return false;
    }
    addr = addrs.front();
    return true;
}

int address::get_rawfamily_by_family(family_t fa)
{
    if (fa == family_t::Ipv4) {
        return AF_INET;
    } else if (fa == family_t::Ipv6) {
        return AF_INET6;
    }
    return AF_UNSPEC;
}

family_t address::get_family_by_rawfamily(int rfa)
{
    if (rfa == AF_INET) {
        return family_t::Ipv4;
    } else if (rfa == AF_INET6) {
        return family_t::Ipv6;
    }
    return family_t::Unknown;
}

bool address::pton(family_t fa, const std::string& addr, uint16_t port)
{
    memset(&_addr, 0, sizeof(_addr));
    if (family_t::Ipv4 == fa) {
        auto addr4 = reinterpret_cast<sockaddr_in*>(_addr);
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        return 1 == ::inet_pton(addr4->sin_family, addr.c_str(), &addr4->sin_addr);
    } else if (family_t::Ipv6 == fa) {
        auto addr6 = reinterpret_cast<sockaddr_in6*>(_addr);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        return 1 == ::inet_pton(addr6->sin6_family, addr.c_str(), &addr6->sin6_addr);
    }
    return false;
}

bool address::ntop(std::string& addr, uint16_t& port) const
{
    addr.clear();
    port = 0;

    char buf[INET6_ADDRSTRLEN] = {};
    const auto fa = get_family();
    if (fa == family_t::Ipv4) {
        const auto addr4 = reinterpret_cast<const sockaddr_in*>(_addr);
        auto sa = const_cast<void*>(static_cast<const void*>(&addr4->sin_addr));
        if (!::inet_ntop(AF_INET, sa, buf, INET_ADDRSTRLEN))
            return false;

        addr = buf;
        port = ::ntohs(addr4->sin_port);
    } else if (fa == family_t::Ipv6) {
        auto addr6 = reinterpret_cast<const sockaddr_in6*>(_addr);
        auto sa = const_cast<void*>(static_cast<const void*>(&addr6->sin6_addr));
        if (!::inet_ntop(AF_INET6, sa, buf, INET6_ADDRSTRLEN))
            return false;

        addr = buf;
        port = ::ntohs(addr6->sin6_port);
    } else {
        return false;
    }
    return true;
}

int address::get_rawfamily() const
{
    return as_ptr<sockaddr_storage>()->ss_family;
}

int address::get_socklen() const
{
    const auto fa = get_family();
    if (fa == family_t::Ipv4) {
        return sizeof(sockaddr_in);
    } else if (fa == family_t::Ipv6) {
        return sizeof(sockaddr_in6);
    }
    static_assert(sizeof(_addr) >= sizeof(sockaddr_storage), "");
    return sizeof(_addr);
}

std::string address::to_string() const
{
    std::string addr;
    uint16_t port = 0;
    if (!ntop(addr, port))
        return "ntop() failed!";
    return addr + ":" + std::to_string(port);
}

} // namespace knet
