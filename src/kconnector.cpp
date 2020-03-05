#include "../include/kconnector.h"
#include "../include/kworker.h"
#include "kinternal.h"


namespace knet
{
    connector::connector(const address& addr, workable* wkr,
        bool reconn, size_t interval_ms)
        : _addr(addr), _wkr(wkr), _reconn(reconn), _interval_ms(interval_ms)
    {
        kassert(nullptr != _wkr);
        _rs = ::socket(_addr.get_family(), SOCK_STREAM, IPPROTO_TCP);
    }

    connector::~connector()
    {
        close_rawsocket(_rs);
    }

    bool connector::update(size_t ms)
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
                if (INVALID_RAWSOCKET != _rs)
                    on_reconnect();
            }
        }

        if (INVALID_RAWSOCKET != _rs)
        {
            auto sa = reinterpret_cast<const sockaddr*>(&_addr.get_sockaddr());
            if (RAWSOCKET_ERROR != connect(_rs, sa, sizeof(sockaddr_storage))
                && set_rawsocket_nonblock(_rs)
                && set_rawsocket_cloexec(_rs))
            {
                _wkr->add_work(_rs);
                _succ = true;
                _rs = INVALID_RAWSOCKET;
                return false;
            }
            else
            {
                close_rawsocket(_rs);
                if (!_reconn)
                    on_reconnect_failed();
            }
        }
        return true;
    }
}
