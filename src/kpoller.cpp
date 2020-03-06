#include "../include/kpoller.h"
#include "kinternal.h"


namespace knet
{
    poller::poller()
    {
#if defined(KNET_USE_IOCP)
        _rp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        if (!SetHandleInformation(_rp, HANDLE_FLAG_INHERIT, 0))
        {
            CloseHandle(_rp);
            _rp = INVALID_RAWPOLLER;
        }
#elif defined(KNET_USE_EPOLL)
        _rp = epoll_create(1);
#else
        _rp = kqueue();
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

    bool poller::add(rawsocket_t rs, void* key)
    {
#if defined(KNET_USE_IOCP)
        auto h = reinterpret_cast<HANDLE>(rs);
        auto ck = reinterpret_cast<ULONG_PTR>(key);
        return nullptr != CreateIoCompletionPort(h, _rp, ck, 0);
#elif defined(KNET_USE_EPOLL)
        epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLET;
        ev.data.ptr = key;
        return 0 == epoll_ctl(_rp, EPOLL_CTL_ADD, rs, &ev);
#else
        struct kevent ev[2];
        EV_SET(&ev[0], rs, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, key);
        EV_SET(&ev[1], rs, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, key);
        return 0 == kevent(_rp, ev, 2, nullptr, 0, nullptr);
#endif
    }

    void poller::poll()
    {
#if defined(KNET_USE_IOCP)

        ULONG num = 0;
        if (!GetQueuedCompletionStatusEx(_rp, _evts, POLL_EVENT_NUM, &num, 0, FALSE))
        {
            if (WAIT_TIMEOUT != WSAGetLastError())
                on_fatal_error(WSAGetLastError(), "GetQueuedCompletionStatusEx");
            return;
        }

        if (num > 0)
        {
            for (ULONG i = 0; i < num; ++i)
            {
                auto key = reinterpret_cast<void*>(_evts[i].lpCompletionKey);
                on_poll(key, _evts[i]);
            }
        }

#elif defined(KNET_USE_EPOLL)

        int num = 0;
        do
            num = epoll_wait(_rp, _evts, POLL_EVENT_NUM, 0);
        while (RAWSOCKET_ERROR == num && EINTR == errno);

        if (RAWSOCKET_ERROR == num)
            on_fatal_error(errno, "epoll_wait");

        if (num > 0)
        {
            for (int i = 0; i < num; ++i)
            {
                auto key = _evts[i].data.ptr;
                on_poll(key, _evts[i]);
            }
        }

#else

        struct timespec ts;
        memset(&ts, 0, sizeof(ts));

        int num = 0;
        do
            num = kevent(_rp, nullptr, 0, _evts, POLL_EVENT_NUM, &ts);
        while (RAWSOCKET_ERROR == num && EINTR == errno);

        if (RAWSOCKET_ERROR == num)
            on_fatal_error(errno, "kevent");

        if (num > 0)
        {
            for (int i = 0; i < num; ++i)
            {
                auto key = _evts[i].udata;
                on_poll(key, _evts[i]);
            }
        }

#endif
    }
}
