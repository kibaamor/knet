#pragma once
#include "../../include/kacceptor.h"
#include "../kinternal.h"
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
    void post_accept();

private:
    workable& _wkr;
    std::unique_ptr<poller> _plr;

    int _domain = AF_UNSPEC;
    rawsocket_t _rs = INVALID_SOCKET;

    struct accept_io;
    std::vector<accept_io> _ios;
    accept_io* _free_ios = nullptr;
};

} // namespace knet
