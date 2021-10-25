#include "kacceptor_unix.h"
#include "../internal/ksocket_utils.h"

namespace knet {

acceptor::impl::impl(workable& wkr)
    : _wkr(wkr)
{
}

acceptor::impl::~impl()
{
    kassert(INVALID_RAWSOCKET == _rs);
}

void acceptor::impl::update()
{
    _plr->poll();
}

bool acceptor::impl::start(const address& addr)
{
    if (INVALID_RAWSOCKET != _rs) {
        return false;
    }

    _family = addr.get_rawfamily();
    _rs = create_rawsocket(_family);
    if (INVALID_RAWSOCKET == _rs) {
        return false;
    }

    if (!set_rawsocket_reuseaddr(_rs)) {
        kdebug("set_rawsocket_reuseaddr() failed!");
        close_rawsocket(_rs);
        return false;
    }

    _plr.reset(new poller(*this));

    if (bind(_rs, addr.as_ptr<sockaddr>(), addr.get_socklen())
        || listen(_rs, SOMAXCONN)
        || !_plr->add(_rs, this)) {
        close_rawsocket(_rs);
        return false;
    }

    return true;
}

void acceptor::impl::stop()
{
    close_rawsocket(_rs);
    _plr.reset();
}

bool acceptor::impl::get_sockaddr(address& addr) const
{
    return INVALID_RAWSOCKET != _rs && get_rawsocket_sockaddr(_rs, addr);
}

bool acceptor::impl::on_pollevent(void* key, void* evt)
{
    while (true) {
#ifdef __APPLE__
        auto rs = TEMP_FAILURE_RETRY(accept(_rs, nullptr, 0));
        if (INVALID_RAWSOCKET == rs) {
            if (ECONNABORTED == errno) {
                continue;
            }
            if (EMFILE == errno || ENFILE == errno) {
                kdebug("too many open file!");
            }
            if (EAGAIN != errno && EWOULDBLOCK != errno) {
                kdebug("accept() failed!");
            }
            break;
        }
        if (!set_rawsocket_nonblock(rs)) {
            kdebug("set_rawsocket_nonblock() failed!");
            close_rawsocket(rs);
            continue;
        }
        if (!set_rawsocket_cloexec(rs)) {
            kdebug("set_rawsocket_cloexec() failed!");
            close_rawsocket(rs);
            continue;
        }
#else
        auto rs = TEMP_FAILURE_RETRY(accept4(_rs, nullptr, 0, SOCK_NONBLOCK | SOCK_CLOEXEC));
        if (INVALID_RAWSOCKET == rs) {
            if (ECONNABORTED == errno) {
                kdebug("connection aborted!");
                continue;
            }
            if (EMFILE == errno || ENFILE == errno) {
                kdebug("too many open file!");
            }
            if (EAGAIN != errno && EWOULDBLOCK != errno) {
                kdebug("accept4() failed!");
            }
            break;
        }
#endif
        _wkr.add_work(rs);
    }
    return true;
}

} // namespace knet
