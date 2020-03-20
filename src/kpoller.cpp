#include "kpoller.h"

#if defined(KNET_POLLER_IOCP)
# include "poller/kpoller_iocp.h"
#elif defined(KNET_POLLER_EPOLL)
# include "poller/kpoller_epoll.h"
#elif defined(KNET_POLLER_KQUEUE)
# include "poller/kpoller_kqueue.h"
#endif

namespace knet {

poller::poller(poller_client& clt)
{
    _impl.reset(new impl(clt));
}

poller::~poller() = default;

bool poller::add(rawsocket_t rs, void* key)
{
    return _impl->add(rs, key);
}

void poller::poll()
{
    return _impl->poll();
}

} // namespace knet
