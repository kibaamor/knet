#pragma once
#include "knetfwd.h"


namespace knet
{
    class poller final : noncopyable
    {
    public:
        class listener
        {
        public:
            virtual ~listener() = default;

            virtual void on_poll(void* key, const pollevent_t& pollevent) = 0;
            virtual void on_postpoll() {}
        };

    public:
        explicit poller(listener* listener) noexcept;
        ~poller();

        bool add(rawsocket_t rawsocket, void* key) noexcept;
        bool poll() noexcept;

    private:
        listener* const _listener = nullptr;
        poller_t _poller = INVALID_POLLER;
        pollevent_t _pollevents[POLL_EVENT_NUM] = {};
    };
}
