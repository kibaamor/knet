#pragma once
#include "kaddress.h"


namespace knet
{
    class workable;
    class connection_factory;

    class connector final : noncopyable
    {
    public:
        connector(const address& addr, workable* wkr, connection_factory* cf, 
            bool reconn = true, size_t interval_ms = 1000);
        ~connector();

        bool update(size_t ms);

        virtual void on_reconn() {}
        virtual void on_reconn_failed() {}

        const address& get_address() const { return _addr; }

    private:
        const address _addr;
        workable* const _wkr = nullptr;
        connection_factory* const _cf = nullptr;

        const bool _reconn = false;
        const size_t _interval_ms = 0;

        rawsocket_t _rs = INVALID_RAWSOCKET;
        size_t _last_interval_ms = 0;
        bool _succ = false;
    };
}
