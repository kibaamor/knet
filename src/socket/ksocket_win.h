#pragma once
#include "../ksocket.h"
#include "../kflag.h"
#include "../kinternal.h"

namespace knet {

class socket::impl {
public:
    impl(rawsocket_t rs);
    ~impl();

    bool init(poller& plr, conn_factory& cf);

    bool write(buffer* buf, size_t num);
    void close();
    bool is_closing() const;
    bool handle_pollevent(void* evt);

private:
    bool start();
    bool try_read();
    bool try_write();
    bool handle_read();
    void handle_write(size_t wrote);

private:
    rawsocket_t _rs;

    flag _f;
    std::unique_ptr<conn, conn_deleter> _c;

    struct sockbuf;
    std::unique_ptr<sockbuf> _rb;
    std::unique_ptr<sockbuf> _wb;
};

} // namespace knet
