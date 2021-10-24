#pragma once
#include <knet/kconn_factory.h>
#include "echo_mgr.h"

using namespace knet;

class echo_conn : public conn {
public:
    explicit echo_conn(conn_factory& cf)
        : conn(cf)
    {
        mgr.inst_num += 1;
    }

    ~echo_conn() override
    {
        mgr.inst_num -= 1;
    }

    bool send_data(const buffer* buf, size_t num)
    {
        if (conn::send_data(buf, num)) {
            for (size_t i = 0; i < num; ++i) {
                mgr.total_send += buf[i].get_size();
            }
            return true;
        }
        return false;
    }

protected:
    void do_on_connected() override
    {
        mgr.conn_num += 1;

        address sockAddr, peerAddr;
        if (!get_sockaddr(sockAddr) || !get_peeraddr(peerAddr)) {
            std::cerr << "get_sockaddr()/get_peeraddr() failed in on_connected\n";
            disconnect();
            return;
        }
        mgr << get_connid() << " " << sockAddr << " <----> " << peerAddr << " on_connected\n";

        if (mgr.disconnect_all) {
            disconnect();
            return;
        }

        if (mgr.random_disconnect > 0 && 0 == u32rand_between(0, mgr.random_disconnect)) {
            std::cerr << get_connid() << " random disconnect at do_on_connected\n";
            disconnect();
            return;
        }

        if (!set_sockbuf_size(mgr.sockbuf_size)) {
            std::cerr << get_connid() << " set_sockbuf_size failed!\n";
        }
    }

    void do_on_disconnect() override
    {
        mgr.conn_num -= 1;
    }

    void do_on_timer(int64_t ms, const userdata& ud) override
    {
        mgr << get_connid() << " on timer: " << ms << "\n";
    }

    size_t do_on_recv_data(char* data, size_t size) override
    {
        mgr << get_connid() << " on_recv_data, size: " << size << "\n";

        if (mgr.disconnect_all) {
            disconnect();
            return size;
        }

        if (mgr.random_disconnect > 0 && 0 == u32rand_between(0, mgr.random_disconnect)) {
            std::cerr << get_connid() << " random disconnect at do_on_recv_data\n";
            disconnect();
            return size;
        }

        return size;
    }
};

template <typename T>
struct echo_conn_factory : conn_factory, conn_factory_creator {
    echo_conn_factory() = default;
    echo_conn_factory(connid_gener gener)
        : conn_factory(gener)
    {
    }

    conn* do_create_conn() override
    {
        return new T(*this);
    }

    conn_factory* do_create_factory(connid_gener gener) override
    {
        return new echo_conn_factory(gener);
    }
};
