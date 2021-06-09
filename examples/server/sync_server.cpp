#include "echo_conn.h"
#include <iostream>
#include <knet/kacceptor.h>
#include <knet/kworker.h>
#include <knet/kutils.h>

int main(int argc, char** argv)
{
    using namespace knet;

    // initialize knet
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // parse command line
    const char* port = argc > 2 ? argv[2] : "8888";
    const auto max_idle_ms = argc > 2 ? std::atoi(argv[2]) : 996;

    // log parameter info
    std::cout << "Hi, KNet(Sync Server)" << std::endl
              << "port: " << port << std::endl
              << "max_idle_ms: " << max_idle_ms << std::endl;

    // parse ip address
    const auto ip = "";
    const auto fa = family_t::Ipv4;
    address addr;
    if (!address::resolve_one(ip, port, fa, addr)) {
        std::cerr << "resolve address " << ip << ":" << port << " failed!" << std::endl;
        return -1;
    }

    // create worker
    secho_conn_factory cf;
    worker wkr(cf);

    // create acceptor
    acceptor acc(wkr);
    if (!acc.start(addr)) {
        std::cerr << "acceptor::start failed" << std::endl;
        return -1;
    }

    address sockAddr;
    if (!acc.get_sockaddr(sockAddr)) {
        std::cerr << "acceptor::get_sockaddr failed" << std::endl;
        acc.stop();
        return -1;
    }
    std::cout << "listening at " << sockAddr << std::endl;

    // check console input
    auto& mgr = echo_mgr::get_instance();
    mgr.set_is_server(true);
    mgr.check_console_input();
    mgr.set_max_idle_ms(max_idle_ms);
    mgr.set_enable_log(true);

    auto last_ms = now_ms();
    while (true) {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        acc.update();
        wkr.update();

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

    return 0;
}
