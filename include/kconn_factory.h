#pragma once
#include "kconn.h"
#include <unordered_map>

namespace knet {

class connid_gener {
public:
    connid_gener(connid_t init = 0, connid_t step = 1)
        : _cur(init)
        , _step(step)
    {
    }

    connid_t gen()
    {
        return _cur += _step;
    }

private:
    connid_t _cur = 0;
    connid_t _step = 1;
};

class conn_factory {
public:
    explicit conn_factory(connid_gener gener = connid_gener());
    virtual ~conn_factory();

    virtual void update();

    conn* create_conn();
    void destroy_conn(conn* c);

    conn* get_conn(connid_t cid) const;
    timerid_t add_timer(connid_t cid, int64_t absms, const userdata& ud);
    void del_timer(connid_t cid, timerid_t tid);

    connid_t get_next_connid() { return _gener.gen(); }

protected:
    virtual conn* do_create_conn() = 0;
    virtual void do_destroy_conn(conn* c) { delete c; }

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

    virtual conn_factory* build_factory(connid_gener gener) = 0;
};

} // namespace knet
