#pragma once
#include <knet/kconn_factory.h>
#include "../echo_conn.h"

using namespace knet;

class secho_conn : public echo_conn {
public:
    using echo_conn::echo_conn;

private:
    void do_on_connected() override
    {
        echo_conn::do_on_connected();
        set_idle_timer();
    }
    void do_on_timer(int64_t ms, const knet::userdata& ud) override
    {
        echo_conn::do_on_timer(ms, ud);

        const auto now = knet::now_ms();
        if (_last_recv_ms > 0 && now > _last_recv_ms + mgr.max_idle_ms) {
            std::cerr << "kick client: " << get_connid()
                      << ", last_recv_ms: " << _last_recv_ms
                      << ", now_ms: " << now
                      << ", delta_ms: " << now - _last_recv_ms << std::endl;
            disconnect();
            return;
        }

        set_idle_timer();
    }

    size_t do_on_recv_data(char* data, size_t size) override
    {
        echo_conn::do_on_recv_data(data, size);
        mgr.total_recv += size;

        _last_recv_ms = knet::now_ms();

        knet::buffer buf(data, size);
        if (!send_data(&buf, 1)) {
            std::cerr << get_connid() << " send_data failed!" << std::endl;
            disconnect();
            return 0;
        }

        return size;
    }

    void set_idle_timer()
    {
        if (mgr.max_idle_ms > 0)
            add_timer(knet::now_ms() + mgr.max_idle_ms, 0);
    }

private:
    int64_t _last_recv_ms = 0;
};
