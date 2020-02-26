#include "echo_mgr.h"
#include <string>
#include <iostream>


echo_mgr& echo_mgr::get_intance()
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

        const auto speed = (1 == total_delta_s
            ? total_send_mb
            : total_send_mb * 1.0 / total_delta_s);
        std::cout << "connection num: " << get_conn_num()
            << ", send speed: " << speed << " MB/S" << std::endl;
    }
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