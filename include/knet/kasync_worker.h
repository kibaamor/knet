#pragma once
#include "kworker.h"
#include <vector>
#include <thread>

namespace knet {

class async_worker : public workable {
public:
    explicit async_worker(conn_factory_creator& cfc);
    ~async_worker() override;

    void add_work(rawsocket_t rs) override;

    bool start(size_t thread_num);
    void stop();

private:
    virtual worker* do_create_worker(conn_factory& cf)
    {
        return new worker(cf);
    }
    virtual bool do_start() { return true; }
    virtual void do_stop() { }

private:
    struct info {
        bool r = true;
        connid_gener gener;
        async_worker* aw = nullptr;
        void* q = nullptr;
        std::thread* t = nullptr;
    };
    static void worker_thread(info* i);

private:
    conn_factory_creator& _cfc;
    std::vector<info> _infos;
    size_t _index = 0;
};

} // namespace knet
