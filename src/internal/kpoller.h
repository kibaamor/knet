#pragma once
#include "../../include/knet/knetfwd.h"
#include "kplatform.h"
#include <unordered_set>

namespace knet {

class poller_client {
public:
    virtual ~poller_client() = default;
    virtual bool on_pollevent(void* key, void* evt) = 0;
};

class poller final {
public:
    explicit poller(poller_client& client);
    ~poller();

    bool add(rawsocket_t rs, void* key);
    void poll();

private:
    poller_client& _client;

#if defined(_WIN32)
    HANDLE _h = nullptr;
    std::array<OVERLAPPED_ENTRY, POLL_EVENT_NUM> _events = {};
#elif defined(__APPLE__)
    int _kq = -1;
    std::array<struct kevent, POLL_EVENT_NUM> _events = {};
    std::unordered_set<void*> _ignores;
    struct timespec _ts = {};
#else
    int _ep = -1;
    std::array<struct epoll_event, POLL_EVENT_NUM> _events = {};
#endif
};

} // namespace knet
