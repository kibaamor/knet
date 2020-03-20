#pragma once
#include <kconnector.h>
#include <ktconnection.h>
#include <atomic>
#include "../echo_mgr.h"

class cecho_conn : public knet::tconnection {
public:
    cecho_conn(knet::connid_t id, knet::tconnection_factory* cf);

    void on_connected() override;
    size_t on_recv_data(char* data, size_t size) override;
    void on_timer(int64_t absms, const knet::userdata& ud) override;

protected:
    virtual void on_attach_socket(knet::rawsocket_t rs) override;

private:
    void generate_packages();
    bool send_package();
    int32_t check_package(char* data, size_t size);

private:
    char _buf[256 * 1024] = {};
    uint32_t _used_buf_size = 0;
    uint32_t _send_buf_size = 0;

    uint32_t _next_send_pkg_id = 0;
    uint32_t _next_recv_pkg_id = 0;
};

class cecho_conn_factory_builder;
class cecho_conn_factory : public knet::tconnection_factory {
public:
    explicit cecho_conn_factory(cecho_conn_factory_builder* cfb = nullptr);

protected:
    knet::tconnection* create_connection_impl() override;
    void destroy_connection_impl(knet::tconnection* tconn) override;

    knet::connid_t get_next_connid();

private:
    cecho_conn_factory_builder* _cfb = nullptr;
    knet::connid_t _next_cid = 0;
};

class cecho_conn_factory_builder : public knet::conn_factory_builder {
public:
    knet::conn_factory* build_factory() override
    {
        return new cecho_conn_factory(this);
    }

    knet::connid_t get_next_connid() { return _next_cid.fetch_add(1); }

private:
    std::atomic<knet::connid_t> _next_cid = { 0 };
};

class cecho_worker : public knet::worker {
public:
    explicit cecho_worker(cecho_conn_factory* cf)
        : worker(cf)
    {
    }

    void update() override
    {
        worker::update();
        get_cf<cecho_conn_factory>()->update();
    }
};

class cecho_async_worker : public knet::async_worker {
public:
    explicit cecho_async_worker(cecho_conn_factory_builder* cfb)
        : async_worker(cfb)
    {
    }

protected:
    knet::worker* create_worker(knet::conn_factory* cf) override
    {
        return new cecho_worker(static_cast<cecho_conn_factory*>(cf));
    }
};

class cecho_connector : public knet::connector {
public:
    cecho_connector(const knet::address& addr, knet::workable* wkr,
        bool reconn = true, size_t interval_ms = 1000);

    void on_reconnect() override;
    void on_reconnect_failed() override;
};
