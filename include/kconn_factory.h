#pragma once
#include "kconn.h"
#include <unordered_map>

namespace knet {

class connid_gener final {
public:
    connid_gener()
        : connid_gener(0, 1)
    {
    }

    connid_gener(connid_t init, connid_t step)
        : _cur(init)
        , _step(step)
    {
    }

    connid_t gen()
    {
        return _cur += _step;
    }

private:
    connid_t _cur;
    connid_t _step;
};

class conn_factory {
public:
    conn_factory();
    explicit conn_factory(connid_gener gener);
    virtual ~conn_factory();

    void update();

    conn* create_conn();
    void destroy_conn(conn* c);

    conn* get_conn(connid_t cid) const;
    timerid_t add_timer(connid_t cid, int64_t absms, const userdata& ud);
    void del_timer(connid_t cid, timerid_t tid);

    connid_t get_next_connid() { return _gener.gen(); }

private:
    virtual conn* do_create_conn() = 0;
    virtual void do_destroy_conn(conn* c) = 0;
    virtual void do_update() { }

private:
    void on_timer(connid_t cid, int64_t absms, const userdata& ud);

private:
    connid_gener _gener;
    std::unordered_map<connid_t, conn*> _conns;

    class timer;
    std::unique_ptr<timer> _timer;
};

class conn_factory_builder {
public:
    virtual ~conn_factory_builder() = default;

    conn_factory* build_factory(connid_gener gener);

private:
    virtual conn_factory* do_build_factory(connid_gener gener) = 0;
};

} // namespace knet
