#pragma once
#include <ktconnection.h>
#include <kaddress.h>
#include "../echo_mgr.h"

class secho_conn : public knet::tconnection {
public:
    secho_conn(knet::connid_t id, knet::tconnection_factory* cf);

    void on_connected() override;
    size_t on_recv_data(char* data, size_t size) override;
    void on_timer(int64_t absms, const knet::userdata& ud) override;

protected:
    virtual void on_attach_socket(knet::rawsocket_t rs) override;

private:
    void set_idle_timer();
    int64_t _last_recv_ms = 0;
};

class secho_conn_factory_builder;
class secho_conn_factory : public knet::tconnection_factory {
public:
    explicit secho_conn_factory(secho_conn_factory_builder* cfb = nullptr);

protected:
    knet::tconnection* create_connection_impl() override;
    void destroy_connection_impl(knet::tconnection* tconn) override;

    knet::connid_t get_next_connid();

private:
    secho_conn_factory_builder* _cfb = nullptr;
    knet::connid_t _next_cid = 0;
};

class secho_conn_factory_builder : public knet::conn_factory_builder {
public:
    knet::conn_factory* build_factory() override
    {
        return new secho_conn_factory(this);
    }

    knet::connid_t get_next_connid() { return _next_cid.fetch_add(1); }

private:
    std::atomic<knet::connid_t> _next_cid = { 0 };
};

class secho_worker : public knet::worker {
public:
    explicit secho_worker(secho_conn_factory* cf)
        : worker(cf)
    {
    }

    void update() override
    {
        worker::update();
        get_cf<secho_conn_factory>()->update();
    }
};

class secho_async_worker : public knet::async_worker {
public:
    explicit secho_async_worker(secho_conn_factory_builder* cfb)
        : async_worker(cfb)
    {
    }

protected:
    knet::worker* create_worker(knet::conn_factory* cf) override
    {
        return new secho_worker(static_cast<secho_conn_factory*>(cf));
    }
};
