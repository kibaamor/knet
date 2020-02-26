#pragma once
#include "kconnection.h"
#include "kpoller.h"
#include <vector>
#include <thread>


namespace knet
{
    class workable
    {
    public:
        virtual ~workable() = default;

        virtual void add_work(rawsocket_t rs) = 0;
    };

    class worker
        : public workable
        , public poller
    {
    public:
        worker(connection_factory* cf);
        ~worker() override;

        void add_work(rawsocket_t rs) override;

        bool poll() override;

    protected:
        void on_poll(void* key, const rawpollevent_t& evt) override;

    private:
        connection_factory* const _cf;
        std::vector<socket*> _adds;
    };

    class async_worker
        : public workable
        , noncopyable
    {
    public:
        async_worker(connection_factory_builder* cfb);
        ~async_worker() override;

        void add_work(rawsocket_t rs) override;

        virtual bool start(size_t thread_num);
        virtual void stop();

    protected:
        virtual worker* create_worker(connection_factory* cf)
        {
            return new worker(cf);
        }

    private:
        struct info
        {
            bool r = true;
            async_worker* aw = nullptr;
            void* q = nullptr;
            std::thread* t = nullptr;
        };
        static void worker_thread(info* i);

    private:
        connection_factory_builder* const _cfb;
        std::vector<info> _infos;
        size_t _index = 0;
    };
}
