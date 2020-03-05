#include "echo_conn.h"
#include <iostream>
#include <thread>
#include <algorithm>


namespace
{
    constexpr int64_t TIMER_ID_SEND_PACKAGE = 1;

    constexpr uint32_t get_min_pkg_size()
    {
        // + sizeof(uint32_t) for package integrity check
        return echo_package::get_hdr_size() + sizeof(uint32_t);
    }
}

cecho_conn::cecho_conn(knet::connid_t id, knet::tconnection_factory* cf)
    : tconnection(id, cf)
{
}

void cecho_conn::on_connected()
{
    auto& mgr = echo_mgr::get_instance();
    if (mgr.get_disconnect_all())
        disconnect();
    
    generate_packages();
    add_timer(knet::now_ms() + mgr.get_delay_ms(), TIMER_ID_SEND_PACKAGE);
}

size_t cecho_conn::on_recv_data(char* data, size_t size)
{
    auto& mgr = echo_mgr::get_instance();
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

    return static_cast<size_t>(len);
}

void cecho_conn::on_timer(int64_t absms, const knet::userdata& ud)
{
    //std::cout << get_connid() << " on timer: " << absms << std::endl;
    send_package();

    auto& mgr = echo_mgr::get_instance();
    add_timer(knet::now_ms() + mgr.get_delay_ms(), TIMER_ID_SEND_PACKAGE);
}

void cecho_conn::on_attach_socket(knet::rawsocket_t rs)
{
    knet::set_rawsocket_bufsize(rs, 256 * 1024);
}

void cecho_conn::generate_packages()
{
    kassert(_send_buf_size == _used_buf_size);

    _send_buf_size = 0;

    constexpr uint32_t min_pkg_size = get_min_pkg_size();
    constexpr uint32_t max_pkg_size = 0xffff;
    constexpr uint32_t max_buf_size = sizeof(_buf);

    _used_buf_size = 0;
    while (_used_buf_size + min_pkg_size < max_buf_size)
    {
        const auto unused_buf_size = max_buf_size - _used_buf_size;

        const auto max_size = (std::min)(max_pkg_size, unused_buf_size);
        auto pkg_size = knet::u32rand_between(min_pkg_size, max_size);

        auto pkg = reinterpret_cast<echo_package*>(_buf + _used_buf_size);
        pkg->size = pkg_size;
        pkg->id = _next_send_pkg_id++;
        pkg->last_u32() = pkg->id;

        _used_buf_size += pkg_size;
    }
}

void cecho_conn::send_package()
{
    if (_send_buf_size == _used_buf_size)
        return;

    kassert(_send_buf_size < _used_buf_size);

    auto send_size = knet::u32rand_between(1, _used_buf_size - _send_buf_size);

    knet::buffer buf(_buf + _send_buf_size, send_size);
    if (!send_data(&buf, 1))
    {
        std::cerr << "send_package failed! size:" << buf.size << std::endl;
        disconnect();
        return;
    }

    _send_buf_size += send_size;

    auto& mgr = echo_mgr::get_instance();
    mgr.add_total_send(send_size);

    if (_send_buf_size == _used_buf_size)
        generate_packages();
}

int32_t cecho_conn::check_package(char* data, size_t size)
{
    constexpr uint32_t min_pkg_size = get_min_pkg_size();
    auto pkg = reinterpret_cast<echo_package*>(data);

    if (size < min_pkg_size || pkg->size > size)
            return 0;

    if (pkg->id != pkg->last_u32())
    {
        std::cerr << "package integrity check failed! id:"
            << pkg->id << ", last_u32:" << pkg->last_u32() << std::endl;
        return -1;
    }

    if (pkg->id != _next_recv_pkg_id)
    {
        std::cerr << "package id mismatch next receive package id! id:"
            << pkg->id << ", next_recv_pkg_id:" << _next_recv_pkg_id << std::endl;
        return -1;
    }

    _next_recv_pkg_id++;
    auto& mgr = echo_mgr::get_instance();
    mgr.inc_total_recv_pkg_num();
        
    return pkg->size;
}


cecho_conn_factory::cecho_conn_factory(cecho_conn_factory_builder* cfb)
    : _cfb(cfb)
{
}

knet::tconnection* cecho_conn_factory::create_connection_impl()
{
    echo_mgr::get_instance().inc_conn_num();
    return new cecho_conn(get_next_connid(), this);
}

void cecho_conn_factory::destroy_connection_impl(knet::tconnection* tconn)
{
    echo_mgr::get_instance().dec_conn_num();
    knet::tconnection_factory::destroy_connection_impl(tconn);
}

knet::connid_t cecho_conn_factory::get_next_connid()
{
    return nullptr != _cfb ? _cfb->get_next_connid() : _next_cid++;
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
