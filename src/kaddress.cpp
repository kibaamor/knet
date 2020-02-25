#include "../include/kaddress.h"
#include <cstring>
#include <iostream>


namespace knet
{
    bool address::pton(sa_family_t family, const std::string& addr, in_port_t port) noexcept
    {
        memset(&_addr, 0, sizeof(_addr));

        if (AF_INET == family)
        {
            auto& addr4 = reinterpret_cast<sockaddr_in&>(_addr);
            addr4.sin_family = family;
            addr4.sin_port = htons(port);
            auto sa = reinterpret_cast<void*>(&addr4.sin_addr);

            return 1 == inet_pton(family, addr.c_str(), sa);
        }
        else if (AF_INET6 == family)
        {
            auto& addr6 = reinterpret_cast<sockaddr_in6&>(_addr);
            addr6.sin6_family = family;
            addr6.sin6_port = htons(port);
            auto sa = reinterpret_cast<void*>(&addr6.sin6_addr);

            return 1 == inet_pton(family, addr.c_str(), sa);
        }
        else
        {
            _addr.ss_family = AF_UNSPEC;
        }

        return false;
    }

    bool address::ntop(std::string& addr, in_port_t& port) const noexcept
    {
        addr.clear();
        port = 0;

        if (AF_INET == get_family())
        {
            char buf[INET_ADDRSTRLEN] = {};
            const auto& addr4 = reinterpret_cast<const sockaddr_in&>(_addr);
            auto sa = static_cast<const void*>(&addr4.sin_addr);

            if (nullptr == inet_ntop(get_family(), sa, buf, INET_ADDRSTRLEN))
                return false;

            addr = buf;
            port = ntohs(addr4.sin_port);
        }
        else if (AF_INET6 == get_family())
        {
            char buf[INET6_ADDRSTRLEN] = {};
            const auto& addr6 = reinterpret_cast<const sockaddr_in6&>(_addr);
            auto sa = static_cast<const void*>(&addr6.sin6_addr);

            if (nullptr == inet_ntop(get_family(), sa, buf, INET6_ADDRSTRLEN))
                return false;

            addr = buf;
            port = ntohs(addr6.sin6_port);
        }
        else
        {
            return false;
        }

        return true;
    }

    std::string address::to_string() const
    {
        std::string addr;
        in_port_t port;
        ntop(addr, port);
        return addr + ":" + std::to_string(port);
    }

    std::ostream& operator<<(std::ostream& os, const address& addr)
    {
        os << addr.to_string();
        return os;
    }

    std::istream& operator>>(std::istream& is, address& addr)
    {
        sa_family_t family;
        std::string ar;
        in_port_t port;
        is >> family >> ar >> port;
        addr.pton(family, ar, port);
        return is;
    }
}

