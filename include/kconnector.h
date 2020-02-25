#pragma once
#include "kworkable.h"
#include "kaddress.h"


namespace knet
{
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
        connector(const address& addr, workable* workable) noexcept;
        connector(const address& addr, workable* workable, bool reconn, 
            size_t interval_ms, listener* listener) noexcept;
        ~connector();

        bool update(size_t ms) noexcept;

    private:
        const address _addr;
        workable* const _workable = nullptr;
        const bool _reconn = false;
        const size_t _interval_ms = 0;
        listener* const _listener = nullptr;

        rawsocket_t _rawsocket = INVALID_RAWSOCKET;
        size_t _last_interval_ms = 0;
        bool _succ = false;
    };
}
