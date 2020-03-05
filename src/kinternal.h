#include "../include/knetfwd.h"


namespace knet
{
    rawsocket_t create_rawsocket(int domain, int type, bool nonblock);
    void close_rawsocket(rawsocket_t& rs);

    bool set_rawsocket_reuse_addr(rawsocket_t rs);

#ifndef KNET_USE_IOCP
    bool set_rawsocket_nonblock(rawsocket_t rs);
    bool set_rawsocket_cloexec(rawsocket_t rs);
#else
    LPFN_ACCEPTEX get_accept_ex(rawsocket_t rs);
#endif // !KNET_USE_IOCP
}
