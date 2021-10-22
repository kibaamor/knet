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
    HANDLE _h = nullptr;
    std::array<OVERLAPPED_ENTRY, POLL_EVENT_NUM> _events = {};
};

} // namespace knet
