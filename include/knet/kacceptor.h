#pragma once
#include "kworker.h"
#include "kaddress.h"

namespace knet {

class acceptor final {
public:
    explicit acceptor(workable& wkr);
    ~acceptor();

    void update();

    bool start(const address& addr);
    void stop();

    bool get_sockaddr(address& addr) const;

private:
    class impl;
    std::unique_ptr<impl> _impl;
};

} // namespace knet
