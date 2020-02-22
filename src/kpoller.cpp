#include "../include/kpoller.h"
#include <cassert>

namespace knet
{
    poller::poller(listener* listener) noexcept
        : _listener(listener)
    {
        assert(nullptr != _listener);
#ifdef KNET_USE_IOCP
        _iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        assert(nullptr != _iocp);
#else
        _epfd = epoll_create(1);
        assert(-1 != _epfd);
#endif
    }

    poller::~poller()
    {
#ifdef KNET_USE_IOCP
        CloseHandle(_iocp);
#else
        close(_epfd);
#endif
    }

    bool poller::add(rawsocket_t rawsocket, void *key) noexcept
    {
#ifdef KNET_USE_IOCP
        return (nullptr != CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(rawsocket), _iocp,
            reinterpret_cast<ULONG_PTR>(key), 0));
#else
        epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLERR | EPOLLET;
        ev.data.ptr = key;
        return (0 == epoll_ctl(_epfd, EPOLL_CTL_ADD, rawsocket, &ev));
#endif
    }

    bool poller::poll() noexcept
    {
#ifdef KNET_USE_IOCP
        ULONG num = 0;
        if (!GetQueuedCompletionStatusEx(_iocp, _pollevents, POLL_EVENT_NUM, &num, 0, FALSE))
            return WAIT_TIMEOUT == GetLastError();
        if (num > 0)
        {
            for (ULONG i = 0; i < num; ++i)
                _listener->on_poll(reinterpret_cast<void*>(_pollevents[i].lpCompletionKey), _pollevents[i]);
            _listener->on_postpoll();
        }
        return true;
#else
        const int num = epoll_wait(_epfd, _pollevents, POLL_EVENT_NUM, 0);
        if (num > 0)
        {
            for (int i = 0; i < num; ++i)
                _listener->on_poll(_pollevents[i].data.ptr, _pollevents[i]);
            _listener->on_postpoll();
            return true;
        }
        return (0 == num || (-1 == num && errno == EINTR));
#endif
    }
}
