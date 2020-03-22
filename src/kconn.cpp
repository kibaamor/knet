#include "../include/kconn.h"
#include "../include/kconn_factory.h"
#include "internal/ksocket.h"

namespace knet {

conn::conn(conn_factory& cf)
    : _cf(cf)
    , _id(cf.get_next_connid())
{
}

conn::~conn() = default;

void conn::on_connected(socket* s)
{
    _s = s;
}

void conn::on_disconnect()
{
    _s = nullptr;
    _cf.destroy_conn(this);
}

timerid_t conn::add_timer(int64_t absms, const userdata& ud)
{
    if (is_disconnecting())
        return INVALID_TIMERID;
    return _cf.add_timer(get_connid(), absms, ud);
}

void conn::del_timer(timerid_t tid)
{
    if (INVALID_TIMERID != tid)
        _cf.del_timer(get_connid(), tid);
}

bool conn::send_data(buffer* buf, size_t num)
{
    return nullptr != _s && _s->write(buf, num);
}

void conn::disconnect()
{
    if (!is_disconnecting())
        _s->close();
}

bool conn::is_disconnecting() const
{
    return nullptr == _s || _s->is_closing();
}

bool conn::set_sockbuf_size(size_t size)
{
    return nullptr != _s && _s->set_sockbuf_size(size);
}

} // namespace knet