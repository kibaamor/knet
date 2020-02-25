#include "../include/kacceptor.h"
#include "../include/kworker.h"

#ifdef _WIN32
# ifndef WSA_FLAG_NO_HANDLE_INHERIT
#  define WSA_FLAG_NO_HANDLE_INHERIT 0x80
# endif
#endif


namespace
{
    using namespace knet;

    void close_rawsocket(rawsocket_t& rs)
    {
        if (INVALID_RAWSOCKET != rs)
        {
            ::closesocket(rs);
            rs = INVALID_RAWSOCKET;
        }
    }

    rawsocket_t create_rawsocket(int domain, int type, bool nonblock) noexcept
    {
#ifdef _WIN32
        auto flag = WSA_FLAG_NO_HANDLE_INHERIT;
        if (nonblock)
            flag |= WSA_FLAG_OVERLAPPED;
        return WSASocketW(domain, type, 0, nullptr, 0, flag);
#else
        auto flag = type | SOCK_CLOEXEC;
        if (nonblock)
            flag |= SOCK_NONBLOCK;
        return ::socket(domain, flag, 0);
#endif
    }

#ifdef _WIN32
    LPFN_ACCEPTEX get_accept_ex(rawsocket_t rs)
    {
        static thread_local LPFN_ACCEPTEX _accept_ex = nullptr;
        if (nullptr == _accept_ex)
        {
            GUID guid = WSAID_ACCEPTEX;
            DWORD dw = 0;
            WSAIoctl(rs, SIO_GET_EXTENSION_FUNCTION_POINTER, 
                &guid, sizeof(guid), &_accept_ex, sizeof(_accept_ex), 
                &dw, nullptr, nullptr);
        }
        return _accept_ex;
    }
#endif

#ifdef KNET_REUSE_ADDR
    bool set_rawsocket_reuse_addr(rawsocket_t rs) noexcept
    {
        int reuse = 1;
        auto optval = reinterpret_cast<const char*>(&reuse);
        return -1 != setsockopt(rs, SOL_SOCKET, SO_REUSEADDR, optval, sizeof(reuse));
    }
#endif
}

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


    acceptor::acceptor(workable* wkr, connection_factory* cf) noexcept
        : _wkr(wkr), _cf(cf), _poller(this)
    {
        kassert(nullptr != _wkr);
        kassert(nullptr != _cf);
    }

    acceptor::~acceptor() noexcept
    {
        kassert(INVALID_RAWSOCKET == _rs);
#ifdef KNET_USE_IOCP
        kassert(nullptr == _ios);
        kassert(nullptr == _free_ios);
#endif
    }

    bool acceptor::start(const address& addr) noexcept
    {
        if (INVALID_RAWSOCKET != _rs)
            return false;

        _family = addr.get_family();
        _rs = create_rawsocket(_family, SOCK_STREAM, true);
        if (INVALID_RAWSOCKET == _rs)
            return false;

#ifdef KNET_REUSE_ADDR
        if (!set_rawsocket_reuse_addr(_rs))
        {
            close_rawsocket(_rs);
            return false;
        }
#endif

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
#endif // KNET_USE_IOCP

        auto sa = reinterpret_cast<const sockaddr*>(&addr.get_sockaddr());
        if (RAWSOCKET_ERROR == bind(_rs, sa, sizeof(sockaddr_storage))
            || RAWSOCKET_ERROR == listen(_rs, SOMAXCONN)
            || !_poller.add(_rs, this))
        {
            close_rawsocket(_rs);
            return false;
        }

#ifdef KNET_USE_IOCP
        post_accept();
#endif
        return true;
    }

    void acceptor::stop() noexcept
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
#ifdef KNET_USE_IOCP
        accept_io* io = CONTAINING_RECORD(evt.lpOverlapped, accept_io, ol);

        if (INVALID_RAWSOCKET != io->rs)
        {
            _wkr->add_work(_cf, io->rs);
            io->rs = INVALID_RAWSOCKET;
        }

        io->rs = INVALID_SOCKET;
        io->next = _free_ios;
        _free_ios = io;
#else // KNET_USE_IOCP
        sockaddr_storage addr{};
        socklen_t addrLen = sizeof(addr);
        auto sa = reinterpret_cast<sockaddr*>(&addr);
        rawsocket_t s = accept4(_rs, sa, &addrLen, SOCK_NONBLOCK);
        while (INVALID_RAWSOCKET != s)
        {
            _wkr->add_work(_cf, s);

            addrLen = sizeof(addr);
            s = accept4(_rs, sa, &addrLen, SOCK_NONBLOCK);
        }
#endif // KNET_USE_IOCP
    }

#ifdef KNET_USE_IOCP
    void acceptor::post_accept() noexcept
    {
        for (; nullptr != _free_ios; _free_ios = _free_ios->next)
        {
            if (!_free_ios->post_accept(_rs, _family))
                return;
        }
    }
#endif
}
