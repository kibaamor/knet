#pragma once
#include <atomic>
#include <knet/kutils.h>

#pragma pack(push)
#pragma pack(1)
struct echo_package {
    // assume that we have same endian between sender and receiver
    uint32_t size; // total package size (include header size)
    uint32_t id; // package id

    static constexpr uint32_t get_hdr_size()
    {
        return sizeof(size) + sizeof(id);
    }

    uint32_t& last_u32()
    {
        auto end = reinterpret_cast<char*>(this) + size;
        return *reinterpret_cast<uint32_t*>(end - sizeof(uint32_t));
    }
};
#pragma pack(pop)

struct echo_mgr final {
    bool is_server = true;
    uint32_t max_delay_ms = 0;
    uint32_t max_idle_ms = 0;
    uint32_t random_disconnect = 10000;
    uint32_t sockbuf_size = 128 * 1024;

    std::atomic_bool can_log = { false };
    std::atomic_bool disconnect_all = { false };

    std::atomic_llong inst_num = { 0 };
    std::atomic_llong conn_num = { 0 };
    std::atomic_llong total_send = { 0 };
    std::atomic_llong total_recv = { 0 };

    ~echo_mgr();
    void update(int64_t delta_ms);
    void check_console_input();

    int64_t get_rand_delay_ms() const { return knet::u32rand_between(0, max_delay_ms); }

private:
    int64_t _total_ms = 0;
    std::thread* _t = nullptr;
};

template <typename T>
echo_mgr& operator<<(echo_mgr& m, T&& v)
{
    if (m.can_log) {
        std::cout << std::forward<T>(v);
    }
    return m;
}

extern echo_mgr mgr;
