#include "ksocket_win.h"
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

void socket::impl::handle_write(size_t size)
{
    _wb->discard_used(size);
}

} // namespace knet