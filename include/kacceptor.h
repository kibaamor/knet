#pragma once
#include "kaddress.h"
#include "kpoller.h"


namespace knet
{
    class workable;
    class connection_factory;

    class acceptor
        : public poller::listener
        , noncopyable
    {
    public:
        acceptor(workable* wkr, connection_factory* cf);
        ~acceptor() override;

        bool start(const address& addr);
        void stop();

        void update() { _poller.poll(); }

        void on_poll(void* key, const rawpollevent_t& evt) override;
#ifdef KNET_USE_IOCP
        void on_postpoll() override { post_accept(); }
#endif

    private:
        workable* const _wkr;
        connection_factory* _cf;

        sa_family_t _family;
        poller _poller;
        rawsocket_t _rs = INVALID_RAWSOCKET;

#ifdef KNET_USE_IOCP
        void post_accept();
        struct accept_io;
        accept_io* _ios = nullptr;
        accept_io* _free_ios = nullptr;
#endif
    };
}
