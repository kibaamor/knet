#include "ksocket_unix.h"
#include "../kpoller.h"
#include "../../../include/knet/kconn_factory.h"
#include <algorithm>

namespace knet {

socket::impl::impl(socket& s, rawsocket_t rs)
    : _s(s)
    , _rs(rs)
{
}

socket::impl::~impl()
{
    kassert(INVALID_RAWSOCKET == _rs);
    kassert(_f.is_close());
}

bool socket::impl::init(poller& plr, conn_factory& cf)
{
    if (INVALID_RAWSOCKET == _rs) {
        return false;
    }

    if (!plr.add(_rs, &_s)) {
        close_rawsocket(_rs);
        _f.mark_close();
        return false;
    }

    _c = cf.create_conn();
    _rb.reset(new sockbuf());
    _wb.reset(new sockbuf());

    if (!start()) {
        close_rawsocket(_rs);
        if (!_f.is_close()) {
            _f.mark_close();
        }

        _c->on_disconnect();
        _c = nullptr;
        return false;
    }

    return true;
}

bool socket::impl::write(const buffer* buf, size_t num)
{
    if (!buf || !num || is_closing()) {
        return false;
    }

    auto total_size = buffer_total_size(buf, num);
    if (total_size > _wb->unused_size()) {
        return false;
    }

    KNET_SOCKET_STAT_CODE(++_stat.send_count)

    if (!_wb->used_size && _f.is_write()) {
        size_t used = 0;
        if (!rawsocket_sendv(_rs, buf, num, used)) {
            this->close();
            return false;
        }

        KNET_SOCKET_STAT_CODE(++_stat.write_count)
        if (used == total_size) {
            return true;
        }

        KNET_SOCKET_STAT_CODE(++_stat.copy_count)
        _f.unmark_write();
        size_t i = 0;
        for (; i < num && used >= buf[i].size; ++i) {
            used -= buf[i].size;
        }
        if (used) {
            _wb->save_data(buf[i].data + used, buf[i].size - used);
            ++i;
        }
        if (i < num) {
            _wb->batch_save_data(&buf[i], num - i);
        }
    } else {
        KNET_SOCKET_STAT_CODE(++_stat.copy_count)
        _wb->batch_save_data(buf, num);
        return try_write();
    }

    return true;
}

void socket::impl::close()
{
    if (!_f.is_close()) {
        _f.mark_close();
    }

    close_rawsocket(_rs);

    if (_f.is_call()) {
        return;
    }

    _c->on_disconnect();
    _c = nullptr;
    _s.dispose();
}

bool socket::impl::handle_pollevent(void* evt)
{
    auto ret = true;

    do {
#ifdef __APPLE__
        auto e = reinterpret_cast<struct kevent*>(evt);
        if (e->flags & EV_EOF) {
            ret = false;
            break;
        }
        if (EVFILT_READ == e->filter && !handle_can_read()) {
            ret = false;
            break;
        }
        if (EVFILT_WRITE == e->filter && !handle_can_write()) {
            ret = false;
            break;
        }
#else
        auto e = reinterpret_cast<struct epoll_event*>(evt);
        if (e->events & (EPOLLERR | EPOLLHUP)) {
            ret = false;
            break;
        }
        if ((e->events & EPOLLIN) && !handle_can_read()) {
            ret = false;
            break;
        }
        if ((e->events & EPOLLOUT) && !handle_can_write()) {
            ret = false;
            break;
        }
#endif
    } while (false);

    if (!ret) {
        close();
    }

    return ret;
}

bool socket::impl::get_stat(conn::stat& s) const
{
#ifdef KNET_SOCKET_STAT
    KNET_SOCKET_STAT_CODE(s = _stat)
    return true;
#else // !KNET_SOCKET_STAT
    return false;
#endif // KNET_SOCKET_STAT
}

bool socket::impl::start()
{
    scoped_call_flag s(_f);
    _c->on_connected(&_s);
    return !_f.is_close();
}

bool socket::impl::handle_can_read()
{
    do {
        KNET_SOCKET_STAT_CODE(++_stat.read_count)
        if (!_rb->try_read(_rs)) {
            return false;
        }

        const auto is_full = 0 == _rb->unused_size();
        if (_rb->used_size && !handle_read()) {
            return false;
        }
        if (!is_full) {
            break;
        }
    } while (true);
    return true;
}

bool socket::impl::handle_can_write()
{
    if (!_f.is_write()) {
        _f.mark_write();
        return try_write();
    }
    return true;
}

bool socket::impl::handle_read()
{
    const auto ptr = _rb->chunk;
    const auto max_size = _rb->used_size;
    size_t size = 0;
    {
        scoped_call_flag s(_f);
        do {
            KNET_SOCKET_STAT_CODE(++_stat.recv_count)
            const auto t = _c->on_recv_data(ptr + size, max_size - size);
            if (!t || _f.is_close()) {
                break;
            }
            size += t;
        } while (size < max_size);
    }

    if (_f.is_close()) {
        return false;
    }
    if (size > 0) {
        _rb->discard_used((std::min)(size, max_size));
#ifdef KNET_SOCKET_STAT
        if (_rb->used_size) {
            KNET_SOCKET_STAT_CODE(++_stat.move_count)
        }
#endif // KNET_SOCKET_STAT
    }

    return true;
}

bool socket::impl::try_write()
{
    if (!_wb->used_size || !_f.is_write()) {
        return true;
    }
    if (!_wb->try_write(_rs)) {
        return false;
    }
    KNET_SOCKET_STAT_CODE(++_stat.write_count)
    if (_wb->used_size) {
        _f.unmark_write();
    }
    return true;
}

} // namespace knet
