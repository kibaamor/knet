#pragma once
#include "kpoller.h"
#include "../../include/kconn_factory.h"

namespace knet {

class socket final {
public:
    socket(rawsocket_t rs);
    ~socket();

    bool init(poller& plr, conn_factory& cf);

    bool write(buffer* buf, size_t num);

    void close();
    bool is_closing() const;

    bool handle_pollevent(void* evt);

    void dispose();

private:
    class impl;
    std::unique_ptr<impl> _impl;
};

} // namespace knet
