#pragma once
#include "knetfwd.h"
#include <iostream>

namespace knet {

enum class family_t : int {
    Unknown = 0,
    Ipv4,
    Ipv6,
};

class address final {
public:
    bool pton(family_t fa, const std::string& addr, uint16_t port);
    bool ntop(std::string& addr, uint16_t& port) const;

    int get_rawfamily() const;
    family_t get_family() const;
    const void* get_sockaddr() const;
    int get_socklen() const;

    std::string to_string() const;

private:
    char _addr[256] = {};
};

std::ostream& operator<<(std::ostream& os, const address& addr);

} // namespace knet

namespace std {

inline string to_string(const knet::address& addr)
{
    return addr.to_string();
}

} // namespace std
