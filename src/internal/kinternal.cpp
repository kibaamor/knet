#include "kinternal.h"
#include "../../include/knet/kaddress.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <iostream>

#ifndef _WIN32
#include <sys/ioctl.h>
#endif

namespace knet {

void on_fatal_error(int err, const char* apiname)
{
    char buf[10240] = {};

#ifdef _WIN32

    LPSTR msg = nullptr;
    const auto flag = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageA(flag, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, nullptr);

    snprintf(buf, sizeof(buf), "%s: (%d) %s", apiname, err, (nullptr != msg ? msg : "Unknown error"));

    if (nullptr != msg)
        LocalFree(msg);

#else

    snprintf(buf, sizeof(buf), "%s: (%d) %s", apiname, err, strerror(err));

#endif

    std::cerr << buf << std::endl;
    std::cerr.flush();
    abort();
}

bool set_rawsocket_bufsize(rawsocket_t rs, size_t size)
{
    auto s = static_cast<int>(size);
    constexpr auto n = static_cast<socklen_t>(sizeof(s));
    return set_rawsocket_opt(rs, SOL_SOCKET, SO_RCVBUF, &s, n)
        && set_rawsocket_opt(rs, SOL_SOCKET, SO_SNDBUF, &s, n);
}

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

#else

    rawsocket_t rs = INVALID_RAWSOCKET;

    do {
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)

        auto flag = type | SOCK_CLOEXEC;
        if (nonblock)
            flag |= SOCK_NONBLOCK;

        rs = ::socket(domain, flag, 0);
        if (INVALID_RAWSOCKET != rs)
            break;

        if (EINVAL != errno) {
            kdebug("socket() failed!");
            return rs;
        }

#endif

        rs = ::socket(domain, type, 0);
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

#ifdef SO_NOSIGPIPE
    if (INVALID_RAWSOCKET != rs) {
        int on = 1;
        if (!set_rawsocket_opt(rs, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on))) {
            kdebug("set_rawsocket_opt(SO_NOSIGPIPE) failed!");
            close_rawsocket(rs);
        }
    }
#endif

    return rs;
#endif
}

void close_rawsocket(rawsocket_t& rs)
{
    if (INVALID_RAWSOCKET == rs)
        return;

#ifdef _WIN32
    ::closesocket(rs);
#else
    ::close(rs);
#endif
    rs = INVALID_RAWSOCKET;
}

bool set_rawsocket_opt(rawsocket_t rs, int level, int optname,
    const void* optval, socklen_t optlen)
{
    auto val = static_cast<const char*>(optval);
    return RAWSOCKET_ERROR != ::setsockopt(rs, level, optname, val, optlen);
}

bool get_rawsocket_sockaddr(rawsocket_t rs, address& addr)
{
    auto ad = addr.as_ptr<sockaddr>();
    socklen_t len = sizeof(sockaddr_storage);
    return RAWSOCKET_ERROR != ::getsockname(rs, ad, &len);
}

bool get_rawsocket_peeraddr(rawsocket_t rs, address& addr)
{
    auto ad = addr.as_ptr<sockaddr>();
    socklen_t len = sizeof(sockaddr_storage);
    return RAWSOCKET_ERROR != ::getpeername(rs, ad, &len);
}

#ifndef _WIN32

bool set_rawsocket_nonblock(rawsocket_t rs)
{
    int set = 1;
    int ret = 0;

    do
        ret = ioctl(rs, FIONBIO, &set);
    while (RAWSOCKET_ERROR == ret && EINTR == errno);

    return 0 == ret;
}

bool set_rawsocket_cloexec(rawsocket_t rs)
{
    int ret = 0;

    do
        ret = ioctl(rs, FIOCLEX);
    while (RAWSOCKET_ERROR == ret && EINTR == errno);

    return 0 == ret;
}

#endif

#ifdef KNET_DEBUG

void kdebug_impl(const std::string& log, const char* file, int line)
{
#ifdef _WIN32
    const auto en = ::WSAGetLastError();
#else
    const auto en = errno;
#endif

    std::cerr << log
              << " errno:" << en
              << " file:" << file
              << " line:" << line
              << std::endl;
}

#endif

} // namespace knet
