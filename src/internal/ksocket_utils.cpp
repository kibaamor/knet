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

#ifdef SO_NOSIGPIPE
    if (INVALID_RAWSOCKET != rs) {
        int on = 1;
        if (!set_rawsocket_opt(rs, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on))) {
            kdebug("set_rawsocket_opt(SO_NOSIGPIPE) failed!");
            close_rawsocket(rs);
        }
    }
#endif // SO_NOSIGPIPE

    return rs;
#endif // _WIN32
}

} // namespace knet
