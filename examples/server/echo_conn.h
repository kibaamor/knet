#pragma once
#include <kconn_factory.h>
#include "../echo_mgr.h"

using namespace knet;

class secho_conn : public conn {
public:
    secho_conn(conn_factory& cf);

    void on_connected(socket* s) override;
    size_t on_recv_data(char* data, size_t size) override;
    void on_timer(int64_t absms, const knet::userdata& ud) override;

private:
    void set_idle_timer();
    int64_t _last_recv_ms = 0;
};

class secho_conn_factory : public conn_factory {
public:
    secho_conn_factory(connid_gener gener = connid_gener());

protected:
    conn* do_create_conn() override;
    void do_destroy_conn(conn* c) override;
};

class secho_conn_factory_builder : public conn_factory_builder {
public:
    conn_factory* build_factory(connid_gener gener) override
    {
        return new secho_conn_factory(gener);
    }
};
