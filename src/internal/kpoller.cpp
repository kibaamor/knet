#include "kpoller.h"

namespace knet {

poller::poller(poller_client& client)
    : _client(client)
{
#if defined(_WIN32)
    _h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (_h) {
        SetHandleInformation(_h, HANDLE_FLAG_INHERIT, 0);
    }
#elif defined(__APPLE__)
    _kq = kqueue();
#else
    _ep = epoll_create(1);
#endif
}

poller::~poller()
{
#if defined(_WIN32)
    if (_h) {
        CloseHandle(_h);
        _h = nullptr;
    }
#elif defined(__APPLE__)
    if (-1 != _kq) {
        close(_kq);
        _kq = -1;
    }
#else
    if (-1 != _ep) {
        close(_ep);
        _ep = -1;
    }
#endif
}

bool poller::add(rawsocket_t rs, void* key)
{
#if defined(_WIN32)
    auto s = reinterpret_cast<HANDLE>(rs);
    auto k = reinterpret_cast<ULONG_PTR>(key);
    return !!CreateIoCompletionPort(s, _h, k, 0);
#elif defined(__APPLE__)
    struct kevent ev[2];
    EV_SET(&ev[0], rs, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, key);
    EV_SET(&ev[1], rs, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, key);
    return !kevent(_kq, ev, 2, nullptr, 0, nullptr)
        && !(ev[0].flags & EV_ERROR)
        && !(ev[1].flags & EV_ERROR);
#else
    struct epoll_event ev = {};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET;
    ev.data.ptr = key;
    return !epoll_ctl(_ep, EPOLL_CTL_ADD, rs, &ev);
#endif
}

void poller::poll()
{
#if defined(_WIN32)
    auto events = _events.data();
    auto size = static_cast<ULONG>(_events.size());

    ULONG num = 0;
    if (!GetQueuedCompletionStatusEx(_h, events, size, &num, 0, FALSE)) {
        if (WAIT_TIMEOUT != WSAGetLastError()) {
            kdebug("GetQueuedCompletionStatusEx() failed!");
        }
        return;
    }
    if (num > 0) {
        for (ULONG i = 0; i < num; ++i) {
            auto& e = events[i];
            auto key = reinterpret_cast<void*>(e.lpCompletionKey);
            _client.on_pollevent(key, &e);
        }
    }
#elif defined(__APPLE__)
    kassert(!_ts.tv_sec && !_ts.tv_nsec);
    auto events = _events.data();
    const auto size = static_cast<int>(_events.size());
    int num = TEMP_FAILURE_RETRY(kevent(_kq, nullptr, 0, events, size, &_ts));
    if (RAWSOCKET_ERROR == num) {
        kdebug("kqueue_wait() failed!");
        return;
    }
    if (num > 0) {
        _ignores.clear();
        for (int i = 0; i < num; ++i) {
            auto& e = events[i];
            auto key = e.udata;
            if (!_ignores.count(key)) {
                if (!_client.on_pollevent(key, &e)) {
                    _ignores.insert(key);
                }
            }
        }
    }
#else
    auto events = _events.data();
    const auto size = static_cast<int>(_events.size());
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
#endif
}

} // namespace knet
