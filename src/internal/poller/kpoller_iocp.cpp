#include "kpoller_iocp.h"

namespace knet {

poller::impl::impl(poller_client& client)
    : _client(client)
{
    _h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (nullptr != _h) {
        SetHandleInformation(_h, HANDLE_FLAG_INHERIT, 0);
    }
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
    return nullptr != CreateIoCompletionPort(s, _h, k, 0);
}

void poller::impl::poll()
{
    auto events = _events.data();
    auto size = static_cast<ULONG>(_events.size());

    ULONG num = 0;
    if (!GetQueuedCompletionStatusEx(_h, events, size, &num, 0, FALSE)) {
        if (WAIT_TIMEOUT != WSAGetLastError()) {
            kdebug("GetQueuedCompletionStatusEx() failed!");
        }
        return;
    }
    if (num > 0) {
        for (ULONG i = 0; i < num; ++i) {
            auto& e = events[i];
            auto key = reinterpret_cast<void*>(e.lpCompletionKey);
            _client.on_pollevent(key, &e);
        }
    }
}

} // namespace knet
