#include "../include/kpoller.h"


namespace knet
{
    poller::poller()
    {
#if defined(KNET_USE_IOCP)
        _rp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
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

    bool poller::poll()
    {
#if defined(KNET_USE_IOCP)
        ULONG num = 0;
        if (!GetQueuedCompletionStatusEx(_rp, _evts, POLL_EVENT_NUM, &num, 0, FALSE))
            return WAIT_TIMEOUT == GetLastError();

        if (num > 0)
        {
            for (ULONG i = 0; i < num; ++i)
            {
                auto key = reinterpret_cast<void*>(_evts[i].lpCompletionKey);
                on_poll(key, _evts[i]);
            }
        }
        return true;
#elif defined(KNET_USE_EPOLL)
        const int num = epoll_wait(_rp, _evts, POLL_EVENT_NUM, 0);
        if (num > 0)
        {
            for (int i = 0; i < num; ++i)
            {
                auto key = _evts[i].data.ptr;
                on_poll(key, _evts[i]);
            }
            return true;
        }
        return (0 == num || (-1 == num && errno == EINTR));
#else
        struct timespec ts;
        memset(&ts, 0, sizeof(ts));
        const int num = kevent(_rp, nullptr, 0, _evts, POLL_EVENT_NUM, &ts);
        if (num > 0)
        {
            for (int i = 0; i < num; ++i)
            {
                auto key = _evts[i].udata;
                on_poll(key, _evts[i]);
            }
        }
        return (0 == num || (-1 == num && errno == EINTR));
#endif
    }
}
