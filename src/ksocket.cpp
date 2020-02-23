#include "../include/ksocket.h"
#include "../include/kworkable.h"
#include <cassert>
#include <cstring>


namespace
{
    constexpr uint8_t FlagRead = 1u << 0u;
    constexpr uint8_t FlagWrite = 1u << 1u;
    constexpr uint8_t FlagCall = 1u << 2u;
    constexpr uint8_t FlagClose = 1u << 3u;

    inline bool is_flag_marked(uint8_t& flag, uint8_t test) noexcept
    {
        return (0 != (flag & test));
    }
    inline void mark_flag(uint8_t& flag, uint8_t test) noexcept
    {
        assert(!is_flag_marked(flag, test));
        flag |= test;
    }
    inline void unmark_flag(uint8_t& flag, uint8_t test) noexcept
    {
        assert(is_flag_marked(flag, test));
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
        char databuf[SOCKET_RWBUF_SIZE] = {};
        size_t datasize = 0;
#ifdef KNET_USE_IOCP
        WSAOVERLAPPED ol = {};
#endif

        char* rwptr() noexcept { return databuf + datasize; }
        size_t validsize() const  noexcept { return sizeof(databuf) - datasize; }
    };
}

namespace knet
{
    socket::socket(worker* wkr, rawsocket_t rawsock, socketid_t socketid, 
        listener* listener) noexcept
        : _worker(wkr), _rawsocket(rawsock)
        , _socketid(socketid), _listener(listener)
    {
        assert(nullptr != _worker);
        assert(INVALID_RAWSOCKET != _rawsocket);
        assert(nullptr != _listener);
    }

    socket::~socket()
    {
        if (!(_flag == FlagClose || _flag == 0))
        {
            __asm int 3;
        };
        _worker->on_socket_destroy(*this);
        assert(INVALID_RAWSOCKET == _rawsocket);
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
    }

    int64_t socket::set_abs_timer(int64_t absms, const userdata& ud)
    {
        return _worker->set_timer(*this, absms, ud);
    }

    void socket::del_timer(int64_t absms)
    {
        _worker->del_timer(*this, absms);
    }

    bool socket::start() noexcept
    {
        if (nullptr == _rbuf)
            _rbuf = new sockbuf();
        if (nullptr == _wbuf)
            _wbuf = new sockbuf();

        mark_flag(_flag, FlagCall);
        _listener->on_conn(*this);
        unmark_flag(_flag, FlagCall);

        if (is_flag_marked(_flag, FlagClose)
#ifdef KNET_USE_IOCP
            || !try_read()
#endif
            )
        {
            _listener->on_close(*this);
            detail::close_rawsocket(_rawsocket);
            return false;
        }
        return true;
    }

    void socket::close() noexcept
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
                CancelIoEx(reinterpret_cast<HANDLE>(_rawsocket), nullptr);
#endif
            }
            return;
        }

        mark_flag(_flag, FlagCall);
        _listener->on_close(*this);
        unmark_flag(_flag, FlagCall);

#ifndef KNET_GRACEFUL_CLOSE_SOCKET
        linger lg = {1, 0};
        setsockopt(_rawsocket, SOL_SOCKET, SO_LINGER, 
            reinterpret_cast<const char*>(&lg), sizeof(lg));
#endif

#ifdef _WIN32
#define SHUT_RD SD_RECEIVE
#endif
        shutdown(_rawsocket, SHUT_RD);
        detail::close_rawsocket(_rawsocket);
        delete this;
    }

    bool socket::write(const writebuf* buf, size_t num) noexcept
    {
        if (is_flag_marked(_flag, FlagClose))
            return false;

        size_t size = 0;
        for (size_t i = 0; i < num; ++i)
            size += buf[i].size;
        if (size > _wbuf->validsize())
            return false;

        for (size_t i = 0; i < num; ++i)
        {
            if (buf[i].size > 0 && nullptr != buf[i].data)
            {
                memcpy(_wbuf->rwptr(), buf[i].data, buf[i].size);
                _wbuf->datasize += buf[i].size;
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

    void socket::on_pollevent(const pollevent_t &pollevent) noexcept
    {
#ifdef KNET_USE_IOCP
        if (0 == pollevent.dwNumberOfBytesTransferred)
        {
            if (&_rbuf->ol == pollevent.lpOverlapped)
                unmark_flag(_flag, FlagRead);
            else
                unmark_flag(_flag, FlagWrite);
            close();
            return;
        }
        if (&_rbuf->ol == pollevent.lpOverlapped)
        {
            _rbuf->datasize += static_cast<size_t>(pollevent.dwNumberOfBytesTransferred);
            if (!handle_read() || is_flag_marked(_flag, FlagClose))
                close();
        }
        else
        {
            handle_write(static_cast<size_t>(pollevent.dwNumberOfBytesTransferred));
            if ((_wbuf->datasize > 0 && !try_write()) || is_flag_marked(_flag, FlagClose))
                close();
        }
#else // KNET_USE_IOCP
        if (0 != (pollevent.events & (EPOLLERR | EPOLLHUP)))
        {
            close();
            return;
        }
        if (0 != (pollevent.events & EPOLLIN))
        {
            size_t count = 0;
            while (true)
            {
                const auto ret = recv(_rawsocket, _rbuf->rwptr(), _rbuf->validsize(), MSG_DONTWAIT);
                if (RAWSOCKET_ERROR == ret)
                {
                    if (EAGAIN != errno && EWOULDBLOCK != errno)
                        count = 0;
                    break;
                }
                else if (0 == ret)
                {
                    break;
                }
                else
                {
                    _rbuf->datasize += ret;
                    ++count;
                }
                if (0 == _rbuf->validsize())
                {
                    if (!handle_read())
                    {
                        count = 0;
                        break;
                    }
                }
            }
            if (0 == count || (_rbuf->datasize > 0 && !handle_read()))
            {
                close();
                return;
            }
        }
        if (0 != (pollevent.events & EPOLLOUT) && !is_flag_marked(_flag, FlagWrite))
        {
            mark_flag(_flag, FlagWrite);
            if (_wbuf->datasize > 0 && !try_write())
            {
                close();
                return;
            }
        }
#endif // KNET_USE_IOCP
    }

    bool socket::get_sockaddr(address &addr) const noexcept
    {
        auto& sa = addr.get_sockaddr();
        socklen_t len = sizeof(sa);
        return (0 == getsockname(_rawsocket, reinterpret_cast<sockaddr*>(&sa), &len));
    }

    bool socket::get_peeraddr(address &addr) const noexcept
    {
        auto& sa = addr.get_sockaddr();
        socklen_t len = sizeof(sa);
        return (0 == getpeername(_rawsocket, reinterpret_cast<sockaddr*>(&sa), &len));
    }

#ifdef KNET_USE_IOCP
    bool socket::try_read() noexcept
    {
        memset(&_rbuf->ol, 0, sizeof(_rbuf->ol));
        _rbuf->buf = _rbuf->rwptr();
        _rbuf->len = static_cast<decltype(_rbuf->len)>(_rbuf->validsize());

        DWORD dw = 0;
        DWORD flag = 0;
        if (RAWSOCKET_ERROR == WSARecv(_rawsocket, _rbuf, 1, &dw, &flag, &_rbuf->ol, nullptr)
            && ERROR_IO_PENDING != WSAGetLastError())
        {
            return false;
        }
        mark_flag(_flag, FlagRead);

        return true;
    }

    void socket::handle_write(size_t wrote) noexcept
    {
        assert(_wbuf->datasize >= wrote);
        _wbuf->datasize -= wrote;
        if (_wbuf->datasize > 0)
            memmove(_wbuf->databuf, _wbuf->databuf + wrote, _wbuf->datasize);
        unmark_flag(_flag, FlagWrite);
    }
#endif // KNET_USE_IOCP

    bool socket::handle_read() noexcept
    {
#ifdef KNET_USE_IOCP
        unmark_flag(_flag, FlagRead);
#endif
        mark_flag(_flag, FlagCall);
        size_t ret = 0;
        do 
        {
            const size_t cost = _listener->on_data(*this, 
                _rbuf->databuf + ret, _rbuf->datasize - ret);
            if (0 == cost)
                break;

            ret += cost;
        } while (ret < _rbuf->datasize);
        unmark_flag(_flag, FlagCall);

        if (ret > _rbuf->datasize)
            ret = _rbuf->datasize;
        _rbuf->datasize -= ret;

        if (0 == _rbuf->validsize())
            mark_flag(_flag, FlagClose);

        if (is_flag_marked(_flag, FlagClose))
            return false;

        if (_rbuf->datasize > 0 && ret > 0)
            memmove(_rbuf->databuf, _rbuf->databuf + ret, _rbuf->datasize);

#ifdef KNET_USE_IOCP
        return try_read();
#else
        return true;
#endif
    }

    bool socket::try_write() noexcept
    {
#ifdef KNET_USE_IOCP
        _wbuf->buf = _wbuf->databuf;
        _wbuf->len = static_cast<decltype(_wbuf->len)>(_wbuf->datasize);
        memset(&_wbuf->ol, 0, sizeof(_wbuf->ol));
        DWORD dw = 0;
        if (RAWSOCKET_ERROR == WSASend(_rawsocket, _wbuf, 1, &dw, 0, &_wbuf->ol, nullptr)
            && ERROR_IO_PENDING != WSAGetLastError())
        {
            return false;
        }
        mark_flag(_flag, FlagWrite);
#else // KNET_USE_IOCP
        size_t wrote = 0;
        while (wrote < _wbuf->datasize)
        {
            const auto ret = send(_rawsocket, _wbuf->databuf + wrote, 
                _wbuf->datasize - wrote, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (RAWSOCKET_ERROR == ret && (EAGAIN == errno || EWOULDBLOCK == errno))
                break;
            else if (RAWSOCKET_ERROR == ret || 0 == ret)
                return false;
            else
                wrote += ret;
        }
        _wbuf->datasize -= wrote;
        if (_wbuf->datasize > 0)
        {
            memmove(_wbuf->databuf, _wbuf->databuf + wrote, _wbuf->datasize);
            unmark_flag(_flag, FlagWrite);
        }
#endif // KNET_USE_IOCP
        return true;
    }
}

