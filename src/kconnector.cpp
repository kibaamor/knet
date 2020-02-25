#include "../include/kconnector.h"


namespace
{
    using namespace knet;
    rawsocket_t create_rawsocket(int domain) noexcept
    {
        rawsocket_t rawsocket = ::socket(domain, SOCK_STREAM, 0);
        if (INVALID_RAWSOCKET != rawsocket)
            set_rawsocket_sndrcvbufsize(rawsocket, SOCKET_SNDRCVBUF_SIZE);
        return rawsocket;
    }
}

namespace knet
{
    connector::connector(const address& addr, workable* workable) noexcept
        : connector(addr, workable, false, 0, nullptr)
    {
    }

    connector::connector(const address& addr, workable* workable, 
        bool reconn, size_t interval_ms, listener* listener) noexcept
        : _addr(addr), _workable(workable), _reconn(reconn)
        , _interval_ms(interval_ms), _listener(listener)
    {
        _rawsocket = create_rawsocket(_addr.get_family());
    }

    connector::~connector()
    {
        if (INVALID_RAWSOCKET != _rawsocket)
        {
            ::closesocket(_rawsocket);
            _rawsocket = INVALID_RAWSOCKET;
        }
    }

    bool connector::update(size_t ms) noexcept
    {
        if (_succ || (INVALID_RAWSOCKET == _rawsocket && !_reconn))
            return false;

        if (INVALID_RAWSOCKET == _rawsocket && _reconn)
        {
            _last_interval_ms += ms;
            if (_last_interval_ms >= _interval_ms)
            {
                _last_interval_ms -= _interval_ms;
                _rawsocket = create_rawsocket(_addr.get_family());
                if (INVALID_RAWSOCKET != _rawsocket && nullptr != _listener)
                    _listener->on_reconn(_addr);
            }
        }
        if (INVALID_RAWSOCKET != _rawsocket)
        {
            if (RAWSOCKET_ERROR != connect(_rawsocket, 
                reinterpret_cast<const sockaddr*>(&_addr.get_sockaddr()),
                sizeof(decltype(_addr.get_sockaddr()))))
            {
                _workable->add_work(_rawsocket);
                _succ = true;
                _rawsocket = INVALID_RAWSOCKET;
                return false;
            }
            else if (!_reconn && nullptr != _listener)
            {
                _listener->on_reconn_failed(_addr);
            }
            _rawsocket = INVALID_RAWSOCKET;
        }
        return true;
    }
}
