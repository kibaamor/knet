#include "../include/kpoller.h"


namespace knet
{
    poller::poller(listener* l) noexcept
        : _l(l)
    {
        kassert(nullptr != _l);
#ifdef KNET_USE_IOCP
        _rp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
#else
        _rp = epoll_create(1);
#endif
        kassert(INVALID_RAWPOLLER != _rp);
    }

    poller::~poller()
    {
#ifdef KNET_USE_IOCP
        CloseHandle(_rp);
#else
        close(_rp);
#endif
    }

    bool poller::add(rawsocket_t rs, void* key) noexcept
    {
#ifdef KNET_USE_IOCP
        auto h = reinterpret_cast<HANDLE>(rs);
        auto ck = reinterpret_cast<ULONG_PTR>(key);
        return nullptr != CreateIoCompletionPort(h, _rp, ck, 0);
#else
        epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET;
        ev.data.ptr = key;
        return 0 == epoll_ctl(_poller, EPOLL_CTL_ADD, rs, &ev);
#endif
    }

    bool poller::poll() noexcept
    {
#ifdef KNET_USE_IOCP
        ULONG num = 0;
        if (!GetQueuedCompletionStatusEx(_rp, _evts, POLL_EVENT_NUM, &num, 0, FALSE))
            return WAIT_TIMEOUT == GetLastError();
        if (num > 0)
        {
            for (ULONG i = 0; i < num; ++i)
                _l->on_poll(reinterpret_cast<void*>(_evts[i].lpCompletionKey), _evts[i]);
            _l->on_postpoll();
        }
        return true;
#else
        const int num = epoll_wait(_rp, _evts, POLL_EVENT_NUM, 0);
        if (num > 0)
        {
            for (int i = 0; i < num; ++i)
                _l->on_poll(_evts[i].data.ptr, _evts[i]);
            _l->on_postpoll();
            return true;
        }
        return (0 == num || (-1 == num && errno == EINTR));
#endif
    }
}
