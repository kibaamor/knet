#include "../include/klistener.h"
#include "../include/kworkable.h"

#ifdef _WIN32
# ifndef WSA_FLAG_NO_HANDLE_INHERIT
#  define WSA_FLAG_NO_HANDLE_INHERIT 0x80
# endif
#endif


namespace
{
    using namespace knet;

    void close_rawsocket(rawsocket_t& rawsocket)
    {
        if (INVALID_RAWSOCKET != rawsocket)
        {
            ::closesocket(rawsocket);
            rawsocket = INVALID_RAWSOCKET;
        }
    }

    rawsocket_t create_rawsocket(int domain, int type, bool nonblock) noexcept
    {
        return
#ifdef _WIN32
            WSASocketW(domain, type, 0, nullptr, 0, 
                (nonblock ? WSA_FLAG_OVERLAPPED : 0) | WSA_FLAG_NO_HANDLE_INHERIT);
#else
            ::socket(domain, type | SOCK_CLOEXEC | (nonblock ? SOCK_NONBLOCK : 0), 0);
#endif
    }

#ifdef _WIN32
    LPFN_ACCEPTEX get_accept_ex(rawsocket_t rawsocket)
    {
        static thread_local LPFN_ACCEPTEX _accept_ex = nullptr;
        if (nullptr == _accept_ex)
        {
            GUID guid = WSAID_ACCEPTEX;
            DWORD dw = 0;
            WSAIoctl(rawsocket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
                &guid, sizeof(guid), &_accept_ex, sizeof(_accept_ex), 
                &dw, nullptr, nullptr);
        }
        return _accept_ex;
    }
#endif

#ifdef KNET_REUSE_ADDR
    bool set_rawsocket_reuse_addr(rawsocket_t rawsocket) noexcept
    {
        int reuse = 1;
        return -1 != setsockopt(rawsocket, SOL_SOCKET, SO_REUSEADDR, 
            reinterpret_cast<char*>(&reuse), sizeof(reuse));
    }
#endif
}

namespace knet
{
#ifdef KNET_USE_IOCP
    struct listener::accept_io
    {
        WSAOVERLAPPED ol = {};
        char buf[(sizeof(sockaddr_storage) + 16) * 2] = {};
        rawsocket_t rawsocket = INVALID_RAWSOCKET;
        accept_io* next = nullptr;

        bool post_accept(rawsocket_t srv_rawsocket, sa_family_t family)
        {
            rawsocket = create_rawsocket(family, SOCK_STREAM, true);
            if (INVALID_SOCKET == rawsocket)
                return false;

            memset(&ol, 0, sizeof(ol));

            DWORD dw = 0;
            const auto accept_ex = get_accept_ex(rawsocket);
            if (nullptr == accept_ex)
                return false;

            if (!accept_ex(srv_rawsocket, rawsocket, buf, 0, 
                sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16, &dw, &ol)
                && ERROR_IO_PENDING != WSAGetLastError())
            {
                close_rawsocket(rawsocket);
                return false;
            }

            return true;
        }
    };
#endif


    listener::listener(const address& addr, workable* workable) noexcept
        : _addr(addr), _workable(workable), _poller(this)
    {
        kassert(nullptr != _workable);
    }

    listener::~listener()
    {
        kassert(INVALID_RAWSOCKET == _rawsocket);
#ifdef KNET_USE_IOCP
        kassert(nullptr == _ios);
        kassert(nullptr == _free_ios);
#endif
    }

    bool listener::start() noexcept
    {
        if (INVALID_RAWSOCKET != _rawsocket)
            return false;

        _rawsocket = create_rawsocket(_addr.get_family(), SOCK_STREAM, true);
        if (INVALID_RAWSOCKET == _rawsocket)
            return false;

#ifdef KNET_REUSE_ADDR
        if (!set_rawsocket_reuse_addr(_rawsocket))
        {
            close_rawsocket(_rawsocket);
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

        if (RAWSOCKET_ERROR ==
            bind(_rawsocket, reinterpret_cast<const sockaddr*>(
                &_addr.get_sockaddr()), sizeof(decltype(_addr.get_sockaddr())))
            || RAWSOCKET_ERROR == listen(_rawsocket, SOMAXCONN)
            || !_poller.add(_rawsocket, this))
        {
            close_rawsocket(_rawsocket);
            return false;
        }

#ifdef KNET_USE_IOCP
        post_accept();
#endif
        return true;
    }

    void listener::stop() noexcept
    {
#ifdef KNET_USE_IOCP
        if (nullptr != _ios)
        {
            for (int i = 0; i < IOCP_PENDING_ACCEPT_NUM; ++i)
                close_rawsocket(_ios[i].rawsocket);
            delete[] _ios;
            _free_ios = _ios = nullptr;
        }
#endif // KNET_USE_IOCP

        close_rawsocket(_rawsocket);
    }

    void listener::on_poll(void* key, const pollevent_t& pollevent)
    {
        (void)key;
#ifdef KNET_USE_IOCP
        accept_io* io = CONTAINING_RECORD(pollevent.lpOverlapped, accept_io, ol);

        if (INVALID_RAWSOCKET != io->rawsocket)
        {
            _workable->add_work(io->rawsocket);
            io->rawsocket = INVALID_RAWSOCKET;
        }

        io->rawsocket = INVALID_SOCKET;
        io->next = _free_ios;
        _free_ios = io;
#else // KNET_USE_IOCP
        sockaddr_storage addr{};
        socklen_t addrLen = sizeof(addr);
        rawsocket_t s = accept4(_rawsocket, reinterpret_cast<sockaddr*>(&addr), 
            &addrLen, SOCK_NONBLOCK);
        while (INVALID_RAWSOCKET != s)
        {
            _workable->add_work(s);

            addrLen = sizeof(addr);
            s = accept4(_rawsocket, reinterpret_cast<sockaddr*>(&addr),
                    &addrLen, SOCK_NONBLOCK);
        }
#endif // KNET_USE_IOCP
    }

#ifdef KNET_USE_IOCP
    void listener::post_accept() noexcept
    {
        for (; nullptr != _free_ios; _free_ios = _free_ios->next)
        {
            if (!_free_ios->post_accept(_rawsocket, _addr.get_family()))
                return;
        }
    }
#endif
}
