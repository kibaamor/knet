#pragma once
#include <atomic>
#include <knet/kutils.h>

#pragma pack(push)
#pragma pack(1)
struct msg_hdr {
    uint32_t size; // data size (without header)
};
#pragma pack(pop)

struct echo_mgr final {
    bool is_server = true;
    uint32_t max_idle_ms = 0;
    uint32_t random_disconnect = 10000;
    uint32_t sockbuf_size = knet::SOCKET_RWBUF_SIZE / 4; // for test purpose

    std::atomic_bool can_log = { false };
    std::atomic_bool disconnect_all = { false };

    std::atomic_llong inst_num = { 0 };
    std::atomic_llong conn_num = { 0 };
    std::atomic_llong total_send = { 0 };
    std::atomic_llong total_recv = { 0 };

    ~echo_mgr();
    void update(int64_t delta_ms);
    void check_console_input();

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
