#include "../include/kpoller.h"
#include <cassert>

namespace knet
{
    poller::poller(listener* listener) noexcept
        : _listener(listener)
    {
        assert(nullptr != _listener);
#ifdef KNET_USE_IOCP
        _poller = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
#else
        _poller = epoll_create(1);
#endif
        assert(INVALID_POLLER != _poller);
    }

    poller::~poller()
    {
#ifdef KNET_USE_IOCP
        CloseHandle(_poller);
#else
        close(_poller);
#endif
    }

    bool poller::add(rawsocket_t rawsocket, void *key) noexcept
    {
#ifdef KNET_USE_IOCP
        return (nullptr != CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(rawsocket), _poller,
            reinterpret_cast<ULONG_PTR>(key), 0));
#else
        epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET;
        ev.data.ptr = key;
        return (0 == epoll_ctl(_poller, EPOLL_CTL_ADD, rawsocket, &ev));
#endif
    }

    bool poller::poll() noexcept
    {
#ifdef KNET_USE_IOCP
        ULONG num = 0;
        if (!GetQueuedCompletionStatusEx(_poller, _pollevents, POLL_EVENT_NUM, &num, 0, FALSE))
            return WAIT_TIMEOUT == GetLastError();
        if (num > 0)
        {
            for (ULONG i = 0; i < num; ++i)
                _listener->on_poll(reinterpret_cast<void*>(_pollevents[i].lpCompletionKey), _pollevents[i]);
            _listener->on_postpoll();
        }
        return true;
#else
        const int num = epoll_wait(_poller, _pollevents, POLL_EVENT_NUM, 0);
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
