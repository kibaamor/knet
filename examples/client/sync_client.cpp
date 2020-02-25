#include "echo_conn.h"
#include <iostream>


int main(int argc, char** argv)
{
    using namespace knet;

    global_init();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const char* ip = argc > 1 ? argv[0] : "127.0.0.1";
    const in_port_t port = in_port_t(argc > 2 ? std::atoi(argv[1]) : 8888);
    const auto max_send_delay = argc > 3 ? std::atoi(argv[2]) : 0;
    const auto client_num = argc > 4 ? std::atoi(argv[4]) : 1000;

    std::cout << "Hi, KNet(Sync Client)" << std::endl
        << "ip:" << ip << std::endl
        << "port: " << port << std::endl
        << "max_send_delay(ms): " << max_send_delay << std::endl
        << "client_num: " << client_num << std::endl;

    address addr;
    if (!addr.pton(AF_INET, ip, port))
    {
        std::cerr << "pton failed" << std::endl;
        return -1;
    }

    auto conn_mgr = std::make_shared <echo_conn_mgr>();
    auto wkr = std::make_shared<worker>(conn_mgr.get());

    auto create_connector = [&addr, &wkr, &conn_mgr]() {
        return std::make_shared<connector>(addr, wkr.get(), true, 1000, conn_mgr.get());
    };

    auto cnctor = create_connector();

    constexpr int64_t min_interval_ms = 50;
    auto last_ms = now_ms();
    int64_t total_delta_ms = 0;

    check_input(conn_mgr.get());
    while (true)
    {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        if (nullptr != cnctor && !cnctor->update(static_cast<size_t>(delta_ms)))
            cnctor = nullptr;

        wkr->update();

        const auto conn_num = conn_mgr->get_conn_num();
        const auto loop = !conn_mgr->get_disconnect_all();
        if (loop)
        {
            if (conn_num < client_num)
                cnctor = create_connector();
        }
        else
        {
            if (0 == conn_num)
                break;
        }

        total_delta_ms += delta_ms;
        if (total_delta_ms > 1000)
        {
            const auto total_delta_s = total_delta_ms / 1000;
            total_delta_ms %= 1000;

            const auto total_send_mb = conn_mgr->get_total_send() / 1024 / 1024;
            conn_mgr->clear_total_send();

            const auto speed = (1 == total_delta_s
                ? total_send_mb
                : total_send_mb * 1.0 / total_delta_s);
            std::cout << "connection num: " << conn_num
                << ", c2s send speed: " << speed << " MB/Second" << std::endl;
        }

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
    }

    return 0;
}

