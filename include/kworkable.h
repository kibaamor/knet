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

        virtual void add_work(rawsocket_t rawsocket) = 0;
    };
    class poller;

    class socketid_gener
    {
    public:
        socketid_gener(socketid_t start = 0, socketid_t step = 1) noexcept
            : _val(start), _step(step)
        {
        }

        socketid_t operator()() noexcept
        {
            _val += _step;
            return _val;
        }

    private:
        socketid_t _val;
        socketid_t _step;
    };

    class worker final
        : public workable
        , public poller::listener
        , noncopyable
    {
    public:
        worker(connection_factory* conn_factory, 
            socketid_gener sid_gener = socketid_gener()) noexcept;
        ~worker() override;

        void update() noexcept;

        void add_work(rawsocket_t rawsocket) override;
        void on_poll(void* key, const pollevent_t& pollevent) override;

        connection_factory* get_connection_factory() const { return _conn_factory; }
        socketid_t get_next_socketid() { return _sid_gener(); }

    private:
        connection_factory* const _conn_factory;
        socketid_gener _sid_gener;
        poller _poller;

        std::vector<socket*> _adds;

    };

    class async_worker final
        : public workable
        , noncopyable
    {
    public:
        explicit async_worker(connection_factory* conn_factory) noexcept;
        ~async_worker() override;

        void add_work(rawsocket_t rawsocket) override;

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
        connection_factory* const _conn_factory;

        std::vector<info> _infos;
        size_t _index = 0;
    };
}
