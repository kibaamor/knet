#pragma once
#include "kconn_factory.h"

namespace knet {

class workable {
public:
    virtual ~workable() = default;
    virtual void add_work(rawsocket_t rs) = 0;
};

class worker : public workable {
public:
    explicit worker(conn_factory& cf);
    ~worker() override;

    virtual void update();

    void add_work(rawsocket_t rs) override;

private:
    class impl;
    std::unique_ptr<impl> _impl;
};

} // namespace knet
