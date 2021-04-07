#include "../include/knet/kasync_worker.h"
#include "../include/knet/kutils.h"
#include "internal/kspscqueue.h"
#include "internal/kinternal.h"

namespace knet {

using workqueue_t = spsc_queue<rawsocket_t, 1024>;

async_worker::async_worker(conn_factory_concreator& cfc)
    : _cfc(cfc)
{
}

async_worker::~async_worker()
{
    kassert(_infos.empty());
}

void async_worker::add_work(rawsocket_t rs)
{
    for (size_t i = 0, N = _infos.size(); i < N; ++i) {
        auto& inf = _infos[_index];
        _index = (_index + 1) % N;

        auto wq = static_cast<workqueue_t*>(inf.q);
        if (wq->push(rs))
            return;
    }
    close_rawsocket(rs);
}

bool async_worker::start(size_t thread_num)
{
    if (0 == thread_num || !_infos.empty())
        return false;

    _infos.resize(thread_num);
    for (size_t i = 0; i < thread_num; ++i) {
        auto& inf = _infos[i];
        inf.gener = connid_gener(static_cast<connid_t>(i), static_cast<connid_t>(thread_num));
        inf.aw = this;
        inf.q = new workqueue_t();
        inf.t = new std::thread(&worker_thread, &inf);
    }

    if (!do_start()) {
        stop();
        return false;
    }

    return true;
}

void async_worker::stop()
{
    if (_infos.empty())
        return;

    do_stop();

    for (auto& inf : _infos)
        inf.r = false;

    for (auto& inf : _infos) {
        auto q = static_cast<workqueue_t*>(inf.q);

        inf.t->join();
        kassert(q->is_empty());
        delete inf.t;
        delete q;
    }
    _infos.clear();
}

void async_worker::worker_thread(info* i)
{
    constexpr int64_t min_interval_ms = 50;
    auto q = static_cast<workqueue_t*>(i->q);

    std::unique_ptr<conn_factory> cf(i->aw->_cfc.concrete_factory(i->gener));
    std::unique_ptr<worker> wkr(i->aw->do_create_worker(*cf));

    rawsocket_t rs;
    while (i->r) {
        const auto beg_ms = now_ms();

        while (q->pop(rs))
            wkr->add_work(rs);

        wkr->update();

        const auto end_ms = now_ms();
        const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
        sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
    }

    wkr.reset();
    cf.reset();

    while (q->pop(rs))
        close_rawsocket(rs);
}

} // namespace knet