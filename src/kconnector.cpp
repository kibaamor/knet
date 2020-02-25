#include "../include/kconnector.h"
#include "../include/kworker.h"


namespace knet
{
    connector::connector(const address& addr, workable* wkr, connection_factory* cf) noexcept
        : connector(addr, wkr, cf, false, 0, nullptr)
    {
    }

    connector::connector(const address& addr, workable* wkr, connection_factory* cf, 
        bool reconn, size_t interval_ms, listener* listener) noexcept
        : _addr(addr), _wkr(wkr), _cf(cf), _reconn(reconn), _interval_ms(interval_ms), _listener(listener)
    {
        kassert(nullptr != _wkr);
        kassert(nullptr != _cf);
        _rs = ::socket(_addr.get_family(), SOCK_STREAM, 0);
    }

    connector::~connector()
    {
        if (INVALID_RAWSOCKET != _rs)
        {
            ::closesocket(_rs);
            _rs = INVALID_RAWSOCKET;
        }
    }

    bool connector::update(size_t ms) noexcept
    {
        if (_succ || (INVALID_RAWSOCKET == _rs && !_reconn))
            return false;

        if (INVALID_RAWSOCKET == _rs && _reconn)
        {
            _last_interval_ms += ms;
            if (_last_interval_ms >= _interval_ms)
            {
                _last_interval_ms -= _interval_ms;
                _rs = ::socket(_addr.get_family(), SOCK_STREAM, 0);
                if (INVALID_RAWSOCKET != _rs && nullptr != _listener)
                    _listener->on_reconn(_addr);
            }
        }

        if (INVALID_RAWSOCKET != _rs)
        {
            auto sa = reinterpret_cast<const sockaddr*>(&_addr.get_sockaddr());
            if (RAWSOCKET_ERROR != connect(_rs, sa, sizeof(sockaddr_storage)))
            {
                _wkr->add_work(_cf, _rs);
                _succ = true;
                _rs = INVALID_RAWSOCKET;
                return false;
            }
            else
            {
                ::closesocket(_rs);
                _rs = INVALID_RAWSOCKET;

                if (!_reconn && nullptr != _listener)
                    _listener->on_reconn_failed(_addr);
            }
        }
        return true;
    }
}
