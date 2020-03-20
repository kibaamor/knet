#include "../include/ktconn.h"
#include "../include/kutils.h"
#include <set>
#include <map>

namespace {
static constexpr int64_t TIMERID_BIT_COUNT = 16;
static constexpr int64_t TIMERID_BIT_MASK = 0xffff;

struct timer_key {
    int64_t tid;
    knet::connid_t cid;

    timer_key(int64_t t = 0, knet::connid_t c = 0)
        : tid(t)
        , cid(c)
    {
    }
};

bool operator<(const timer_key& lhs, const timer_key& rhs)
{
    const int64_t a = (lhs.tid >> TIMERID_BIT_COUNT);
    const int64_t b = (rhs.tid >> TIMERID_BIT_COUNT);
    return (a < b) || (a == b && lhs.cid < rhs.cid);
}
} // namespace

namespace knet {
class _tconn_timer {
public:
    explicit _tconn_timer(tconnection_factory* cf)
        : _cf(cf)
    {
    }

    timerid_t add_timer(connid_t cid, int64_t absms, const userdata& ud)
    {
        int64_t tid = (_next_tid++) & TIMERID_BIT_MASK;
        tid |= (absms << TIMERID_BIT_COUNT);
        _timer2add.emplace(timer_key(tid, cid), ud);
        return tid;
    }

    void del_timer(connid_t cid, int64_t absms)
    {
        const int64_t tid = (absms << TIMERID_BIT_COUNT);
        _timer2del.emplace(tid, cid);
    }

    void update()
    {
        if (!_timers.empty()) {
            const auto absms = now_ms();
            const int64_t base_tid = (absms << TIMERID_BIT_COUNT) | TIMERID_BIT_MASK;
            for (auto iter = _timers.begin(); iter != _timers.end();) {
                const auto& tk = iter->first;
                if (tk.tid > base_tid)
                    break;

                _cf->on_timer(tk.cid, (tk.tid >> TIMERID_BIT_COUNT), iter->second);
                iter = _timers.erase(iter);
            }
        }

        if (!_timer2del.empty()) {
            for (const auto& tk : _timer2del)
                _timers.erase(tk);
            _timer2del.clear();
        }

        if (!_timer2add.empty()) {
            for (const auto& pr : _timer2add)
                _timers.emplace(pr);
            _timer2add.clear();
        }
    }

private:
    tconnection_factory* _cf;

    int64_t _next_tid = 0;
    std::map<timer_key, userdata> _timers;
    std::map<timer_key, userdata> _timer2add;
    std::set<timer_key> _timer2del;
};

timerid_t tconnection::add_timer(int64_t absms, const userdata& ud)
{
    if (is_disconnecting())
        return INVALID_TIMERID;
    return _cf->add_timer(_id, absms, ud);
}

void tconnection::del_timer(int64_t absms)
{
    _cf->del_timer(_id, absms);
}

tconnection_factory::tconnection_factory()
{
    _timer = new _tconn_timer(this);
}

tconnection_factory::~tconnection_factory()
{
    delete _timer;
}

conn* tconnection_factory::create_conn()
{
    auto tconn = create_connection_impl();
    _tconns.insert(std::make_pair(tconn->get_connid(), tconn));
    return tconn;
}

void tconnection_factory::destroy_conn(conn* conn)
{
    auto tconn = static_cast<tconnection*>(conn);
    _tconns.erase(tconn->get_connid());
    destroy_connection_impl(tconn);
}

void tconnection_factory::update()
{
    _timer->update();
}

tconnection* tconnection_factory::get_conn(connid_t id) const
{
    auto iter = _tconns.find(id);
    return iter != _tconns.end() ? iter->second : nullptr;
}

timerid_t tconnection_factory::add_timer(connid_t cid, int64_t absms, const userdata& ud)
{
    return _timer->add_timer(cid, absms, ud);
}

void tconnection_factory::del_timer(connid_t cid, int64_t absms)
{
    _timer->del_timer(cid, absms);
}

void tconnection_factory::on_timer(connid_t cid, int64_t absms, const userdata& ud)
{
    auto conn = get_conn(cid);
    if (nullptr != conn)
        conn->on_timer(absms, ud);
}
} // namespace knet
