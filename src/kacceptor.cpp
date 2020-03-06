#include "../include/kacceptor.h"
#include "../include/kworker.h"
#include "kinternal.h"


namespace knet
{
#ifdef KNET_USE_IOCP
    struct acceptor::accept_io
    {
        WSAOVERLAPPED ol = {};
        char buf[(sizeof(sockaddr_storage) + 16) * 2] = {};
        rawsocket_t rs = INVALID_RAWSOCKET;
        accept_io* next = nullptr;

        bool post_accept(rawsocket_t srv_rs, sa_family_t family)
        {
            rs = create_rawsocket(family, SOCK_STREAM, true);
            if (INVALID_SOCKET == rs)
                return false;

            memset(&ol, 0, sizeof(ol));

            const auto accept_ex = get_accept_ex(rs);
            if (nullptr == accept_ex)
                return false;

            DWORD dw = 0;
            const auto size = sizeof(SOCKADDR_STORAGE) + 16;
            if (!accept_ex(srv_rs, rs, buf, 0, size, size, &dw, &ol)
                && ERROR_IO_PENDING != WSAGetLastError())
            {
                close_rawsocket(rs);
                return false;
            }

            return true;
        }
    };
#endif

    acceptor::acceptor(workable* wkr)
        : _wkr(wkr)
    {
        kassert(nullptr != _wkr);
    }

    acceptor::~acceptor()
    {
        kassert(INVALID_RAWSOCKET == _rs);
#ifdef KNET_USE_IOCP
        kassert(nullptr == _ios);
        kassert(nullptr == _free_ios);
#endif
    }

    void acceptor::poll()
    {
        poller::poll();
#ifdef KNET_USE_IOCP
        post_accept();
#endif
    }

    bool acceptor::start(const address& addr)
    {
        if (INVALID_RAWSOCKET != _rs)
            return false;

        _family = addr.get_family();
        _rs = create_rawsocket(_family, SOCK_STREAM, true);
        if (INVALID_RAWSOCKET == _rs)
            return false;

        int on = 1;
        if (!set_rawsocket_opt(_rs, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
        {
            close_rawsocket(_rs);
            return false;
        }

        const auto sa = addr.get_sockaddr();
        const auto salen = addr.get_socklen();
        if (RAWSOCKET_ERROR == bind(_rs, sa, salen)
            || RAWSOCKET_ERROR == listen(_rs, SOMAXCONN)
            || !add(_rs, this))
        {
            close_rawsocket(_rs);
            return false;
        }

#ifdef KNET_USE_IOCP
        kassert(nullptr == _ios);
        kassert(nullptr == _free_ios);
        _ios = new accept_io[IOCP_PENDING_ACCEPT_NUM];
        for (int i = 0; i < IOCP_PENDING_ACCEPT_NUM; ++i)
        {
            if (IOCP_PENDING_ACCEPT_NUM != i + 1)
                _ios[i].next = &_ios[i + 1];
        }
        _free_ios = _ios;

        post_accept();
#endif // KNET_USE_IOCP
        return true;
    }

    void acceptor::stop()
    {
#ifdef KNET_USE_IOCP
        if (nullptr != _ios)
        {
            for (int i = 0; i < IOCP_PENDING_ACCEPT_NUM; ++i)
                close_rawsocket(_ios[i].rs);
            delete[] _ios;
            _free_ios = _ios = nullptr;
        }
#endif // KNET_USE_IOCP

        close_rawsocket(_rs);
    }

    void acceptor::on_poll(void* key, const rawpollevent_t& evt)
    {
        (void)key;

#if defined(KNET_USE_IOCP)
        accept_io* io = CONTAINING_RECORD(evt.lpOverlapped, accept_io, ol);

        if (INVALID_RAWSOCKET != io->rs)
        {
            _wkr->add_work(io->rs);
            io->rs = INVALID_RAWSOCKET;
        }

        io->rs = INVALID_SOCKET;
        io->next = _free_ios;
        _free_ios = io;
#elif defined(KNET_USE_EPOLL)
        while (true)
        {
            auto s = accept4(_rs, nullptr, 0, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (INVALID_RAWSOCKET != s)
            {
                _wkr->add_work(s);
                continue;
            }

            if (EINTR != errno)
                break;
        }
#else
        while (true)
        {
            auto s = accept(_rs, nullptr, 0);
            if (INVALID_RAWSOCKET == s)
            {
                if (EINTR == errno)
                    continue;
                break;
            }

            if (!set_rawsocket_nonblock(s) || !set_rawsocket_cloexec(s))
                closesocket(s);
            else
                _wkr->add_work(s);
        }
#endif // KNET_USE_IOCP
    }

#ifdef KNET_USE_IOCP
    void acceptor::post_accept()
    {
        for (; nullptr != _free_ios; _free_ios = _free_ios->next)
        {
            if (!_free_ios->post_accept(_rs, _family))
                return;
        }
    }
#endif
}
