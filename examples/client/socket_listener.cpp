#include "socket_listener.h"
#include <iostream>
#include <random>
using namespace knet;


namespace client
{
    socket_listener::socket_listener(bool enable_log, uint32_t max_send_delay)
        : _enable_log(enable_log), _max_send_delay(max_send_delay)
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

        if (0 == _max_send_delay)
            send_package(sock);
        else
            sock.set_rel_timer(u32rand() % _max_send_delay, 0);
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

        auto len = check_package(data, size);
        if (len < 0)
        {
            std::cerr << "invalid package length:" << len << std::endl;
            sock.close();
            return 0;
        }
        
        if (len == 0)
            return 0;

        _total_wrote.fetch_add(size, std::memory_order_release);

        if (0 == _max_send_delay)
            send_package(sock);
        else
            sock.set_rel_timer(u32rand() % _max_send_delay, 0);

        return len;
    }

    void socket_listener::on_timer(knet::socket& sock, const knet::userdata& ud, int64_t absms)
    {
        send_package(sock);
    }

    void socket_listener::on_reconn(const address& addr)
    {
        if (_enable_log)
            std::cout << "on_reconn to " << addr << std::endl;
    }

    void socket_listener::on_reconn_failed(const address& addr)
    {
        if (_enable_log)
            std::cout << "on_reconn_failed to " << addr << std::endl;
    }

    int32_t socket_listener::check_package(char* data, size_t size)
    {
        if (size < sizeof(int32_t) * 2)
            return 0;

        const auto len = *(int32_t*)data;
        if (static_cast<int32_t>(size) < len)
            return 0;

        const auto flag = *(int32_t*)(data + len - sizeof(int32_t));
        if (len != flag)
            return -1;

        return len;
    }

    void socket_listener::send_package(knet::socket& sock)
    {
        static thread_local char buf[SOCKET_RWBUF_SIZE] = {};

        const auto len = u32rand_between(8, sizeof(buf));
        auto p = (char*)buf;
        *(int32_t*)p = len;
        *(int32_t*)(p + len - sizeof(int32_t)) = len;

        socket::writebuf wbuf(buf, len);
        if (!sock.write(&wbuf, 1))
        {
            std::cerr << "sock write failed! " << __FUNCTION__ << std::endl;
            sock.close();
        }
    }
}