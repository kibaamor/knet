#include "cecho_conn.h"
#include <knet/kutils.h>
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

void cecho_conn::do_on_connected()
{
    echo_conn::do_on_connected();

    generate_packages();
    add_timer(now_ms() + mgr.get_rand_delay_ms(), TIMER_ID_SEND_PACKAGE);
}

void cecho_conn::do_on_timer(int64_t ms, const userdata& ud)
{
    echo_conn::do_on_timer(ms, ud);

    if (!send_package()) {
        disconnect();
        return;
    }

    add_timer(now_ms() + mgr.get_rand_delay_ms(), TIMER_ID_SEND_PACKAGE);
}

size_t cecho_conn::do_on_recv_data(char* data, size_t size)
{
    echo_conn::do_on_recv_data(data, size);

    const auto len = check_package(data, size);
    if (len < 0) {
        std::cerr << "invalid package length:" << len << std::endl;
        disconnect();
        return 0;
    }

    mgr.total_recv += 1;

    _recv_buf_size += len;
    if (_recv_buf_size == _send_buf_size && _send_buf_size == _used_buf_size) {
        generate_packages();
    }

    return static_cast<size_t>(len);
}

void cecho_conn::generate_packages()
{
    assert(_recv_buf_size == _send_buf_size && _send_buf_size == _used_buf_size);

    _send_buf_size = 0;
    _used_buf_size = 0;
    _recv_buf_size = 0;

    constexpr uint32_t min_pkg_size = get_min_pkg_size();
    constexpr uint32_t max_pkg_size = 0xffff;
    constexpr uint32_t max_buf_size = sizeof(_buf);

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
    assert(_send_buf_size <= _used_buf_size);
    if (_send_buf_size == _used_buf_size) {
        return true;
    }

    const auto send_size = u32rand_between(1, _used_buf_size - _send_buf_size);
    buffer buf(_buf + _send_buf_size, send_size);
    if (!send_data(&buf, 1)) {
        std::cerr << "send_package failed! size:" << buf.get_size() << std::endl;
        return false;
    }

    _send_buf_size += send_size;

    return true;
}

int32_t cecho_conn::check_package(char* data, size_t size)
{
    constexpr uint32_t min_pkg_size = get_min_pkg_size();
    const auto pkg = reinterpret_cast<echo_package*>(data);

    if (size < min_pkg_size || pkg->size > size) {
        return 0;
    }

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

    ++_next_recv_pkg_id;

    return pkg->size;
}
