#include "../include/knet/kconnector.h"
#include "internal/ksocket_utils.h"

namespace knet {

connector::connector(workable& wkr)
    : _wkr(wkr)
{
}

connector::~connector() = default;

bool connector::connect(const address& addr)
{
    auto rs = create_rawsocket(addr.get_rawfamily(), false);
    if (INVALID_RAWSOCKET == rs) {
        return false;
    }

    if (RAWSOCKET_ERROR == ::connect(rs, addr.as_ptr<sockaddr>(), addr.get_socklen())) {
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
