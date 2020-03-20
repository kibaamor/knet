#include "kacceptor_unix.h"
#include "../kinternal.h"

namespace knet {

acceptor::impl::impl(workable& wkr)
    : _wkr(wkr)
{
    _plr.reset(new poller(*this));
}

acceptor::impl::~impl()
{
    kassert(-1 == _rs);
}

void acceptor::impl::update()
{
    _plr->poll();
}

bool acceptor::impl::start(const address& addr)
{
    if (-1 != _rs)
        return false;

    _family = addr.get_family();
    _rs = create_rawsocket(_family, SOCK_STREAM, true);
    if (-1 == _rs)
        return false;

    int on = 1;
    if (!set_rawsocket_opt(_rs, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        close_rawsocket(_rs);
        return false;
    }

    _plr.reset(new poller(*this));

    const auto sa = addr.get_sockaddr();
    const auto salen = addr.get_socklen();
    if (-1 == bind(_rs, sa, salen)
        || -1 == listen(_rs, SOMAXCONN)
        || !_plr->add(_rs, this)) {
        close_rawsocket(_rs);
        return false;
    }

    return true;
}

void acceptor::impl::stop()
{
    close_rawsocket(_rs);
}

bool acceptor::impl::on_pollevent(void* key, void* evt)
{
    (void)key;

#if defined(KNET_POLLER_EPOLL)
    while (true) {
        auto s = accept4(_rs, nullptr, 0, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (-1 != s) {
            _wkr->add_work(s);
            continue;
        }

        if (EINTR != errno)
            break;
    }
#else
    while (true) {
        auto rs = accept(_rs, nullptr, 0);
        if (-1 == rs) {
            if (EINTR == errno)
                continue;
            break;
        }

        if (!set_rawsocket_nonblock(rs) || !set_rawsocket_cloexec(rs))
            close_rawsocket(rs);
        else
            _wkr->add_work(rs);
    }
#endif
    return true;
}

} // namespace knet
