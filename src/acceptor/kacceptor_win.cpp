#include "kacceptor_win.h"
#include "../internal/ksocket_utils.h"
#include <array>

namespace {

using namespace knet;

LPFN_ACCEPTEX get_accept_ex(rawsocket_t rs)
{
    static thread_local LPFN_ACCEPTEX _accept_ex = nullptr;
    if (!_accept_ex) {
        GUID guid = WSAID_ACCEPTEX;
        DWORD dw = 0;
        WSAIoctl(rs, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &guid, sizeof(guid), &_accept_ex, sizeof(_accept_ex),
            &dw, nullptr, nullptr);
    }
    return _accept_ex;
}

struct accept_io {
    WSAOVERLAPPED ol = {};
    char buf[(sizeof(sockaddr_storage) + 16) * 2] = {};
    rawsocket_t rs = INVALID_RAWSOCKET;
    accept_io* next = nullptr;

    void clear()
    {
        close_rawsocket(rs);
        next = nullptr;
    }

    bool post(rawsocket_t srv_rs, int family)
    {
        rs = create_rawsocket(family);
        if (INVALID_RAWSOCKET == rs) {
            return false;
        }

        memset(&ol, 0, sizeof(ol));

        const auto accept_ex = get_accept_ex(rs);
        if (!accept_ex) {
            return false;
        }

        DWORD dw = 0;
        constexpr auto size = sizeof(SOCKADDR_STORAGE) + 16;
        if (!accept_ex(srv_rs, rs, buf, 0, size, size, &dw, &ol)
            && ERROR_IO_PENDING != WSAGetLastError()) {
            close_rawsocket(rs);
            return false;
        }

        return true;
    }
};

} // namespace

namespace knet {

class acceptor::impl::pending_ios {
public:
    pending_ios()
    {
        for (size_t i = 0, N = _ios.size(); i < N; ++i) {
            if (N != i + 1) {
                _ios[i].next = &_ios[i + 1];
            }
        }
        _free_ios = &_ios[0];
    }

    ~pending_ios()
    {
        for (auto& io : _ios) {
            io.clear();
        }
        _free_ios = nullptr;
    }

    void recycle(accept_io* io)
    {
        io->next = _free_ios;
        _free_ios = io;
    }

    void post_all(rawsocket_t srv_rs, int family)
    {
        while (_free_ios && _free_ios->post(srv_rs, family)) {
            _free_ios = _free_ios->next;
        }
    }

private:
    std::array<accept_io, IOCP_PENDING_ACCEPT_NUM> _ios;
    accept_io* _free_ios = nullptr;
};

acceptor::impl::impl(workable& wkr)
    : _wkr(wkr)
{
}

acceptor::impl::~impl()
{
    kassert(INVALID_RAWSOCKET == _rs);
}

void acceptor::impl::update()
{
    _plr->poll();
    _ios->post_all(_rs, _family);
}

bool acceptor::impl::start(const address& addr)
{
    if (INVALID_RAWSOCKET != _rs) {
        return false;
    }

    _family = addr.get_rawfamily();
    _rs = create_rawsocket(_family);
    if (INVALID_RAWSOCKET == _rs) {
        return false;
    }

    if (!set_rawsocket_reuseaddr(_rs)) {
        kdebug("set_rawsocket_reuseaddr() failed!");
        close_rawsocket(_rs);
        return false;
    }

    _plr.reset(new poller(*this));

    if (bind(_rs, addr.as_ptr<sockaddr>(), addr.get_socklen())
        || listen(_rs, SOMAXCONN)
        || !_plr->add(_rs, this)) {
        close_rawsocket(_rs);
        return false;
    }

    _ios.reset(new pending_ios());
    _ios->post_all(_rs, _family);

    return true;
}

void acceptor::impl::stop()
{
    close_rawsocket(_rs);
    _plr.reset();
    _ios.reset();
}

bool acceptor::impl::get_sockaddr(address& addr) const
{
    return INVALID_RAWSOCKET != _rs && get_rawsocket_sockaddr(_rs, addr);
}

bool acceptor::impl::on_pollevent(void* key, void* evt)
{
    const auto e = static_cast<OVERLAPPED_ENTRY*>(evt);
    accept_io* io = CONTAINING_RECORD(e->lpOverlapped, accept_io, ol);

    if (INVALID_RAWSOCKET == io->rs) {
        return false;
    }

    auto rs = io->rs;
    io->rs = INVALID_RAWSOCKET;
    _ios->recycle(io);

    if (!set_rawsocket_opt(rs, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, &_rs, sizeof(_rs))) {
        kdebug("set_rawsocket_opt(SO_UPDATE_ACCEPT_CONTEXT) failed!");
        close_rawsocket(_rs);
        return false;
    }

    _wkr.add_work(rs);

    return true;
}

} // namespace knet
