#include "kworker_impl.h"
#include "../ksocket.h"

namespace knet {
worker::impl::impl(conn_factory& cf)
    : _cf(cf)
{
    _plr.reset(new poller(*this));
}

worker::impl::~impl() = default;

void worker::impl::add_work(rawsocket_t rs)
{
    std::unique_ptr<socket> s(new socket(rs));
    if (s->init(*_plr, _cf))
        s.release();
}

void worker::impl::update()
{
    _plr->poll();
}

bool on_pollevent(void* key, void* evt)
{
    auto s = static_cast<socket*>(key);
    return s->handle_pollevent(evt);
}

} // namespace knet