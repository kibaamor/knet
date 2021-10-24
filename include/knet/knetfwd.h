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

#ifdef _WIN32
#include <ws2def.h>
#else // !_WIN32
#include <sys/uio.h>
#endif // _WIN32

namespace knet {

#ifdef _WIN32
using rawsocket_t = uintptr_t;
#else // !_WIN32
using rawsocket_t = int;
#endif // _WIN32

constexpr int IOCP_PENDING_ACCEPT_NUM = 64;
constexpr int POLL_EVENT_NUM = 128;
constexpr int SOCKET_RWBUF_SIZE = 256 * 1024;

#ifdef _WIN32
struct buffer : WSABUF {
    explicit buffer(void* d = nullptr, size_t s = 0)
    {
        buf = d;
        len = s;
    }

    char* get_data() const { return static_cast<char*>(buf); }
    void set_data(void* d) { buf = d; }
    size_t get_size() const { return len; }
    void set_size(size_t s) { len = s; }
};
#else // !_WIN32
struct buffer : iovec {
    explicit buffer(void* d = nullptr, size_t s = 0)
    {
        iov_base = d;
        iov_len = s;
    }

    char* get_data() const { return static_cast<char*>(iov_base); }
    void set_data(void* d) { iov_base = d; }
    size_t get_size() const { return iov_len; }
    void set_size(size_t s) { iov_len = s; }
};
#endif // _WIN32

} // namespace knet
