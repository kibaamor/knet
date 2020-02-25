#include "../include/kworkable.h"
#include "../include/kpoller.h"
#include "ksocket.h"
#include "kspscqueue.h"


namespace knet
{
    struct work
    {
        work(connection_factory* c = nullptr, rawsocket_t s = INVALID_RAWSOCKET)
            : cf(c), rs(s)
        {
        }

        connection_factory* cf;
        rawsocket_t rs;
    };
    using workqueue_t = spsc_queue<work, 1024>;

    worker::worker() noexcept
        :_poller(this)
    {
    }

    worker::~worker()
    {
        for (auto sock : _adds)
            delete sock;
        std::vector<socket*>().swap(_adds);
    }

    void worker::update() noexcept
    {
        _poller.poll();

        for (auto sock : _adds)
        {
            if (!sock->attach_poller(_poller))
                delete sock;
        }
        _adds.clear();
    }

    void worker::add_work(connection_factory* cf, rawsocket_t rs)
    {
        auto sock = new socket(cf, rs);
        _adds.push_back(sock);
    }

    void worker::on_poll(void* key, const rawpollevent_t& evt)
    {
        auto sock = static_cast<socket*>(key);
        kassert(nullptr != sock);
        sock->on_rawpollevent(evt);
    }

    async_worker::~async_worker()
    {
        stop();
        kassert(_infos.empty());
    }

    void async_worker::add_work(connection_factory* cf, rawsocket_t rs)
    {
        for (size_t i = 0, N = _infos.size(); i < N; ++i)
        {
            auto& info = _infos[_index];
            _index = (_index + 1) % N;

            auto wq = static_cast<workqueue_t*>(info.q);
            if (wq->push(work(cf, rs)))
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
            info.q = new workqueue_t();
            info.w = new worker();
            info.t = new std::thread(&worker_thread, &info);
        }

        return true;
    }

    void async_worker::stop() noexcept
    {
        if (_infos.empty())
            return;

        for (auto& info : _infos)
            info.r = false;

        for (auto& info : _infos)
        {
            info.t->join();

            auto wq = static_cast<workqueue_t*>(info.q);
            kassert(wq->is_empty());
            delete info.t;
            delete info.w;
            delete wq;
        }
        std::vector<info>().swap(_infos);
    }

    void async_worker::worker_thread(info* i)
    {
        constexpr int64_t min_interval_ms = 50;
        auto queue = static_cast<workqueue_t*>(i->q);
        auto worker = i->w;

        work wk;
        while (i->r)
        {
            const auto beg_ms = now_ms();
            worker->update();

            while (queue->pop(wk))
                worker->add_work(wk.cf, wk.rs);

            const auto end_ms = now_ms();
            const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
            sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
        }

        while (queue->pop(wk))
            ::closesocket(wk.rs);
    }
}

