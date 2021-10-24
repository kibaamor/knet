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

#ifdef _WIN32
    size_t size;
    char* data;
#else // !_WIN32
    char* data;
    size_t size;
#endif // _WIN32

    explicit buffer(void* d = nullptr, size_t s = 0)
    {
        data = static_cast<char*>(d);
        size = s;
    }
};

} // namespace knet
