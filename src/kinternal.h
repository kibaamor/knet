#pragma once
#include "../include/knetfwd.h"
#include "../include/kconn.h"
#include "../include/kaddress.h"

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Win7
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <ws2tcpip.h>
#include <mswsock.h>

namespace knet {
constexpr rawsocket_t INVALID_RAWSOCKET = INVALID_SOCKET;
constexpr int RAWSOCKET_ERROR = SOCKET_ERROR;
} // namespace knet

#else

#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace {
constexpr rawsocket_t INVALID_RAWSOCKET = -1;
constexpr int RAWSOCKET_ERROR = -1;
} // namespace

#endif // _WIN32

#include <cassert>
#define kassert assert

namespace knet {

void on_fatal_error(int err, const char* apiname);

bool set_rawsocket_bufsize(rawsocket_t rs, int size);

rawsocket_t create_rawsocket(int domain, int type, bool nonblock);
void close_rawsocket(rawsocket_t& rs);

bool set_rawsocket_opt(rawsocket_t rs, int level, int optname,
    const void* optval, socklen_t optlen);

#ifdef KNET_PLATFORM_WIN
LPFN_ACCEPTEX get_accept_ex(rawsocket_t rs);
#endif

#ifdef KNET_PLATFORM_UNIX
bool set_rawsocket_nonblock(rawsocket_t rs);
bool set_rawsocket_cloexec(rawsocket_t rs);
#endif

class conn_deleter {
public:
    void set_factory(conn_factory* cf) { _cf = cf; }

    void operator()(conn* c) const
    {
        if (nullptr != c && nullptr != _cf)
            _cf->destroy_conn(c);
    }

private:
    conn_factory* _cf = nullptr;
};

} // namespace knet
