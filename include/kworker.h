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

        virtual void add_work(connection_factory* cf, rawsocket_t rs) = 0;
    };

    class worker final
        : public workable
        , public poller
    {
    public:
        worker() = default;
        ~worker() override;

        void add_work(connection_factory* cf, rawsocket_t rs) override;

        bool poll() override;

    protected:
        void on_poll(void* key, const rawpollevent_t& evt) override;

    private:
        std::vector<socket*> _adds;
    };

    class async_worker final
        : public workable
        , noncopyable
    {
    public:
        async_worker() = default;
        ~async_worker() override;

        void add_work(connection_factory* cf, rawsocket_t rs) override;

        bool start(size_t thread_num);
        void stop();

    private:
        struct info
        {
            bool r = true;
            void* q = nullptr;
            std::thread* t = nullptr;
        };
        static void worker_thread(info* i);

    private:
        std::vector<info> _infos;
        size_t _index = 0;
    };
}
