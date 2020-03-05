#include "kinternal.h"

#ifndef KNET_USE_IOCP
# include <sys/ioctl.h>
#endif


namespace knet
{
    rawsocket_t create_rawsocket(int domain, int type)
    {
#ifdef KNET_USE_IOCP

        auto rs = WSASocketW(domain, type, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);

        if (INVALID_RAWSOCKET != rs)
        {
            auto h = reinterpret_cast<HANDLE>(rs);
            if (!SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0))
                close_rawsocket(rs);
        }

        return rs;

#else // !KNET_USE_IOCP

        rawsocket_t rs = INVALID_RAWSOCKET;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
        rs = ::socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

        if (INVALID_RAWSOCKET != rs)
            return rs;

        if (EINVAL != errno)
            return rs;
#endif

        rs = ::socket(domain, type, 0);
        if (INVALID_RAWSOCKET == rs)
            return rs;

        if (!set_rawsocket_nonblock(rs) || !set_rawsocket_cloexec(rs))
            closesocket(rs);

#ifdef SO_NOSIGPIPE
        int on = 1;
        auto optval = reinterpret_cast<const char*>(&on);
        setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, optval, sizeof(on));
#endif

        return rs;
#endif // KNET_USE_IOCP
    }
    
    void close_rawsocket(rawsocket_t& rs)
    {
        if (INVALID_RAWSOCKET == rs)
            return;

        closesocket(rs);
        rs = INVALID_RAWSOCKET;
    }

    bool set_rawsocket_reuse_addr(rawsocket_t rs)
    {
        int reuse = 1;
        auto optval = reinterpret_cast<const char*>(&reuse);
        return RAWSOCKET_ERROR != setsockopt(rs, SOL_SOCKET, SO_REUSEADDR, 
            optval, sizeof(reuse));
    }

#ifndef KNET_USE_IOCP

    bool set_rawsocket_nonblock(rawsocket_t rs)
    {
        int set = 1;
        int ret = 0;

        do
            ret = ioctl(rs, FIONBIO, &set);
        while (ret == RAWSOCKET_ERROR && EINTR == errno);

        return 0 == ret;
    }

    bool set_rawsocket_cloexec(rawsocket_t rs)
    {
        int ret = 0;

        do
            ret = ioctl(rs, FIOCLEX);
        while (ret == RAWSOCKET_ERROR && EINTR == errno);

        return 0 == ret;
    }

#else

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

#endif // !KNET_USE_IOCP

}
