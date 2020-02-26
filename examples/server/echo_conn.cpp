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
    auto& mgr = echo_mgr::get_intance();
    if (mgr.get_disconnect_all())
        disconnect();
}

size_t secho_conn::on_recv_data(char* data, size_t size)
{
    auto& mgr = echo_mgr::get_intance();
    if (mgr.get_disconnect_all())
    {
        disconnect();
        return 0;
    }

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
    std::cout << get_connid() << " on timer: " << absms << std::endl;
}

void secho_conn::on_attach_socket(knet::rawsocket_t rs)
{
    knet::set_rawsocket_sndrcvbufsize(rs, 256 * 1204);
}

knet::tconnection* secho_conn_factory::create_connection_impl()
{
    echo_mgr::get_intance().inc_conn_num();
    return new secho_conn(get_next_connid(), this);
}

void secho_conn_factory::destroy_connection_impl(knet::tconnection* tconn)
{
    echo_mgr::get_intance().dec_conn_num();
    knet::tconnection_factory::destroy_connection_impl(tconn);
}
