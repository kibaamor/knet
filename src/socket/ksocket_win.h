#pragma once
#include "../internal/ksocket.h"
#include "../internal/kflag.h"

namespace knet {

class socket::impl {
public:
    impl(socket& s, rawsocket_t rs);
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
    socket& _s;
    rawsocket_t _rs;

    flag _f;
    conn* _c = nullptr;

    struct sockbuf;
    std::unique_ptr<sockbuf> _rb;
    std::unique_ptr<sockbuf> _wb;
};

} // namespace knet
