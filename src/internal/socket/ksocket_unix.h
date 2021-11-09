#pragma once
#include "../ksocket.h"
#include "../kflag.h"
#include "ksockbuf.h"

namespace knet {

class conn;

class socket::impl {
public:
    impl(socket& s, rawsocket_t rs);
    ~impl();

    bool init(poller& plr, conn_factory& cf);
    bool write(const buffer* buf, size_t num);
    void close();
    bool is_closing() const { return _f.is_close(); }
    bool handle_pollevent(void* evt);
    bool get_stat(conn::stat& s) const;

    rawsocket_t get_rawsocket() const { return _rs; }

private:
    bool start();
    bool handle_can_read();
    bool handle_can_write();
    bool handle_read();
    bool try_write();

private:
    socket& _s;
    rawsocket_t _rs;

    flag _f;
    conn* _c = nullptr;

    std::unique_ptr<sockbuf> _rb;
    std::unique_ptr<sockbuf> _wb;

    KNET_SOCKET_STAT_CODE(conn::stat _stat)
};

} // namespace knet
