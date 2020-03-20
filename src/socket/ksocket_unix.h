#pragma once
#include "../ksocket.h"
#include "../kflag.h"
#include "../kutils.h"

namespace knet {

class socket::impl {
public:
    impl(rawsocket_t rs);
    ~impl();

    bool init(poller* poller, conn_factory* cf);

    bool write(buffer* buf, size_t num);
    void close();
    bool is_closing() const;
    bool handle_pollevent(void* evt);

private:
    bool start();
    bool handle_can_read();
    bool handle_can_write();
    bool handle_read();
    bool try_write();

private:
    rawsocket_t _rs;

    flag _f;
    std::unique_ptr<conn, conn_deleter> _c;

    struct sockbuf;
    std::unique_ptr<sockbuf> _rb;
    std::unique_ptr<sockbuf> _wb;
};

} // namespace knet
