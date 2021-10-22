#include "secho_conn.h"
#include <iostream>
#include <knet/kacceptor.h>
#include <knet/kasync_worker.h>
#include <knet/kutils.h>

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // parse command line
    const char* port = argc > 2 ? argv[2] : "8888";
    const auto max_idle_ms = argc > 2 ? std::atoi(argv[2]) : 996;
    const auto thread_num = argc > 3 ? std::atoi(argv[3]) : 16;

    // log parameter info
    std::cout << "Hi, KNet(Async Server)" << std::endl
              << "port: " << port << std::endl
              << "max_idle_ms: " << max_idle_ms << std::endl
              << "thread_num: " << thread_num << std::endl;

    // parse ip address
    address addr;
    if (!address::resolve_one("", port, family_t::Ipv4, addr)) {
        std::cerr << "resolve address :" << port << " failed!" << std::endl;
        return -1;
    }

    // create worker
    echo_conn_factory<secho_conn> cf;
    async_worker wkr(cf);
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

    address sockAddr;
    if (!acc.get_sockaddr(sockAddr)) {
        std::cerr << "acceptor::get_sockaddr failed" << std::endl;
        acc.stop();
        wkr.stop();
        return -1;
    }
    std::cout << "listening at " << sockAddr << std::endl;

    // check console input
    mgr.is_server = true;
    mgr.max_idle_ms = max_idle_ms;
    mgr.can_log = false;
    mgr.check_console_input();

    auto last_ms = now_ms();
    while (true) {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        acc.update();

        if (mgr.disconnect_all && 0 == mgr.inst_num) {
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
