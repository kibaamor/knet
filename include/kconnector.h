#pragma once
#include "kaddress.h"


namespace knet
{
    class workable;
    class connector : noncopyable
    {
    public:
        connector(const address& addr, workable* wkr,
            bool reconn = true, size_t interval_ms = 1000);
        virtual ~connector();

        bool update(size_t ms);

        virtual void on_reconnect() {}
        virtual void on_reconnect_failed() {}

        const address& get_address() const { return _addr; }

    private:
        const address _addr;
        workable* const _wkr = nullptr;

        const bool _reconn = false;
        const size_t _interval_ms = 0;

        rawsocket_t _rs = INVALID_RAWSOCKET;
        size_t _last_interval_ms = 0;
        bool _succ = false;
    };
}
