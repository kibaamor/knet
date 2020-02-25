#include "echo_conn.h"
#include <iostream>
#include <thread>


void echo_conn::on_connected()
{
    //std::cout << "on_connected: " << _sockaddr << " <===> " << _peeraddr << std::endl;
    if (_mgr->get_disconnect_all())
        disconnect();
}

size_t echo_conn::on_recv_data(char* data, size_t size)
{
    if (_mgr->get_disconnect_all())
    {
        disconnect();
        return 0;
    }

    knet::buffer buf(data, size);
    if (!send_data(&buf, 1))
    {
        std::cerr << "send_data failed!" << std::endl;
        disconnect();
        return 0;
    }

    _mgr->add_send(size);

    return size;
}

void echo_conn::on_disconnect()
{
    //std::cout << "on_disconnect: " << _sockaddr << " <=/=> " << _peeraddr << std::endl;
}

void echo_conn::on_attach_socket(knet::rawsocket_t rs)
{
    knet::set_rawsocket_sndrcvbufsize(rs, 256 * 1204);
    {
        auto& sa = _sockaddr.get_sockaddr();
        socklen_t len = sizeof(sa);
        getsockname(rs, reinterpret_cast<sockaddr*>(&sa), &len);
    }
    {
        auto& sa = _peeraddr.get_sockaddr();
        socklen_t len = sizeof(sa);
        getpeername(rs, reinterpret_cast<sockaddr*>(&sa), &len);
    }
}

knet::connection* echo_conn_mgr::create_connection()
{
    _conn_num.fetch_add(1, std::memory_order_release);
    auto conn = new echo_conn();
    conn->set_conn_mgr(this);
    return conn;
}

void echo_conn_mgr::destroy_connection(knet::connection* conn)
{
    delete conn;
    _conn_num.fetch_sub(1, std::memory_order_release);
}

void check_input(echo_conn_mgr* mgr)
{
    auto thd = std::thread([](echo_conn_mgr* mgr) {
        std::string s;
        while (true)
        {
            std::cin >> s;
            if (s == "exit")
            {
                mgr->set_disconnect_all();
                break;
            }
        }
    }, mgr);
    thd.detach();
    std::cout << R"(enter "exit" to exit program)" << std::endl;
}
