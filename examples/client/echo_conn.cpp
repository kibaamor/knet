#include "echo_conn.h"
#include <iostream>
#include <thread>


cecho_conn::cecho_conn(knet::connid_t id, knet::tconnection_factory* cf)
    : tconnection(id, cf)
{
}

void cecho_conn::on_connected()
{
    auto& mgr = echo_mgr::get_intance();
    if (mgr.get_disconnect_all())
        disconnect();
    else
        send_package();
}

size_t cecho_conn::on_recv_data(char* data, size_t size)
{
    auto& mgr = echo_mgr::get_intance();
    if (mgr.get_disconnect_all())
    {
        disconnect();
        return 0;
    }

    auto len = check_package(data, size);
    if (len < 0)
    {
        std::cerr << "invalid package length:" << len << std::endl;
        disconnect();
        return 0;
    }

    if (len == 0)
        return 0;

    mgr.add_total_send(len);

    send_package();

    return len;
}

void cecho_conn::on_timer(int64_t absms, const knet::userdata& ud)
{
    std::cout << get_connid() << " on timer: " << absms << std::endl;
}

void cecho_conn::send_package()
{
    using namespace knet;
    static thread_local char data[SOCKET_RWBUF_SIZE] = {};

    const auto len = u32rand_between(8, sizeof(data));
    auto p = (char*)data;
    *(int32_t*)p = len;
    *(int32_t*)(p + len - sizeof(int32_t)) = len;

    buffer buf(data, len);
    if (!send_data(&buf, 1))
    {
        std::cerr << "send_package failed!" << std::endl;
        disconnect();
    }
}

int32_t cecho_conn::check_package(char* data, size_t size)
{
    if (size < sizeof(int32_t) * 2)
        return 0;

    const auto len = *(int32_t*)data;
    if (static_cast<int32_t>(size) < len)
        return 0;

    const auto flag = *(int32_t*)(data + len - sizeof(int32_t));
    if (len != flag)
        return -1;

    return len;
}

void cecho_conn::on_attach_socket(knet::rawsocket_t rs)
{
    knet::set_rawsocket_sndrcvbufsize(rs, 256 * 1204);
}

knet::tconnection* cecho_conn_factory::create_connection_impl()
{
    echo_mgr::get_intance().inc_conn_num();
    return new cecho_conn(get_next_connid(), this);
}

void cecho_conn_factory::destroy_connection_impl(knet::tconnection* tconn)
{
    echo_mgr::get_intance().dec_conn_num();
    knet::tconnection_factory::destroy_connection_impl(tconn);
}

cecho_connector::cecho_connector(const knet::address& addr, 
    knet::workable* wkr, bool reconn, size_t interval_ms)
    : connector(addr, wkr, reconn, interval_ms)
{
}

void cecho_connector::on_reconnect()
{
    std::cout << "on reconnect: " << get_address() << std::endl;
}

void cecho_connector::on_reconnect_failed()
{
    std::cout << "on reconnect failed: " << get_address() << std::endl;
}
