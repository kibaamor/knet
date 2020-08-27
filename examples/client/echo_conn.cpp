#include "echo_conn.h"
#include <knet/kutils.h>
#include <iostream>
#include <cassert>
#include <algorithm>

namespace {

constexpr int64_t TIMER_ID_SEND_PACKAGE = 1;

constexpr uint32_t get_min_pkg_size()
{
    // + sizeof(uint32_t) for package integrity check
    return echo_package::get_hdr_size() + sizeof(uint32_t);
}

} // namespace

cecho_conn::cecho_conn(conn_factory& cf)
    : conn(cf)
{
}

void cecho_conn::do_on_connected()
{
    auto& mgr = echo_mgr::get_instance();

    if (mgr.get_enable_log()) {
        address sockAddr;
        address peerAddr;

        if (!get_sockaddr(sockAddr) || !get_peeraddr(peerAddr)) {
            std::cerr << "get_sockaddr()/get_peeraddr() failed in on_connected" << std::endl;
            disconnect();
            return;
        }

        std::cout << get_connid() << " "
                  << sockAddr << " <----> " << peerAddr << " on_connected" << std::endl;
    }

    // for test purpose, direct disconnect
    if (0 == u32rand_between(0, 499)) {
        std::cerr << "direct disconnect at on_connected" << std::endl;
        disconnect();
        return;
    }

    if (!set_sockbuf_size(128 * 1024))
        std::cerr << get_connid() << " set_sockbuf_size failed!" << std::endl;

    if (mgr.get_disconnect_all()) {
        disconnect();
        return;
    }

    generate_packages();
    add_timer(now_ms() + mgr.get_delay_ms(), TIMER_ID_SEND_PACKAGE);
}

size_t cecho_conn::do_on_recv_data(char* data, size_t size)
{
    auto& mgr = echo_mgr::get_instance();

    if (mgr.get_enable_log())
        std::cout << get_connid() << " on_recv_data, size: " << size << std::endl;

    if (mgr.get_disconnect_all()) {
        disconnect();
        return 0;
    }

    auto len = check_package(data, size);
    if (len < 0) {
        std::cerr << "invalid package length:" << len << std::endl;
        disconnect();
        return 0;
    }

    return static_cast<size_t>(len);
}

void cecho_conn::do_on_timer(int64_t absms, const userdata& /*ud*/)
{
    auto& mgr = echo_mgr::get_instance();

    if (mgr.get_enable_log())
        std::cout << get_connid() << " on timer: " << absms << std::endl;

    if (!send_package()) {
        disconnect();
        return;
    }

    add_timer(now_ms() + mgr.get_delay_ms(), TIMER_ID_SEND_PACKAGE);
}

void cecho_conn::generate_packages()
{
    assert(_send_buf_size == _used_buf_size);

    _send_buf_size = 0;

    constexpr uint32_t min_pkg_size = get_min_pkg_size();
    constexpr uint32_t max_pkg_size = 0xffff;
    constexpr uint32_t max_buf_size = sizeof(_buf);

    _used_buf_size = 0;
    while (_used_buf_size + min_pkg_size < max_buf_size) {
        const auto unused_buf_size = max_buf_size - _used_buf_size;

        const auto max_size = (std::min)(max_pkg_size, unused_buf_size);
        auto pkg_size = u32rand_between(min_pkg_size, max_size);

        auto pkg = reinterpret_cast<echo_package*>(_buf + _used_buf_size);
        pkg->size = pkg_size;
        pkg->id = _next_send_pkg_id++;
        pkg->last_u32() = pkg->id;

        _used_buf_size += pkg_size;
    }
}

bool cecho_conn::send_package()
{
    if (_send_buf_size == _used_buf_size)
        return true;

    assert(_send_buf_size < _used_buf_size);

    auto send_size = u32rand_between(1, _used_buf_size - _send_buf_size);

    buffer buf(_buf + _send_buf_size, send_size);
    if (!send_data(&buf, 1)) {
        std::cerr << "send_package failed! size:" << buf.size << std::endl;
        return false;
    }

    _send_buf_size += send_size;

    auto& mgr = echo_mgr::get_instance();
    mgr.add_total_send(send_size);

    if (_send_buf_size == _used_buf_size)
        generate_packages();

    return true;
}

int32_t cecho_conn::check_package(char* data, size_t size)
{
    constexpr uint32_t min_pkg_size = get_min_pkg_size();
    auto pkg = reinterpret_cast<echo_package*>(data);

    if (size < min_pkg_size || pkg->size > size)
        return 0;

    if (pkg->id != pkg->last_u32()) {
        std::cerr << "package integrity check failed! id:"
                  << pkg->id << ", last_u32:" << pkg->last_u32() << std::endl;
        return -1;
    }

    if (pkg->id != _next_recv_pkg_id) {
        std::cerr << "package id mismatch next receive package id! id:"
                  << pkg->id << ", next_recv_pkg_id:" << _next_recv_pkg_id << std::endl;
        return -1;
    }

    _next_recv_pkg_id++;
    auto& mgr = echo_mgr::get_instance();
    mgr.inc_total_recv_pkg_num();

    return pkg->size;
}

cecho_conn_factory::cecho_conn_factory()
{
}

cecho_conn_factory::cecho_conn_factory(connid_gener gener)
    : conn_factory(gener)
{
}

conn* cecho_conn_factory::do_create_conn()
{
    echo_mgr::get_instance().inc_conn_num();
    return new cecho_conn(*this);
}

void cecho_conn_factory::do_destroy_conn(conn* c)
{
    echo_mgr::get_instance().dec_conn_num();
    delete c;
}
