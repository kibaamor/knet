#include "../include/knet/kconn_factory.h"
#include "../include/knet/kutils.h"
#include <set>
#include <map>

namespace {
using namespace knet;

static constexpr timerid_t TIMERID_BIT_COUNT = 16;
static constexpr timerid_t TIMERID_BIT_MASK = 0xffff;

struct timer_key {
    timerid_t tid;
    connid_t cid;

    timer_key(timerid_t t = 0, connid_t c = 0)
        : tid(t)
        , cid(c)
    {
    }
};

bool operator==(const timer_key& lhs, const timer_key& rhs)
{
    return lhs.tid == rhs.tid && lhs.cid == rhs.cid;
}

bool operator<(const timer_key& lhs, const timer_key& rhs)
{
    const timerid_t a = (lhs.tid >> TIMERID_BIT_COUNT);
    const timerid_t b = (rhs.tid >> TIMERID_BIT_COUNT);
    return (a < b) || (a == b && lhs.cid < rhs.cid);
}

} // namespace

namespace knet {

class conn_factory::timer {
public:
    explicit timer(conn_factory& cf)
        : _cf(cf)
    {
    }

    timerid_t add_timer(connid_t cid, int64_t ms, const userdata& ud)
    {
        auto tid = (_next_tid++) & TIMERID_BIT_MASK;
        tid |= (ms << TIMERID_BIT_COUNT);
        _timer2add.emplace(timer_key(tid, cid), ud);
        return tid;
    }

    void del_timer(connid_t cid, timerid_t tid)
    {
        _timer2del.emplace(tid, cid);
    }

    void update()
    {
        if (!_timer2del.empty()) {
            for (const auto& tk : _timer2del) {
                _timers.erase(tk);
            }
            _timer2del.clear();
        }

        if (!_timers.empty()) {
            const auto ms = now_ms();
            const timerid_t base_tid = (ms << TIMERID_BIT_COUNT) | TIMERID_BIT_MASK;

            for (auto iter = _timers.begin(); iter != _timers.end();) {
                const auto& tk = iter->first;
                if (tk.tid > base_tid) {
                    break;
                }

                _cf.on_timer(tk.cid, (tk.tid >> TIMERID_BIT_COUNT), iter->second);
                iter = _timers.erase(iter);
            }
        }

        if (!_timer2add.empty()) {
            for (const auto& pr : _timer2add) {
                _timers.emplace(pr);
            }
            _timer2add.clear();
        }
    }

private:
    conn_factory& _cf;

    timerid_t _next_tid = 0;
    std::map<timer_key, userdata> _timers;
    std::map<timer_key, userdata> _timer2add;
    std::set<timer_key> _timer2del;
};

conn_factory::conn_factory()
    : conn_factory(connid_gener())
{
}

conn_factory::conn_factory(connid_gener gener)
    : _gener(gener)
{
    _timer.reset(new timer(*this));
}

conn_factory::~conn_factory() = default;

void conn_factory::update()
{
    _timer->update();
    do_update();
}

conn* conn_factory::create_conn()
{
    auto c = do_create_conn();
    _conns.insert(std::make_pair(c->get_connid(), c));
    return c;
}

void conn_factory::destroy_conn(conn* c)
{
    _conns.erase(c->get_connid());
    do_destroy_conn(c);
}

conn* conn_factory::get_conn(connid_t cid) const
{
    auto iter = _conns.find(cid);
    return iter != _conns.end() ? iter->second : nullptr;
}

timerid_t conn_factory::add_timer(connid_t cid, int64_t ms, const userdata& ud)
{
    return _timer->add_timer(cid, ms, ud);
}

void conn_factory::del_timer(connid_t cid, timerid_t tid)
{
    _timer->del_timer(cid, tid);
}

void conn_factory::on_timer(connid_t cid, int64_t ms, const userdata& ud)
{
    auto c = get_conn(cid);
    if (c) {
        c->on_timer(ms, ud);
        do_on_timer(c, ms, ud);
    }
}

} // namespace knet
