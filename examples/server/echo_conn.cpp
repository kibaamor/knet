#include "echo_conn.h"
#include "../echo_mgr.h"
#include <iostream>
#include <thread>


secho_conn::secho_conn(knet::connid_t id, knet::tconnection_factory* cf)
    : tconnection(id, cf)
{
}

void secho_conn::on_connected()
{
    auto& mgr = echo_mgr::get_instance();
    if (mgr.get_disconnect_all())
    {
        disconnect();
        return;
    }

    set_idle_timer();
}

size_t secho_conn::on_recv_data(char* data, size_t size)
{
    auto& mgr = echo_mgr::get_instance();
    if (mgr.get_disconnect_all())
    {
        disconnect();
        return 0;
    }

    _last_recv_ms = knet::now_ms();
//     std::cout << "recv_data from " << get_connid() 
//         << " at " << _last_recv_ms << std::endl;

    knet::buffer buf(data, size);
    if (!send_data(&buf, 1))
    {
        std::cerr << "send_data failed!" << std::endl;
        disconnect();
        return 0;
    }

    mgr.add_total_send(size);

    return size;
}

void secho_conn::on_timer(int64_t absms, const knet::userdata& ud)
{
    //std::cout << get_connid() << " on timer: " << absms << std::endl;
    const auto nowms = knet::now_ms();
    auto& mgr = echo_mgr::get_instance();
    if (_last_recv_ms > 0 && nowms > _last_recv_ms + mgr.get_max_idle_ms())
    {
        std::cerr << "kick client: " << get_connid() 
            << ", last_recv_ms: " << _last_recv_ms 
            << ", now_ms: " << nowms << std::endl;
        disconnect();
        return;
    }
    
    set_idle_timer();
}

void secho_conn::on_attach_socket(knet::rawsocket_t rs)
{
    knet::set_rawsocket_bufsize(rs, 256 * 1024);
}

void secho_conn::set_idle_timer()
{
    auto& mgr = echo_mgr::get_instance();
    const auto max_idle_ms = mgr.get_max_idle_ms();
    if (max_idle_ms > 0)
        add_timer(knet::now_ms() + max_idle_ms, 0);
}


secho_conn_factory::secho_conn_factory(secho_conn_factory_builder* cfb)
    : _cfb(cfb)
{
}

knet::tconnection* secho_conn_factory::create_connection_impl()
{
    echo_mgr::get_instance().inc_conn_num();
    return new secho_conn(get_next_connid(), this);
}

void secho_conn_factory::destroy_connection_impl(knet::tconnection* tconn)
{
    echo_mgr::get_instance().dec_conn_num();
    knet::tconnection_factory::destroy_connection_impl(tconn);
}

knet::connid_t secho_conn_factory::get_next_connid()
{
    return nullptr != _cfb ? _cfb->get_next_connid() : _next_cid++;
}
