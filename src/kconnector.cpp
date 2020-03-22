#include "../include/kconnector.h"
#include "internal/kinternal.h"

namespace knet {

connector::connector(workable& wkr)
    : _wkr(wkr)
{
}

connector::~connector() = default;

bool connector::connect(const address& addr)
{
    auto rs = create_rawsocket(addr.get_rawfamily(), SOCK_STREAM, false);
    if (INVALID_RAWSOCKET == rs)
        return false;

    const auto sa = static_cast<const sockaddr*>(addr.get_sockaddr());
    const auto salen = addr.get_socklen();
    if (-1 != ::connect(rs, sa, salen)
#ifndef KNET_POLLER_IOCP
        && set_rawsocket_nonblock(rs)
#endif
    ) {
        _wkr.add_work(rs);
        return true;
    }
    close_rawsocket(rs);
    return false;
}

} // namespace knet
