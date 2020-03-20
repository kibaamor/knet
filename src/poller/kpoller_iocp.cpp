#include "kpoller_iocp.h"
#include "../kinternal.h"

namespace knet {

poller::impl::impl(poller_client& clt)
    : _clt(clt)
{
    _h = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (nullptr != _h)
        SetHandleInformation(_h, HANDLE_FLAG_INHERIT, 0);
}

poller::impl::~impl()
{
    if (nullptr != _h) {
        CloseHandle(_h);
        _h = nullptr;
    }
}

bool poller::impl::add(rawsocket_t rs, void* key)
{
    auto s = reinterpret_cast<HANDLE>(rs);
    auto k = reinterpret_cast<ULONG_PTR>(key);
    return nullptr != ::CreateIoCompletionPort(s, _h, k, 0);
}

void poller::impl::poll()
{
    auto evts = _evts.data();
    auto size = static_cast<ULONG>(_evts.size());

    ULONG num = 0;
    if (!::GetQueuedCompletionStatusEx(_h, evts, size, &num, 0, FALSE)) {
        if (WAIT_TIMEOUT != WSAGetLastError())
            on_fatal_error(WSAGetLastError(), "GetQueuedCompletionStatusEx");
        return;
    }

    if (num > 0) {
        for (ULONG i = 0; i < num; ++i) {
            auto& e = evts[i];
            auto key = reinterpret_cast<void*>(e.lpCompletionKey);
            _clt.on_pollevent(key, &e);
        }
    }
}

} // namespace knet
