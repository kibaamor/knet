#include "kpoller.h"

#if defined(_WIN32)
#include "poller/kpoller_iocp.h"
#elif defined(__APPLE__)
#include "poller/kpoller_kqueue.h"
#else
#include "poller/kpoller_epoll.h"
#endif

namespace knet {

poller::poller(poller_client& client)
{
    _impl.reset(new impl(client));
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
