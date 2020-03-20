#pragma once
#include "../../include/kacceptor.h"
#include "../kpoller.h"

namespace knet {

class acceptor::impl : public poller_client {
public:
    explicit impl(workable& wkr);
    ~impl() override;

    void update();

    bool start(const address& addr);
    void stop();

    bool on_pollevent(void* key, void* evt) override;

private:
    workable& _wkr;
    std::unique_ptr<poller> _plr;

    sa_family_t _family = AF_UNSPEC;
    rawsocket_t _rs = -1;
};

} // namespace knet
