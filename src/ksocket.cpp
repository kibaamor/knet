#include "ksocket.h"

#ifdef KNET_PLATFORM_WIN
#include "socket/ksocket_win.h"
#else
#include "socket/ksocket_unix.h"
#endif

namespace knet {

socket::socket(rawsocket_t rs)
{
    _impl.reset(new impl(rs));
}

socket::~socket() = default;

bool socket::init(poller& plr, conn_factory& cf)
{
    return _impl->init(plr, cf);
}

bool socket::write(buffer* buf, size_t num)
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

} // namespace knet