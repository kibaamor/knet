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
    const char* ip = argc > 1 ? argv[1] : "127.0.0.1";
    const in_port_t port = in_port_t(argc > 2 ? std::atoi(argv[2]) : 8888);
    const auto client_num = argc > 3 ? std::atoi(argv[3]) : 1;
    const auto max_delay_ms = argc > 4 ? std::atoi(argv[4]) : 1000;

    // log parameter info
    std::cout << "Hi, KNet(Sync Client)" << std::endl
        << "ip:" << ip << std::endl
        << "port: " << port << std::endl
        << "client_num: " << client_num << std::endl
        << "max_delay_ms: " << max_delay_ms << std::endl;

    // parse ip address
    address addr;
    if (!addr.pton(AF_INET, ip, port))
    {
        std::cerr << "pton failed" << std::endl;
        return -1;
    }

    // create worker
    auto cf = std::make_shared<cecho_conn_factory>();
    auto wkr = std::make_shared<cecho_worker>(cf.get());

    // create connector
    auto connector_builder = [&addr, &wkr]() {
        return std::make_shared<cecho_connector>(addr, wkr.get(), wkr.get());
    };
    auto cnctor = connector_builder();

    // check console input
    auto& mgr = echo_mgr::get_instance();
    mgr.check_console_input();
    mgr.set_max_delay_ms(max_delay_ms);

    auto last_ms = now_ms();
    while (true)
    {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        if (nullptr != cnctor && !cnctor->update(static_cast<size_t>(delta_ms)))
            cnctor = nullptr;

        wkr->poll();

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

    return 0;
}

