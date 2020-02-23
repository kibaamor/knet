#pragma once
#include "kaddress.h"
#include "kworkable.h"


namespace knet
{
    class listener
        : public poller::listener
        , noncopyable
    {
    public:
        listener(const address& addr, workable* workable) noexcept;
        ~listener() override;

        bool start() noexcept;
        void stop() noexcept;

        void update() noexcept { _poller.poll(); }

        void on_poll(void* key, const pollevent_t& pollevent) override;
#ifdef KNET_USE_IOCP
        void on_postpoll() override { post_accept(); }
#endif

    private:
        const address _addr;
        workable* _workable = nullptr;
        poller _poller;
        rawsocket_t _rawsocket = INVALID_RAWSOCKET;

#ifdef KNET_USE_IOCP
        void post_accept() noexcept;
        struct accept_io;
        accept_io* _pending_accepts = nullptr;
        accept_io* _free_pending_accepts = nullptr;
#endif
    };
}
