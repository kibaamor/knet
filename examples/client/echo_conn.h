#pragma once
#include <kconnection.h>
#include <kaddress.h>
#include <kconnector.h>
#include <atomic>


class echo_conn_mgr;

class echo_conn : public knet::connection
{
public:
    void on_connected() override;
    size_t on_recv_data(char* data, size_t size) override;
    void on_disconnect() override;

    void set_conn_mgr(echo_conn_mgr* mgr) { _mgr = mgr; }

private:
    void send_package();
    int32_t check_package(char* data, size_t size);

protected:
    virtual void on_attach_socket(knet::rawsocket_t rs) override;

private:
    knet::address _sockaddr;
    knet::address _peeraddr;
    echo_conn_mgr* _mgr = nullptr;
};

//-------------------------------------------------------------------------

class echo_conn_mgr
    : public knet::connection_factory
    , public knet::connector::listener
{
public:
    knet::connection* create_connection() override;

    void destroy_connection(knet::connection* conn) override;

    void on_reconn(const knet::address& addr) override;

    void on_reconn_failed(const knet::address& addr) override;

    int64_t get_conn_num() const noexcept
    {
        return _conn_num.load(std::memory_order_acquire);
    }

    int64_t get_total_send() const noexcept
    {
        return _total_send.load(std::memory_order_acquire);
    }

    void add_send(int64_t num) noexcept
    {
        _total_send.fetch_add(num, std::memory_order_release);
    }

    void clear_total_send() noexcept
    {
        _total_send.store(0, std::memory_order_release);
    }

    void set_disconnect_all() noexcept { _disconnect_all = true; }
    bool get_disconnect_all() const noexcept { return _disconnect_all; }

private:
    std::atomic<int64_t> _conn_num = { 0 };
    std::atomic<int64_t> _total_send = { 0 };
    bool _disconnect_all = false;
};

//-------------------------------------------------------------------------

void check_input(echo_conn_mgr* mgr);
