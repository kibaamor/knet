#include "../include/knet/kconn.h"
#include "../include/knet/kconn_factory.h"
#include "internal/ksocket.h"

namespace knet {

conn::conn(conn_factory& cf)
    : _cf(cf)
    , _id(cf.get_next_connid())
{
}

conn::~conn() = default;

bool conn::send_data(const buffer* buf, size_t num)
{
    return _s && _s->write(buf, num);
}

void conn::on_connected(socket* s)
{
    _s = s;
    do_on_connected();
}

void conn::on_disconnect()
{
    do_on_disconnect();
    _s = nullptr;
    _cf.destroy_conn(this);
}

size_t conn::on_recv_data(char* data, size_t size)
{
    return do_on_recv_data(data, size);
}

void conn::disconnect()
{
    if (!is_disconnecting()) {
        _s->close();
    }
}

bool conn::is_disconnecting() const
{
    return !_s || _s->is_closing();
}

timerid_t conn::add_timer(int64_t ms, const userdata& ud)
{
    if (is_disconnecting()) {
        return INVALID_TIMERID;
    }
    return _cf.add_timer(get_connid(), ms, ud);
}

void conn::del_timer(timerid_t tid)
{
    if (INVALID_TIMERID != tid) {
        _cf.del_timer(get_connid(), tid);
    }
}

void conn::on_timer(int64_t ms, const userdata& ud)
{
    do_on_timer(ms, ud);
}

bool conn::get_stat(stat& s) const
{
    return _s && _s->get_stat(s);
}

bool conn::get_sockaddr(address& addr) const
{
    return _s && _s->get_sockaddr(addr);
}

bool conn::get_peeraddr(address& addr) const
{
    return _s && _s->get_peeraddr(addr);
}

bool conn::set_sockbuf_size(size_t size)
{
    return _s && _s->set_sockbuf_size(size);
}

} // namespace knet
