#include "kacceptor_unix.h"

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
    if (INVALID_RAWSOCKET != _rs)
        return false;

    _family = addr.get_rawfamily();
    _rs = create_rawsocket(_family, true);
    if (INVALID_RAWSOCKET == _rs)
        return false;

    int on = 1;
    if (!set_rawsocket_opt(_rs, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        kdebug("set_rawsocket_opt(SO_REUSEADDR) failed!");
        close_rawsocket(_rs);
        return false;
    }

    _plr.reset(new poller(*this));

    const auto sa = addr.as_ptr<sockaddr>();
    const auto salen = addr.get_socklen();
    if (RAWSOCKET_ERROR == ::bind(_rs, sa, salen)
        || RAWSOCKET_ERROR == ::listen(_rs, SOMAXCONN)
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

bool acceptor::impl::on_pollevent(void* key, void* evt)
{
    (void)key;

#ifdef __linux__
    while (true) {
        auto rs = ::accept4(_rs, nullptr, 0, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (INVALID_RAWSOCKET == rs) {
            if (EINTR == errno)
                continue;

            if (EAGAIN != errno)
                kdebug("accept4() failed!");
            break;
        }

        _wkr.add_work(rs);
    }
#else
    while (true) {
        auto rs = ::accept(_rs, nullptr, 0);
        if (INVALID_RAWSOCKET == rs) {
            if (EINTR == errno)
                continue;

            if (EAGAIN != errno)
                kdebug("accept() failed!");
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

        _wkr.add_work(rs);
    }
#endif
    return true;
} // namespace knet

} // namespace knet
