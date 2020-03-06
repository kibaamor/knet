#include "../include/kworker.h"
#include "../include/kpoller.h"
#include "ksocket.h"
#include "kspscqueue.h"
#include <memory>


namespace knet
{
    using workqueue_t = spsc_queue<rawsocket_t, 1024>;

    worker::worker(connection_factory* cf)
        : _cf(cf)
    {
    }

    worker::~worker()
    {
        for (auto sock : _adds)
            delete sock;
        std::vector<socket*>().swap(_adds);

        for (auto sock : _dels)
            delete sock;
        std::vector<socket*>().swap(_dels);
    }

    void worker::poll()
    {
        poller::poll();

        if (!_adds.empty())
        {
            for (auto sock : _adds)
            {
                if (!sock->attach_poller(this))
                    delete sock;
            }
            _adds.clear();
        }
        if (!_dels.empty())
        {
            for (auto sock : _dels)
                delete sock;
            _dels.clear();
        }
    }

    void worker::add_work(rawsocket_t rs)
    {
        auto sock = new socket(_cf, rs);
        _adds.push_back(sock);
    }

    void worker::on_poll(void* key, const rawpollevent_t& evt)
    {
        auto sock = static_cast<socket*>(key);
        kassert(nullptr != sock);

#ifdef KNET_USE_KQUEUE
        // only kqueue need this test, it has split read write event
        if (!sock->is_deletable())
#endif
        {
            sock->on_rawpollevent(evt);
            if (sock->is_deletable())
                _dels.push_back(sock);
        }
    }

    async_worker::async_worker(connection_factory_builder* cfb)
        : _cfb(cfb)
    {
    }

    async_worker::~async_worker()
    {
        kassert(_infos.empty());
    }

    void async_worker::add_work(rawsocket_t rs)
    {
        for (size_t i = 0, N = _infos.size(); i < N; ++i)
        {
            auto& info = _infos[_index];
            _index = (_index + 1) % N;

            auto wq = static_cast<workqueue_t*>(info.q);
            if (wq->push(rs))
                return;
        }
        ::closesocket(rs);
    }

    bool async_worker::start(size_t thread_num)
    {
        if (thread_num <= 0 || !_infos.empty())
            return false;

        _infos.resize(thread_num);
        for (size_t i = 0; i < thread_num; ++i)
        {
            auto& info = _infos[i];
            info.aw = this;
            info.q = new workqueue_t();
            info.t = new std::thread(&worker_thread, &info);
        }

        return true;
    }

    void async_worker::stop()
    {
        if (_infos.empty())
            return;

        for (auto& info : _infos)
            info.r = false;

        for (auto& info : _infos)
        {
            auto q = static_cast<workqueue_t*>(info.q);

            info.t->join();
            kassert(q->is_empty());
            delete info.t;
            delete q;
        }
        std::vector<info>().swap(_infos);
    }

    void async_worker::worker_thread(info* i)
    {
        constexpr int64_t min_interval_ms = 50;
        auto q = static_cast<workqueue_t*>(i->q);

        std::unique_ptr<connection_factory> cf(i->aw->_cfb->build_factory());
        std::unique_ptr<worker> wkr(i->aw->create_worker(cf.get()));

        rawsocket_t rs;
        while (i->r)
        {
            const auto beg_ms = now_ms();

            while (q->pop(rs))
                wkr->add_work(rs);

            wkr->poll();

            const auto end_ms = now_ms();
            const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
            sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
        }

        wkr.reset();
        cf.reset();

        while (q->pop(rs))
            ::closesocket(rs);
    }
}

