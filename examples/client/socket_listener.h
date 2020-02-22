#pragma once
#include <ksocket.h>
#include <kconnector.h>
#include <atomic>


namespace client
{
    class socket_listener
        : public knet::socket::listener
        , public knet::connector::listener
    {
    public:
        socket_listener(bool enable_log, uint32_t max_send_delay);
        ~socket_listener() = default;

        void on_conn(knet::socket& sock) override;
        void on_close(knet::socket& sock) override;
        size_t on_data(knet::socket& sock, char* data, size_t size) override;
        void on_timer(knet::socket& sock, const knet::userdata& ud, int64_t absms) override;

        void on_reconn(const knet::address& addr) override;
        void on_reconn_failed(const knet::address& addr) override;

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
        int32_t check_package(char* data, size_t size);
        void send_package(knet::socket& sock);

    private:
        const bool _enable_log = false;
        const uint32_t _max_send_delay = 1000;

        bool need_disconnect = false;
        std::atomic<int> _conn_num = {0};
        std::atomic<int64_t> _total_wrote = {0};
    };
}
