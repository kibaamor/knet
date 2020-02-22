#pragma once
#include "knetfwd.h"


namespace knet
{
    class address
    {
    public:
        bool pton(sa_family_t family, const std::string& addr, in_port_t port) noexcept;
        bool ntop(std::string& addr, in_port_t& port) const noexcept;

        std::string to_string() const;

        sa_family_t get_family() const noexcept { return _addr.ss_family; }
        sockaddr_storage& get_sockaddr() noexcept { return _addr; }
        const sockaddr_storage& get_sockaddr() const noexcept { return _addr; }

    private:
        sockaddr_storage _addr = {};
    };

    std::ostream& operator<<(std::ostream& os, const address& addr);
    std::istream& operator>>(std::istream& is, address& addr);
}

namespace std
{
    inline string to_string(const knet::address& addr)
    {
        return addr.to_string();
    }
}
