#include "kacceptor_win.h"
#include "../kinternal.h"

namespace knet {

struct acceptor::impl::accept_io {
    WSAOVERLAPPED ol = {};
    char buf[(sizeof(sockaddr_storage) + 16) * 2] = {};
    rawsocket_t rs = INVALID_SOCKET;
    accept_io* next = nullptr;

    bool post_accept(rawsocket_t srv_rs, int domain)
    {
        rs = create_rawsocket(domain, SOCK_STREAM, true);
        if (INVALID_SOCKET == rs)
            return false;

        memset(&ol, 0, sizeof(ol));

        const auto accept_ex = get_accept_ex(rs);
        if (nullptr == accept_ex)
            return false;

        DWORD dw = 0;
        const auto size = sizeof(SOCKADDR_STORAGE) + 16;
        if (!accept_ex(srv_rs, rs, buf, 0, size, size, &dw, &ol)
            && ERROR_IO_PENDING != WSAGetLastError()) {
            close_rawsocket(rs);
            return false;
        }

        return true;
    }
};

acceptor::impl::impl(workable& wkr)
    : _wkr(wkr)
{
    _plr.reset(new poller(*this));
}

acceptor::impl::~impl()
{
    kassert(INVALID_SOCKET == _rs);
    kassert(_ios.empty());
    kassert(nullptr == _free_ios);
}

void acceptor::impl::update()
{
    _plr->poll();
    post_accept();
}

bool acceptor::impl::start(const address& addr)
{
    if (INVALID_SOCKET != _rs)
        return false;

    _domain = addr.get_rawfamily();
    _rs = create_rawsocket(_domain, SOCK_STREAM, true);
    if (INVALID_SOCKET == _rs)
        return false;

    int on = 1;
    if (!set_rawsocket_opt(_rs, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        close_rawsocket(_rs);
        return false;
    }

    _plr.reset(new poller(*this));

    const auto sa = static_cast<const sockaddr*>(addr.get_sockaddr());
    const auto salen = addr.get_socklen();
    if (SOCKET_ERROR == bind(_rs, sa, salen)
        || SOCKET_ERROR == listen(_rs, SOMAXCONN)
        || !_plr->add(_rs, this)) {
        close_rawsocket(_rs);
        return false;
    }

    kassert(_ios.empty());
    kassert(nullptr == _free_ios);
    _ios.resize(64);
    for (size_t i = 0, N = _ios.size(); i < N; ++i) {
        if (N != i + 1)
            _ios[i].next = &_ios[i + 1];
    }
    _free_ios = &_ios[0];

    post_accept();
    return true;
}

void acceptor::impl::stop()
{
    if (!_ios.empty()) {
        for (auto& io : _ios)
            close_rawsocket(io.rs);
        _ios.clear();
        _free_ios = nullptr;
    }

    close_rawsocket(_rs);
}

bool acceptor::impl::on_pollevent(void* key, void* evt)
{
    (void)key;

    const auto& e = *static_cast<OVERLAPPED_ENTRY*>(evt);
    accept_io* io = CONTAINING_RECORD(e.lpOverlapped, accept_io, ol);

    if (INVALID_SOCKET != io->rs) {
        _wkr.add_work(io->rs);
        io->rs = INVALID_SOCKET;
    }

    io->next = _free_ios;
    _free_ios = io;
    return true;
}

void acceptor::impl::post_accept()
{
    for (; nullptr != _free_ios; _free_ios = _free_ios->next) {
        if (!_free_ios->post_accept(_rs, _domain))
            return;
    }
}

} // namespace knet
