#include "ksocket_utils.h"

namespace knet {

rawsocket_t create_rawsocket(int domain, bool nonblock)
{
    constexpr int type = SOCK_STREAM;

#ifdef _WIN32

    auto rs = WSASocketW(domain, type, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_RAWSOCKET != rs) {
        auto h = reinterpret_cast<HANDLE>(rs);
        if (!SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0)) {
            kdebug("SetHandleInformation() failed!");
            close_rawsocket(rs);
        }
    } else {
        kdebug("WSASocketW() failed!");
    }
    return rs;

#else // !_WIN32

    rawsocket_t rs = INVALID_RAWSOCKET;

    do {
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
        const auto flag = type | SOCK_CLOEXEC | (nonblock ? SOCK_NONBLOCK : 0);
        rs = socket(domain, flag, 0);
        if (INVALID_RAWSOCKET != rs) {
            break;
        }

        if (EINVAL != errno) {
            kdebug("socket() failed!");
            return rs;
        }
#endif // SOCK_NONBLOCK && SOCK_CLOEXEC
        rs = socket(domain, type, 0);
        if (INVALID_RAWSOCKET == rs) {
            kdebug("socket() failed!");
            break;
        }

        if (!set_rawsocket_cloexec(rs)) {
            kdebug("set_rawsocket_cloexec() failed!");
            close_rawsocket(rs);
            break;
        }
        if (nonblock && !set_rawsocket_nonblock(rs)) {
            kdebug("set_rawsocket_nonblock() failed!");
            close_rawsocket(rs);
            break;
        }
    } while (false);

    return rs;
#endif // _WIN32
}

bool rawsocket_recv(rawsocket_t rs, void* buf, size_t num, size_t& used)
{
    used = 0;

#ifdef _WIN32

    const int n = recv(rs, buf, num, 0);
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

bool rawsocket_sendv(rawsocket_t rs, const buffer* buf, size_t num, size_t& used)
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
    if (!WSASend(rs, b, num, &n, 0, nullptr, nullptr)) {
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
