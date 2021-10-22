#include "kpoller_epoll.h"

namespace knet {

poller::impl::impl(poller_client& client)
    : _client(client)
{
    _ep = epoll_create(1);
}

poller::impl::~impl()
{
    if (-1 != _ep) {
        close(_ep);
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
    auto events = _events.data();
    auto size = static_cast<int>(_events.size());
    int num = TEMP_FAILURE_RETRY(epoll_wait(_ep, events, size, 0));
    if (RAWSOCKET_ERROR == num) {
        kdebug("epoll_wait() failed!");
        return;
    }
    if (num > 0) {
        for (int i = 0; i < num; ++i) {
            auto& e = events[i];
            auto key = e.data.ptr;
            _client.on_pollevent(key, &e);
        }
    }
}

} // namespace knet
