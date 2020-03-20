#pragma once
#include "../kpoller.h"
#include "../kinternal.h"
#include <array>

namespace knet {

class poller::impl {
public:
    impl(poller_client& clt);
    ~impl();

    bool add(rawsocket_t rs, void* key);
    void poll();

private:
    poller_client& _clt;
    HANDLE _h = nullptr;
    std::array<OVERLAPPED_ENTRY, 128> _evts;
};

} // namespace knet
