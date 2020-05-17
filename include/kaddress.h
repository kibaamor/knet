#pragma once
#include "knetfwd.h"
#include <iostream>
#include <vector>

namespace knet {

enum class family_t : int {
    Unknown = 0,
    Ipv4,
    Ipv6,
};

class address final {
public:
    static bool resolve_all(const std::string& node_name, const std::string& service_name,
        std::vector<address>& addrs);
    static bool resolve_one(const std::string& node_name, const std::string& service_name,
        family_t fa, address& addr);

public:
    bool pton(family_t fa, const std::string& addr, uint16_t port);
    bool ntop(std::string& addr, uint16_t& port) const;

    template <typename T>
    T* as_ptr()
    {
        static_assert(sizeof(_addr) >= sizeof(T), "invalid address type size");
        return reinterpret_cast<T*>(_addr);
    }

    template <typename T>
    const T* as_ptr() const
    {
        static_assert(sizeof(_addr) >= sizeof(T), "invalid address type size");
        return reinterpret_cast<const T*>(_addr);
    }

    int get_rawfamily() const;
    family_t get_family() const;
    int get_socklen() const;
    std::string to_string() const;

private:
    char _addr[256] = {};
};

} // namespace knet

namespace std {

inline string to_string(knet::family_t fa)
{
    switch (fa) {
    case knet::family_t::Ipv4:
        return "Ipv4";
    case knet::family_t::Ipv6:
        return "Ipv6";
    default:
        break;
    }
    return "Unknown";
}

inline string to_string(const knet::address& addr)
{
    return addr.to_string();
}

} // namespace std

namespace knet {

inline std::ostream& operator<<(std::ostream& os, family_t fa)
{
    return os << std::to_string(fa);
}

inline std::ostream& operator<<(std::ostream& os, const address& addr)
{
    return os << std::to_string(addr);
}

} // namespace knet
