#include "kpoller_epoll.h"
#include "../kinternal.h"
#include <sys/epoll.h>

namespace knet {

poller::impl::impl(poller_client& clt)
    : _clt(clt)
{
    _ep = ::epoll_create(1);
}

poller::impl::~impl()
{
    if (-1 != _ep) {
        ::close(_ep);
        _ep = -1;
    }
}

bool poller::impl::add(rawsocket_t rs, void* key)
{
    struct epoll_event ev = {};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET;
    ev.data.ptr = key;
    return 0 == epoll_ctl(_ep, EPOLL_CTL_ADD, rs, &ev);
}

void poller::impl::poll()
{
    auto evts = _evts.data();
    auto size = static_cast<int>(_evts.size());

    int num = 0;
    do
        num = epoll_wait(_rp, evts, size, 0);
    while (-1 == num && EINTR == errno);

    if (-1 == num)
        on_fatal_error(errno, "epoll_wait");

    if (num > 0) {
        for (int i = 0; i < num; ++i) {
            auto& e = evts[i];
            auto key = e.data.ptr;
            _clt.on_pollevent(key, &e);
        }
    }
}

} // namespace knet
