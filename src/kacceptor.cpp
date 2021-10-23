#include "../include/knet/kacceptor.h"

#ifdef _WIN32
#include "acceptor/kacceptor_win.h"
#else
#include "acceptor/kacceptor_unix.h"
#endif

namespace knet {

acceptor::acceptor(workable& wkr)
{
    _impl.reset(new impl(wkr));
}

acceptor::~acceptor() = default;

void acceptor::update()
{
    _impl->update();
}

bool acceptor::start(const address& addr)
{
    return _impl->start(addr);
}

void acceptor::stop()
{
    _impl->stop();
}

bool acceptor::get_sockaddr(address& addr) const
{
    return _impl->get_sockaddr(addr);
}

} // namespace knet
