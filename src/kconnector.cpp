#include "../include/knet/kconnector.h"
#include "internal/ksocket_utils.h"

namespace knet {

connector::connector(workable& wkr)
    : _wkr(wkr)
{
}

connector::~connector() = default;

bool connector::connect(const address& addr, int timeout_ms)
{
    auto rs = create_rawsocket(addr.get_rawfamily());

    if (INVALID_RAWSOCKET == rs) {
        return false;
    }

    if (!::connect(rs, addr.as_ptr<sockaddr>(), addr.get_socklen())) {
        _wkr.add_work(rs);
        return true;
    }

#ifdef _WIN32
    if (WSAEWOULDBLOCK != WSAGetLastError()) {
#else // !_WIN32
    if (EINPROGRESS != errno) {
#endif // _WIN32
        kdebug("connect() failed!");
        close_rawsocket(rs);
        return false;
    }

    fd_set ws;
    FD_ZERO(&ws);
    FD_SET(rs, &ws);

    timeval tv, *ptv = nullptr;
    if (timeout_ms >= 0) {
        ptv = &tv;

        tv.tv_sec = timeout_ms / 1000;
        timeout_ms %= 1000;
        tv.tv_usec = timeout_ms * 1000;
    }

    do {
        const auto ret = TEMP_FAILURE_RETRY(select(static_cast<int>(rs) + 1, nullptr, &ws, nullptr, ptv));
        if (RAWSOCKET_ERROR == ret) {
            kdebug("select() failed!");
            break;
        } else if (!ret) {
            kdebug("select() timeout!");
            break;
        }

        int opt = 0;
        socklen_t len = sizeof(opt);
        if (getsockopt(rs, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&opt), &len)) {
            kdebug("getsockopt(SO_ERROR) failed!");
            break;
        }
        if (opt) {
            errno = opt;
            kdebug("connect() failed!");
            break;
        }

        _wkr.add_work(rs);
        return true;
    } while (false);

    close_rawsocket(rs);
    return false;
}

} // namespace knet
