#include "echo_conn.h"
#include <kutils.h>
#include <iostream>
#include <cassert>

secho_conn::secho_conn(conn_factory& cf)
    : conn(cf)
{
}

void secho_conn::on_connected(socket* s)
{
    conn::on_connected(s);

    // for test purpose, direct disconnect
    if (0 == u32rand_between(0, 499)) {
        std::cerr << "direct disconnect at on_connected" << std::endl;
        disconnect();
        return;
    }

    if (!set_sockbuf_size(128 * 1024))
        std::cerr << get_connid() << " set_sockbuf_size failed!" << std::endl;

    auto& mgr = echo_mgr::get_instance();

    if (mgr.get_enable_log())
        std::cout << get_connid() << " on_connected" << std::endl;

    if (mgr.get_disconnect_all()) {
        disconnect();
        return;
    }

    set_idle_timer();
}

size_t secho_conn::on_recv_data(char* data, size_t size)
{
    auto& mgr = echo_mgr::get_instance();

    if (mgr.get_enable_log())
        std::cout << get_connid() << " on_recv_data, size: " << size << std::endl;

    if (mgr.get_disconnect_all()) {
        disconnect();
        return 0;
    }

    _last_recv_ms = knet::now_ms();

    knet::buffer buf(data, size);
    if (!send_data(&buf, 1)) {
        std::cerr << "send_data failed!" << std::endl;
        disconnect();
        return 0;
    }

    mgr.add_total_send(size);

    return size;
}

void secho_conn::on_timer(int64_t absms, const knet::userdata& ud)
{
    auto& mgr = echo_mgr::get_instance();

    if (mgr.get_enable_log())
        std::cout << get_connid() << " on timer: " << absms
                  << ", last_recv ms: " << _last_recv_ms << std::endl;

    const auto nowms = knet::now_ms();
    if (_last_recv_ms > 0 && nowms > _last_recv_ms + mgr.get_max_idle_ms()) {
        std::cerr << "kick client: " << get_connid()
                  << ", last_recv_ms: " << _last_recv_ms
                  << ", now_ms: " << nowms
                  << ", delta_ms: " << nowms - _last_recv_ms << std::endl;
        disconnect();
        return;
    }

    set_idle_timer();
}

void secho_conn::set_idle_timer()
{
    auto& mgr = echo_mgr::get_instance();
    const auto max_idle_ms = mgr.get_max_idle_ms();
    if (max_idle_ms > 0)
        add_timer(knet::now_ms() + max_idle_ms, 0);
}

secho_conn_factory::secho_conn_factory(connid_gener gener)
    : conn_factory(gener)
{
}

conn* secho_conn_factory::do_create_conn()
{
    echo_mgr::get_instance().inc_conn_num();
    return new secho_conn(*this);
}

void secho_conn_factory::do_destroy_conn(conn* c)
{
    echo_mgr::get_instance().dec_conn_num();
    conn_factory::do_destroy_conn(c);
}
