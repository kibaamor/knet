#pragma once
#include "../../include/knet/kaddress.h"
#include "kplatform.h"

namespace knet {

inline bool set_rawsocket_nonblock(rawsocket_t rs)
{
    unsigned long set = 1;
    return RAWSOCKET_ERROR != ioctl(rs, FIONBIO, &set);
}

inline bool set_rawsocket_cloexec(rawsocket_t rs)
{
#ifdef _WIN32
    return SetHandleInformation(reinterpret_cast<HANDLE>(rs), HANDLE_FLAG_INHERIT, 0);
#else // !_WIN32
    return RAWSOCKET_ERROR != ioctl(rs, FIOCLEX);
#endif // _WIN32
}

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

inline rawsocket_t create_rawsocket(int domain)
{
    auto rs = ::socket(domain, SOCK_STREAM, 0);
    do {
        if (INVALID_RAWSOCKET == rs) {
            kdebug("socket() failed!");
            break;
        }
        if (!set_rawsocket_nonblock(rs)) {
            kdebug("set_rawsocket_nonblock() failed!");
            break;
        }
        if (!set_rawsocket_cloexec(rs)) {
            kdebug("set_rawsocket_cloexec() failed!");
            break;
        }

        return rs;
    } while (false);

    close_rawsocket(rs);
    return rs;
}

inline bool rawsocket_recv(rawsocket_t rs, void* buf, size_t num, size_t& used)
{
    used = 0;

#ifdef _WIN32
    const int n = recv(rs, static_cast<char*>(buf), static_cast<int>(num), 0);
    if (n > 0) {
        used = static_cast<size_t>(n);
        return true;
    } else if (!n) {
        return false;
    } else {
        return WSAEWOULDBLOCK == WSAGetLastError();
    }
#else // !_WIN32
    ssize_t n = TEMP_FAILURE_RETRY(recv(rs, buf, num, 0));
    if (n > 0) {
        used = static_cast<size_t>(n);
        return true;
    } else if (!n) {
        return false;
    } else {
        return EAGAIN == errno || EWOULDBLOCK == errno;
    }
#endif // _WIN32
}

inline bool rawsocket_sendv(rawsocket_t rs, const buffer* buf, size_t num, size_t& used)
{
    used = 0;

#ifdef _WIN32
    static_assert(sizeof(buffer) == sizeof(WSABUF), "buffer and WSABUF must be same size");
    static_assert(offsetof(buffer, data) == offsetof(WSABUF, buf),
        "buffer and WSABUF must be same memory layout for data field");
    static_assert(offsetof(buffer, size) == offsetof(WSABUF, len),
        "buffer and WSABUF must be same memory layout for size field");

    DWORD n = 0;
    WSABUF* b = const_cast<WSABUF*>(reinterpret_cast<const WSABUF*>(buf));
    if (!WSASend(rs, b, static_cast<DWORD>(num), &n, 0, nullptr, nullptr)) {
        used = static_cast<size_t>(n);
        return true;
    } else {
        return WSAEWOULDBLOCK == WSAGetLastError();
    }
#else // !_WIN32
    static_assert(sizeof(buffer) == sizeof(iovec), "buffer and iovec must be same size");
    static_assert(offsetof(buffer, data) == offsetof(iovec, iov_base),
        "buffer and iovec must be same memory layout for data field");
    static_assert(offsetof(buffer, size) == offsetof(iovec, iov_len),
        "buffer and iovec must be same memory layout for size field");

    const ssize_t n = TEMP_FAILURE_RETRY(writev(rs, reinterpret_cast<const iovec*>(buf), num));
    if (n >= 0) {
        used = static_cast<size_t>(n);
        return true;
    } else {
        return EAGAIN == errno || EWOULDBLOCK == errno;
    }
#endif // _WIN32
}

} // namespace knet
