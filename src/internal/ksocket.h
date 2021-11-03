#pragma once
#include "../../include/knet/kconn.h"

namespace knet {

class poller;
class conn_factory;
class address;

class socket final {
public:
    explicit socket(rawsocket_t rs);
    ~socket();

    bool init(poller& plr, conn_factory& cf);
    bool write(const buffer* buf, size_t num);
    void close();
    bool is_closing() const;
    bool handle_pollevent(void* evt);
    void dispose();
    bool get_stat(conn::stat& s) const;
    bool get_sockaddr(address& addr) const;
    bool get_peeraddr(address& addr) const;
    bool set_sockbuf_size(size_t size);

private:
    class impl;
    impl* _impl;
};

} // namespace knet
