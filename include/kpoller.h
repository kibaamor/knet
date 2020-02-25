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

            virtual void on_poll(void* key, const rawpollevent_t& evt) = 0;
            virtual void on_postpoll() {}
        };

    public:
        explicit poller(listener* l);
        ~poller();

        bool add(rawsocket_t rs, void* key);
        bool poll();

    private:
        listener* const _l = nullptr;
        rawpoller_t _rp = INVALID_RAWPOLLER;
        rawpollevent_t _evts[POLL_EVENT_NUM] = {};
    };
}
