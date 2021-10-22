#pragma once
#include "../kpoller.h"
#include "../kplatform.h"

namespace knet {

class poller::impl {
public:
    explicit impl(poller_client& client);
    ~impl();

    bool add(rawsocket_t rs, void* key);
    void poll();

private:
    poller_client& _client;
    int _ep = -1;
    std::array<struct epoll_event, POLL_EVENT_NUM> _events = {};
};

} // namespace knet
