#include "../include/knet/kworker.h"
#include "internal/kpoller.h"
#include "internal/kinternal.h"
#include "internal/ksocket.h"

namespace knet {

class worker::impl : public poller_client {
public:
    explicit impl(conn_factory& cf)
        : _cf(cf)
    {
        _plr.reset(new poller(*this));
    }

    void add_work(rawsocket_t rs)
    {
        std::unique_ptr<socket> s(new socket(rs));
        if (s->init(*_plr, _cf))
            s.release();
    }
    void update()
    {
        _plr->poll();
        _cf.update();
    }

    bool on_pollevent(void* key, void* evt) override
    {
        auto s = static_cast<socket*>(key);
        return s->handle_pollevent(evt);
    }

private:
    conn_factory& _cf;
    std::unique_ptr<poller> _plr;
};

worker::worker(conn_factory& cf)
{
    _impl.reset(new impl(cf));
}

worker::~worker() = default;

void worker::update()
{
    _impl->update();
    do_update();
}

void worker::add_work(rawsocket_t rs)
{
    _impl->add_work(rs);
}

} // namespace knet
