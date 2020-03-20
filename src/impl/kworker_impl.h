#pragma once
#include "../include/kworker.h"
#include "../kpoller.h"

namespace knet {

class worker::impl : public poller_client {
public:
    explicit impl(conn_factory& cf);
    ~impl();

    void add_work(rawsocket_t rs);
    void update();

    bool on_pollevent(void* key, void* evt) override;

private:
    conn_factory& _cf;
    std::unique_ptr<poller> _plr;
};

} // namespace knet