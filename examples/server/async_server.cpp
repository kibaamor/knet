#include "echo_conn.h"
#include <iostream>
#include <kacceptor.h>
#include <kasync_worker.h>
#include <kutils.h>

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // parse command line
    const uint16_t port = uint16_t(argc > 1 ? std::atoi(argv[1]) : 8888);
    const auto max_idle_ms = argc > 2 ? std::atoi(argv[2]) : 996;
    const auto thread_num = argc > 3 ? std::atoi(argv[3]) : 8;

    // log parameter info
    std::cout << "Hi, KNet(Async Server)" << std::endl
              << "port: " << port << std::endl
              << "max_idle_ms: " << max_idle_ms << std::endl
              << "thread_num: " << thread_num << std::endl;

    // parse ip address
    address addr;
    if (!addr.pton(family_t::Ipv4, "0.0.0.0", port)) {
        std::cerr << "pton failed" << std::endl;
        return -1;
    }

    // create worker
    secho_conn_factory_builder cfb;
    async_worker wkr(cfb);
    if (!wkr.start(thread_num)) {
        std::cerr << "async_worker::start failed" << std::endl;
        return -1;
    }

    // create acceptor
    acceptor acc(wkr);
    if (!acc.start(addr)) {
        std::cerr << "acceptor::start failed" << std::endl;
        wkr.stop();
        return -1;
    }

    // check console input
    auto& mgr = echo_mgr::get_instance();
    mgr.check_console_input();
    mgr.set_max_idle_ms(max_idle_ms);
    mgr.set_enable_log(false);

    auto last_ms = now_ms();
    while (true) {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        acc.update();

        const auto conn_num = mgr.get_conn_num();
        if (mgr.get_disconnect_all()) {
            if (0 == conn_num)
                break;
        }

        mgr.update(delta_ms);

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        constexpr int64_t min_interval_ms = 50;
        sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
    }

    acc.stop();
    wkr.stop();

    return 0;
}
