#include "ksocket.h"
#include "ksocket_utils.h"

#ifdef _WIN32
#include "socket/ksocket_win.h"
#else
#include "socket/ksocket_unix.h"
#endif

namespace knet {

socket::socket(rawsocket_t rs)
{
    _impl = new impl(*this, rs);
}

socket::~socket()
{
    delete _impl;
}

bool socket::init(poller& plr, conn_factory& cf)
{
    return _impl->init(plr, cf);
}

bool socket::write(const buffer* buf, size_t num)
{
    return _impl->write(buf, num);
}

void socket::close()
{
    return _impl->close();
}

bool socket::is_closing() const
{
    return _impl->is_closing();
}

bool socket::handle_pollevent(void* evt)
{
    return _impl->handle_pollevent(evt);
}

void socket::dispose()
{
    delete this;
}

bool socket::get_stat(conn::stat& s) const
{
    return _impl->get_stat(s);
}

bool socket::get_sockaddr(address& addr) const
{
    auto rs = _impl->get_rawsocket();
    return INVALID_RAWSOCKET != rs && get_rawsocket_sockaddr(rs, addr);
}

bool socket::get_peeraddr(address& addr) const
{
    auto rs = _impl->get_rawsocket();
    return INVALID_RAWSOCKET != rs && get_rawsocket_peeraddr(rs, addr);
}

bool socket::set_sockbuf_size(size_t size)
{
    auto rs = _impl->get_rawsocket();
    return INVALID_RAWSOCKET != rs && set_rawsocket_bufsize(rs, size);
}

} // namespace knet