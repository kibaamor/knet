#include "socket_listener.h"
#include <iostream>
#include <random>
using namespace knet;


namespace server
{
    socket_listener::socket_listener(bool enable_log)
        : _enable_log(enable_log)
    {
    }

    void socket_listener::on_conn(knet::socket& sock)
    {
        _conn_num.fetch_add(1, std::memory_order_release);

        if (need_disconnect)
        {
            sock.close();
            return;
        }

        if (_enable_log)
        {
            address sockaddr;
            if (!sock.get_sockaddr(sockaddr))
                std::cerr << "get_sockaddr failed" << std::endl;

            address peeraddr;
            if (!sock.get_peeraddr(peeraddr))
                std::cerr << "get_peeraddr failed" << std::endl;

            std::cout << "on_conn." << std::endl
                << "sockaddr:" << sockaddr << std::endl
                << "peeraddr:" << peeraddr << std::endl;
        }
    }

    void socket_listener::on_close(knet::socket& sock)
    {
        _conn_num.fetch_sub(1, std::memory_order_release);
        if (_enable_log)
            std::cout << "on_close" << std::endl;
    }

    size_t socket_listener::on_data(knet::socket& sock, char* data, size_t size)
    {
        if (need_disconnect)
        {
            sock.close();
            return 0;
        }

        socket::writebuf wbuf(data, size);
        if (!sock.write(&wbuf, 1))
        {
            std::cerr << "sock write failed!" << std::endl;
            sock.close();
            return size;
        }

        _total_wrote.fetch_add(size, std::memory_order_release);
        return size;
    }
}
