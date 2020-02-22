#include "../include/klistener.h"
#include <cassert>


namespace
{
    rawsocket_t create_rawsocket(int domain, int type, bool nonblock) noexcept
    {
        rawsocket_t rawsocket =
#ifdef _WIN32
#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif
            WSASocketW(domain, type, 0, nullptr, 0, 
                (nonblock ? WSA_FLAG_OVERLAPPED : 0) | WSA_FLAG_NO_HANDLE_INHERIT);
#else // _WIN32
            ::socket(domain, type | SOCK_CLOEXEC | (nonblock ? SOCK_NONBLOCK : 0), 0);
#endif // _WIN32

        if (INVALID_RAWSOCKET != rawsocket)
            knet::set_rawsocket_sndrcvbufsize(rawsocket, knet::SOCKET_SNDRCVBUF_SIZE);

        return rawsocket;
    }

    bool set_rawsocket_reuse_addr(rawsocket_t rawsocket) noexcept
    {
        int reuse = 1;
        return -1 != setsockopt(rawsocket, SOL_SOCKET, SO_REUSEADDR, 
            reinterpret_cast<char*>(&reuse), sizeof(reuse));
    }
}

namespace knet
{
    listener::listener(const address& addr, workable* workable) noexcept
        : _addr(addr), _workable(workable), _poller(this)
    {
        assert(nullptr != _workable);
    }

    listener::~listener()
    {
        assert(INVALID_RAWSOCKET == _rawsocket);
    }

    bool listener::start() noexcept
    {
        if (INVALID_RAWSOCKET != _rawsocket)
            return false;

        _rawsocket = create_rawsocket(_addr.get_family(), SOCK_STREAM, true);
        if (INVALID_RAWSOCKET == _rawsocket)
            return false;

#if 0
        if (!set_rawsocket_reuse_addr(rawsocket_))
        {
            closesocket(rawsocket_);
            rawsocket_ = INVALID_RAWSOCKET;
            return false;
        }
#endif

#ifdef KNET_USE_IOCP
        GUID guid = WSAID_ACCEPTEX;
        DWORD dw = 0;
        if (SOCKET_ERROR == WSAIoctl(_rawsocket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
            &guid, sizeof(guid), &_accept_ex, sizeof(_accept_ex), &dw, nullptr, nullptr))
        {
            closesocket(_rawsocket);
            _rawsocket = INVALID_RAWSOCKET;
            return false;
        }

        for (int i = 0, N = _countof(_pending_accepts); i < N; ++i)
        {
            if (N != i + 1)
                _pending_accepts[i].next = &_pending_accepts[i + 1];
            else
                _pending_accepts[i].next = nullptr;
        }
        _free_pending_accepts = _pending_accepts;
#endif // KNET_USE_IOCP

        if (RAWSOCKET_ERROR ==
            bind(_rawsocket, reinterpret_cast<const sockaddr*>(
                &_addr.get_sockaddr()), sizeof(decltype(_addr.get_sockaddr())))
            || RAWSOCKET_ERROR == listen(_rawsocket, SOMAXCONN)
            || !_poller.add(_rawsocket, this))
        {
            closesocket(_rawsocket);
            _rawsocket = INVALID_RAWSOCKET;
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
        for (int i = 0, N = _countof(_pending_accepts); i < N; ++i)
        {
            if (INVALID_SOCKET != _pending_accepts[i].rawsocket)
            {
                closesocket(_pending_accepts[i].rawsocket);
                _pending_accepts[i].rawsocket = INVALID_RAWSOCKET;
            }
        }
#endif // KNET_USE_IOCP

        if (INVALID_RAWSOCKET != _rawsocket)
        {
            closesocket(_rawsocket);
            _rawsocket = INVALID_RAWSOCKET;
        }
    }

    void listener::on_poll(void* key, const pollevent_t& pollevent)
    {
        (void)key;
#ifdef KNET_USE_IOCP
        accept_io* io = CONTAINING_RECORD(pollevent.lpOverlapped, accept_io, ol);
        SOCKET s = io->rawsocket;
        io->rawsocket = INVALID_SOCKET;
        io->next = _free_pending_accepts;
        _free_pending_accepts = io;
        if (INVALID_SOCKET != s
            && SOCKET_ERROR != ::setsockopt(s, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
                reinterpret_cast<char*>(&_rawsocket), sizeof(_rawsocket)))
        {
            _workable->addwork(s);
        }
#else // KNET_USE_IOCP
        sockaddr_storage addr{};
        socklen_t addrLen = sizeof(addr);
        rawsocket_t s = accept4(_rawsocket, reinterpret_cast<sockaddr*>(&addr), 
            &addrLen, SOCK_NONBLOCK);
        while (INVALID_RAWSOCKET != s)
        {
            _workable->addwork(s);
            addrLen = sizeof(addr);
            s = accept4(_rawsocket, reinterpret_cast<sockaddr*>(&addr),
                    &addrLen, SOCK_NONBLOCK);
        }
#endif // KNET_USE_IOCP
    }

#ifdef KNET_USE_IOCP
    void listener::post_accept() noexcept
    {
        for (; nullptr != _free_pending_accepts;
            _free_pending_accepts = _free_pending_accepts->next)
        {
            _free_pending_accepts->rawsocket = create_rawsocket(
                _addr.get_family(), SOCK_STREAM, true);
            if (INVALID_SOCKET == _free_pending_accepts->rawsocket)
                return;

            memset(&_free_pending_accepts->ol, 0, sizeof(_free_pending_accepts->ol));
            DWORD dw = 0;
            if (!_accept_ex(_rawsocket, _free_pending_accepts->rawsocket, 
                _free_pending_accepts->buf, 0, sizeof(SOCKADDR_STORAGE) + 16, 
                sizeof(SOCKADDR_STORAGE) + 16, &dw, &_free_pending_accepts->ol)
                && ERROR_IO_PENDING != WSAGetLastError())
            {
                closesocket(_free_pending_accepts->rawsocket);
                _free_pending_accepts->rawsocket = INVALID_SOCKET;
                return;
            }
        }
    }
#endif
}
