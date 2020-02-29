#include "echo_conn.h"
#include <kacceptor.h>
#include <kworker.h>
#include <iostream>


int main(int argc, char** argv)
{
    using namespace knet;

    // initialize knet
    global_init();

    // parse command line
    const in_port_t port = in_port_t(argc > 1 ? std::atoi(argv[1]) : 8888);
    const auto max_idle_ms = argc > 2 ? std::atoi(argv[2]) : 996;

    // log parameter info
    std::cout << "Hi, KNet(Sync Server)" << std::endl
        << "port: " << port << std::endl
        << "max_idle_ms: " << max_idle_ms << std::endl;

    // parse ip address
    address addr;
    if (!addr.pton(AF_INET, "0.0.0.0", port))
    {
        std::cerr << "pton failed" << std::endl;
        return -1;
    }

    // create worker
    auto cf = std::make_shared<secho_conn_factory>();
    auto wkr = std::make_shared<secho_worker>(cf.get());

    // create acceptor
    auto acc = std::make_shared<acceptor>(wkr.get());
    if (!acc->start(addr))
    {
        std::cerr << "acceptor::start failed" << std::endl;
        return -1;
    }

    // check console input
    auto& mgr = echo_mgr::get_instance();
    mgr.check_console_input();
    mgr.set_max_idle_ms(max_idle_ms);

    auto last_ms = now_ms();
    while (true)
    {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        acc->poll();
        wkr->poll();

        const auto conn_num = mgr.get_conn_num();
        if (mgr.get_disconnect_all())
        {
            if (0 == conn_num)
                break;
        }

        mgr.update(delta_ms);

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        constexpr int64_t min_interval_ms = 50;
        sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
    }

    acc->stop();

    return 0;
}
