#pragma once
#include "kaddress.h"


namespace knet
{
    class workable;
    class connection_factory;

    class connector final : noncopyable
    {
    public:
        class listener
        {
        public:
            virtual ~listener() = default;

            virtual void on_reconn(const address& addr) = 0;
            virtual void on_reconn_failed(const address& addr) = 0;
        };

    public:
        connector(const address& addr, workable* wkr, connection_factory* cf) noexcept;
        connector(const address& addr, workable* wkr, connection_factory* cf, 
            bool reconn, size_t interval_ms, listener* listener) noexcept;
        ~connector() noexcept;

        bool update(size_t ms) noexcept;

    private:
        const address _addr;
        workable* const _wkr = nullptr;
        connection_factory* const _cf = nullptr;

        const bool _reconn = false;
        const size_t _interval_ms = 0;
        listener* const _listener = nullptr;

        rawsocket_t _rs = INVALID_RAWSOCKET;
        size_t _last_interval_ms = 0;
        bool _succ = false;
    };
}
