#pragma once
#include <knet/kconn_factory.h>
#include "../echo_mgr.h"

using namespace knet;

class secho_conn : public conn {
public:
    explicit secho_conn(conn_factory& cf);

private:
    void do_on_connected() override;
    size_t do_on_recv_data(char* data, size_t size) override;
    void do_on_timer(int64_t ms, const knet::userdata& ud) override;

    void set_idle_timer();

private:
    int64_t _last_recv_ms = 0;
};

class secho_conn_factory : public conn_factory {
public:
    secho_conn_factory();
    explicit secho_conn_factory(connid_gener gener);

private:
    conn* do_create_conn() override;
    void do_destroy_conn(conn* c) override;
};

class secho_conn_factory_concreator : public conn_factory_concreator {
    conn_factory* do_concrete_factory(connid_gener gener) override
    {
        return new secho_conn_factory(gener);
    }
};
