#include "kinternal.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <iostream>

#ifndef KNET_USE_IOCP
# include <sys/ioctl.h>
#endif


namespace knet
{
    void on_fatal_error(int err, const char* apiname)
    {
        char buf[10240] = {};

#ifdef _WIN32

        LPSTR msg = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, nullptr);

        snprintf(buf, sizeof(buf), "%s: (%d) %s", apiname, err,
            (nullptr != msg ? msg : "Unknown error"));

        if (nullptr != msg)
            LocalFree(msg);

#else

        snprintf(buf, sizeof(buf), "%s: (%d) %s", apiname, err, strerror(err));

#endif

        std::cerr << buf << std::endl;
        std::cerr.flush();
        throw new std::runtime_error(std::string(buf));
    }

    rawsocket_t create_rawsocket(int domain, int type, bool nonblock)
    {
#ifdef KNET_USE_IOCP

        (void)nonblock;
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

        do
        {
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)

            auto flag = type | SOCK_CLOEXEC;
            if (nonblock)
                flag |= SOCK_NONBLOCK;

            rs = ::socket(domain, flag, 0);
            if (INVALID_RAWSOCKET != rs)
                break;

            if (EINVAL != errno)
                return rs;

#endif

            rs = ::socket(domain, type, 0);
            if (INVALID_RAWSOCKET == rs)
                break;

            if (!set_rawsocket_cloexec(rs)
                || (nonblock && !set_rawsocket_nonblock(rs)))
            {
                close_rawsocket(rs);
                break;
            }
        } while (false);

#ifdef SO_NOSIGPIPE
        if (INVALID_RAWSOCKET != rs)
        {
            int on = 1;
            if (!set_rawsocket_opt(rs, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)))
                close_rawsocket(rs);
        }
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

    bool set_rawsocket_opt(rawsocket_t rs, int level, int optname, 
        const void* optval, socklen_t optlen)
    {
        auto val = static_cast<const char*>(optval);
        return RAWSOCKET_ERROR != setsockopt(rs, level, optname, val, optlen);
    }

#ifndef KNET_USE_IOCP

    bool set_rawsocket_nonblock(rawsocket_t rs)
    {
        int set = 1;
        int ret = 0;

        do
            ret = ioctl(rs, FIONBIO, &set);
        while (RAWSOCKET_ERROR == ret && EINTR == errno);

        return 0 == ret;
    }

    bool set_rawsocket_cloexec(rawsocket_t rs)
    {
        int ret = 0;

        do
            ret = ioctl(rs, FIOCLEX);
        while (RAWSOCKET_ERROR == ret && EINTR == errno);

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
