#pragma once
#include <cstdint>
#include <atomic>
#include <thread>

#pragma pack(push)
#pragma pack(1)
struct echo_package {
    uint32_t size; // total package size
    uint32_t id; // package id

    static constexpr uint32_t get_hdr_size()
    {
        return sizeof(size) + sizeof(id);
    }

    char* get_data()
    {
        return reinterpret_cast<char*>(this) + echo_package::get_hdr_size();
    }

    uint32_t& last_u32()
    {
        auto end = reinterpret_cast<char*>(this) + size;
        return *reinterpret_cast<uint32_t*>(end - sizeof(uint32_t));
    }
};
#pragma pack(pop)

class echo_mgr {
public:
    static echo_mgr& get_instance();

public:
    void update(int64_t delta_ms);

    void set_is_server(bool b) { _is_server = b; }
    bool get_is_server() const { return _is_server; }

    void set_enable_log(bool b) { _enable_log = b; }
    bool get_enable_log() const { return _enable_log; }

    int64_t get_delay_ms() const;
    int64_t get_max_delay_ms() const { return _max_delay_ms; }
    void set_max_delay_ms(int64_t ms) { _max_delay_ms = ms; }

    int64_t get_max_idle_ms() const { return _max_idle_ms; }
    void set_max_idle_ms(int64_t ms) { _max_idle_ms = ms; }

    int64_t get_conn_num() const { return _conn_num.load(); }
    void inc_conn_num() { _conn_num.fetch_add(1); }
    void dec_conn_num() { _conn_num.fetch_sub(1); }

    int64_t get_total_send() const { return _total_send.load(); }
    void add_total_send(int64_t n) { _total_send.fetch_add(n); }
    void zero_total_send() { _total_send.store(0); }

    int64_t get_total_recv_pkg_num() const { return _total_recv_pkg_num.load(); }
    void inc_total_recv_pkg_num() { _total_recv_pkg_num.fetch_add(1); }
    void zero_total_recv_pkg_num() { _total_recv_pkg_num.store(0); }

    void set_disconnect_all() { _disconnect_all = true; }
    bool get_disconnect_all() const { return _disconnect_all; }

    void check_console_input();

private:
    echo_mgr() { }
    ~echo_mgr();

private:
    int64_t _total_ms = 0;

    bool _is_server = true;
    bool _enable_log = false;

    int64_t _max_delay_ms = 0;
    int64_t _max_idle_ms = 0;

    std::atomic<int64_t> _conn_num = { 0 };
    std::atomic<int64_t> _total_send = { 0 };
    std::atomic<int64_t> _total_recv_pkg_num = { 0 };

    bool _disconnect_all = false;
    std::thread* _t = nullptr;
};
