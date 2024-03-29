#include "ksocket_win.h"
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

    if (!_wb->used_size && !_f.is_write()) {
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

    if (_f.is_call() || _f.is_read() || _f.is_write()) {
        _rb->cancel = _wb->cancel = true;
        CancelIoEx(reinterpret_cast<HANDLE>(_rs), nullptr);
        return;
    }

    close_rawsocket(_rs);

    _c->on_disconnect();
    _c = nullptr;
    _s.dispose();
}

bool socket::impl::handle_pollevent(void* evt)
{
    auto e = reinterpret_cast<OVERLAPPED_ENTRY*>(evt);
    const auto buf = CONTAINING_RECORD(e->lpOverlapped, sockbuf, ol);
    const auto size = static_cast<size_t>(e->dwNumberOfBytesTransferred);

    kassert(buf == _rb.get() || buf == _wb.get());
    auto ret = true;
    do {
        if (buf == _rb.get()) {
            _f.unmark_read();
        } else {
            _f.unmark_write();
        }

        if (!size || buf->cancel) {
            ret = false;
            break;
        }

        if (buf == _rb.get()) {
            if (!handle_read(size) || !try_read()) {
                ret = false;
                break;
            }
        } else {
            handle_write(size);
            if (!try_write()) {
                ret = false;
                break;
            }
        }
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
    {
        scoped_call_flag s(_f);
        _c->on_connected(&_s);
    }

    if (_f.is_close() || !try_read()) {
        if (_f.is_write()) {
            close();
            return true;
        }
        return false;
    }

    return true;
}

bool socket::impl::try_read()
{
    if (!_rb->unused_size() || _f.is_read()) {
        return true;
    }
    if (!_rb->post_read(_rs)) {
        return false;
    }
    KNET_SOCKET_STAT_CODE(++_stat.read_count)
    _f.mark_read();
    return true;
}

bool socket::impl::try_write()
{
    if (!_wb->used_size || _f.is_write()) {
        return true;
    }
    if (!_wb->post_write(_rs)) {
        return false;
    }
    KNET_SOCKET_STAT_CODE(++_stat.write_count)
    _f.mark_write();
    return true;
}

bool socket::impl::handle_read(size_t size)
{
    _rb->used_size += size;

    const auto ptr = _rb->chunk;
    const auto max_size = _rb->used_size;
    size = 0;
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

void socket::impl::handle_write(size_t size)
{
    _wb->discard_used(size);
}

} // namespace knet