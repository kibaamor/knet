#include "cecho_conn.h"
#include <knet/kconnector.h>
#include <knet/kasync_worker.h>
#include <knet/kutils.h>

int main(int argc, char** argv)
{
    // parse command line
    const char* ip = argc > 1 ? argv[1] : "localhost";
    const char* port = argc > 2 ? argv[2] : "8888";
    const auto client_num = argc > 3 ? std::atoi(argv[3]) : 1000;
    const auto timeout_ms = argc > 4 ? std::atoi(argv[4]) : -1;
    const auto thread_num = argc > 5 ? std::atoi(argv[5]) : std::thread::hardware_concurrency();

    // log parameter info
    std::cout << "Hi, KNet(Async Client)" << std::endl
              << "ip:" << ip << std::endl
              << "port: " << port << std::endl
              << "client_num: " << client_num << std::endl
              << "timeout_ms: " << timeout_ms << std::endl
              << "thread_num: " << thread_num << std::endl;

    // parse ip address
    address addr;
    if (!address::resolve_one(ip, port, family_t::Ipv4, addr)) {
        std::cerr << "resolve address " << ip << ":" << port << " failed!" << std::endl;
        return -1;
    }

    // create worker
    echo_conn_factory<cecho_conn> cf;
    async_worker wkr(cf);
    if (!wkr.start(thread_num)) {
        std::cerr << "async_echo_conn_mgr::start failed" << std::endl;
        return -1;
    }

    // create connector
    connector cnctor(wkr);

    // check console input
    mgr.is_server = false;
    mgr.can_log = false;
    mgr.check_console_input();

    auto last_ms = now_ms();
    while (true) {
        const auto beg_ms = now_ms();
        const auto delta_ms = (beg_ms > last_ms ? beg_ms - last_ms : 0);
        last_ms = beg_ms;

        const auto inst_num = mgr.inst_num.load();
        if (mgr.disconnect_all) {
            if (0 == inst_num) {
                break;
            }
        } else if (inst_num < client_num) {
            if (!cnctor.connect(addr, timeout_ms)) {
                std::cerr << "connect failed! address: " << addr << std::endl;
            }
        }

        mgr.update(delta_ms);

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        constexpr int64_t min_interval_ms = 50;
        sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
    }

    wkr.stop();

    return 0;
}
