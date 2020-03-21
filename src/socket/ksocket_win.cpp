#include "ksocket_win.h"
#include "../internal/kinternal.h"

namespace knet {

struct socket::impl::sockbuf {
    WSAOVERLAPPED ol = {};
    char chunk[SOCKET_RWBUF_SIZE] = {};
    size_t used_size = 0;
    bool cancel = false;

    char* unused_ptr()
    {
        return chunk + used_size;
    }
    size_t unused_size() const
    {
        return sizeof(chunk) - used_size;
    }

    bool can_read() const
    {
        return used_size < sizeof(chunk);
    }
    bool can_write() const
    {
        return used_size > 0;
    }

    bool post_read(rawsocket_t rs)
    {
        memset(&ol, 0, sizeof(ol));

        WSABUF buf;
        buf.buf = unused_ptr();
        buf.len = static_cast<ULONG>(unused_size());

        DWORD dw = 0;
        DWORD flag = 0;
        const auto ret = ::WSARecv(rs, &buf, 1, &dw, &flag, &ol, nullptr);
        if (SOCKET_ERROR == ret && ERROR_IO_PENDING != ::WSAGetLastError())
            return false;

        return true;
    }

    bool post_write(rawsocket_t rs)
    {
        memset(&ol, 0, sizeof(ol));

        WSABUF buf;
        buf.buf = chunk;
        buf.len = static_cast<ULONG>(used_size);

        DWORD dw = 0;
        const auto ret = ::WSASend(rs, &buf, 1, &dw, 0, &ol, nullptr);
        if (SOCKET_ERROR == ret && ERROR_IO_PENDING != ::WSAGetLastError())
            return false;

        return true;
    }

    void discard_used(size_t num)
    {
        kassert(used_size >= num);
        used_size -= num;
        if (used_size > 0)
            ::memmove(chunk, chunk + num, used_size);
    }
};

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

        _c->on_disconnect();
        _c = nullptr;

        return false;
    }

    return true;
}

bool socket::impl::write(buffer* buf, size_t num)
{
    if (is_closing())
        return false;

    size_t total_size = 0;
    for (size_t i = 0; i < num; ++i)
        total_size += buf[i].size;

    if (0 == total_size)
        return true;

    if (total_size > _wb->unused_size())
        return false;

    for (size_t i = 0; i < num; ++i) {
        auto b = buf + i;
        if (b->size > 0 && nullptr != b->data) {
            memcpy(_wb->unused_ptr(), b->data, b->size);
            _wb->used_size += b->size;
        }
    }

    if (!_f.is_write())
        return try_write();
    return true;
}

void socket::impl::close()
{
    if (!_f.is_close())
        _f.mark_close();

    if (_f.is_call() || _f.is_read() || _f.is_write()) {
        _rb->cancel = _wb->cancel = true;
        ::CancelIoEx(reinterpret_cast<HANDLE>(_rs), nullptr);
        return;
    }

    close_rawsocket(_rs);

    _c->on_disconnect();
    _c = nullptr;

    delete this;
}

bool socket::impl::is_closing() const
{
    return _f.is_close();
}

bool socket::impl::handle_pollevent(void* evt)
{
    auto e = reinterpret_cast<OVERLAPPED_ENTRY*>(evt);

    const auto buf = CONTAINING_RECORD(e->lpOverlapped, socket::impl::sockbuf, ol);
    const auto size = static_cast<size_t>(e->dwNumberOfBytesTransferred);
    if (buf == _rb.get()) {
        if (0 == size || buf->cancel) {
            _f.unmark_read();
            goto failed;
        }

        _rb->used_size += size;
        if (!handle_read()) {
            goto failed;
        }

        if (_rb->can_read() && !try_read())
            goto failed;
    } else if (buf == _wb.get()) {
        if (0 == size || buf->cancel) {
            _f.unmark_write();
            goto failed;
        }

        handle_write(size);
        if (_wb->can_write() && !try_write())
            goto failed;
    } else {
        on_fatal_error(WSAGetLastError(), "invalid event in handle_pollevent");
        goto failed;
    }

    return true;

failed:
    close();
    return false;
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
    if (_rb->post_read(_rs)) {
        _f.mark_read();
        return true;
    }
    return false;
}

bool socket::impl::try_write()
{
    if (_wb->post_write(_rs)) {
        _f.mark_write();
        return true;
    }
    return false;
}

bool socket::impl::handle_read()
{
    _f.unmark_read();

    const auto ptr = _rb->chunk;
    const auto max_size = _rb->used_size;
    size_t size = 0;
    {
        scoped_call_flag s(_f);
        do {
            const auto t = _c->on_recv_data(ptr + size, max_size - size);

            if (0 == t || _f.is_close())
                break;

            size += t;
        } while (size < max_size);
    }
    if (_f.is_close())
        return false;

    if (size > max_size)
        size = max_size;
    _rb->discard_used(size);

    return true;
}

void socket::impl::handle_write(size_t wrote)
{
    _f.unmark_write();
    _wb->discard_used(wrote);
}

} // namespace knet