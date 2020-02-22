#include "../include/kworkable.h"
#include <cassert>


namespace
{
    using namespace knet;
    void async_worker_thread(worker* wkr, spsc_queue<rawsocket_t, 1024>* wq, bool& running)
    {
        constexpr int64_t max_interval = 50;
        constexpr int64_t min_interval = 3;

        rawsocket_t rs;
        while (running)
        {
            const auto beg = now_ms();
            wkr->update(beg);

            while (wq->pop(rs))
                wkr->addwork(rs);

            const auto end = now_ms();
            const auto cost = end > beg ? end - beg : 0;
            sleep_ms(max_interval > cost + min_interval ? max_interval - cost : min_interval);
        }

        while (wq->pop(rs))
            closesocket(rs);
    }

    static constexpr int64_t TIMERID_BIT_COUNT = 16;
    static constexpr int64_t TIMERID_BIT_MASK = 0xffff;
}

namespace knet
{
    bool worker::timer_key_cmp::operator()(
        const timer_key& lhs, const timer_key& rhs) const noexcept
    {
        const int64_t a = (lhs.tid >> TIMERID_BIT_COUNT);
        const int64_t b = (rhs.tid >> TIMERID_BIT_COUNT);
        return (a < b) || (a == b && lhs.sid < rhs.sid);
    }

    worker::worker(socket::listener *listener) noexcept
        : worker(listener, 0, 1)
    {
    }

    worker::worker(socket::listener *listener, socketid_t start_sid, 
        socketid_t sid_inc) noexcept
        : _listener(listener), _next_sid(start_sid)
        , _sid_inc(sid_inc), _poller(this)
    {
        assert(nullptr != listener);
    }

    void worker::update(int64_t absms) noexcept
    {
        _poller.poll();

        if (!_timers.empty())
        {
            const int64_t base_tid = (absms << TIMERID_BIT_COUNT) | TIMERID_BIT_MASK;
            for (auto iter = _timers.begin(); iter != _timers.end();)
            {
                const auto& tk = iter->first;
                if (tk.tid > base_tid)
                    break;

                auto it = _socks.find(tk.sid);
                if (it != _socks.end())
                    _listener->on_timer(*it->second, iter->second, (tk.tid >> TIMERID_BIT_COUNT));
                iter = _timers.erase(iter);
            }
        }

        if (!_timer_to_del.empty())
        {
            for (const auto& tk : _timer_to_del)
                _timers.erase(tk);
            _timer_to_del.clear();
        }
        if (!_timer_to_add.empty())
        {
            for (const auto& pr : _timer_to_add)
                _timers.emplace(pr);
            _timer_to_add.clear();
        }
    }

    int64_t worker::set_timer(socket& sock, int64_t absms, const userdata& ud)
    {
        const int64_t tid = (absms << TIMERID_BIT_COUNT) | ((_next_tid++) & TIMERID_BIT_MASK);
        _timer_to_add.emplace(timer_key(tid, sock.get_socketid()), ud);
        return tid;
    }

    void worker::del_timer(socket& sock, int64_t absms)
    {
        const int64_t tid = (absms << TIMERID_BIT_COUNT);
        _timer_to_del.emplace(tid, sock.get_socketid());
    }

    void worker::on_socket_destroy(socket& sock)
    {
        _socks.erase(sock.get_socketid());
    }

    bool worker::addwork(rawsocket_t rawsocket)
    {
        auto sock = new socket(this, rawsocket, _next_sid, _listener);
        _next_sid += _sid_inc;
        if (!_poller.add(rawsocket, sock) || !sock->start())
        {
            delete sock;
            return false;
        }
        _socks.emplace(sock->get_socketid(), sock);
        return true;
    }

    void worker::on_poll(void *key, const pollevent_t &pollevent)
    {
        auto sock = static_cast<socket*>(key);
        assert(nullptr != sock);
        sock->on_pollevent(pollevent);
    }

    async_worker::async_worker(socket::listener *listener) noexcept
        : _listener(listener)
    {
        assert(nullptr != listener);
    }

    async_worker::~async_worker()
    {
        assert(_threadinfos.empty());
    }

    bool async_worker::start(size_t thread_num)
    {
        if (thread_num <= 0 || !_threadinfos.empty())
            return false;

        _running = true;
        _threadinfos.reserve(thread_num);
        for (size_t i = 0; i < thread_num; ++i)
        {
            _threadinfos.emplace_back();
            auto& ti = _threadinfos.back();
            ti.wq = new spsc_queue<rawsocket_t, 1024>();
            ti.wkr = new worker(_listener, static_cast<socketid_t>(i), static_cast<socketid_t>(thread_num));
            ti.thd = new std::thread(&async_worker_thread, ti.wkr, ti.wq, std::ref(_running));
        }

        return true;
    }

    void async_worker::stop() noexcept
    {
        if (_threadinfos.empty())
            return;

        _running = false;
        for (auto& ti : _threadinfos)
        {
            ti.thd->join();
            assert(ti.wq->is_empty());
            delete ti.thd;
            delete ti.wkr;
            delete ti.wq;
        }
        _threadinfos.clear();
    }

    bool async_worker::addwork(rawsocket_t rawsocket)
    {
        if (_threadinfos.empty())
        {
            closesocket(rawsocket);
            return false;
        }

        auto& ti = _threadinfos[_next_thread_index];
        if (!ti.wq->push(rawsocket))
        {
            closesocket(rawsocket);
            return false;
        }
        _next_thread_index = (_next_thread_index + 1) % _threadinfos.size();
        return true;
    }
}

