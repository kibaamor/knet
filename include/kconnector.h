#pragma once
#include "kworker.h"
#include "kaddress.h"

namespace knet {

class connector final {
public:
    explicit connector(workable& wkr);
    ~connector();

    bool connect(const address& addr);

private:
    workable& _wkr;
};

} // namespace knet
