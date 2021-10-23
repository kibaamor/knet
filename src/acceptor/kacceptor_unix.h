#pragma once
#include "../../include/knet/kacceptor.h"
#include "../internal/kplatform.h"
#include "../internal/kpoller.h"

namespace knet {

class acceptor::impl : public poller_client {
public:
    explicit impl(workable& wkr);
    ~impl() override;

    void update();
    bool start(const address& addr);
    void stop();
    bool get_sockaddr(address& addr) const;

    bool on_pollevent(void* key, void* evt) override;

private:
    workable& _wkr;
    std::unique_ptr<poller> _plr;

    int _family = AF_UNSPEC;
    rawsocket_t _rs = INVALID_RAWSOCKET;
};

} // namespace knet
