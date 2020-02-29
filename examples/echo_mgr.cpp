#include "echo_mgr.h"
#include <knetfwd.h>
#include <string>
#include <iostream>


echo_mgr& echo_mgr::get_instance()
{
    static echo_mgr mgr;
    return mgr;
}

echo_mgr::~echo_mgr()
{
    set_disconnect_all();
    if (nullptr != _t)
    {
        _t->join();
        delete _t;
    }
}

void echo_mgr::update(int64_t delta_ms)
{
    _total_ms += delta_ms;
    if (_total_ms > 1000)
    {
        const auto total_delta_s = _total_ms / 1000;
        _total_ms %= 1000;

        const auto total_send_mb = get_total_send() / 1024 / 1024;
        zero_total_send();
        const auto total_recv_pkg_num = get_total_recv_pkg_num();
        zero_total_recv_pkg_num();

        auto send_mb = total_send_mb;
        auto recv_pkg_num = total_recv_pkg_num;

        if (total_delta_s > 1)
        {
            send_mb /= total_delta_s;
            recv_pkg_num /= total_delta_s;
        }

        if (_is_server)
        {
            std::cout << "connection: " << get_conn_num()
                << ", send: " << send_mb << " MB/S"
                << std::endl;
        }
        else
        {
            std::cout << "connection: " << get_conn_num()
                << ", send: " << send_mb << " MB/S"
                << ", recv package: " << recv_pkg_num << " Pkg/S"
                << std::endl;
        }
    }
}

int64_t echo_mgr::get_delay_ms() const
{
    if (_max_delay_ms > 1)
        return knet::u32rand_between(0, static_cast<uint32_t>(_max_delay_ms));
    return 1;
}

void echo_mgr::check_console_input()
{
    if (nullptr != _t)
        return;

    _t = new std::thread([](echo_mgr* mgr) {
        std::string s;
        while (!mgr->get_disconnect_all())
        {
            std::cin >> s;
            if (s == "exit")
            {
                mgr->set_disconnect_all();
                break;
            }
        }
    }, this);
    std::cout << R"(enter "exit" to exit program)" << std::endl;
}