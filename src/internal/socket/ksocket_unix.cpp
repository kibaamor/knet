#include "ksocket_unix.h"
#include "../kpoller.h"
#include "../ksocket_utils.h"
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

bool socket::impl::write(buffer* buf, size_t num)
{
    if (!buf || !num || is_closing() || !_wb->can_save_data(buf, num)) {
        return false;
    }
    _wb->save_data(buf, num);
    return try_write();
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

bool socket::impl::start()
{
    scoped_call_flag s(_f);
    _c->on_connected(&_s);
    return !_f.is_close();
}

bool socket::impl::handle_can_read()
{
    do {
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
    if (_wb->used_size) {
        _f.unmark_write();
    }
    return true;
}

} // namespace knet
