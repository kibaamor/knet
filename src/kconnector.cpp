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
    auto rs = create_rawsocket(addr.get_rawfamily(), false);
    if (INVALID_RAWSOCKET == rs)
        return false;

    const auto sa = addr.as_ptr<sockaddr>();
    const auto salen = addr.get_socklen();
    if (RAWSOCKET_ERROR == ::connect(rs, sa, salen)) {
        kdebug("connect() failed");
        close_rawsocket(rs);
        return false;
    }

#ifndef _WIN32
    if (!set_rawsocket_nonblock(rs)) {
        kdebug("set_rawsocket_nonblock() failed");
        close_rawsocket(rs);
        return false;
    }
#endif

    _wkr.add_work(rs);
    return true;
}

} // namespace knet
