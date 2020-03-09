#pragma once
#include "knetfwd.h"

namespace knet {
class address {
public:
    bool pton(sa_family_t family, const std::string& addr, in_port_t port);
    bool ntop(std::string& addr, in_port_t& port) const;

    std::string to_string() const;

    sa_family_t get_family() const { return _addr.ss_family; }
    const sockaddr* get_sockaddr() const;
    socklen_t get_socklen() const;

private:
    sockaddr_storage _addr = {};
};

std::ostream& operator<<(std::ostream& os, const address& addr);
std::istream& operator>>(std::istream& is, address& addr);
} // namespace knet

namespace std {
inline string to_string(const knet::address& addr)
{
    return addr.to_string();
}
} // namespace std
