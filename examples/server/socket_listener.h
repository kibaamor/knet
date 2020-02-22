#pragma once
#include <ksocket.h>
#include <atomic>


namespace server
{
    class socket_listener
        : public knet::socket::listener
    {
    public:
        socket_listener(bool enable_log);
        ~socket_listener() = default;

        void on_conn(knet::socket& sock) override;
        void on_close(knet::socket& sock) override;
        size_t on_data(knet::socket& sock, char* data, size_t size) override;

        void disconnect_all()
        {
            need_disconnect = true;
        }
        int get_conn_num() const
        {
            return _conn_num.load(std::memory_order_acquire);
        }
        int64_t get_total_wrote() const
        {
            return _total_wrote.load(std::memory_order_acquire);
        }
        void clear_total_wrote()
        {
            _total_wrote.store(0, std::memory_order_release);
        }

    private:
        const bool _enable_log = false;

        bool need_disconnect = false;
        std::atomic<int> _conn_num = {0};
        std::atomic<int64_t> _total_wrote = {0};
    };
}
