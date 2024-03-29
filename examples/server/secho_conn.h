#pragma once
#include <knet/kconn_factory.h>
#include "../echo_conn.h"

using namespace knet;

constexpr int64_t TIMER_ID_IDLE_CHECK = 9999;

class secho_conn : public echo_conn {
public:
    using echo_conn::echo_conn;

private:
    void do_on_connected() override
    {
        echo_conn::do_on_connected();
        if (mgr.max_idle_ms) {
            add_timer(knet::now_ms() + mgr.max_idle_ms, TIMER_ID_IDLE_CHECK);
        }
    }
    void do_on_timer(int64_t ms, const knet::userdata& ud) override
    {
        echo_conn::do_on_timer(ms, ud);

        if (ud.data.i64 == TIMER_ID_IDLE_CHECK) {
            const auto now = knet::now_ms();
            if (!_last_recv_ms) {
                _last_recv_ms = now;
            } else if (now > _last_recv_ms + mgr.max_idle_ms) {
                mgr << "kick client: " << get_connid()
                    << ", last_recv_ms: " << _last_recv_ms
                    << ", now_ms: " << now
                    << ", delta_ms: " << now - _last_recv_ms << "\n";

                if (!get_connid()) {
                    std::cerr << "connid: " << get_connid()
                              << ", last_recv_ms: " << _last_recv_ms
                              << ", now_ms: " << now
                              << ", delta_ms: " << now - _last_recv_ms << "\n";
                } else {
                    disconnect();
                    return;
                }
            }

            add_timer(knet::now_ms() + mgr.max_idle_ms, TIMER_ID_IDLE_CHECK);
        }
    }

    size_t do_on_recv_data(char* data, size_t size) override
    {
        echo_conn::do_on_recv_data(data, size);

        const auto hdr = reinterpret_cast<msg_hdr*>(data);
        if (size < sizeof(msg_hdr) || size < sizeof(msg_hdr) + hdr->size) {
            return 0;
        }
        ++mgr.total_recv;

        size = static_cast<size_t>(sizeof(msg_hdr) + hdr->size);

        knet::buffer buf(data, size);
        if (!send_data(&buf, 1)) {
            std::cerr << get_connid() << " send_data failed!\n";
            disconnect();
            return 0;
        }

        _last_recv_ms = knet::now_ms();

        return size;
    }

    void set_idle_timer()
    {
        if (mgr.max_idle_ms) {
            add_timer(knet::now_ms() + mgr.max_idle_ms, TIMER_ID_IDLE_CHECK);
        }
    }

private:
    int64_t _last_recv_ms = 0;
};
