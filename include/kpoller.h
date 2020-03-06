#pragma once
#include "knetfwd.h"


namespace knet
{
    class poller : noncopyable
    {
    public:
        poller();
        virtual ~poller();

        virtual bool add(rawsocket_t rs, void* key);
        virtual void poll();

    protected:
        virtual void on_poll(void* key, const rawpollevent_t& evt) = 0;

    private:
        rawpoller_t _rp = INVALID_RAWPOLLER;
        rawpollevent_t _evts[POLL_EVENT_NUM] = {};
    };
}
