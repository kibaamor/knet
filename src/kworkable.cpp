#include "../include/kworkable.h"
#include "../include/kpoller.h"
#include "ksocket.h"
#include "kspscqueue.h"


namespace knet
{
    using queue_t = spsc_queue<rawsocket_t, 1024>;

    worker::worker(connection_factory* conn_factory, socketid_gener sid_gener) noexcept
        : _conn_factory(conn_factory)
        , _sid_gener(sid_gener)
        , _poller(this)
    {
        kassert(nullptr != _conn_factory);
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

    void worker::add_work(rawsocket_t rawsocket)
    {
        auto sock = new socket(this, rawsocket);
        _adds.push_back(sock);
    }

    void worker::on_poll(void* key, const pollevent_t& pollevent)
    {
        auto sock = static_cast<socket*>(key);
        kassert(nullptr != sock);
        sock->on_pollevent(pollevent);
    }

    async_worker::async_worker(connection_factory* conn_factory) noexcept
        : _conn_factory(conn_factory)
    {
        kassert(nullptr != _conn_factory);
    }

    async_worker::~async_worker()
    {
        stop();
        kassert(_infos.empty());
    }

    void async_worker::add_work(rawsocket_t rawsocket)
    {
        for (size_t i = 0, N = _infos.size(); i < N; ++i)
        {
            auto& info = _infos[_index];
            _index = (_index + 1) % N;

            if (static_cast<queue_t*>(info.q)->push(rawsocket))
                return;
        }
        ::closesocket(rawsocket);
    }
    bool async_worker::start(size_t thread_num)
    {
        if (thread_num <= 0 || !_infos.empty())
            return false;

        _infos.resize(thread_num);
        for (size_t i = 0; i < thread_num; ++i)
        {
            auto& info = _infos[i];
            info.q = new queue_t();
            info.w = new worker(_conn_factory, socketid_gener(i, thread_num));
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
            kassert(static_cast<queue_t*>(info.q)->is_empty());
            delete info.t;
            delete info.w;
            delete static_cast<queue_t*>(info.q);
        }
        std::vector<info>().swap(_infos);
    }

    void async_worker::worker_thread(info* i)
    {
        constexpr int64_t min_interval_ms = 50;
        auto queue = static_cast<queue_t*>(i->q);
        auto worker = i->w;

        rawsocket_t rs;
        while (i->r)
        {
            const auto beg_ms = now_ms();
            worker->update();

            while (queue->pop(rs))
                worker->add_work(rs);

            const auto end_ms = now_ms();
            const auto cost_ms = end_ms > beg_ms ? end_ms - beg_ms : 0;
            sleep_ms(cost_ms < min_interval_ms ? min_interval_ms - cost_ms : 1);
        }

        while (queue->pop(rs))
            ::closesocket(rs);
    }
}

