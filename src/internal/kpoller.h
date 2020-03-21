#pragma once
#include "knetfwd.h"

namespace knet {

class poller_client {
public:
    virtual ~poller_client() = default;

    virtual bool on_pollevent(void* key, void* evt) = 0;
};

class poller final {
public:
    poller(poller_client& clt);
    ~poller();

    bool add(rawsocket_t rs, void* key);
    void poll();

private:
    class impl;
    std::unique_ptr<impl> _impl;
};

} // namespace knet
