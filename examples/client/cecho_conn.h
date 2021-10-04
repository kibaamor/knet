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

    void generate_packages();
    bool send_package();
    int32_t check_package(char* data, size_t size);

private:
    char _buf[128 * 1024] = {};
    uint32_t _used_buf_size = 0;
    uint32_t _send_buf_size = 0;
    uint32_t _recv_buf_size = 0;

    uint32_t _next_send_pkg_id = 0;
    uint32_t _next_recv_pkg_id = 0;
};
