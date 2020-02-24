#pragma once
#include "ksocket.h"
#include "kpoller.h"
#include "kspscqueue.h"
#include <thread>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>


namespace knet
{
    class workable
    {
    public:
        virtual ~workable() = default;

        virtual bool addwork(rawsocket_t rawsocket) = 0;
    };

    class worker;
    class socket_creator
    {
    public:
        virtual ~socket_creator() = default;

        virtual socket* create_socket(worker* wkr, rawsocket_t rawsock) = 0;
    };

    class worker final
        : public workable
        , public poller::listener
        , noncopyable
    {
    public:
        worker(socket::listener* listener, socket_creator* creator = nullptr, 
            socketid_t start_sid = 0, socketid_t sid_inc = 1) noexcept;
        ~worker() override = default;

        void update(int64_t absms) noexcept;

        int64_t set_timer(socket& sock, int64_t absms, const userdata& ud);
        void del_timer(socket& sock, int64_t absms);

        void on_socket_destroy(socket& sock);

        bool addwork(rawsocket_t rawsocket) override;
        void on_poll(void* key, const pollevent_t& pollevent) override;

        socketid_t get_next_socketid();
        socket::listener* get_socket_listener() const { return _listener; }

    private:
        socket::listener* const _listener = nullptr;
        socket_creator* const _creator = nullptr;
        const socketid_t _sid_inc = 1;
        socketid_t _next_sid = 0;
        poller _poller;

        std::unordered_map<socketid_t, socket*> _socks;

        struct timer_key
        {
            int64_t tid;
            socketid_t sid;
            timer_key(int64_t t = 0, socketid_t s = 0) : tid(t), sid(s) {}
        };
        struct timer_key_cmp
        {
            bool operator()(
                const timer_key& lhs, const timer_key& rhs) const noexcept;
        };
        int64_t _next_tid = 0;
        std::map<timer_key, userdata, timer_key_cmp> _timers;
        std::map<timer_key, userdata, timer_key_cmp> _timer_to_add;
        std::set<timer_key, timer_key_cmp> _timer_to_del;
    };

    class async_worker final
        : public workable
        , noncopyable
    {
    public:
        explicit async_worker(socket::listener* listener, 
            socket_creator* creator = nullptr) noexcept;
        ~async_worker() override;

        bool start(size_t thread_num);
        void stop() noexcept;

        bool addwork(rawsocket_t rawsocket) override;

    private:
        socket::listener *const _listener = nullptr;
        socket_creator* const _creator = nullptr;
        bool _running = false;

        struct threadinfo
        {
            std::thread* thd = nullptr;
            worker* wkr = nullptr;
            spsc_queue<rawsocket_t, 1024>* wq = nullptr;
        };
        std::vector<threadinfo> _threadinfos;
        size_t _next_thread_index = 0;
    };
}
