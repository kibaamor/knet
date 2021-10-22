#include "kpoller_kqueue.h"
#include <set>

namespace knet {

poller::impl::impl(poller_client& client)
    : _client(client)
{
    _kq = kqueue();
}

poller::impl::~impl()
{
    if (-1 != _kq) {
        close(_kq);
        _kq = -1;
    }
}

bool poller::impl::add(rawsocket_t rs, void* key)
{
    struct kevent ev[2];
    EV_SET(&ev[0], rs, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, key);
    EV_SET(&ev[1], rs, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, key);
    return 0 == kevent(_kq, ev, 2, nullptr, 0, nullptr)
        && 0 == (ev[0].flags & EV_ERROR)
        && 0 == (ev[1].flags & EV_ERROR);
}

void poller::impl::poll()
{
    struct timespec ts;
    memset(&ts, 0, sizeof(ts));

    auto events = _events.data();
    auto size = static_cast<int>(_events.size());

    int num = TEMP_FAILURE_RETRY(kevent(_kq, nullptr, 0, events, size, &ts));
    if (RAWSOCKET_ERROR == num) {
        kdebug("kqueue_wait() failed!");
        return;
    }
    if (num > 0) {
        std::set<void*> ignores;
        for (int i = 0; i < num; ++i) {
            auto& e = events[i];
            auto key = e.udata;
            if (ignores.find(key) == ignores.end()) {
                if (!_client.on_pollevent(key, &e))
                    ignores.insert(key);
            }
        }
    }
}

} // namespace knet
