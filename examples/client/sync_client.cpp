#include "socket_listener.h"
#include <iostream>


auto g_loop = true;

void check_input(bool& flag)
{
    auto thd = std::thread([](bool& b) {
        std::string s;
        while (true)
        {
            std::cin >> s;
            if (s == "exit")
            {
                b = false;
                break;
            }
        }
    }, std::ref(flag));
    thd.detach();
    std::cout << R"(enter "exit" to exit program)" << std::endl;
}

int main(int argc, char** argv)
{
    using namespace knet;

    global_init();
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const char* ip = argc > 1 ? argv[0] : "127.0.0.1";
    const in_port_t port = in_port_t(argc > 2 ? std::atoi(argv[1]) : 8888);
    const auto max_send_delay = argc > 3 ? std::atoi(argv[2]) : 0;
    const auto client_num = argc > 4 ? std::atoi(argv[4]) : 100;

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

    auto lsner = std::make_shared<client::socket_listener>(false, max_send_delay);
    auto wkr = std::make_shared<worker>(lsner.get());

    auto cnctor = std::make_shared<connector>(addr, wkr.get(), true, 1000, lsner.get());

    constexpr int64_t max_interval_ms = 50;
    auto last_ms = now_ms();
    int64_t total_delta_ms = 0;

    check_input(g_loop);
    while (true)
    {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        if (nullptr != cnctor && !cnctor->update(static_cast<size_t>(delta_ms)))
            cnctor = nullptr;

        wkr->update(beg_ms);

        const auto conn_num = lsner->get_conn_num();
        if (g_loop)
        {
            if (conn_num < client_num)
                cnctor = std::make_shared<connector>(addr, wkr.get(), true, 1000, lsner.get());
        }
        else
        {
            if (0 == conn_num)
                break;
            lsner->disconnect_all();
        }

        total_delta_ms += delta_ms;
        if (total_delta_ms > 1000)
        {
            const auto total_delta_s = total_delta_ms / 1000;
            total_delta_ms %= 1000;

            const auto total_wrote_kb = lsner->get_total_wrote() / 1024;
            lsner->clear_total_wrote();

            const auto speed = total_wrote_kb * 1.0 / total_delta_s;
            std::cout << "connection num: " << conn_num
                << ", c2s send speed: " << speed << " KB/second" << std::endl;
        }

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        sleep_ms(cost_ms < max_interval_ms ? max_interval_ms - cost_ms : 1);
    }

    return 0;
}

