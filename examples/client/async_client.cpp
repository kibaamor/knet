#include "echo_conn.h"
#include <kconnector.h>
#include <kworker.h>
#include <iostream>


int main(int argc, char** argv)
{
    using namespace knet;

    // initialize knet
    global_init();

    // parse command line
    const char* ip = argc > 1 ? argv[0] : "127.0.0.1";
    const in_port_t port = in_port_t(argc > 2 ? std::atoi(argv[1]) : 8888);
    const auto client_num = argc > 3 ? std::atoi(argv[3]) : 1000;
    const auto thread_num = argc > 4 ? std::atoi(argv[4]) : 8;

    // log parameter info
    std::cout << "Hi, KNet(Async Client)" << std::endl
        << "ip:" << ip << std::endl
        << "port: " << port << std::endl
        << "client_num: " << client_num << std::endl
        << "thread_num: " << thread_num << std::endl;

    // parse ip address
    address addr;
    if (!addr.pton(AF_INET, ip, port))
    {
        std::cerr << "pton failed" << std::endl;
        return -1;
    }

    // create worker
    auto cfb = std::make_shared<cecho_conn_factory_builder>();
    auto wkr = std::make_shared<async_worker>(cfb.get());
    if (!wkr->start(thread_num))
    {
        std::cerr << "async_echo_conn_mgr::start failed" << std::endl;
        return -1;
    }

    // create connector
    auto connector_builder = [&addr, &wkr]() {
        return std::make_shared<cecho_connector>(addr, wkr.get(), wkr.get());
    };
    auto cnctor = connector_builder();

    // check console input
    auto& mgr = echo_mgr::get_intance();
    mgr.check_console_input();

    auto last_ms = now_ms();
    while (true)
    {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        if (nullptr != cnctor && !cnctor->update(static_cast<size_t>(delta_ms)))
            cnctor = nullptr;

        const auto conn_num = mgr.get_conn_num();
        if (mgr.get_disconnect_all())
        {
            if (0 == conn_num)
                break;
        }
        else if (nullptr == cnctor && conn_num < client_num)
        {
            cnctor = connector_builder();
        }

        mgr.update(delta_ms);

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        constexpr int64_t min_interval_ms = 50;
        sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
    }

    wkr->stop();

    return 0;
}

