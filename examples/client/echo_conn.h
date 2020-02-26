#pragma once
#include <kconnector.h>
#include <ktconnection.h>
#include "../echo_mgr.h"


class cecho_conn : public knet::tconnection
{
public:
    cecho_conn(knet::connid_t id, knet::tconnection_factory* cf);

    void on_connected() override;
    size_t on_recv_data(char* data, size_t size) override;
    void on_timer(int64_t absms, const knet::userdata& ud) override;

private:
    void send_package();
    int32_t check_package(char* data, size_t size);

protected:
    virtual void on_attach_socket(knet::rawsocket_t rs) override;
};

class cecho_conn_factory : public knet::tconnection_factory
{
protected:
    knet::tconnection* create_connection_impl() override;
    void destroy_connection_impl(knet::tconnection* tconn) override;
};

class cecho_conn_factory_builder : public knet::connection_factory_builder
{
public:
    knet::connection_factory* build_factory() override
    {
        return new cecho_conn_factory();
    }
};

class cecho_connector : public knet::connector
{
public:
    cecho_connector(const knet::address& addr, knet::workable* wkr, 
        bool reconn = true, size_t interval_ms = 1000);

    void on_reconnect() override;
    void on_reconnect_failed() override;
};
