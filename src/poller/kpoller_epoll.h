#pragma once
#include "../internal/kpoller.h"
#include "../internal/kinternal.h"
#include <array>

namespace knet {

class poller::impl {
public:
    explicit impl(poller_client& clt);
    ~impl();

    bool add(rawsocket_t rs, void* key);
    void poll();

private:
    poller_client& _clt;
    int _ep = -1;
    std::array<struct epoll_event, POLL_EVENT_NUM> _evts = {};
};

} // namespace knet
