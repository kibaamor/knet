#pragma once
#include <knet/kconn_factory.h>
#include "../echo_conn.h"

class cecho_conn : public echo_conn {
public:
    using echo_conn::echo_conn;

protected:
    void do_on_connected() override;
    void do_on_timer(int64_t ms, const userdata& ud) override;
    size_t do_on_recv_data(char* data, size_t size) override;

    bool send_msg(size_t size);

private:
    uint32_t _token;
    msg_hdr _hdr;
    buffer _bufs[2];
};
