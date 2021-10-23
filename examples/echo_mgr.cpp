#include "echo_mgr.h"
#include <knet/kutils.h>
#include <string>
#include <iostream>

echo_mgr mgr;

echo_mgr::~echo_mgr()
{
    disconnect_all = true;
    if (_t) {
        _t->join();
        delete _t;
    }
}

void echo_mgr::update(int64_t delta_ms)
{
    _total_ms += delta_ms;
    if (_total_ms < 1000) {
        return;
    }

    const auto total_send_mb = total_send.load() / 1024 / 1024;
    total_send = 0;
    const auto total_recv_num = total_recv.load();
    total_recv = 0;

    const auto total_delta_s = _total_ms / 1000;
    _total_ms %= 1000;

    const auto send_mb = total_send_mb / total_delta_s;
    const auto recv_num = total_recv_num / total_delta_s;

    std::cout << "instance: " << inst_num
              << ", connection: " << conn_num
              << ", send: " << send_mb << " MB/S";
    if (is_server) {
        std::cout << ", recv: " << recv_num / 1024 / 1024 << " MB/S\n";
    } else {
        std::cout << ", recv: " << recv_num << " Pkg/S\n";
    }
    std::cout.flush();
}

void echo_mgr::check_console_input()
{
    if (_t) {
        return;
    }

    _t = new std::thread([](echo_mgr* m) {
        std::string s;
        while (!m->disconnect_all) {
            std::cin >> s;
            if (s == "exit") {
                m->disconnect_all = true;
                break;
            } else if (s == "log") {
                m->can_log = !m->can_log;
            }
        }
    },
        this);
    std::cout << "enter \"exit\" to exit program" << std::endl;
}
