#include "socket_listener.h"
#include <klistener.h>
#include <kworkable.h>
#include <iostream>


bool g_loop = true;

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

    const in_port_t port = in_port_t(argc > 2 ? std::atoi(argv[1]) : 8888);

    std::cout << "Hi, KNet(Sync Server)" << std::endl
        << "port: " << port << std::endl;

    address addr;
    if (!addr.pton(AF_INET, "0.0.0.0", port))
    {
        std::cerr << "pton failed" << std::endl;
        return -1;
    }

    auto lsner = std::make_shared<server::socket_listener>(false);
    auto wkr = std::make_shared<worker>(lsner.get());

    auto srv_listener = std::make_shared<listener>(addr, wkr.get());
    if (!srv_listener->start())
    {
        std::cerr << "srv_listener::start failed" << std::endl;
        return -1;
    }

    constexpr int64_t max_interval_ms = 50;
    auto last_ms = now_ms();
    int64_t total_delta_ms = 0;

    check_input(g_loop);
    while (true)
    {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        srv_listener->update();
        wkr->update(beg_ms);

        const auto conn_num = lsner->get_conn_num();
        if (!g_loop)
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

            const auto total_wrote_mb = lsner->get_total_wrote() / 1024 / 1024;
            lsner->clear_total_wrote();

            const auto speed = (1 == total_delta_s
                ? total_wrote_mb
                : total_wrote_mb * 1.0 / total_delta_s);
            std::cout << "connection num: " << conn_num
                << ", s2c send speed: " << speed << " MB/Second" << std::endl;
        }

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        sleep_ms(cost_ms < max_interval_ms ? max_interval_ms - cost_ms : 1);
    }

    srv_listener->stop();

    return 0;
}
