#pragma once
#include "knetcfg.h"
#include <ctime>
#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <iostream>
#include <unordered_map>

namespace knet {

#ifdef _WIN32
using rawsocket_t = uintptr_t;
#else // !_WIN32
using rawsocket_t = int;
#endif // _WIN32

constexpr int IOCP_PENDING_ACCEPT_NUM = 64;
constexpr int POLL_EVENT_NUM = 128;
constexpr int SOCKET_RWBUF_SIZE = 256 * 1024;

struct buffer {
    const void* data;
    int size;

    explicit buffer(const void* d = nullptr, int s = 0)
        : data(d)
        , size(s)
    {
    }
};

} // namespace knet
