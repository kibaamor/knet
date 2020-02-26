#pragma once
#include <cstdint>
#include <atomic>
#include <thread>


class echo_mgr
{
public:
    static echo_mgr& get_intance();

public:
    void update(int64_t delta_ms);

    int64_t get_conn_num() const { return _conn_num.load(); }
    void inc_conn_num() { _conn_num.fetch_add(1); }
    void dec_conn_num() { _conn_num.fetch_sub(1); }

    int64_t get_total_send() const { return _total_send.load(); }
    void add_total_send(int64_t n) { _total_send.fetch_add(n); }
    void zero_total_send() { _total_send.store(0); }

    void set_disconnect_all() { _disconnect_all = true; }
    bool get_disconnect_all() const { return _disconnect_all; }

    void check_console_input();

private:
    echo_mgr() {}
    ~echo_mgr();

private:
    int64_t _total_ms = 0;

    std::atomic<int64_t> _conn_num = { 0 };
    std::atomic<int64_t> _total_send = { 0 };
    bool _disconnect_all = false;
    std::thread* _t = nullptr;
};
