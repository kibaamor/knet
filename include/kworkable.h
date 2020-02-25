#pragma once
#include "kconnection.h"
#include "kpoller.h"
#include <vector>
#include <unordered_map>
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
        , public poller::listener
        , noncopyable
    {
    public:
        worker() noexcept;
        ~worker() override;

        void update() noexcept;

        void add_work(connection_factory* cf, rawsocket_t rs) override;
        void on_poll(void* key, const rawpollevent_t& evt) override;

    private:
        poller _poller;
        std::vector<socket*> _adds;
    };

    class async_worker final
        : public workable
        , noncopyable
    {
    public:
        async_worker() noexcept = default;
        ~async_worker() override;

        void add_work(connection_factory* cf, rawsocket_t rs) override;

        bool start(size_t thread_num);
        void stop() noexcept;

    private:
        struct info
        {
            bool r = true;
            void* q = nullptr;
            worker* w = nullptr;
            std::thread* t = nullptr;
        };
        static void worker_thread(info* i);

    private:
        std::vector<info> _infos;
        size_t _index = 0;
    };
}
