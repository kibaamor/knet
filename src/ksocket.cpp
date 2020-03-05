#include "ksocket.h"
#include "../include/kworker.h"
#include "../include/kpoller.h"
#include "kinternal.h"
#include <cstring>


namespace
{
    // when using IOCP, FlagRead means socket has pending WSARecv
    // when using epoll, FlagRead means socket can read
    constexpr uint8_t FlagRead = 1u << 0u;
    // when using IOCP, FlagRead means socket has pending WSASend
    // when using epoll, FlagRead means socket can write
    constexpr uint8_t FlagWrite = 1u << 1u;
    // FlagCall means socket processing user callback
    constexpr uint8_t FlagCall = 1u << 2u;
    // FlagClose means socket will be closed
    constexpr uint8_t FlagClose = 1u << 3u;

    inline bool is_flag_marked(uint8_t& flag, uint8_t test)
    {
        return (0 != (flag & test));
    }
    inline void mark_flag(uint8_t& flag, uint8_t test)
    {
        kassert(!is_flag_marked(flag, test));
        flag |= test;
    }
    inline void unmark_flag(uint8_t& flag, uint8_t test)
    {
        kassert(is_flag_marked(flag, test));
        flag &= ~test;
    }
}

namespace knet
{
    struct sockbuf
#ifdef KNET_USE_IOCP
        : WSABUF
#endif
    {
        char chunk[SOCKET_RWBUF_SIZE] = {};
        size_t used_size = 0;
#ifdef KNET_USE_IOCP
        WSAOVERLAPPED ol = {};
#endif

        char* wptr() { return chunk + used_size; }
        size_t unused_size() const  { return sizeof(chunk) - used_size; }
    };
}

namespace knet
{
    socket::socket(connection_factory* cf, rawsocket_t rs)
        : _cf(cf), _rs(rs)
    {
        kassert(nullptr != _cf);
        kassert(INVALID_RAWSOCKET != _rs);
    }

    socket::~socket()
    {
        kassert(FlagClose == _flag || 0 == _flag);

        if (nullptr != _rbuf)
        {
            delete _rbuf;
            _rbuf = nullptr;
        }
        if (nullptr != _wbuf)
        {
            delete _wbuf;
            _wbuf = nullptr;
        }
        _cf->destroy_connection(_conn);
        close_rawsocket(_rs);
    }

    bool socket::attach_poller(poller* poller)
    {
        if (!poller->add(_rs, this))
            return false;

        kassert(INVALID_RAWSOCKET != _rs);
        kassert(nullptr == _conn);
        kassert(nullptr == _rbuf);
        kassert(nullptr == _wbuf);

        _conn = _cf->create_connection();
        _rbuf = new sockbuf();
        _wbuf = new sockbuf();

        _conn->_socket = this;
        _conn->on_attach_socket(_rs);

        return start();
    }

    bool socket::start()
    {
        mark_flag(_flag, FlagCall);
        _conn->on_connected();
        unmark_flag(_flag, FlagCall);

        if (is_flag_marked(_flag, FlagClose)
#ifdef KNET_USE_IOCP
            || !try_read()
#endif
            )
        {
#ifdef KNET_USE_IOCP
            if (is_flag_marked(_flag, FlagWrite))
            {
                // we have pending WSASend
                close();
                return true;
            }
#endif
            _conn->on_disconnect();
            return false;
        }
        return true;
    }

    void socket::close()
    {
        if (is_flag_marked(_flag, FlagCall
#ifdef KNET_USE_IOCP
            | FlagRead | FlagWrite
#endif
        ))
        {
            if (!is_flag_marked(_flag, FlagClose))
            {
                mark_flag(_flag, FlagClose);
#ifdef KNET_USE_IOCP
                CancelIoEx(reinterpret_cast<HANDLE>(_rs), nullptr);
#endif
            }
            return;
        }

        mark_flag(_flag, FlagCall);
        _conn->on_disconnect();
        unmark_flag(_flag, FlagCall);

#ifdef _WIN32
#define SHUT_RDWR SD_BOTH
#endif
        shutdown(_rs, SHUT_RDWR);
        delete this;
    }

    bool socket::write(buffer* buf, size_t num)
    {
        if (is_flag_marked(_flag, FlagClose))
            return false;

        size_t size = 0;
        for (size_t i = 0; i < num; ++i)
            size += buf[i].size;
        if (size > _wbuf->unused_size())
            return false;

        for (size_t i = 0; i < num; ++i)
        {
            if (buf[i].size > 0 && nullptr != buf[i].data)
            {
                memcpy(_wbuf->wptr(), buf[i].data, buf[i].size);
                _wbuf->used_size += buf[i].size;
            }
        }

#ifdef KNET_USE_IOCP
        if (!is_flag_marked(_flag, FlagWrite))
#else
        if (is_flag_marked(_flag, FlagWrite))
#endif
            return try_write();
        return true;
    }

    void socket::on_rawpollevent(const rawpollevent_t& evt)
    {
#ifdef KNET_USE_IOCP
        if (0 == evt.dwNumberOfBytesTransferred)
        {
            if (&_rbuf->ol == evt.lpOverlapped)
                unmark_flag(_flag, FlagRead);
            else
                unmark_flag(_flag, FlagWrite);
            close();
            return;
        }
        
        const auto size = static_cast<size_t>(evt.dwNumberOfBytesTransferred);
        if (&_rbuf->ol == evt.lpOverlapped)
        {
            _rbuf->used_size += size;
            if (!handle_read())
                close();
        }
        else
        {
            handle_write(size);
            if (_wbuf->used_size > 0 && !try_write())
                close();
        }
#elif defined(KNET_USE_EPOLL)
        if ((0 != (evt.events & (EPOLLERR | EPOLLHUP)))
            || (0 != (evt.events & EPOLLIN) && !handle_can_read())
            || (0 != (evt.events & EPOLLOUT) && !handle_can_write()))
        {
            close();
            return;
        }
#else
        if (EVFILT_READ == evt.filter)
        {
            if (!handle_can_read())
                close();
        }
        else if (EVFILT_WRITE == evt.filter)
        {
            if (!handle_can_write())
                close();
        }
        else
        {
            close();
        }
#endif // KNET_USE_IOCP
    }

#ifdef KNET_USE_IOCP
    bool socket::try_read()
    {
        if (0 == _rbuf->unused_size())
            return false;

        memset(&_rbuf->ol, 0, sizeof(_rbuf->ol));
        _rbuf->buf = _rbuf->wptr();
        _rbuf->len = static_cast<ULONG>(_rbuf->unused_size());

        DWORD dw = 0;
        DWORD flag = 0;
        if (RAWSOCKET_ERROR == WSARecv(_rs, _rbuf, 1, &dw, &flag, &_rbuf->ol, nullptr)
            && ERROR_IO_PENDING != WSAGetLastError())
        {
            return false;
        }
        mark_flag(_flag, FlagRead);

        return true;
    }

    void socket::handle_write(size_t wrote)
    {
        kassert(_wbuf->used_size >= wrote);
        _wbuf->used_size -= wrote;
        if (_wbuf->used_size > 0)
            memmove(_wbuf->chunk, _wbuf->chunk + wrote, _wbuf->used_size);
        unmark_flag(_flag, FlagWrite);
    }
#else
    bool socket::handle_can_read()
    {
        ssize_t ret = 0;
        while (_rbuf->unused_size() > 0)
        {
            do
                ret = ::read(_rs, _rbuf->wptr(), _rbuf->unused_size());
            while (RAWSOCKET_ERROR == ret && EINTR == errno);

            if (RAWSOCKET_ERROR == ret)
            {
                if (EAGAIN == errno || EWOULDBLOCK == errno)
                    break;
                return false;
            }

            if (0 == ret)
                return false;

            _rbuf->used_size += ret;
            if (_rbuf->used_size > 0 && !handle_read())
                return false;
        }
        return true;
    }

    bool socket::handle_can_write()
    {
        if (!is_flag_marked(_flag, FlagWrite))
        {
            mark_flag(_flag, FlagWrite);
            if (_wbuf->used_size > 0 && !try_write())
                close();
        }
        return true;
    }
#endif // KNET_USE_IOCP

    bool socket::handle_read()
    {
#ifdef KNET_USE_IOCP
        unmark_flag(_flag, FlagRead);
#endif
        mark_flag(_flag, FlagCall);
        size_t ret = 0;
        do 
        {
            const size_t cost = _conn->on_recv_data(
                _rbuf->chunk + ret, _rbuf->used_size - ret);
            if (0 == cost || is_flag_marked(_flag, FlagClose))
                break;

            ret += cost;
        } while (ret < _rbuf->used_size);
        unmark_flag(_flag, FlagCall);

        if (ret > _rbuf->used_size)
            ret = _rbuf->used_size;
        _rbuf->used_size -= ret;

        if (0 == _rbuf->unused_size())
            mark_flag(_flag, FlagClose);

        if (is_flag_marked(_flag, FlagClose))
            return false;

        if (_rbuf->used_size > 0 && ret > 0)
            memmove(_rbuf->chunk, _rbuf->chunk + ret, _rbuf->used_size);

#ifdef KNET_USE_IOCP
        return try_read();
#else
        return true;
#endif
    }

    bool socket::try_write()
    {
#ifdef KNET_USE_IOCP

        _wbuf->buf = _wbuf->chunk;
        _wbuf->len = static_cast<decltype(_wbuf->len)>(_wbuf->used_size);
        memset(&_wbuf->ol, 0, sizeof(_wbuf->ol));
        DWORD dw = 0;
        if (RAWSOCKET_ERROR == WSASend(_rs, _wbuf, 1, &dw, 0, &_wbuf->ol, nullptr)
            && ERROR_IO_PENDING != WSAGetLastError())
        {
            return false;
        }
        mark_flag(_flag, FlagWrite);

#else // KNET_USE_IOCP

        kassert(_wbuf->used_size > 0);

        size_t wrote = 0;
        ssize_t ret = 0;
        while (wrote < _wbuf->used_size)
        {
            do
                ret = ::write(_rs, _wbuf->chunk + wrote, _wbuf->used_size - wrote);
            while (RAWSOCKET_ERROR == ret && EINTR == errno);

            if (RAWSOCKET_ERROR == ret)
            {
                if (EAGAIN == errno || EWOULDBLOCK == errno)
                    break;
                return false;
            }

            if (0 == ret)
                return false;
            
            wrote += ret;
        }

        if (wrote > 0)
        {
            _wbuf->used_size -= wrote;
            if (_wbuf->used_size > 0)
            {
                memmove(_wbuf->chunk, _wbuf->chunk + wrote, _wbuf->used_size);
                unmark_flag(_flag, FlagWrite);
            }
        }
#endif // KNET_USE_IOCP
        return true;
    }
}

