#pragma once
#include "kaddress.h"
#include "kpoller.h"

namespace knet {
class workable;
class acceptor : public poller {
public:
    acceptor(workable* wkr);
    ~acceptor() override;

    void poll() override;

    bool start(const address& addr);
    void stop();

protected:
    bool on_poll(void* key, const rawpollevent_t& evt) override;

private:
    workable* const _wkr;

    sa_family_t _family = AF_UNSPEC;
    rawsocket_t _rs = INVALID_RAWSOCKET;

#ifdef KNET_USE_IOCP
    void post_accept();

    struct accept_io;
    accept_io* _ios = nullptr;
    accept_io* _free_ios = nullptr;
#endif
};
} // namespace knet
