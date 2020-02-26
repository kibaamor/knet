#pragma once
#include <ktconnection.h>
#include <kaddress.h>
#include "../echo_mgr.h"


class secho_conn : public knet::tconnection
{
public:
    secho_conn(knet::connid_t id, knet::tconnection_factory* cf);

    void on_connected() override;
    size_t on_recv_data(char* data, size_t size) override;
    void on_timer(int64_t absms, const knet::userdata& ud) override;

protected:
    virtual void on_attach_socket(knet::rawsocket_t rs) override;
};

class secho_conn_factory : public knet::tconnection_factory
{
protected:
    knet::tconnection* create_connection_impl() override;
    void destroy_connection_impl(knet::tconnection* tconn) override;
};

class secho_conn_factory_builder : public knet::connection_factory_builder
{
public:
    knet::connection_factory* build_factory() override
    {
        return new secho_conn_factory();
    }
};
