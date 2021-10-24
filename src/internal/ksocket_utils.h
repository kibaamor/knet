#pragma once
#include "../../include/knet/kaddress.h"
#include "kplatform.h"

namespace knet {

#ifndef _WIN32

inline bool set_rawsocket_nonblock(rawsocket_t rs)
{
    int set = 1;
    return RAWSOCKET_ERROR != ioctl(rs, FIONBIO, &set);
}

inline bool set_rawsocket_cloexec(rawsocket_t rs)
{
    return RAWSOCKET_ERROR != ioctl(rs, FIOCLEX);
}

#endif // !_WIN32

inline size_t buffer_total_size(const buffer* buf, size_t num)
{
    size_t total_size = 0;
    for (size_t i = 0; i < num; ++i) {
        kassert(buf[i].data && buf[i].size);
        total_size += buf[i].size;
    }
    return total_size;
}

inline bool set_rawsocket_opt(rawsocket_t rs, int level, int optname,
    const void* optval, socklen_t optlen)
{
    const auto val = static_cast<const char*>(optval);
    return RAWSOCKET_ERROR != setsockopt(rs, level, optname, val, optlen);
}

inline bool set_rawsocket_bufsize(rawsocket_t rs, size_t size)
{
    const auto s = static_cast<int>(size);
    constexpr auto n = static_cast<socklen_t>(sizeof(s));
    return set_rawsocket_opt(rs, SOL_SOCKET, SO_RCVBUF, &s, n)
        && set_rawsocket_opt(rs, SOL_SOCKET, SO_SNDBUF, &s, n);
}

inline bool set_rawsocket_reuseaddr(rawsocket_t rs)
{
    int on = 1;
    return set_rawsocket_opt(rs, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

inline bool get_rawsocket_sockaddr(rawsocket_t rs, address& addr)
{
    socklen_t len = sizeof(sockaddr_storage);
    return RAWSOCKET_ERROR != getsockname(rs, addr.as_ptr<sockaddr>(), &len);
}

inline bool get_rawsocket_peeraddr(rawsocket_t rs, address& addr)
{
    socklen_t len = sizeof(sockaddr_storage);
    return RAWSOCKET_ERROR != getpeername(rs, addr.as_ptr<sockaddr>(), &len);
}

inline void close_rawsocket(rawsocket_t& rs)
{
    if (INVALID_RAWSOCKET != rs) {
#ifdef _WIN32
        closesocket(rs);
#else
        close(rs);
#endif
        rs = INVALID_RAWSOCKET;
    }
}

rawsocket_t create_rawsocket(int domain, bool nonblock);
bool rawsocket_recv(rawsocket_t rs, void* buf, size_t num, size_t& used);
bool rawsocket_sendv(rawsocket_t rs, const buffer* buf, size_t num, size_t& used);

} // namespace knet
